#include <taglib/audioproperties.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>

#include <C2Core/c2_log.hpp>
#include <algorithm>
#include <cctype>
#include <iostream>
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

std::vector<Track> scanDirectory(const std::string& directoryPath) {
    std::vector<Track> tracks;
    namespace fs = std::filesystem;

    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        return tracks;
    }

    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            if (ext == ".mp3" || ext == ".wav" || ext == ".flac") {
                TagLib::FileRef f(entry.path().string().c_str());
                if (f.isNull()) {
                    std::cerr << "Error: Failed to open the file or the format "
                                 "is unsupported: "
                              << entry.path().string() << '\n';
                    continue;
                }
                if (f.tag()) {
                    Track track;
                    TagLib::Tag* tag = f.tag();
                    track.filepath = entry.path().string();
                    track.artist = tag->artist().to8Bit(true) != ""
                                       ? tag->artist().to8Bit(true)
                                       : "Undefined";

                    track.title = tag->title().to8Bit(true) != ""
                                      ? tag->title().to8Bit(true)
                                      : entry.path().filename().string();

                    track.duration = f.audioProperties()->lengthInSeconds();

                    tracks.push_back(track);
                }
            }
        }
    }
    return tracks;
}

void printAudioMetadata(const std::string& filePath) {
    // 1. Create a FileRef object.
    // It automatically detects the format (MP3, FLAC, etc.) and parses the
    // headers.
    TagLib::FileRef f(filePath.c_str());

    // Check if the file was successfully opened and the format is supported
    if (f.isNull()) {
        std::cerr
            << "Error: Failed to open the file or the format is unsupported: "
            << filePath << std::endl;
        return;
    }

    std::cout << "========================================\n";
    std::cout << "File: " << filePath << "\n";

    // 2. Read text metadata (Tags)
    if (f.tag()) {
        TagLib::Tag* tag = f.tag();

        // IMPORTANT: TagLib returns strings in its own TagLib::String class.
        // The to8Bit(true) method converts them to a standard std::string
        // (in UTF-8 encoding). The true flag enforces strict UTF-8 conversion,
        // which is ideal for Dear ImGui.

        std::cout << "--- Metadata ---\n";
        std::cout << "Title      : " << tag->title().to8Bit(true) << "\n";
        std::cout << "Artist     : " << tag->artist().to8Bit(true) << "\n";
        std::cout << "Album      : " << tag->album().to8Bit(true) << "\n";
        std::cout << "Genre      : " << tag->genre().to8Bit(true) << "\n";
        std::cout << "Comment    : " << tag->comment().to8Bit(true) << "\n";
        std::cout << "Year       : " << tag->year() << "\n";
        std::cout << "Track      : " << tag->track() << "\n";
    } else {
        std::cout << "No metadata tags found in the file.\n";
    }

    // 3. Read technical properties (Audio properties)
    if (f.audioProperties()) {
        TagLib::AudioProperties* properties = f.audioProperties();

        std::cout << "--- Audio Properties ---\n";
        // Length in seconds (useful for a progress bar in the player)
        std::cout << "Length     : " << properties->lengthInSeconds()
                  << " sec.\n";
        std::cout << "Bitrate    : " << properties->bitrate() << " kbps\n";
        std::cout << "Sample Rate: " << properties->sampleRate() << " Hz\n";
        std::cout << "Channels   : " << properties->channels() << "\n";
    }
    std::cout << "========================================\n\n";
}
}  // namespace c2::audio
