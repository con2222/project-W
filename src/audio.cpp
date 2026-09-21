#include <C2Core/c2_log.hpp>
#include <audio.hpp>

namespace c2::audio {

ma_result initAudio(AudioState& audio) {
    ma_result result;
    audio.engine = new ma_engine;
    result = ma_engine_init(nullptr, audio.engine);
    return result;
}

void shutdownAudio(AudioState& audio) {
    ma_sound_uninit(&audio.sound);
    ma_engine_uninit(audio.engine);
    delete audio.engine;
}

ma_result initSoundFromFile(AudioState& audio, const std::string& filename) {
    ma_result result;
    result = ma_sound_init_from_file(audio.engine, filename.data(), 0, nullptr,
                                     nullptr, &audio.sound);
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

void soundSetVolume(AudioState& audio, float volumeValue) {
    ma_sound_set_volume(&audio.sound, volumeValue);
}

ma_result getLengthPCMFrames(AudioState& audio, ma_uint64& outValue) {
    return ma_sound_get_length_in_pcm_frames(&audio.sound, &outValue);
}

ma_result getCursorPCMFrames(AudioState& audio, ma_uint64& outValue) {
    return ma_sound_get_cursor_in_pcm_frames(&audio.sound, &outValue);
}
ma_result getSoundData(AudioState& audio, AudioFormatInfo& outData) {
    return ma_sound_get_data_format(&audio.sound, &outData.pFormat,
                                    &outData.pChannels, &outData.pSampleRate,
                                    outData.pChannelMap, MA_MAX_CHANNELS);
}

ma_result soundSeekToPCMFrame(AudioState& audio, ma_uint64 frameIndex) {
    return ma_sound_seek_to_pcm_frame(&audio.sound, frameIndex);
}

void uninitSound(AudioState& audio) { ma_sound_uninit(&audio.sound); }

}  // namespace c2::audio