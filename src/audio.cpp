#include <C2Core/c2_log.hpp>
#include <algorithm>
#include <audio.hpp>
#include <hardcode.hpp>

namespace c2::audio {

ma_result initAudio(AudioState& audio) {
    audio.engine = new ma_engine{};
    ma_result result = ma_engine_init(nullptr, audio.engine);
    if (result != MA_SUCCESS) {
        delete audio.engine;
        audio.engine = nullptr;
    }

    return result;
}

void shutdownAudio(AudioState& audio) {
    uninitSound(audio);
    ma_node_uninit(&audio.audioAnalysisNode, nullptr);  //
    if (audio.engine != nullptr) {
        ma_engine_uninit(audio.engine);
        delete audio.engine;
        audio.engine = nullptr;
    }
}

ma_result loadSoundFromFile(AudioState& audio, const std::string& filename) {
    uninitSound(audio);
    // Stream the selected track instead of keeping the whole encoded file in
    // memory.
    ma_result result = ma_sound_init_from_file(audio.engine, filename.c_str(),
                                               MA_SOUND_FLAG_STREAM, nullptr,
                                               nullptr, &audio.sound);
    audio.hasSound = (result == MA_SUCCESS);
    return result;
}

ma_result playSound(AudioState& audio) {
    if (!audio.hasSound) {
        return MA_INVALID_OPERATION;
    }
    return ma_sound_start(&audio.sound);
}

ma_result pauseSound(AudioState& audio) {
    if (!audio.hasSound) {
        return MA_INVALID_OPERATION;
    }
    return ma_sound_stop(&audio.sound);
}

ma_result getLengthPCMFrames(AudioState& audio, ma_uint64& outValue) {
    return ma_sound_get_length_in_pcm_frames(&audio.sound, &outValue);
}

ma_result getCursorPCMFrames(AudioState& audio, ma_uint64& outValue) {
    return ma_sound_get_cursor_in_pcm_frames(&audio.sound, &outValue);
}
ma_result getSoundFormat(AudioState& audio, AudioFormatInfo& outData) {
    return ma_sound_get_data_format(&audio.sound, &outData.pFormat,
                                    &outData.pChannels, &outData.pSampleRate,
                                    outData.pChannelMap, MA_MAX_CHANNELS);
}

ma_result soundSeekToPCMFrame(AudioState& audio, ma_uint64 frameIndex) {
    if (!audio.hasSound) {
        return MA_INVALID_OPERATION;
    }
    return ma_sound_seek_to_pcm_frame(&audio.sound, frameIndex);
}

void uninitSound(AudioState& audio) {
    if (audio.hasSound) {
        ma_sound_uninit(&audio.sound);
        audio.hasSound = false;
    }
}

bool isSoundPlaying(AudioState& audio) {
    return ma_sound_is_playing(&audio.sound);
}

bool isSoundAtEnd(AudioState& audio) { return ma_sound_at_end(&audio.sound); }

void setSoundVolume(AudioState& audio, float volumeValue) {
    ma_sound_set_volume(&audio.sound, volumeValue);
}

float getSoundVolume(AudioState& audio) {
    return ma_sound_get_volume(&audio.sound);
}

void processAudioAnalysisNode(ma_node* pNode, const float** ppFramesIn,
                              ma_uint32* pFrameCountIn, float** ppFramesOut,
                              ma_uint32* pFrameCountOut) {
    const float* pFramesIn_0 = ppFramesIn[0];
    float* pFramesOut_0 = ppFramesOut[0];

    AudioAnalysisNode* audioAnalysisNode =
        static_cast<AudioAnalysisNode*>(pNode);

    int sampleCount = *pFrameCountOut * audioAnalysisNode->channels;

    ma_result result;
    void* rbPtr;
    ma_uint32 remainingFrames = *pFrameCountIn;
    ma_uint32 frameOffset = 0;

    while (remainingFrames > 0) {
        ma_uint32 framesToAcquire = remainingFrames;
        result = ma_pcm_rb_acquire_write(&audioAnalysisNode->pcmRingBuffer,
                                         &framesToAcquire, &rbPtr);
        if (result != MA_SUCCESS || framesToAcquire == 0) {
            break;
        }

        std::copy(pFramesIn_0 + frameOffset * audioAnalysisNode->channels,
                  pFramesIn_0 + (frameOffset + framesToAcquire) *
                                    audioAnalysisNode->channels,
                  static_cast<float*>(rbPtr));
        result = ma_pcm_rb_commit_write(&audioAnalysisNode->pcmRingBuffer,
                                        framesToAcquire);

        if (result != MA_SUCCESS) {
            break;
        }

        remainingFrames -= framesToAcquire;
        frameOffset += framesToAcquire;
    }

    float peak = 0.f;
    float sumSamples = 0.f;

    for (int i = 0; i < sampleCount; i++) {
        float sample = pFramesIn_0[i];
        peak = std::max(peak, std::abs(sample));
        sumSamples += sample * sample;
        pFramesOut_0[i] = sample;
    }

    if (sampleCount != 0) {
        audioAnalysisNode->rms.store(std::sqrt(sumSamples / sampleCount),
                                     std::memory_order_relaxed);
    }

    audioAnalysisNode->peak.store(peak, std::memory_order_relaxed);
}

ma_result initAudioAnalysisNode(AudioState& audio) {
    ma_uint32 channels = ma_engine_get_channels(audio.engine);
    ma_uint32 sampleRate = ma_engine_get_sample_rate(audio.engine);
    ma_node_graph* nodeGraph = ma_engine_get_node_graph(audio.engine);

    ma_result result;

    result = ma_pcm_rb_init(ma_format_f32, channels, c2::hard::RG_BUFFER_SIZE,
                            nullptr, nullptr,
                            &audio.audioAnalysisNode.pcmRingBuffer);

    ma_uint32 inputChannels[1];   // array size = input bus count;
    ma_uint32 outputChannels[1];  // array size = output bus count

    inputChannels[0] = channels;
    outputChannels[0] = channels;

    static ma_node_vtable vtableNode;
    vtableNode.onProcess = processAudioAnalysisNode;
    vtableNode.inputBusCount = 1;
    vtableNode.outputBusCount = 1;
    vtableNode.onGetRequiredInputFrameCount = nullptr;
    vtableNode.flags = MA_NODE_FLAG_CONTINUOUS_PROCESSING;

    ma_node_config nodeConfig = ma_node_config_init();
    nodeConfig.vtable = &vtableNode;
    nodeConfig.pInputChannels = inputChannels;
    nodeConfig.pOutputChannels = outputChannels;

    result =
        ma_node_init(nodeGraph, &nodeConfig, nullptr, &audio.audioAnalysisNode);

    if (result == MA_SUCCESS) {
        audio.audioAnalysisNode.channels = channels;
        audio.analysisNodeInitialized = true;
    }

    return result;
}

ma_result attachAudioAnalysisNodeToEngine(AudioState& audio) {
    return ma_node_attach_output_bus(
        &audio.audioAnalysisNode, 0, ma_engine_get_endpoint(audio.engine),
        0);  // source[index] -> destination[index] (customNode output bus ->
             // engine input bus)
}

ma_result attachAudioAnalysisNodeToSound(AudioState& audio) {
    return ma_node_attach_output_bus(
        &audio.sound, 0, &audio.audioAnalysisNode,
        0);  // sound output bus -> customNode input bus
}

float computeAudioIntensity(AudioAnalysisNode& node, float deltaTime) {
    float alphaAttack = 1 - std::exp(-deltaTime / node.attackTime);
    float alphaRelease = 1 - std::exp(-deltaTime / node.releaseTime);

    float currentRms = node.rms.load(std::memory_order_relaxed);

    if (currentRms > node.smoothed) {
        node.smoothed =
            node.smoothed + alphaAttack * (currentRms - node.smoothed);
    } else {
        node.smoothed =
            node.smoothed + alphaRelease * (currentRms - node.smoothed);
    }

    float rmsDbfs;
    if (node.smoothed > 1e-12) {
        rmsDbfs = amplitudeToDbfs<float>(node.smoothed);
    } else {
        rmsDbfs = -96.f;
    }

    float audioIntensity = std::clamp((rmsDbfs + 60.f) / 60.f, 0.f, 1.f);
    return audioIntensity;
}

ma_uint32 readAnalysisFrames(AudioAnalysisNode& node, float* framesOut,
                             ma_uint32 frameCountOut) {
    ma_result result;
    ma_uint32 remainingFrames = frameCountOut;
    ma_uint32 frameOffset = 0;

    while (remainingFrames > 0) {
        void* rgPtr;
        ma_uint32 framesToAcquire = remainingFrames;
        result = ma_pcm_rb_acquire_read(&node.pcmRingBuffer, &framesToAcquire,
                                        &rgPtr);

        if (result != MA_SUCCESS || framesToAcquire == 0) {
            break;
        }

        const float* data = static_cast<float*>(rgPtr);

        std::copy(data, data + framesToAcquire * node.channels,
                  framesOut + frameOffset * node.channels);

        result = ma_pcm_rb_commit_read(&node.pcmRingBuffer, framesToAcquire);

        if (result != MA_SUCCESS) {
            break;
        }

        remainingFrames -= framesToAcquire;
        frameOffset += framesToAcquire;
    }

    return frameOffset;
}

}  // namespace c2::audio
