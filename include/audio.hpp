#pragma once
#include <miniaudio.h>

#include <memory>
#include <string>

namespace c2::audio {

struct AudioState {
    ma_engine* engine;
    ma_sound sound;
};

struct AudioFormatInfo {
    ma_format pFormat;
    ma_uint32 pChannels;
    ma_uint32 pSampleRate;
    ma_channel pChannelMap[MA_MAX_CHANNELS];
};

struct PlayerViewData {
    bool isPlaying = false;
    bool atEnd = true;
    float volume = 0.75f;
    uint32_t soundCurrentLengthSeconds = 0;
    uint32_t soundLengthSeconds = 0;
    ma_uint64 currentSoundLength = 0;
    ma_uint64 soundLength = 0;
};

struct PlayerActions {
    bool isPlaying;
    float
};

ma_result initAudio(AudioState& audio);
void shutdownAudio(AudioState& audio);
ma_result initSoundFromFile(AudioState& audio, const std::string& filename);
ma_result playSound(AudioState& audio);
ma_result pauseSound(AudioState& audio);
ma_result getLengthPCMFrames(AudioState& audio, ma_uint64& outValue);
ma_result getCursorPCMFrames(AudioState& audio, ma_uint64& outValue);
ma_result getSoundData(AudioState& audio, AudioFormatInfo& outData);
ma_result soundSeekToPCMFrame(AudioState& audio, ma_uint64 frameIndex);
bool isSoundPlaying(AudioState& audio);
bool isSoundAtEnd(AudioState& audio);

void setSoundVolume(AudioState& audio, float volumeValue);
float getSoundVolume(AudioState& audio);

void uninitSound(AudioState& audio);

}  // namespace c2::audio