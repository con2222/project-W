#include <C2Core/c2_log.hpp>
#include <audio.hpp>

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
    ma_result result = ma_sound_start(&audio.sound);
    return result;
}

ma_result pauseSound(AudioState& audio) {
    ma_result result = ma_sound_stop(&audio.sound);
    return result;
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

}  // namespace c2::audio
