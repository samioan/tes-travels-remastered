// M10 smoke test: PlayerMovement's position/facing math, cross-level
// boundary recentering, walkability gating, and fatigue cost -- both
// against small hand-built synthetic levels (for exact, deterministic
// coverage of the recentering arithmetic and the edge cases real
// generated data may never actually exercise) and against real hub +
// neighbor levels from M6's DungeonGenerator (to confirm the whole
// pipeline runs cleanly against real geometry, no crashes/exceptions).
#include <cstdio>
#include <map>
#include <string>

#include "assets/asset_root.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "player/player_movement.h"
#include "world/dungeon_generator.h"

namespace {

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

stormhold::GeneratedLevel MakeLevel(int number, int width, int height) {
    stormhold::GeneratedLevel level;
    level.number = number;
    level.width = width;
    level.height = height;
    level.tiles.assign(static_cast<size_t>(width), std::vector<uint8_t>(static_cast<size_t>(height), 0));
    return level;
}

void TestTurning() {
    std::printf("-- turning --\n");
    stormhold::PlayerState p;
    p.facing = 1;
    stormhold::PlayerMovement::LevelLookup neverCalled = [](int) -> stormhold::GeneratedLevel& {
        throw std::runtime_error("turning should never look up a level");
    };

    int expectedRight[4] = {2, 3, 4, 1};
    for (int i = 0; i < 4; i++) {
        stormhold::PlayerMovement::ComputeMoveTarget(p, 3, neverCalled);
        Expect(p.pendingFacing == expectedRight[i], "dir=3 facing sequence should be 1->2->3->4->1");
        Expect(p.pendingTileX == p.tileX && p.pendingTileY == p.tileY, "turning should not move position");
        p.facing = p.pendingFacing;
    }
    Expect(p.facing == 1, "4 turns should return to the original facing");

    int expectedLeft[4] = {4, 3, 2, 1};
    for (int i = 0; i < 4; i++) {
        stormhold::PlayerMovement::ComputeMoveTarget(p, 4, neverCalled);
        Expect(p.pendingFacing == expectedLeft[i], "dir=4 facing sequence should be 1->4->3->2->1");
        p.facing = p.pendingFacing;
    }
    Expect(p.facing == 1, "4 opposite turns should return to the original facing");
}

void TestStepWithinLevel() {
    std::printf("-- step within a level (no boundary) --\n");
    stormhold::GeneratedLevel level = MakeLevel(5, 35, 35);
    std::map<int, stormhold::GeneratedLevel> cache;
    cache[5] = level;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& { return cache.at(n); };

    struct Case {
        int facing;
        int dx, dy;  // expected forward-step delta
    };
    Case cases[4] = {{1, 0, -1}, {3, 0, 1}, {2, 1, 0}, {4, -1, 0}};

    for (const Case& c : cases) {
        stormhold::PlayerState p;
        p.currentLevel = 5;
        p.tileX = 17;
        p.tileY = 17;
        p.facing = static_cast<int8_t>(c.facing);
        p.coreStats[6] = 100;
        bool moved = stormhold::PlayerMovement::CommitMove(p, 1, lookup);
        Expect(moved, "forward step in an open level should succeed");
        Expect(p.tileX == 17 + c.dx && p.tileY == 17 + c.dy, "forward step should move exactly one tile per facing");
        Expect(p.facing == c.facing, "a plain step should not change facing");
        Expect(cache.at(5).visited, "CommitMove should mark the target level visited");

        // Backward step should undo it.
        bool movedBack = stormhold::PlayerMovement::CommitMove(p, 2, lookup);
        Expect(movedBack, "backward step should succeed");
        Expect(p.tileX == 17 && p.tileY == 17, "backward step should return to the starting tile");
    }
}

void TestHubToStandardCrossing() {
    std::printf("-- hub<->standard boundary recentering --\n");
    stormhold::GeneratedLevel hub = MakeLevel(1, 19, 19);
    hub.neighborNorth = 2;
    stormhold::GeneratedLevel standard = MakeLevel(2, 35, 35);
    standard.neighborSouth = 1;

    std::map<int, stormhold::GeneratedLevel> cache;
    cache[1] = hub;
    cache[2] = standard;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& { return cache.at(n); };

    stormhold::PlayerState p;
    p.currentLevel = 1;
    p.tileX = 9;
    p.tileY = 0;
    p.facing = 1;  // north
    stormhold::PlayerMovement::ComputeMoveTarget(p, 1, lookup);

    // Hand-derived: 19-wide hub centered within the 35-wide standard level
    // -> X offset (35-19)/2 = 8, so hub X=9 lands at standard X=17 -- which
    // also happens to match the standard levels' own fixed stairwell
    // X-coordinate (M6's CarveStairwayCorridor N/S doors are both at
    // X=17), a nice independent sanity check that this recentering formula
    // really does line up both levels' edges the way the game intends.
    Expect(p.pendingLevel == 2, "should cross into the hub's north neighbor");
    Expect(p.pendingTileX == 17, "hub->standard X should recenter to 17");
    Expect(p.pendingTileY == 34, "hub->standard Y should land on the neighbor's far edge (height-1)");
    Expect(p.crossingLevelBoundary, "crossingLevelBoundary should be true");
}

void TestStandardToStandardCrossing() {
    std::printf("-- standard<->standard boundary (no recentering) --\n");
    stormhold::GeneratedLevel a = MakeLevel(10, 35, 35);
    a.neighborEast = 11;
    stormhold::GeneratedLevel b = MakeLevel(11, 35, 35);
    b.neighborWest = 10;

    std::map<int, stormhold::GeneratedLevel> cache;
    cache[10] = a;
    cache[11] = b;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& { return cache.at(n); };

    stormhold::PlayerState p;
    p.currentLevel = 10;
    p.tileX = 34;
    p.tileY = 17;
    p.facing = 2;  // east
    stormhold::PlayerMovement::ComputeMoveTarget(p, 1, lookup);

    Expect(p.pendingLevel == 11, "should cross into the east neighbor");
    Expect(p.pendingTileX == 0, "standard->standard X should land at the opposite edge");
    Expect(p.pendingTileY == 17, "standard->standard Y should pass through unchanged (both 35x35)");
}

void TestWalkabilityGating() {
    std::printf("-- walkability gating --\n");
    stormhold::GeneratedLevel level = MakeLevel(5, 35, 35);
    level.tiles[18][17] = 1;  // wall, directly east of (17,17)
    std::map<int, stormhold::GeneratedLevel> cache;
    cache[5] = level;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& { return cache.at(n); };

    stormhold::PlayerState p;
    p.currentLevel = 5;
    p.tileX = 17;
    p.tileY = 17;
    p.facing = 2;  // east, straight into the wall
    int16_t fatigueBefore = p.coreStats[6] = 100;
    bool moved = stormhold::PlayerMovement::CommitMove(p, 1, lookup);
    Expect(!moved, "stepping into a wall tile should fail");
    Expect(p.tileX == 17 && p.tileY == 17, "position should not change on a blocked move");
    Expect(p.coreStats[6] == fatigueBefore, "fatigue should not be spent on a blocked move");
}

void TestEnteredLeftLevelZoneFinding() {
    std::printf("-- enteredNewLevelZone / leftLevelZone (a confirmed dead-code finding) --\n");
    stormhold::GeneratedLevel level = MakeLevel(5, 35, 35);
    level.tiles[17][17] = 32;  // player's own starting tile: blocked (unwalkable)
    std::map<int, stormhold::GeneratedLevel> cache;
    cache[5] = level;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& { return cache.at(n); };

    stormhold::PlayerState p;
    p.currentLevel = 5;
    p.tileX = 17;
    p.tileY = 17;
    p.facing = 2;  // step east onto a plain walkable tile
    p.coreStats[6] = 100;
    bool moved = stormhold::PlayerMovement::CommitMove(p, 1, lookup);
    Expect(moved, "should be able to step off an unwalkable starting tile onto a walkable one");
    Expect(p.enteredNewLevelZone, "enteredNewLevelZone should be true (old unwalkable, new walkable)");
    Expect(!p.leftLevelZone,
           "leftLevelZone should be false here -- and, per CommitMove's own header comment, is UNREACHABLE from "
           "ANY successful CommitMove call: the method already returns false before this point whenever the "
           "target tile isn't walkable, so newWalkable is always true when leftLevelZone is computed");
}

void TestNoNeighborThrows() {
    std::printf("-- no-neighbor edge (believed-unreachable landmine, guarded) --\n");
    stormhold::GeneratedLevel level = MakeLevel(5, 35, 35);  // neighborNorth left at 0 (no neighbor)
    std::map<int, stormhold::GeneratedLevel> cache;
    cache[5] = level;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& { return cache.at(n); };

    stormhold::PlayerState p;
    p.currentLevel = 5;
    p.tileX = 17;
    p.tileY = 0;
    p.facing = 1;  // north, straight off the map edge
    bool threw = false;
    try {
        stormhold::PlayerMovement::ComputeMoveTarget(p, 1, lookup);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Expect(threw, "crossing an edge with no neighbor should throw rather than silently misbehave");
}

void TestFatigueCost() {
    std::printf("-- fatigue cost + ailment multiplier --\n");
    stormhold::GeneratedLevel level = MakeLevel(5, 35, 35);
    std::map<int, stormhold::GeneratedLevel> cache;
    cache[5] = level;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& { return cache.at(n); };

    stormhold::PlayerState p;
    p.currentLevel = 5;
    p.tileX = 17;
    p.tileY = 17;
    p.facing = 2;
    p.coreStats[6] = 5;
    stormhold::PlayerMovement::CommitMove(p, 1, lookup);
    Expect(p.coreStats[6] == 4, "a plain step should cost 1 fatigue with no ailment");

    p.ailmentMask = 1;  // bit 0: Stone Blood
    stormhold::PlayerMovement::CommitMove(p, 2, lookup);
    Expect(p.coreStats[6] == 1, "a step under ailment bit 0 should cost 3 fatigue (4 - 3 = 1)");

    stormhold::PlayerMovement::CommitMove(p, 1, lookup);
    Expect(p.coreStats[6] == 0, "fatigue should clamp at 0, not go negative");

    p.ailmentMask = 0;
    bool moved = stormhold::PlayerMovement::CommitMove(p, 2, lookup);
    Expect(!moved, "no move should succeed once fatigue is depleted");
}

void TestStrafe() {
    std::printf("-- strafe (turn, step, turn back) --\n");
    stormhold::GeneratedLevel level = MakeLevel(5, 35, 35);
    std::map<int, stormhold::GeneratedLevel> cache;
    cache[5] = level;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& { return cache.at(n); };

    stormhold::PlayerState p;
    p.currentLevel = 5;
    p.tileX = 17;
    p.tileY = 17;
    p.facing = 1;  // north
    p.coreStats[6] = 100;
    stormhold::PlayerMovement::Move(p, 3, /*strafe=*/true, lookup);  // strafe right

    // Facing north, turning right (dir 3) faces east, stepping forward
    // moves +X, turning back restores facing to north.
    Expect(p.tileX == 18 && p.tileY == 17, "strafe-right should displace exactly one tile east");
    Expect(p.facing == 1, "strafe should restore the original facing after the turn-back");
}

void TestAgainstRealData(const std::string& root) {
    std::printf("-- real hub + neighbor data (M6's DungeonGenerator) --\n");
    stormhold::AssetRoot assets(root);
    stormhold::ItemDatabase items = stormhold::ItemDatabase::Load(assets);
    stormhold::MonsterDatabase monsters = stormhold::MonsterDatabase::Load(assets);
    stormhold::DungeonGeometry geometry = stormhold::DungeonGeometry::Load(assets);

    std::map<int, stormhold::GeneratedLevel> cache;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& {
        auto it = cache.find(n);
        if (it != cache.end()) return it->second;
        stormhold::GeneratedLevel level = (n == 1) ? stormhold::DungeonGenerator::BuildHubLevel(geometry.rows[0])
                                                     : stormhold::DungeonGenerator::PopulateLevel(
                                                           n, geometry.rows[n - 1], items, monsters);
        return cache.emplace(n, std::move(level)).first->second;
    };

    stormhold::PlayerState p;  // hub spawn: level 1, (9, 10), facing 1 (M9's own defaults)
    p.coreStats[6] = 200;

    int succeeded = 0, failed = 0, thrown = 0;
    int dirs[] = {1, 3, 3, 1, 2, 4, 1, 1, 3, 1};
    for (int dir : dirs) {
        try {
            bool moved = stormhold::PlayerMovement::Move(p, dir, /*strafe=*/false, lookup);
            if (moved) {
                succeeded++;
            } else {
                failed++;
            }
        } catch (const std::exception& e) {
            thrown++;
            std::printf("  note: move threw: %s\n", e.what());
        }
    }
    std::printf("  ended at level=%d (%d,%d) facing=%d -- %d succeeded, %d blocked, %d threw\n", p.currentLevel,
                p.tileX, p.tileY, p.facing, succeeded, failed, thrown);
    Expect(thrown == 0, "a short real-data walk from spawn should never hit the no-neighbor guard");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        TestTurning();
        TestStepWithinLevel();
        TestHubToStandardCrossing();
        TestStandardToStandardCrossing();
        TestWalkabilityGating();
        TestEnteredLeftLevelZoneFinding();
        TestNoNeighborThrows();
        TestFatigueCost();
        TestStrafe();
        TestAgainstRealData(root);

        if (!g_ok) {
            std::fprintf(stderr, "m10_player_movement_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m10_player_movement_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
