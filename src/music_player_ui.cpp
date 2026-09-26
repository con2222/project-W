#include <C2Core/c2_log.hpp>
#include <cstdint>
#include <iostream>
#include <music_player_ui.hpp>

#include "audio.hpp"
#include "imgui.h"

int GetMaxCharactersThatFit(const char* text, float availableWidth) {
    if (!text || availableWidth <= 0.0f) return 0;

    int low = 0;
    int high = (int)strlen(text);
    int result = 0;

    // Быстрый бинарный поиск по длине строки
    while (low <= high) {
        int mid = low + (high - low) / 2;

        // Считаем размер подстроки от 0 до mid
        ImVec2 size = ImGui::CalcTextSize(text, text + mid);

        if (size.x <= availableWidth) {
            result = mid;   // Запоминаем, этот кусок влезает
            low = mid + 1;  // Пробуем взять больше символов
        } else {
            high = mid - 1;  // Не влезает, уменьшаем длину
        }
    }

    return result;
}

void DrawMusicPlayerUI(wgpu::TextureView& imageView,
                       const c2::audio::PlayerViewData& viewData,
                       c2::audio::PlayerState& player, ImGuiID dockspaceId) {
    c2::audio::PlayerViewData current = viewData;
    static bool repeat = false, shuffle = false;
    static ImGuiTextFilter search;

    static std::vector<int> counters;
    static std::vector<float> textTimers;

    static std::vector<float> scrollOffsets;

    if (scrollOffsets.size() < player.playlist.tracks.size()) {
        scrollOffsets.resize(player.playlist.tracks.size(), 0.0f);
    }

    if (counters.size() < player.playlist.tracks.size()) {
        counters.resize(player.playlist.tracks.size(), 0);
        textTimers.resize(player.playlist.tracks.size(), 0.0f);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(16, 16));  // Отступ от края окна сверху и снизу
    ImGui::PushStyleVar(
        ImGuiStyleVar_ItemSpacing,
        ImVec2(
            10,
            8));  // Автоматически добавляет оступ между кнопками и элементами
    ImGui::PushStyleVar(
        ImGuiStyleVar_FramePadding,
        ImVec2(10, 6));  // внутренний отступ самих элементов (фреймов)
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,
                        12.0f);  // Закругляет интерфейс радиус скругления углов
                                 // элементов, по типу кнопок, ползунков и тд
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,
                        8.0f);  // задает радиус закругления для чайлд
                                // компонентов, например список треков

    ImGui::SetNextWindowSize(
        ImVec2(940, 640),
        ImGuiCond_FirstUseEver);  // Ставит размер окна начальный, и что надо
                                  // только в первый раз его так настраивать,
                                  // затем брать настройки из .ini
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(660, 480),
        ImVec2(FLT_MAX,
               FLT_MAX));  // нельзя уменьшить окно прям в полный 0, но при этом
                           // увеличивать можно хоть на сколько
    if (dockspaceId != 0)
        ImGui::SetNextWindowDockID(
            dockspaceId,
            ImGuiCond_FirstUseEver);  // Прикрепляет к определенному доку

    if (ImGui::Begin("Music Player", nullptr, ImGuiWindowFlags_NoCollapse)) {
        const float spacing =
            ImGui::GetStyle()
                .ItemSpacing.y;  // равен  ImGuiStyleVar_ItemSpacing.y
        const float footerHeight =
            3.0f * ImGui::GetFrameHeightWithSpacing() +
            ImGui::GetTextLineHeightWithSpacing() +
            2.0f * spacing;  // GetFrameHeightWithSpacing() возвращает нам
                             // высоту элемента(по типу кнопки, ползунка и тд) с
                             // небольшим оступом + высота текста с оступом +
                             // наш оступ по вертикали в запасе

        const float bodyHeight =
            (std::max)(120.0f, ImGui::GetContentRegionAvail().y - footerHeight);

        // СЛЕВА: список треков. Поменяй 250, чтобы изменить ширину панели.
        if (ImGui::BeginChild("Library", ImVec2(250, bodyHeight),
                              ImGuiChildFlags_Borders)) {
            ImGui::TextUnformatted("LIBRARY");
            ImGui::TextDisabled("%zu tracks", player.playlist.tracks.size());
            search.Draw("##search", -FLT_MIN);
            if (ImGui::IsItemHovered())  // Проверяет последний добавленный
                                         // элемент, то есть search.draw()
                ImGui::SetTooltip("Filter by title or artist");
            ImGui::Separator();

            for (int i = 0; i < static_cast<int>(player.playlist.tracks.size());
                 ++i) {
                const auto& track = player.playlist.tracks[i];

                ImGuiTextBuffer searchable;
                searchable.appendf("%s %s",
                                   player.playlist.tracks[i].title.c_str(),
                                   player.playlist.tracks[i].artist.c_str());
                if (!search.PassFilter(searchable.c_str())) continue;

                ImGui::PushID(i);

                std::string selectableId =
                    "###Track_Selectable_" + std::to_string(i);
                if (ImGui::Selectable(selectableId.c_str(),
                                      player.playlist.currentIndex == i)) {
                    c2::audio::selectTrack(player, i);
                    c2::audio::playSound(player.audio);
                    c2::audio::updatePlayerViewData(player, current);
                }

                ImVec2 itemMin = ImGui::GetItemRectMin();
                ImVec2 itemMax = ImGui::GetItemRectMax();
                bool isHovered = ImGui::IsItemHovered();

                ImGui::PushClipRect(itemMin, itemMax, true);

                float textWidth = ImGui::CalcTextSize(track.title.c_str()).x;
                float availWidth = itemMax.x - itemMin.x;

                if (isHovered && textWidth > availWidth) {
                    scrollOffsets[i] += 50.f * ImGui::GetIO().DeltaTime;

                    if (scrollOffsets[i] > textWidth + 20.f) {
                        scrollOffsets[i] = -availWidth;
                    }
                } else if (!isHovered) {
                    scrollOffsets[i] = 0.f;
                }

                ImVec2 textPos =
                    ImVec2(itemMin.x - scrollOffsets[i], itemMin.y);
                ImGui::GetWindowDrawList()->AddText(
                    textPos, ImGui::GetColorU32(ImGuiCol_Text),
                    track.title.c_str());

                ImGui::PopClipRect();

                if (track.duration > 0) {
                    ImGui::TextDisabled("%s  /  %d:%02d", track.artist.c_str(),
                                        track.duration / 60,
                                        track.duration % 60);
                } else {
                    ImGui::TextDisabled("%s  /  --:--", track.artist.c_str());
                }
                ImGui::Spacing();
                ImGui::PopID();
            }
        }
        ImGui::EndChild();  // Нужен даже при BeginChild() == false.

        if (c2::audio::isSoundAtEnd(player.audio)) {
            if (player.playlist.currentIndex + 1 <
                player.playlist.tracks.size()) {
                c2::audio::selectTrack(player,
                                       player.playlist.currentIndex + 1);
                c2::audio::playSound(player.audio);
            } else {
                c2::audio::selectTrack(player, 0);
                c2::audio::playSound(player.audio);
            }
        }

        ImGui::SameLine();

        // СПРАВА: ширина 0 занимает оставшееся место.
        if (ImGui::BeginChild("NowPlaying", ImVec2(0, bodyHeight),
                              ImGuiChildFlags_Borders)) {
            ImGui::TextDisabled("NOW PLAYING");
            const int index = player.playlist.currentIndex;
            if (player.audio.hasSound && index >= 0 &&
                static_cast<std::size_t>(index) <
                    player.playlist.tracks.size()) {
                const auto& track = player.playlist.tracks[index];
                ImGui::TextUnformatted(track.title.c_str());
                ImGui::TextDisabled("%s", track.artist.c_str());
            } else {
                ImGui::TextDisabled("No track selected");
            }
            ImGui::Separator();

            // Display the offscreen texture rendered in main.
            const ImVec2 available = ImGui::GetContentRegionAvail();
            const ImVec2 size((std::max)(1.0f, available.x),
                              (std::max)(1.0f, available.y));

            ImGui::Image((ImTextureID)(std::intptr_t)imageView.Get(), size);
        }
        ImGui::EndChild();

        ImGui::Separator();
        ImGui::BeginDisabled(player.playlist.tracks.empty());
        if (ImGui::Button("Previous", ImVec2(90, 0))) {
            const int index =
                player.playlist.currentIndex <= 0
                    ? static_cast<int>(player.playlist.tracks.size()) - 1
                    : player.playlist.currentIndex - 1;
            c2::audio::selectTrack(player, index);
            c2::audio::playSound(player.audio);
            c2::audio::updatePlayerViewData(player, current);
        }
        ImGui::EndDisabled();
        ImGui::SameLine();

        ImGui::BeginDisabled(!player.audio.hasSound);
        const char* playLabel = current.isPlaying ? "Pause###play"
                                : current.atEnd   ? "Replay###play"
                                                  : "Play###play";
        if (ImGui::Button(playLabel, ImVec2(100, 0))) {
            const ma_result result = current.isPlaying
                                         ? c2::audio::pauseSound(player.audio)
                                         : c2::audio::playSound(player.audio);
            if (result != MA_SUCCESS) {
                C2Core::Log::error("Can't change playback state: %d", result);
            }
            c2::audio::updatePlayerViewData(player, current);
        }
        ImGui::EndDisabled();
        ImGui::SameLine();

        ImGui::BeginDisabled(player.playlist.tracks.empty());
        if (ImGui::Button("Next", ImVec2(90, 0))) {
            const int index = (player.playlist.currentIndex + 1) %
                              static_cast<int>(player.playlist.tracks.size());
            c2::audio::selectTrack(player, index);
            if (c2::audio::playSound(player.audio) != MA_SUCCESS) {
                C2Core::Log::error("Can't set next track");
            }
            c2::audio::updatePlayerViewData(player, current);
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        // These two controls are still placeholders for later playlist
        // behavior.
        ImGui::Checkbox("Repeat", &repeat);
        ImGui::SameLine();
        ImGui::Checkbox("Shuffle", &shuffle);

        // Edit a local value, then submit a seek in the sound's PCM frame
        // units.
        float position = current.positionSeconds;
        ImGui::BeginDisabled(!player.audio.hasSound);
        ImGui::SetNextItemWidth(-FLT_MIN);

        bool currentDurationChanged = ImGui::SliderFloat(
            "##position", &position, 0.0f, current.durationSeconds, "",
            ImGuiSliderFlags_AlwaysClamp);

        if (ImGui::IsItemActive()) {
            c2::audio::pauseSound(player.audio);
        }

        if (currentDurationChanged) {
            const double target = static_cast<double>(position) *
                                  player.soundAudioFormat.pSampleRate;
            const auto frame = static_cast<ma_uint64>(std::clamp(
                target, 0.0, static_cast<double>(player.durationFrames)));
            const ma_result result =
                c2::audio::soundSeekToPCMFrame(player.audio, frame);
            if (result != MA_SUCCESS) {
                C2Core::Log::error("Can't seek sound: %d", result);
            }
            c2::audio::updatePlayerViewData(player, current);
        }

        if (ImGui::IsItemDeactivated()) {
            c2::audio::playSound(player.audio);
        }

        ImGui::EndDisabled();
        const int seconds = static_cast<int>(current.positionSeconds);
        const int duration = static_cast<int>(current.durationSeconds);
        ImGui::TextDisabled("%02d:%02d / %02d:%02d", seconds / 60, seconds % 60,
                            duration / 60, duration % 60);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Volume");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);
        float volume = current.volume * 100.0f;
        if (ImGui::SliderFloat("##volume", &volume, 0.0f, 100.0f, "%.0f%%",
                               ImGuiSliderFlags_AlwaysClamp)) {
            player.volume = volume / 100.0f;
            if (player.audio.hasSound) {
                c2::audio::setSoundVolume(player.audio, player.volume);
            }
        }
    }
    ImGui::End();  // Нужен даже при Begin() == false.
    ImGui::PopStyleVar(5);
}
