#pragma once
#include <audio.hpp>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

namespace c2::audio {

struct Command;

struct Track {
    std::string title;
    std::string artist;
    std::string filepath;
    int duration = 0;
};

struct PlayerParams {
    bool isShuffle = false;
    bool isRepeat = false;
};

struct Playlist {
    std::vector<Track> tracks;
    int currentIndex = -1;

    std::vector<int> shuffleQueue;
    int shufflePosition = 0;
    std::mt19937 rng{std::random_device{}()};
};

struct PlayerState {
    AudioState audio;
    Playlist playlist;
    PlayerParams params;
    float volume = 1.0f;
    AudioFormatInfo soundAudioFormat;
    ma_uint64 durationFrames = 0;
};

void updatePlayerViewData(PlayerState& player, PlayerViewData& viewData);
std::vector<Track> scanDirectory(const std::string& directoryPath);
void printAudioMetadata(const std::string& filePath);
void generateShuffleQueue(PlayerState& player);

bool selectTrack(PlayerState& player, int index);
bool nextTrack(PlayerState& player);
bool prevTrack(PlayerState& player);
ma_result playTrack(PlayerState& player);
ma_result pauseTrack(PlayerState& player);
ma_result seekTrack(PlayerState& player, ma_uint64 frame);
void setTrackVolume(PlayerState& player);
void updatePlayer(PlayerState& player, std::vector<Command>& commandQueue);

}  // namespace c2::audio
