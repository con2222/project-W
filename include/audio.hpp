#pragma once
#include <miniaudio.h>

#include <string>

namespace c2::audio {

struct AudioState {
    ma_engine* engine = nullptr;
    ma_sound sound{};
    bool hasSound = false;
};

struct AudioFormatInfo {
    ma_format pFormat{};
    ma_uint32 pChannels = 0;
    ma_uint32 pSampleRate = 0;
    ma_channel pChannelMap[MA_MAX_CHANNELS]{};
};

struct PlayerViewData {
    bool isPlaying = false;
    bool atEnd = false;
    float volume = 1.0f;
    float positionSeconds = 0.0f;
    float durationSeconds = 0.0f;
    ma_uint64 positionFrames = 0;
    ma_uint64 durationFrames = 0;
};

ma_result initAudio(AudioState& audio);
void shutdownAudio(AudioState& audio);
ma_result loadSoundFromFile(AudioState& audio, const std::string& filename);
ma_result playSound(AudioState& audio);
ma_result pauseSound(AudioState& audio);
ma_result getLengthPCMFrames(AudioState& audio, ma_uint64& outValue);
ma_result getCursorPCMFrames(AudioState& audio, ma_uint64& outValue);
ma_result getSoundFormat(AudioState& audio, AudioFormatInfo& outData);
ma_result soundSeekToPCMFrame(AudioState& audio, ma_uint64 frameIndex);
bool isSoundPlaying(AudioState& audio);
bool isSoundAtEnd(AudioState& audio);
void setSoundVolume(AudioState& audio, float volumeValue);
float getSoundVolume(AudioState& audio);
void uninitSound(AudioState& audio);

}  // namespace c2::audio
