// M13 smoke test: PlayerMovement against the real generated world (M6's
// DungeonGenerator + M9's DungeonView). No JVM ground truth is available
// here either (same ESGame/stub-jar block as M6/M9/M11), so this checks
// hand-derived expectations from ../../../src/Player.java's source
// directly, the same methodology M12 used for its traitor-packing bug:
//  - isWalkable()'s bit tests, checked against each documented bit alone.
//  - Turning (direction 3/4) wraps 1<->4 correctly and never moves or
//    costs fatigue.
//  - Fatigue<=0 blocks every direction, including turns.
//  - A successful forward step costs exactly 1*fatigueCostMultiplier()
//    fatigue and updates prevTileX/Y; a blocked step costs none.
//  - Strafing (direction 3/4 with strafe=true) always returns to the
//    original facing (turn, step, turn-back -- turning itself is never
//    blocked by walls, only by fatigue<=0, which the test avoids).
//  - Crossing a hub-level (19x19) border into a neighbor (35x35) level
//    applies the exact recentering formula computeMoveTarget() uses,
//    hand-derived here independently and compared against the port's
//    output -- found by scanning the hub's real border tiles for a
//    walkable exit in each of the 4 directions rather than assuming
//    hardcoded coordinates.
#include <cstdio>
#include <string>
#include <vector>

#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "player/player_movement.h"
#include "player/player_state.h"
#include "world/dungeon_generator.h"
#include "world/dungeon_view.h"

namespace {

using dawnstar::GeneratedLevel;
using dawnstar::PlayerMovement;
using dawnstar::PlayerState;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

// Finds a walkable tile on `level`'s border facing outward in
// `direction` (1=N,2=E,3=S,4=W) whose geomin.dat neighbor in that
// direction is real (>0). Returns false if none exists.
bool FindEdgeExit(const GeneratedLevel& level, int direction, int* outX, int* outY) {
    if (direction == 1 && level.neighborNorth > 0) {
        for (int x = 0; x < level.width; x++) {
            if (PlayerMovement::IsWalkable(level.tiles[static_cast<size_t>(x)][0])) {
                *outX = x;
                *outY = 0;
                return true;
            }
        }
    } else if (direction == 3 && level.neighborSouth > 0) {
        for (int x = 0; x < level.width; x++) {
            if (PlayerMovement::IsWalkable(level.tiles[static_cast<size_t>(x)][static_cast<size_t>(level.height - 1)])) {
                *outX = x;
                *outY = level.height - 1;
                return true;
            }
        }
    } else if (direction == 2 && level.neighborEast > 0) {
        for (int y = 0; y < level.height; y++) {
            if (PlayerMovement::IsWalkable(level.tiles[static_cast<size_t>(level.width - 1)][static_cast<size_t>(y)])) {
                *outX = level.width - 1;
                *outY = y;
                return true;
            }
        }
    } else if (direction == 4 && level.neighborWest > 0) {
        for (int y = 0; y < level.height; y++) {
            if (PlayerMovement::IsWalkable(level.tiles[0][static_cast<size_t>(y)])) {
                *outX = 0;
                *outY = y;
                return true;
            }
        }
    }
    return false;
}

// Finds any interior (non-border) walkable tile, for tests that don't
// care about crossing a level boundary.
bool FindInteriorWalkable(const GeneratedLevel& level, int* outX, int* outY) {
    for (int x = 1; x < level.width - 1; x++) {
        for (int y = 1; y < level.height - 1; y++) {
            if (PlayerMovement::IsWalkable(level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)])) {
                *outX = x;
                *outY = y;
                return true;
            }
        }
    }
    return false;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        dawnstar::MonsterDatabase monsters = dawnstar::MonsterDatabase::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);

        std::vector<GeneratedLevel> levels;
        for (size_t i = 0; i < geometry.rows.size(); i++) {
            int levelNumber = static_cast<int>(i) + 1;
            levels.push_back(levelNumber == 1 ? dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[i])
                                               : dawnstar::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[i],
                                                                                            items, monsters));
        }
        std::printf("generated %zu levels\n", levels.size());

        // --- isWalkable ---
        Check(PlayerMovement::IsWalkable(0) == true, "isWalkable(0) should be walkable");
        Check(PlayerMovement::IsWalkable(1) == false, "isWalkable: wall bit blocks");
        Check(PlayerMovement::IsWalkable(2) == false, "isWalkable: monster bit blocks");
        Check(PlayerMovement::IsWalkable(32) == false, "isWalkable: blocked bit blocks");
        Check(PlayerMovement::IsWalkable(4) == true, "isWalkable: dropped-item bit alone doesn't block");
        Check(PlayerMovement::IsWalkable(8) == true, "isWalkable: no-spawn bit alone doesn't block");
        Check(PlayerMovement::IsWalkable(16) == true, "isWalkable: chest bit alone doesn't block");
        Check(PlayerMovement::IsWalkable(64) == true, "isWalkable: edge-marker bit alone doesn't block");

        // --- turning ---
        {
            PlayerState p;
            p.currentLevel = 1;
            p.tileX = 9;
            p.tileY = 9;
            p.facing = 1;
            p.coreStats[6] = 100;

            int expected[5] = {1, 2, 3, 4, 1};
            for (int i = 0; i < 4; i++) {
                bool moved = PlayerMovement::Move(p, 3, false, levels);
                Check(moved, "turn right should always succeed with fatigue available");
                Check(p.facing == expected[i + 1], "turn right facing sequence");
                Check(p.tileX == 9 && p.tileY == 9, "turning must not move position");
            }
            Check(p.coreStats[6] == 100, "turning must not cost fatigue");

            p.facing = 1;
            int expectedLeft[5] = {1, 4, 3, 2, 1};
            for (int i = 0; i < 4; i++) {
                PlayerMovement::Move(p, 4, false, levels);
                Check(p.facing == expectedLeft[i + 1], "turn left facing sequence");
            }
        }

        // --- fatigue<=0 blocks everything ---
        {
            PlayerState p;
            p.currentLevel = 1;
            p.tileX = 9;
            p.tileY = 9;
            p.facing = 1;
            p.coreStats[6] = 0;
            for (int dir = 1; dir <= 4; dir++) {
                bool moved = PlayerMovement::Move(p, dir, false, levels);
                Check(!moved, "zero fatigue must block every direction");
                Check(p.tileX == 9 && p.tileY == 9 && p.facing == 1, "zero-fatigue attempt must not change state");
            }
        }

        // --- forward step: cost, prevTile, corridorView ---
        {
            int x = 0, y = 0;
            Check(FindInteriorWalkable(levels[0], &x, &y), "need an interior walkable hub tile");

            PlayerState p;
            p.currentLevel = 1;
            p.tileX = x;
            p.tileY = y;
            p.facing = 1;  // facing north: forward decreases tileY
            p.coreStats[6] = 50;

            bool northWalkable = PlayerMovement::IsWalkable(levels[0].tiles[static_cast<size_t>(x)][static_cast<size_t>(y - 1)]);
            bool moved = PlayerMovement::Move(p, 1, false, levels);
            if (northWalkable) {
                Check(moved, "forward step onto a walkable tile should succeed");
                Check(p.tileX == x && p.tileY == y - 1, "forward step should move north (tileY-1)");
                Check(p.prevTileX == x && p.prevTileY == y, "prevTileX/Y should record the pre-move tile");
                Check(p.coreStats[6] == 49, "a successful forward step should cost exactly 1 fatigue (no ailment)");

                dawnstar::DungeonView view(levels[0]);
                uint8_t selfTile = view.TileAt(p.tileX, p.tileY);
                Check(p.corridorView[1][0] == selfTile, "corridorView[1][0] should be the tile under the player");
            } else {
                Check(!moved, "forward step into a wall should fail");
                Check(p.tileX == x && p.tileY == y, "a blocked step must not move the player");
                Check(p.coreStats[6] == 50, "a blocked step must not cost fatigue");
            }
        }

        // --- strafing always returns to the original facing ---
        {
            int x = 0, y = 0;
            Check(FindInteriorWalkable(levels[0], &x, &y), "need an interior walkable hub tile");

            for (int dir : {3, 4}) {
                PlayerState p;
                p.currentLevel = 1;
                p.tileX = x;
                p.tileY = y;
                p.facing = 1;
                p.coreStats[6] = 50;
                PlayerMovement::Move(p, dir, true, levels);
                Check(p.facing == 1, "strafing must restore the original facing (turns are never wall-blocked)");
                Check(p.currentLevel == 1, "strafing on an interior tile must not change level");
            }
        }

        // --- cross-level boundary + hub<->standard-level recentering ---
        int crossed = 0;
        for (int dir = 1; dir <= 4; dir++) {
            int x = 0, y = 0;
            if (!FindEdgeExit(levels[0], dir, &x, &y)) continue;
            crossed++;

            int expectedNeighbor = dir == 1   ? levels[0].neighborNorth
                                    : dir == 2 ? levels[0].neighborEast
                                    : dir == 3 ? levels[0].neighborSouth
                                                : levels[0].neighborWest;
            const GeneratedLevel& target = levels[static_cast<size_t>(expectedNeighbor - 1)];

            int expectedX = x, expectedY = y;
            if (dir == 1) {
                expectedX = x + (target.width - levels[0].width) / 2;
                expectedY = target.height - 1;
            } else if (dir == 3) {
                expectedX = x + (target.width - levels[0].width) / 2;
                expectedY = 0;
            } else if (dir == 2) {
                expectedX = 0;
                expectedY = y + (target.height - levels[0].height) / 2;
            } else if (dir == 4) {
                expectedX = target.width - 1;
                expectedY = y + (target.height - levels[0].height) / 2;
            }

            PlayerState p;
            p.currentLevel = 1;
            p.tileX = x;
            p.tileY = y;
            p.facing = dir;
            p.coreStats[6] = 50;

            bool targetWalkable =
                expectedX >= 0 && expectedX < target.width && expectedY >= 0 && expectedY < target.height &&
                PlayerMovement::IsWalkable(target.tiles[static_cast<size_t>(expectedX)][static_cast<size_t>(expectedY)]);

            bool moved = PlayerMovement::Move(p, 1, false, levels);
            std::printf("  hub edge dir=%d exit=(%d,%d) -> level %d expected=(%d,%d) targetWalkable=%d moved=%d\n", dir,
                        x, y, expectedNeighbor, expectedX, expectedY, targetWalkable, moved);

            if (targetWalkable) {
                Check(moved, "crossing into a walkable target tile should succeed");
                Check(p.currentLevel == expectedNeighbor, "crossing should land on the geomin.dat neighbor level");
                Check(p.tileX == expectedX && p.tileY == expectedY,
                      "crossing should land at the hand-derived recentered coordinate");
            } else {
                Check(!moved, "crossing into a non-walkable target tile should fail");
                Check(p.currentLevel == 1 && p.tileX == x && p.tileY == y, "a failed crossing must not change state");
            }
        }
        std::printf("tested %d of 4 possible hub border crossings (some directions may have no exit)\n", crossed);
        Check(crossed > 0, "expected at least one hub border direction to have a real exit");

        if (!g_ok) {
            std::fprintf(stderr, "m13_player_movement_smoke: FAILED\n");
            return 1;
        }
        std::printf("all movement checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m13_player_movement_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
