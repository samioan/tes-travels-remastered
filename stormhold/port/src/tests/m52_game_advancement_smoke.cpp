// M52 smoke test: GameAdvancement::Level/OpenZone (ESGame.
// getGameAdvancementLevel()/checkOpenAndPopulateDungeons(), the dungeon
// zone-gating system world/dungeon_generator.h's own GeneratedLevel::
// populated header comment had flagged as unmodeled since M6/M10), plus
// the real GeneratedLevel::populated defaults (world/dungeon_generator.h)
// and the real PlayerMovement::CommitMove gate (player/player_movement.h)
// this milestone finally wires them both into.
#include <cstdio>
#include <map>
#include <stdexcept>
#include <string>

#include "assets/asset_root.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "player/player_movement.h"
#include "world/dungeon_generator.h"
#include "world/game_advancement.h"

namespace {

using namespace stormhold;

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

GeneratedLevel MakeLevel(int number, bool populated, int width = 35, int height = 35) {
    GeneratedLevel level;
    level.number = number;
    level.width = width;
    level.height = height;
    level.tiles.assign(static_cast<size_t>(width), std::vector<uint8_t>(static_cast<size_t>(height), 0));
    level.populated = populated;
    return level;
}

void TestLevelBuckets() {
    std::printf("-- GameAdvancement::Level's own 0-8 bucket boundaries --\n");
    // Transcribed directly from ESGame.getGameAdvancementLevel()'s own
    // cascading `< N` thresholds: 9/13/17/23/28/34/40/48.
    struct Case {
        int giftPoints;
        int expected;
    };
    Case cases[] = {
        {0, 0},  {8, 0},   {9, 1},   {12, 1},  {13, 2},  {16, 2},  {17, 3},  {22, 3},
        {23, 4}, {27, 4},  {28, 5},  {33, 5},  {34, 6},  {39, 6},  {40, 7},  {47, 7},
        {48, 8}, {100, 8},
    };
    for (const Case& c : cases) {
        Expect(GameAdvancement::Level(c.giftPoints) == c.expected, "a giftPoints value should map to its own zone");
    }
}

void TestOpenZoneOpensExactlyItsOwnLevels() {
    std::printf("-- GameAdvancement::OpenZone opens ONLY its own zone's levels --\n");
    // Every level this test ever asks about, all starting closed (the
    // real standard-level default since M52 -- see
    // GeneratedLevel::populated's own header comment).
    std::map<int, GeneratedLevel> cache;
    for (int n : {2, 3, 4, 5, 6, 7, 8, 11, 12, 13, 20, 21, 22, 29, 30, 31}) {
        cache.emplace(n, MakeLevel(n, /*populated=*/false));
    }
    GameAdvancement::LevelLookup lookup = [&cache](int n) -> GeneratedLevel& { return cache.at(n); };

    // Zone 1 == {5, 6, 7} (ESGame's own zoneLevels[1]).
    GameAdvancement::OpenZone(1, lookup);
    Expect(cache.at(5).populated && cache.at(6).populated && cache.at(7).populated,
           "every level belonging to zone 1 should now be populated");
    Expect(!cache.at(2).populated && !cache.at(8).populated,
           "a level belonging to a DIFFERENT zone should be untouched");

    // Zone 0 == {2, 3, 4, 11, 12, 13, 20, 21, 22, 29, 30, 31} (ESGame's
    // own zoneLevels[0] -- the "starter" zone opened at New Game
    // creation).
    GameAdvancement::OpenZone(0, lookup);
    for (int n : {2, 3, 4, 11, 12, 13, 20, 21, 22, 29, 30, 31}) {
        Expect(cache.at(n).populated, "every level belonging to zone 0 should now be populated too");
    }
    Expect(cache.at(5).populated, "zone 1's own earlier OpenZone call should still hold -- opening is additive");
}

void TestOpenZoneOutOfRangeThrows() {
    std::printf("-- GameAdvancement::OpenZone(9, ...) throws, matching a Java ArrayIndexOutOfBoundsException --\n");
    GameAdvancement::LevelLookup neverCalled = [](int) -> GeneratedLevel& {
        throw std::runtime_error("an out-of-range zone should never look up a single level");
    };
    bool threw = false;
    try {
        GameAdvancement::OpenZone(9, neverCalled);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    Expect(threw, "zone 9 (there are only 9 zones, 0-8) should throw std::out_of_range");
}

void TestCommitMoveGatesOnPopulated(const ItemDatabase& items) {
    std::printf("-- PlayerMovement::CommitMove: real Player.commitMove() `!target.populated` gate --\n");
    MonsterDatabase monsters{};
    WardenState warden{};

    // Same-level step into an UNPOPULATED current level -- the confirmed
    // "can't even move in place" softlock shape this milestone's own
    // GameSave::Load header-comment note documents (see player/
    // player_movement.h's own CommitMove declaration comment).
    {
        GeneratedLevel level = MakeLevel(5, /*populated=*/false);
        std::map<int, GeneratedLevel> cache;
        cache[5] = level;
        PlayerMovement::LevelLookup lookup = [&cache](int n) -> GeneratedLevel& { return cache.at(n); };
        WorldRegistry world(37);

        PlayerState p;
        p.currentLevel = 5;
        p.tileX = 17;
        p.tileY = 17;
        p.facing = 1;
        p.coreStats[6] = 100;

        bool moved = PlayerMovement::CommitMove(p, 1, lookup, world, items, monsters, warden);
        Expect(!moved, "a step within an UNPOPULATED level should fail, exactly like hitting a wall");
        Expect(p.tileX == 17 && p.tileY == 17, "a blocked step should leave the player's own position untouched");
    }

    // Cross-level step INTO an unpopulated neighbor.
    {
        GeneratedLevel hub = MakeLevel(1, /*populated=*/true, 19, 19);
        hub.neighborNorth = 2;
        std::map<int, GeneratedLevel> cache;
        cache[1] = hub;
        cache.emplace(2, MakeLevel(2, /*populated=*/false));
        // Lazy: GameAdvancement::OpenZone(0, ...) below touches EVERY
        // level in zone 0's own list (12 levels), not just level 2 --
        // matching TestOpenZoneOpensExactlyItsOwnLevels's own real
        // behavior. On-demand creation (rather than hand-listing all 12)
        // avoids this test needing its own copy of the zone table.
        PlayerMovement::LevelLookup lookup = [&cache](int n) -> GeneratedLevel& {
            auto it = cache.find(n);
            if (it != cache.end()) return it->second;
            return cache.emplace(n, MakeLevel(n, /*populated=*/false)).first->second;
        };
        WorldRegistry world(37);

        PlayerState p;
        p.currentLevel = 1;
        p.tileX = 9;
        p.tileY = 0;
        p.facing = 1;  // north -- steps toward y<0, crossing into neighborNorth
        p.coreStats[6] = 100;

        bool moved = PlayerMovement::CommitMove(p, 1, lookup, world, items, monsters, warden);
        Expect(!moved, "crossing into an unpopulated neighbor level should fail");
        Expect(p.currentLevel == 1, "a blocked cross-level step should leave currentLevel untouched");

        // Now open it for real via GameAdvancement -- level 2 belongs to
        // zone 0 -- and confirm the SAME move now succeeds.
        GameAdvancement::OpenZone(0, lookup);
        moved = PlayerMovement::CommitMove(p, 1, lookup, world, items, monsters, warden);
        Expect(moved, "the same move should succeed once GameAdvancement::OpenZone has opened that level");
        Expect(p.currentLevel == 2, "the player should have actually crossed into level 2");
    }
}

void TestGeneratorDefaults(const std::string& root) {
    std::printf("-- DungeonGenerator's own real populated defaults (hub true, standard false) --\n");
    AssetRoot assets(root);
    ItemDatabase items = ItemDatabase::Load(assets);
    MonsterDatabase monsters = MonsterDatabase::Load(assets);
    DungeonGeometry geometry = DungeonGeometry::Load(assets);

    GeneratedLevel hub = DungeonGenerator::BuildHubLevel(geometry.rows[0]);
    Expect(hub.populated, "BuildHubLevel's own output should start populated=true, matching the hub's real "
                           "unconditional constructor");

    GeneratedLevel standard = DungeonGenerator::PopulateLevel(2, geometry.rows[1], items, monsters);
    Expect(!standard.populated, "PopulateLevel's own output should start populated=false, matching a standard "
                                 "level's real constructor default -- only GameAdvancement::OpenZone ever flips it");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assets(root);
        ItemDatabase items = ItemDatabase::Load(assets);

        TestLevelBuckets();
        TestOpenZoneOpensExactlyItsOwnLevels();
        TestOpenZoneOutOfRangeThrows();
        TestCommitMoveGatesOnPopulated(items);
        TestGeneratorDefaults(root);

        if (!g_ok) {
            std::fprintf(stderr, "m52_game_advancement_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m52_game_advancement_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
