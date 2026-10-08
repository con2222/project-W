#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <player.hpp>
#include <random>
#include <ranges>
#include <vector>

TEST_CASE("Shuffle queue logic works", "[shuffle]") {
    using namespace c2::audio;

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