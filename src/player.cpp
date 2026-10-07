#include <taglib/audioproperties.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>

#include <C2Core/c2_log.hpp>
#include <algorithm>
#include <cctype>
#include <command.hpp>
#include <iostream>
#include <numeric>
#include <player.hpp>
#include <random>

#include "audio.hpp"

namespace c2::audio {

bool selectTrack(PlayerState& player, int index) {
    if (index < 0 ||
        static_cast<std::size_t>(index) >= player.playlist.tracks.size()) {
        C2Core::Log::error("Incorrect track index: %d", index);
        return false;
    }

    // Metadata belongs to the newly loaded sound, including its sample rate.
    player.playlist.currentIndex = -1;
    player.durationFrames = 0;
    player.soundAudioFormat = {};

    ma_result result =
        loadSoundFromFile(player.audio, player.playlist.tracks[index].filepath);
    if (result != MA_SUCCESS) {
        C2Core::Log::error("Can't load sound: %s (error %d)",
                           player.playlist.tracks[index].filepath.c_str(),
                           result);
        return false;
    }

    result = attachAudioAnalysisNodeToSound(player.audio);
    if (result != MA_SUCCESS) {
        C2Core::Log::error(
            "Can't attach audio analysis node to sound: %s (error %d)",
            player.playlist.tracks[index].filepath.c_str(), result);
        uninitSound(player.audio);
        return false;
    }

    result = getSoundFormat(player.audio, player.soundAudioFormat);
    if (result == MA_SUCCESS) {
        result = getLengthPCMFrames(player.audio, player.durationFrames);
    }
    if (result != MA_SUCCESS) {
        C2Core::Log::error("Can't get sound information: %d", result);
        uninitSound(player.audio);
        player.soundAudioFormat = {};
        player.durationFrames = 0;
        return false;
    }

    player.playlist.currentIndex = index;
    player.playlist.tracks[index].duration = static_cast<int>(
        player.durationFrames / player.soundAudioFormat.pSampleRate);

    setSoundVolume(player.audio, player.volume);

    return true;
}

void updatePlayerViewData(PlayerState& player, PlayerViewData& viewData) {
    // This is a fresh snapshot for drawing; playback remains owned by
    // miniaudio.
    viewData = {};
    viewData.volume = player.volume;
    if (!player.audio.hasSound) return;

    viewData.isPlaying = isSoundPlaying(player.audio);
    viewData.atEnd = isSoundAtEnd(player.audio);
    viewData.durationFrames = player.durationFrames;
    getCursorPCMFrames(player.audio, viewData.positionFrames);

    const double sampleRate = player.soundAudioFormat.pSampleRate;
    viewData.positionSeconds =
        static_cast<double>(viewData.positionFrames / sampleRate);
    viewData.durationSeconds =
        static_cast<double>(viewData.durationFrames / sampleRate);
}

std::vector<Track> scanDirectory(const std::string& directoryPath) {
    std::vector<Track> tracks;
    namespace fs = std::filesystem;

    std::error_code scanError;
    fs::directory_iterator it(directoryPath, scanError);
    const fs::directory_iterator end;

    for (; !scanError && it != end; it.increment(scanError)) {
        std::error_code fileError;
        const bool isFile = it->is_regular_file(fileError);

        if (fileError) {
            C2Core::Log::error("File status error: %s",
                               fileError.message().c_str());
            continue;
        }

        if (!isFile) {
            continue;
        }

        const auto& entry = *it;
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (ext == ".mp3" || ext == ".wav" || ext == ".flac") {
            TagLib::FileRef f(entry.path().string().c_str());
            if (f.isNull()) {
                C2Core::Log::error(
                    "Failed to open the file or the format is unsupported: "
                    "%s",
                    entry.path().string().c_str());
                continue;
            }
            if (f.tag()) {
                Track track;
                TagLib::Tag* tag = f.tag();
                track.filepath = entry.path().string();
                track.artist = tag->artist().to8Bit(true) != ""
                                   ? tag->artist().to8Bit(true)
                                   : "Undefined";

                track.title = tag->title().to8Bit(true) != ""
                                  ? tag->title().to8Bit(true)
                                  : entry.path().filename().string();

                if (f.audioProperties() != nullptr) {
                    track.duration = f.audioProperties()->lengthInSeconds();
                }

                tracks.push_back(track);
            }
        }
    }

    if (scanError) {
        C2Core::Log::error("Directory scan error: ",
                           scanError.message().c_str());
    }

    return tracks;
}

void printAudioMetadata(const std::string& filePath) {
    // 1. Create a FileRef object.
    // It automatically detects the format (MP3, FLAC, etc.) and parses the
    // headers.
    TagLib::FileRef f(filePath.c_str());

    // Check if the file was successfully opened and the format is supported
    if (f.isNull()) {
        std::cerr
            << "Error: Failed to open the file or the format is unsupported: "
            << filePath << std::endl;
        return;
    }

    std::cout << "========================================\n";
    std::cout << "File: " << filePath << "\n";

    // 2. Read text metadata (Tags)
    if (f.tag()) {
        TagLib::Tag* tag = f.tag();

        // IMPORTANT: TagLib returns strings in its own TagLib::String class.
        // The to8Bit(true) method converts them to a standard std::string
        // (in UTF-8 encoding). The true flag enforces strict UTF-8 conversion,
        // which is ideal for Dear ImGui.

        std::cout << "--- Metadata ---\n";
        std::cout << "Title      : " << tag->title().to8Bit(true) << "\n";
        std::cout << "Artist     : " << tag->artist().to8Bit(true) << "\n";
        std::cout << "Album      : " << tag->album().to8Bit(true) << "\n";
        std::cout << "Genre      : " << tag->genre().to8Bit(true) << "\n";
        std::cout << "Comment    : " << tag->comment().to8Bit(true) << "\n";
        std::cout << "Year       : " << tag->year() << "\n";
        std::cout << "Track      : " << tag->track() << "\n";
    } else {
        std::cout << "No metadata tags found in the file.\n";
    }

    // 3. Read technical properties (Audio properties)
    if (f.audioProperties()) {
        TagLib::AudioProperties* properties = f.audioProperties();

        std::cout << "--- Audio Properties ---\n";
        // Length in seconds (useful for a progress bar in the player)
        std::cout << "Length     : " << properties->lengthInSeconds()
                  << " sec.\n";
        std::cout << "Bitrate    : " << properties->bitrate() << " kbps\n";
        std::cout << "Sample Rate: " << properties->sampleRate() << " Hz\n";
        std::cout << "Channels   : " << properties->channels() << "\n";
    }
    std::cout << "========================================\n\n";
}

ma_result playTrack(PlayerState& player) {
    return c2::audio::playSound(player.audio);
}
ma_result pauseTrack(PlayerState& player) {
    return c2::audio::pauseSound(player.audio);
}

bool nextTrack(PlayerState& player) {
    if (player.playlist.tracks.empty()) {
        return false;
    }

    if (player.playlist.currentIndex < 0 ||
        static_cast<std::size_t>(player.playlist.currentIndex) >=
            player.playlist.tracks.size()) {
        return false;
    }

    if (player.playlist.tracks.size() == 1) {
        return selectTrack(player, 0);
    }

    int trackIndex = 0;

    if (player.params.isShuffle) {
        if (player.playlist.shufflePosition >=
            player.playlist.shuffleQueue.size()) {
            generateShuffleQueue(player);
            player.playlist.shufflePosition = 1;
        }

        if (player.playlist.shufflePosition < 1 ||
            static_cast<std::size_t>(player.playlist.shufflePosition) >=
                player.playlist.shuffleQueue.size()) {
            return false;
        }

        trackIndex =
            player.playlist.shuffleQueue[player.playlist.shufflePosition];

        if (!selectTrack(player, trackIndex)) {
            return false;
        }
        player.playlist.shufflePosition++;
        return true;
    } else {
        if (player.playlist.currentIndex + 1 < player.playlist.tracks.size()) {
            trackIndex = player.playlist.currentIndex + 1;
        } else {
            trackIndex = 0;
        }
    }
    return selectTrack(player, trackIndex);
}

bool prevTrack(PlayerState& player) {
    if (player.playlist.tracks.empty()) {
        return false;
    }

    if (player.playlist.currentIndex < 0 ||
        static_cast<std::size_t>(player.playlist.currentIndex) >=
            player.playlist.tracks.size()) {
        return false;
    }

    if (player.playlist.tracks.size() == 1) {
        return selectTrack(player, 0);
    }

    int trackIndex = 0;

    if (player.params.isShuffle) {
        if (player.playlist.shuffleQueue.size() !=
                player.playlist.tracks.size() ||
            player.playlist.shuffleQueue.empty()) {
            return false;
        }

        if (player.playlist.shufflePosition < 1 ||
            static_cast<std::size_t>(player.playlist.shufflePosition) >
                player.playlist.shuffleQueue.size()) {
            return false;
        }

        int target = player.playlist.shufflePosition - 2;
        if (target < 0) {
            target += player.playlist.shuffleQueue.size();
        }
        trackIndex = player.playlist.shuffleQueue[target];
        if (!selectTrack(player, trackIndex)) {
            return false;
        }
        player.playlist.shufflePosition = target + 1;
        return true;
    } else {
        if (player.playlist.currentIndex - 1 >= 0) {
            trackIndex = player.playlist.currentIndex - 1;
        } else {
            trackIndex = player.playlist.tracks.size() - 1;
        }
    }
    return selectTrack(player, trackIndex);
}

ma_result seekTrack(PlayerState& player, ma_uint64 frame) {
    return c2::audio::soundSeekToPCMFrame(player.audio, frame);
}

void setTrackVolume(PlayerState& player, float volume) {
    c2::audio::setSoundVolume(player.audio, volume);
    player.volume = volume;
}

void updatePlayer(PlayerState& player, std::vector<Command>& commandQueue) {
    for (const auto& cmd : commandQueue) {
        switch (cmd.command) {
            case CommandType::Next: {
                if (!nextTrack(player)) {
                    C2Core::Log::error("Failed to switch to the next track.");
                    break;
                }
                ma_result result = playTrack(player);
                if (result != MA_SUCCESS) {
                    C2Core::Log::error("Failed to play track: %d", result);
                    break;
                }
                break;
            }
            case CommandType::Pause: {
                ma_result result = pauseTrack(player);
                if (result != MA_SUCCESS) {
                    C2Core::Log::error("Failed to pause track: %d", result);
                }
                break;
            }
            case CommandType::Prev: {
                if (!prevTrack(player)) {
                    C2Core::Log::error(
                        "Failed to switch to the previous track.");
                    break;
                }
                ma_result result = playTrack(player);
                if (result != MA_SUCCESS) {
                    C2Core::Log::error("Failed to play track: %d", result);
                    break;
                }
                break;
            }
            case CommandType::Play: {
                ma_result result = playTrack(player);
                if (result != MA_SUCCESS) {
                    C2Core::Log::error("Failed to play track: %d", result);
                }
                break;
            }
            case CommandType::Select: {
                if (!selectTrack(player, cmd.index)) {
                    C2Core::Log::error("Failed to select track at index: %d",
                                       cmd.index);
                    break;
                }
                if (player.params.isShuffle) {
                    generateShuffleQueue(player);
                    player.playlist.shufflePosition = 1;
                }
                break;
            }
            case CommandType::Seek: {
                ma_result result = seekTrack(player, cmd.frame);
                if (result != MA_SUCCESS) {
                    C2Core::Log::error("Failed to seek to frame %llu: %d",
                                       cmd.frame, result);
                }
                break;
            }
            case CommandType::SetVolume: {
                setTrackVolume(player, cmd.volume);
                break;
            }
            case CommandType::ToggleShuffle: {
                if (!player.params.isShuffle) {
                    player.params.isShuffle = true;
                    generateShuffleQueue(player);
                    player.playlist.shufflePosition = 1;
                    C2Core::Log::info("get %d", player.params.isShuffle);
                } else {
                    player.params.isShuffle = false;
                }
                break;
            }
            case CommandType::ToggleRepeat: {
                player.params.isRepeat = player.params.isRepeat ? false : true;
                break;
            }
        }
    }

    commandQueue.clear();

    if (player.audio.hasSound) {
        if (c2::audio::isSoundAtEnd(player.audio)) {
            if (!player.params.isRepeat) {
                if (!c2::audio::nextTrack(player)) {
                    C2Core::Log::error("Failed to auto-switch track");
                    return;
                }
                ma_result result = playTrack(player);
                if (result != MA_SUCCESS) {
                    C2Core::Log::error("Failed to play track: %d", result);
                }
            } else {
                ma_result result;
                result = seekTrack(player, 0);
                if (result != MA_SUCCESS) {
                    C2Core::Log::error("Failed to rewind track: %d", result);
                    return;
                }
                result = playTrack(player);
                if (result != MA_SUCCESS) {
                    C2Core::Log::error("Failed to play track: %d", result);
                    return;
                }
            }
        }
    }
}

void generateShuffleQueue(PlayerState& player) {
    auto& playlist = player.playlist;

    if (!player.params.isShuffle || playlist.currentIndex < 0 ||
        static_cast<std::size_t>(playlist.currentIndex) >=
            playlist.tracks.size()) {
        return;
    }

    auto& queue = playlist.shuffleQueue;
    queue.resize(playlist.tracks.size());

    std::iota(queue.begin(), queue.end(), 0);
    std::swap(queue[0], queue[playlist.currentIndex]);
    std::shuffle(queue.begin() + 1, queue.end(), playlist.rng);
}

ma_uint64 convertSecondsToPSMFrames(const PlayerState& player, double seconds) {
    const double target = seconds * player.soundAudioFormat.pSampleRate;
    const auto frame = static_cast<ma_uint64>(
        std::clamp(target, 0.0, static_cast<double>(player.durationFrames)));
    return frame;
}
}  // namespace c2::audio
