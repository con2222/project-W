#pragma once

#include <atomic>
#include <cmath>
#include <string>

extern "C" {
#include <miniaudio.h>
}

namespace c2::audio {

struct AudioAnalysisNode {
    ma_node_base base;
    ma_uint32 channels = 1;

    float attackTime = 0.02f;
    float releaseTime = 0.3f;

    std::atomic<float> rms = 0.f;
    std::atomic<float> peak = 0.f;
};

struct AudioState {
    ma_engine* engine = nullptr;
    ma_sound sound{};
    AudioAnalysisNode audioAnalysisNode;
    bool hasSound = false;
    bool analysisNodeInitialized = false;
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
    double positionSeconds = 0.0f;
    double durationSeconds = 0.0f;
    ma_uint64 positionFrames = 0;
    ma_uint64 durationFrames = 0;
};

template <typename T>
T amplitudeToDbfs(T value) {
    return 20 * std::log10(value);
}

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
void processAudioAnalysisNode(ma_node* pNode, const float** ppFramesIn,
                              ma_uint32* pFrameCountIn, float** ppFramesOut,
                              ma_uint32* pFrameCountOut);

ma_result initAudioAnalysisNode(AudioState& audio);
ma_result attachAudioAnalysisNodeToEngine(AudioState& audio);
ma_result attachAudioAnalysisNodeToSound(AudioState& audio);

float computeAudioIntensity(AudioAnalysisNode& node, float deltaTime);

}  // namespace c2::audio
