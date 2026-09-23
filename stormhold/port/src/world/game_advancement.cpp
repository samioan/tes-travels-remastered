#include "world/game_advancement.h"

#include <array>
#include <vector>

namespace stormhold {

namespace {

// ESGame.java's own private static `zoneLevels` table -- zone index (0-8,
// GameAdvancement::Level's own return range) -> its member dungeon level
// numbers, transcribed verbatim. Level 1 (the hub) belongs to no zone --
// BuildHubLevel sets its own `populated=true` unconditionally, matching
// Dungeon's own hub constructor exactly (see GeneratedLevel::populated's
// header comment).
const std::vector<std::vector<int>>& ZoneLevels() {
    static const std::vector<std::vector<int>> table = {
        {2, 3, 4, 11, 12, 13, 20, 21, 22, 29, 30, 31},
        {5, 6, 7},
        {23, 24, 25},
        {14, 15, 16},
        {8, 9, 10},
        {32, 33, 34},
        {26, 27, 28},
        {17, 18, 19},
        {35, 36, 37},
    };
    return table;
}

}  // namespace

int GameAdvancement::Level(int giftPoints) {
    if (giftPoints < 9) return 0;
    if (giftPoints < 13) return 1;
    if (giftPoints < 17) return 2;
    if (giftPoints < 23) return 3;
    if (giftPoints < 28) return 4;
    if (giftPoints < 34) return 5;
    if (giftPoints < 40) return 6;
    return giftPoints < 48 ? 7 : 8;
}

void GameAdvancement::OpenZone(int zone, const LevelLookup& levels) {
    // .at() rather than []: the real `zoneLevels[gameAdvLevel]` is a bare
    // Java array index too, and an out-of-range `zone` (Level() itself
    // can never produce one, 0-8 always) would throw an
    // ArrayIndexOutOfBoundsException there -- .at()'s own out_of_range
    // throw is the closer C++ match, not a silent OOB read.
    for (int levelNumber : ZoneLevels().at(static_cast<size_t>(zone))) {
        levels(levelNumber).populated = true;
    }
}

}  // namespace stormhold
