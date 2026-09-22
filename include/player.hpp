#pragma once
#include <audio.hpp>
#include <string>
#include <vector>

namespace c2::audio {

struct Track {
    std::string title;
    std::string artist;
    std::string filepath;
    int duration = 0;
};

struct Playlist {
    std::vector<Track> tracks;
    int currentIndex = -1;
};

struct PlayerState {
    AudioState audio;
    Playlist playlist;
    float volume = 1.0f;
    AudioFormatInfo soundAudioFormat;
    ma_uint64 durationFrames = 0;
};

bool selectTrack(PlayerState& player, int index);
void updatePlayerViewData(PlayerState& player, PlayerViewData& viewData);

}  // namespace c2::audio
