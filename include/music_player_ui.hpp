#pragma once

#include <imgui.h>

#include <algorithm>
#include <audio.hpp>
#include <cfloat>
#include <cmath>

// Dear ImGui 1.91.1+ (docking).
// Вызывать внутри своего кадра, после DockSpaceOverViewport().
// Здесь только интерфейс и демонстрационное состояние, без аудиодвижка.
inline void DrawMusicPlayerUI(wgpu::TextureView& imageView,
                              const c2::audio::PlayerViewData& viewData,
                              c2::audio::AudioState& audioEngine,
                              ImGuiID dockspaceId = 0) {
    struct Track {
        const char* title;
        const char* artist;
        int duration;
    };
    static constexpr Track tracks[] = {
        {"Midnight Walk", "Afterglow", 224},
        {"Soft Signals", "Northbound", 187},
        {"Blue Hour", "Paper Moon", 253},
        {"Stay Awhile", "Low Tide", 198},
        {"Neon Rain", "Afterglow", 276},
        {"A Quiet Place", "Northbound", 211},
    };
    constexpr int trackCount = IM_ARRAYSIZE(tracks);

    // Заглушки для экспериментов с UI. Потом подключи своё состояние плеера.
    static int selected = 0;
    static bool playing = false, repeat = false, shuffle = false;
    static float position = 42.0f, volume = 65.0f, animationTime = 0.0f;
    static ImGuiTextFilter search;
    if (playing) animationTime += ImGui::GetIO().DeltaTime;

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
            ImGui::TextDisabled("%d tracks", trackCount);
            search.Draw("##search", -FLT_MIN);
            if (ImGui::IsItemHovered())  // Проверяет последний добавленный
                                         // элемент, то есть searct.draw()
                ImGui::SetTooltip("Filter by title or artist");
            ImGui::Separator();

            for (int i = 0; i < trackCount; ++i) {
                // Проверяем название и исполнителя одной строкой.
                ImGuiTextBuffer searchable;
                searchable.appendf("%s %s", tracks[i].title, tracks[i].artist);
                if (!search.PassFilter(searchable.c_str())) continue;

                ImGui::PushID(i);
                if (ImGui::Selectable(tracks[i].title, selected == i)) {
                    selected = i;
                    position = 0.0f;
                }
                ImGui::TextDisabled("%s  /  %d:%02d", tracks[i].artist,
                                    tracks[i].duration / 60,
                                    tracks[i].duration % 60);
                ImGui::Spacing();
                ImGui::PopID();
            }
        }
        ImGui::EndChild();  // Нужен даже при BeginChild() == false.

        ImGui::SameLine();

        // СПРАВА: ширина 0 занимает оставшееся место.
        if (ImGui::BeginChild("NowPlaying", ImVec2(0, bodyHeight),
                              ImGuiChildFlags_Borders)) {
            ImGui::TextDisabled("NOW PLAYING");
            ImGui::TextUnformatted(tracks[selected].title);
            ImGui::TextDisabled("%s", tracks[selected].artist);
            ImGui::Separator();

            // Обложка-заглушка и декоративные полоски через ImDrawList.
            // Полоски НЕ анализируют звук: это просто анимация макета.
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const ImVec2 available = ImGui::GetContentRegionAvail();
            const ImVec2 size((std::max)(1.0f, available.x),
                              (std::max)(1.0f, available.y));
            // ImGui::Dummy(size);  // Резервируем место для собственной
            // отрисовки.

            ImGui::Image((ImTextureID)(intptr_t)imageView.Get(), size);

            /*
            ImDrawList* draw = ImGui::GetWindowDrawList();
            draw->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y),
                                IM_COL32(21, 22, 31, 255), 8.0f);

            const ImVec2 center(p.x + size.x * 0.5f, p.y + size.y * 0.41f);
            const float radius = (std::min)(size.x, size.y) * 0.28f;
            draw->AddCircleFilled(center, radius, IM_COL32(40, 34, 61, 255),
                                  64);
            for (int ring = 1; ring <= 5; ++ring)
                draw->AddCircle(center, radius * ring / 5.0f,
                                IM_COL32(90, 73, 128, 255), 64, 1.0f);
            draw->AddCircleFilled(center, radius * 0.28f,
                                  IM_COL32(175, 139, 237, 255), 48);
            draw->AddCircleFilled(center, radius * 0.07f,
                                  IM_COL32(21, 22, 31, 255), 24);

            constexpr int bars = 40;
            const float step = size.x * 0.8f / bars;
            const float baseY = p.y + size.y * 0.91f;
            for (int i = 0; i < bars; ++i) {
                const float wave =
                    0.5f + 0.5f * std::sin(animationTime * 3.0f + i * 0.65f);
                const float height = size.y * (0.035f + 0.15f * wave);
                const float x = p.x + size.x * 0.1f + i * step;
                draw->AddRectFilled(ImVec2(x, baseY - height),
                                    ImVec2(x + step * 0.6f, baseY),
                                    IM_COL32(175, 139, 237, 255), 2.0f);
            }
            */
        }
        ImGui::EndChild();

        // СНИЗУ: управление. Кнопки меняют только демонстрационное состояние.
        ImGui::Separator();
        if (ImGui::Button("Previous", ImVec2(90, 0))) {
            selected = (selected + trackCount - 1) % trackCount;
            position = 0.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button(playing ? "Pause###play" : "Play###play",
                          ImVec2(100, 0)))
            playing = !playing;
        ImGui::SameLine();
        if (ImGui::Button("Next", ImVec2(90, 0))) {
            selected = (selected + 1) % trackCount;
            position = 0.0f;
        }
        ImGui::SameLine();
        ImGui::Checkbox("Repeat", &repeat);
        ImGui::SameLine();
        ImGui::Checkbox("Shuffle", &shuffle);

        // -FLT_MIN растягивает следующий элемент до правого края.
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::SliderFloat("##position", &position, 0.0f,
                           static_cast<float>(tracks[selected].duration), "",
                           ImGuiSliderFlags_AlwaysClamp);
        const int seconds = static_cast<int>(position);
        ImGui::TextDisabled("%02d:%02d / %02d:%02d", seconds / 60, seconds % 60,
                            tracks[selected].duration / 60,
                            tracks[selected].duration % 60);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Volume");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::SliderFloat("##volume", &volume, 0.0f, 100.0f, "%.0f%%",
                           ImGuiSliderFlags_AlwaysClamp);
    }
    ImGui::End();  // Нужен даже при Begin() == false.
    ImGui::PopStyleVar(5);
}
