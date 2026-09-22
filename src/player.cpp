#include <C2Core/c2_log.hpp>
#include <player.hpp>

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

    result = playSound(player.audio);
    if (result != MA_SUCCESS) {
        C2Core::Log::error("Can't start sound: %d", result);
        return false;
    }
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
        static_cast<float>(viewData.positionFrames / sampleRate);
    viewData.durationSeconds =
        static_cast<float>(viewData.durationFrames / sampleRate);
}

}  // namespace c2::audio
