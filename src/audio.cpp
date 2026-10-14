#include <C2Core/c2_log.hpp>
#include <algorithm>
#include <audio.hpp>
#include <bit>
#include <cmath>
#include <complex>
#include <hardcode.hpp>
#include <numbers>
#include <vector>

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

bool isFrequencyRangeValid(const Range& range) {
    return range.max > range.min && range.min >= 0 &&
           std::isfinite(range.max) && std::isfinite(range.min);
}

bool initAudioAnalysis(AudioAnalysisState& state,
                       const AudioAnalysisConfig& config, ma_uint32 channels,
                       ma_uint32 sampleRate) {
    if (!std::has_single_bit(config.analysisSize) || config.hopSize == 0 ||
        config.hopSize > config.analysisSize) {
        C2Core::Log::error("Incorrect config");
        return false;
    }

    if (!isFrequencyRangeValid(config.bassRange) ||
        !isFrequencyRangeValid(config.midRange) ||
        !isFrequencyRangeValid(config.trebleRange)) {
        C2Core::Log::error("Incorrect bands range");
        return false;
    }

    if (config.intensityDbRange.min >= config.intensityDbRange.max ||
        !std::isfinite(config.intensityDbRange.min) ||
        !std::isfinite(config.intensityDbRange.max)) {
        C2Core::Log::error("Incorrect intensity Db range");
        return false;
    }

    if (config.releaseTime <= 0 || config.attackTime <= 0 ||
        !std::isfinite(config.releaseTime) ||
        !std::isfinite(config.attackTime)) {
        C2Core::Log::error("Incorrect release time or attack time");
        return false;
    }

    if (sampleRate <= 0 || channels <= 0) {
        C2Core::Log::error("Incorrect sample rate or channels");
        return false;
    }

    const std::size_t& analysisSize = config.analysisSize;
    state.config = config;
    state.filled = 0;
    state.analysisBlock.resize(analysisSize);
    state.windowedBlock.resize(analysisSize);
    state.result = AudioAnalysisResult{};
    state.sampleRate = sampleRate;
    state.channels = channels;

    return true;
}

void processPCM(const std::vector<float>& samples, ma_uint32 framesRead,
                AudioAnalysisState& state) {
    if (framesRead == 0) {
        return;
    }

    float peak = 0.f;
    for (std::size_t i = 0; i < framesRead * state.channels; i++) {
        float sample = samples[i];
        peak = std::max(peak, std::abs(sample));
    }
    state.result.peak = peak;

    for (std::size_t i = 0; i < framesRead; i++) {
        state.analysisBlock[state.filled] = samples[i * state.channels];
        state.filled++;
        if (state.filled == state.config.analysisSize) {
            state.windowedBlock = state.analysisBlock;
            c2::audio::applyHannWindow(state.windowedBlock);

            state.result.amplitudes = c2::audio::amplitudeSpectrum(
                c2::audio::fft(state.windowedBlock),
                state.windowedBlock.size() / 2.0f);

            std::copy(state.analysisBlock.begin() + state.config.hopSize,
                      state.analysisBlock.end(), state.analysisBlock.begin());

            state.result.bands.bass = c2::audio::bandPeak(
                state.result.amplitudes, state.config.analysisSize,
                state.sampleRate, state.config.bassRange.min,
                state.config.bassRange.max);
            state.result.bands.mid = c2::audio::bandPeak(
                state.result.amplitudes, state.config.analysisSize,
                state.sampleRate, state.config.midRange.min,
                state.config.midRange.max);
            state.result.bands.treble = c2::audio::bandPeak(
                state.result.amplitudes, state.config.analysisSize,
                state.sampleRate, state.config.trebleRange.min,
                state.config.trebleRange.max);

            state.result.spectrumDb =
                c2::audio::spectrumToDb(state.result.amplitudes);
            state.filled = state.config.analysisSize - state.config.hopSize;
        }
    }
}

void shutdownAudio(AudioState& audio) {
    uninitSound(audio);
    ma_node_uninit(&audio.audioAnalysisNode, nullptr);
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

ma_uint32 getEngineSampleRate(AudioState& audio) {
    return ma_engine_get_sample_rate(audio.engine);
}

ma_uint32 getEngineChannels(AudioState& audio) {
    return ma_engine_get_channels(audio.engine);
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
    if (result != MA_SUCCESS) {
        C2Core::Log::error("Can't init ring buffer");
        return result;
    }

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
    } else {
        ma_pcm_rb_uninit(&audio.audioAnalysisNode.pcmRingBuffer);
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

std::vector<std::complex<float>> dft(const std::vector<float>& samples) {
    const std::size_t N = samples.size();
    constexpr float pi = std::numbers::pi_v<float>;
    std::vector<std::complex<float>> result(N);

    for (std::size_t k = 0; k < N; ++k) {
        float real = 0.0f;
        float imag = 0.0f;

        for (std::size_t n = 0; n < N; ++n) {
            float angle = 2.0f * pi * k * n / N;

            real += samples[n] * std::cos(angle);
            imag -= samples[n] * std::sin(angle);
        }
        result[k] = std::complex<float>(real, imag);
    }
    return result;
}

// Without Hann window normalization = N.
// Periodic Hann normalization = N / 2
// Symetric Hann normalization = (N - 1) / 2, N >= 3
std::vector<float> amplitudeSpectrum(
    const std::vector<std::complex<float>>& spectrum, float normalization) {
    const std::size_t N = spectrum.size();

    if (!(std::isfinite(normalization) && normalization > 0)) {
        C2Core::Log::error("normalization <= 0 or isn't finite");
        return {};
    }

    if (N == 0) {
        return {};
    }

    std::vector<float> result(N / 2 + 1);

    for (std::size_t k = 0; k < result.size(); k++) {
        if (k == 0 || (N % 2 == 0 && k == N / 2)) {
            result[k] = std::abs(spectrum[k]) / normalization;
        } else {
            result[k] = std::abs(spectrum[k]) * 2.0f / normalization;
        }
    }
    return result;
}

std::vector<std::complex<float>> fft(
    const std::vector<std::complex<float>>& samples) {
    const std::size_t N = samples.size();

    if (!std::has_single_bit(N)) {
        C2Core::Log::error("Samples vector size must be a power of 2 (got %zu)",
                           N);
        return {};
    }

    if (N == 1) {
        return samples;
    }

    std::vector<std::complex<float>> even;
    std::vector<std::complex<float>> odd;
    even.reserve(N / 2.0f);
    odd.reserve(N / 2.0f);

    std::vector<std::complex<float>> result(N);

    for (std::size_t i = 0; i < N; i += 2) {
        even.push_back(samples[i]);
        odd.push_back(samples[i + 1]);
    }

    auto E = fft(even);
    auto O = fft(odd);

    for (std::size_t k = 0; k < N / 2; ++k) {
        float angle = -2.0 * std::numbers::pi * k / N;

        std::complex<float> w{std::cos(angle), std::sin(angle)};

        auto t = w * O[k];

        result[k] = E[k] + t;
        result[k + N / 2] = E[k] - t;
    }

    return result;
}
std::vector<std::complex<float>> fft(const std::vector<float>& samples) {
    std::vector<std::complex<float>> complexSamples;
    complexSamples.reserve(samples.size());

    for (float sample : samples) {
        complexSamples.emplace_back(sample, 0.f);
    }

    return fft(complexSamples);
}

void applyHannWindow(std::vector<float>& samples) {
    const std::size_t N = samples.size();

    if (N < 2) {
        return;
    }
    constexpr float pi = std::numbers::pi_v<float>;
    for (std::size_t n = 0; n < N; ++n) {
        samples[n] *= 0.5f * (1.f - std::cos(2.f * pi * n / N));
    }
}

std::vector<float> spectrumToDb(const std::vector<float>& amplitudes) {
    std::vector<float> result(amplitudes.size());
    for (std::size_t i = 0; i < amplitudes.size(); ++i) {
        result[i] = amplitudeToDbfs<float>(amplitudes[i]);
    }
    return result;
}

float binToFrequency(std::size_t bin, std::size_t fftSize, float sampleRate) {
    if (fftSize == 0) {
        return 0.0f;
    }

    if (bin > fftSize / 2) {
        return (sampleRate / 2.0f);
    }
    return static_cast<float>(bin) * sampleRate / static_cast<float>(fftSize);
}

float bandPeak(const std::vector<float>& amplitudes, std::size_t fftSize,
               float sampleRate, float minFrequency, float maxFrequency) {
    float maxAmplitude = 0.f;

    for (std::size_t i = 0; i < amplitudes.size(); ++i) {
        float frequency = binToFrequency(i, fftSize, sampleRate);
        if (frequency >= minFrequency && frequency < maxFrequency) {
            maxAmplitude = std::max(amplitudes[i], maxAmplitude);
        }
    }
    return maxAmplitude;
}

// attackTime = 0.02f ms
// releaseTime = 0.2f ms
float smoothEnvelope(float current, float target, float deltaTime,
                     float attackTime, float releaseTime) {
    float alpha = 0.f;
    if (target > current) {
        alpha = 1 - std::exp(-deltaTime / attackTime);
    } else {
        alpha = 1 - std::exp(-deltaTime / releaseTime);
    }
    return current + alpha * (target - current);
}

float amplitudeToIntensity(float amplitude, float minDb, float maxDb) {
    float intensity = 0.f;
    if (maxDb > minDb) {
        intensity = std::clamp(
            (amplitudeToDbfs<float>(amplitude) - minDb) / (maxDb - minDb), 0.f,
            1.f);
    }
    return intensity;
}

void updateAnalysisSmoothing(AudioAnalysisState& state, float deltaTime) {
    c2::audio::FrequencyBands& smoothedBands = state.result.smoothedBands;
    smoothedBands.bass = c2::audio::smoothEnvelope(
        smoothedBands.bass,
        c2::audio::amplitudeToIntensity(state.result.bands.bass,
                                        state.config.intensityDbRange.min,
                                        state.config.intensityDbRange.max),
        deltaTime, state.config.attackTime, state.config.releaseTime);
    smoothedBands.mid = c2::audio::smoothEnvelope(
        smoothedBands.mid,
        c2::audio::amplitudeToIntensity(state.result.bands.mid,
                                        state.config.intensityDbRange.min,
                                        state.config.intensityDbRange.max),
        deltaTime, state.config.attackTime, state.config.releaseTime);
    smoothedBands.treble = c2::audio::smoothEnvelope(
        smoothedBands.treble,
        c2::audio::amplitudeToIntensity(state.result.bands.treble,
                                        state.config.intensityDbRange.min,
                                        state.config.intensityDbRange.max),
        deltaTime, state.config.attackTime, state.config.releaseTime);
}
}  // namespace c2::audio
