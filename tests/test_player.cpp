#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <command.hpp>
#include <player.hpp>
#include <random>
#include <ranges>
#include <vector>

using namespace c2::audio;

TEST_CASE("Shuffle queue logic works", "[shuffle]") {
    PlayerState player{};
    player.params.isShuffle = true;

    SECTION("Shuffled vector has same element and size one") {
        player.playlist.tracks.push_back(Track());
        player.playlist.currentIndex = 0;

        generateShuffleQueue(player);

        REQUIRE(player.playlist.shuffleQueue.size() == 1);
        REQUIRE(player.playlist.shuffleQueue.back() == 0);
    }

    SECTION("Multiple operations on vector") {
        for (int i = 0; i < 5; i++) {
            player.playlist.tracks.push_back(Track());
        }

        player.playlist.currentIndex = 2;
        generateShuffleQueue(player);

        REQUIRE(player.playlist.shuffleQueue.size() == 5);
        REQUIRE(player.playlist.shuffleQueue[0] == 2);

        auto expectedIndices5 = std::views::iota(0u, 5u);
        REQUIRE(std::ranges::is_permutation(player.playlist.shuffleQueue,
                                            expectedIndices5));

        player.playlist.currentIndex = 4;
        generateShuffleQueue(player);

        REQUIRE(player.playlist.shuffleQueue[0] == 4);
        REQUIRE(std::ranges::is_permutation(player.playlist.shuffleQueue,
                                            expectedIndices5));

        player.playlist.currentIndex = 0;
        generateShuffleQueue(player);

        REQUIRE(player.playlist.shuffleQueue[0] == 0);
        REQUIRE(std::ranges::is_permutation(player.playlist.shuffleQueue,
                                            expectedIndices5));

        generateShuffleQueue(player);

        REQUIRE(std::ranges::is_permutation(player.playlist.shuffleQueue,
                                            expectedIndices5));

        player.playlist.tracks.resize(2);
        player.playlist.currentIndex = 1;
        generateShuffleQueue(player);

        REQUIRE(player.playlist.shuffleQueue.size() == 2);
        auto expectedIndices2 = std::views::iota(0u, 2u);
        REQUIRE(std::ranges::is_permutation(player.playlist.shuffleQueue,
                                            expectedIndices2));

        player.params.isShuffle = false;
        auto savedQueue = player.playlist.shuffleQueue;
        generateShuffleQueue(player);

        REQUIRE(player.playlist.shuffleQueue == savedQueue);

        player.params.isShuffle = true;
        player.playlist.currentIndex = -1;
        generateShuffleQueue(player);

        REQUIRE(player.playlist.shuffleQueue == savedQueue);

        player.playlist.currentIndex = player.playlist.tracks.size();
        generateShuffleQueue(player);

        REQUIRE(player.playlist.shuffleQueue == savedQueue);

        player.playlist.tracks.clear();
        generateShuffleQueue(player);

        REQUIRE(player.playlist.shuffleQueue == savedQueue);
    }
}

TEST_CASE("Seconds are converted to PCM frames within track bounds",
          "[player][seek]") {
    PlayerState player{};
    player.soundAudioFormat.pSampleRate = 48000;
    player.durationFrames = 96000;

    SECTION("Zero seconds maps to the start") {
        REQUIRE(convertSecondsToPSMFrames(player, 0.0) == 0);
    }

    SECTION("Half a second maps to 24000 frames") {
        REQUIRE(convertSecondsToPSMFrames(player, 0.5) == 24000);
    }

    SECTION("Two seconds maps to the end") {
        REQUIRE(convertSecondsToPSMFrames(player, 2.0) == 96000);
    }

    SECTION("Negative time is clamped to the start") {
        REQUIRE(convertSecondsToPSMFrames(player, -1.0) == 0);
    }

    SECTION("Time beyond the duration is clamped to the end") {
        REQUIRE(convertSecondsToPSMFrames(player, 3.0) == 96000);
    }
}

TEST_CASE("View data clears stale playback information when no sound is loaded",
          "[player][view]") {
    PlayerState player{};
    player.volume = 0.25f;

    PlayerViewData viewData{};
    viewData.isPlaying = true;
    viewData.atEnd = true;
    viewData.volume = 0.75f;
    viewData.positionSeconds = 1.0;
    viewData.durationSeconds = 2.0;
    viewData.positionFrames = 48000;
    viewData.durationFrames = 96000;

    updatePlayerViewData(player, viewData);

    REQUIRE_FALSE(viewData.isPlaying);
    REQUIRE_FALSE(viewData.atEnd);
    REQUIRE(viewData.positionSeconds == 0.0);
    REQUIRE(viewData.durationSeconds == 0.0);
    REQUIRE(viewData.positionFrames == 0);
    REQUIRE(viewData.durationFrames == 0);
    REQUIRE(viewData.volume == player.volume);
}

TEST_CASE("Playback operations fail when no sound is loaded",
          "[player][audio]") {
    PlayerState player{};

    SECTION("Play fails") {
        REQUIRE(playTrack(player) == MA_INVALID_OPERATION);
    }

    SECTION("Pause fails") {
        REQUIRE(pauseTrack(player) == MA_INVALID_OPERATION);
    }

    SECTION("Seek fails") {
        REQUIRE(seekTrack(player, 48000) == MA_INVALID_OPERATION);
    }
}

TEST_CASE("Selecting an invalid track index preserves player data",
          "[player][select]") {
    PlayerState player{};
    player.playlist.tracks = {
        {"First", "Artist A", "first.wav", 1},
        {"Second", "Artist B", "second.wav", 2},
        {"Third", "Artist C", "third.wav", 3},
    };
    player.playlist.currentIndex = 1;
    player.playlist.shuffleQueue = {1, 2, 0};
    player.playlist.shufflePosition = 1;
    player.params.isShuffle = true;
    player.params.isRepeat = true;
    player.volume = 0.25f;

    player.durationFrames = 96000;
    player.soundAudioFormat.pFormat = ma_format_f32;
    player.soundAudioFormat.pChannels = 2;
    player.soundAudioFormat.pSampleRate = 48000;

    const auto tracksBefore = player.playlist.tracks;
    const auto queueBefore = player.playlist.shuffleQueue;
    int invalidIndex = -1;

    SECTION("Negative index") { invalidIndex = -1; }

    SECTION("Index equal to the track count") { invalidIndex = 3; }

    SECTION("Index greater than the track count") { invalidIndex = 10; }

    REQUIRE_FALSE(selectTrack(player, invalidIndex));

    REQUIRE(player.playlist.currentIndex == 1);
    REQUIRE(player.playlist.shuffleQueue == queueBefore);
    REQUIRE(player.playlist.shufflePosition == 1);
    REQUIRE(player.params.isShuffle);
    REQUIRE(player.params.isRepeat);
    REQUIRE(player.volume == 0.25f);
    REQUIRE(player.durationFrames == 96000);
    REQUIRE(player.soundAudioFormat.pFormat == ma_format_f32);
    REQUIRE(player.soundAudioFormat.pChannels == 2);
    REQUIRE(player.soundAudioFormat.pSampleRate == 48000);
    REQUIRE_FALSE(player.audio.hasSound);
    REQUIRE(player.audio.engine == nullptr);

    REQUIRE(player.playlist.tracks.size() == tracksBefore.size());
    for (std::size_t i = 0; i < tracksBefore.size(); ++i) {
        REQUIRE(player.playlist.tracks[i].title == tracksBefore[i].title);
        REQUIRE(player.playlist.tracks[i].artist == tracksBefore[i].artist);
        REQUIRE(player.playlist.tracks[i].filepath == tracksBefore[i].filepath);
        REQUIRE(player.playlist.tracks[i].duration == tracksBefore[i].duration);
    }
}

TEST_CASE(
    "Track navigation rejects empty playlists and invalid current indices",
    "[player][navigation]") {
    PlayerState player{};
    player.playlist.tracks.resize(3);

    SECTION("Empty playlist") {
        player.playlist.tracks.clear();
        player.playlist.currentIndex = -1;
    }

    SECTION("No track is selected in a nonempty playlist") {
        player.playlist.currentIndex = -1;
    }

    SECTION("Current index equals the track count") {
        player.playlist.currentIndex = 3;
    }

    SECTION("Current index exceeds the track count") {
        player.playlist.currentIndex = 10;
    }

    const int indexBefore = player.playlist.currentIndex;

    REQUIRE_FALSE(nextTrack(player));
    REQUIRE(player.playlist.currentIndex == indexBefore);

    REQUIRE_FALSE(prevTrack(player));
    REQUIRE(player.playlist.currentIndex == indexBefore);
}

TEST_CASE("Repeat commands toggle the flag and are consumed",
          "[player][commands][repeat]") {
    PlayerState player{};
    std::vector<Command> commands;
    bool expectedRepeat = false;

    SECTION("One command enables repeat") {
        commands.push_back(Command{.command = CommandType::ToggleRepeat});
        expectedRepeat = true;
    }

    SECTION("One command disables repeat") {
        player.params.isRepeat = true;
        commands.push_back(Command{.command = CommandType::ToggleRepeat});
        expectedRepeat = false;
    }

    SECTION("Two commands preserve initially disabled repeat") {
        commands.push_back(Command{.command = CommandType::ToggleRepeat});
        commands.push_back(Command{.command = CommandType::ToggleRepeat});
        expectedRepeat = false;
    }

    SECTION("Two commands preserve initially enabled repeat") {
        player.params.isRepeat = true;
        commands.push_back(Command{.command = CommandType::ToggleRepeat});
        commands.push_back(Command{.command = CommandType::ToggleRepeat});
        expectedRepeat = true;
    }

    updatePlayer(player, commands);

    REQUIRE(player.params.isRepeat == expectedRepeat);
    REQUIRE(commands.empty());
}

TEST_CASE("Enabling shuffle anchors the queue to the selected track",
          "[player][commands][shuffle]") {
    PlayerState player{};
    player.playlist.tracks.resize(5);
    player.playlist.currentIndex = 2;

    std::vector<Command> commands{
        Command{.command = CommandType::ToggleShuffle},
    };

    updatePlayer(player, commands);

    REQUIRE(player.params.isShuffle);
    REQUIRE(player.playlist.currentIndex == 2);
    REQUIRE(player.playlist.shuffleQueue.size() == 5);
    REQUIRE(player.playlist.shuffleQueue.front() == 2);
    REQUIRE(player.playlist.shufflePosition == 1);
    REQUIRE(commands.empty());

    auto sortedQueue = player.playlist.shuffleQueue;
    std::ranges::sort(sortedQueue);

    const std::vector<int> expectedIndices{0, 1, 2, 3, 4};
    REQUIRE(sortedQueue == expectedIndices);
}

TEST_CASE("Updating with an empty command queue does not repeat old commands",
          "[player][commands]") {
    PlayerState player{};
    std::vector<Command> commands{
        Command{.command = CommandType::ToggleRepeat},
    };

    updatePlayer(player, commands);
    REQUIRE(player.params.isRepeat);
    REQUIRE(commands.empty());

    updatePlayer(player, commands);
    REQUIRE(player.params.isRepeat);
    REQUIRE(commands.empty());
}