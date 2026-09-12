// M23 smoke test: the three PlayerMovement/ComputeMoveTarget gaps M22's
// live registry unblocked -- dropped-item auto-loot on arrival
// (CommitMove), the instant-lethal-tile (bit 8) camp trigger
// (CommitMove, now really calling MarkCampAndReturnToTown instead of
// only committing position/facing), and the "remove roaming gehen on
// level change" cleanup (ComputeMoveTarget/ResetToHubPosition) -- against
// the real 37-level generated world (M6), the same methodology as
// M6/M9/M11/M13-M22 (no JVM ground truth available -- see
// player/player_movement.h's header comment for why that's still true
// here).
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "dungeon/dungeon_runtime.h"
#include "player/player_creation.h"
#include "player/player_movement.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::DungeonRuntime;
using dawnstar::GeneratedLevel;
using dawnstar::ItemDatabase;
using dawnstar::MonsterDatabase;
using dawnstar::PlayerMovement;
using dawnstar::PlayerState;
using dawnstar::WorldRegistry;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

bool IsPlainlyWalkable(const GeneratedLevel& level, int x, int y) {
    if (x < 0 || y < 0 || x >= level.width || y >= level.height) return false;
    uint8_t flags = level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)];
    return (flags & (1 | 2 | 4 | 8 | 16 | 32)) == 0;
}

bool FindInteriorWalkable(const GeneratedLevel& level, int* outX, int* outY) {
    for (int x = 1; x < level.width - 1; x++) {
        for (int y = 1; y < level.height - 1; y++) {
            if (IsPlainlyWalkable(level, x, y)) {
                *outX = x;
                *outY = y;
                return true;
            }
        }
    }
    return false;
}

// Finds (x,y) such that BOTH (x,y) and (x,y+1) are plainly walkable --
// i.e. a real approach the player can actually walk into (x,y) from,
// standing at (x,y+1) facing north (facing=1; forward decreases
// tileY -- y increases southward, see world/dungeon_view.h) -- for the
// auto-loot/lethal-tile tests below, which need the MOVE to actually
// succeed, not just the destination tile to be walkable in isolation.
bool FindWalkableApproachFromSouth(const GeneratedLevel& level, int* outX, int* outY) {
    for (int x = 1; x < level.width - 1; x++) {
        for (int y = 1; y < level.height - 2; y++) {
            if (IsPlainlyWalkable(level, x, y) && IsPlainlyWalkable(level, x, y + 1)) {
                *outX = x;
                *outY = y;
                return true;
            }
        }
    }
    return false;
}

bool FindEdgeExit(const GeneratedLevel& level, int direction, int* outX, int* outY) {
    if (direction == 1 && level.neighborNorth > 0) {
        for (int x = 0; x < level.width; x++) {
            if ((level.tiles[static_cast<size_t>(x)][0] & 1) == 0) {
                *outX = x;
                *outY = 0;
                return true;
            }
        }
    } else if (direction == 3 && level.neighborSouth > 0) {
        for (int x = 0; x < level.width; x++) {
            if ((level.tiles[static_cast<size_t>(x)][static_cast<size_t>(level.height - 1)] & 1) == 0) {
                *outX = x;
                *outY = level.height - 1;
                return true;
            }
        }
    } else if (direction == 2 && level.neighborEast > 0) {
        for (int y = 0; y < level.height; y++) {
            if ((level.tiles[static_cast<size_t>(level.width - 1)][static_cast<size_t>(y)] & 1) == 0) {
                *outX = level.width - 1;
                *outY = y;
                return true;
            }
        }
    } else if (direction == 4 && level.neighborWest > 0) {
        for (int y = 0; y < level.height; y++) {
            if ((level.tiles[0][static_cast<size_t>(y)] & 1) == 0) {
                *outX = 0;
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
        dawnstar::CharacterData charData = dawnstar::CharacterData::Load(archive);
        ItemDatabase items = ItemDatabase::Load(archive);
        MonsterDatabase monsterDb = MonsterDatabase::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);

        std::vector<GeneratedLevel> levels;
        for (size_t i = 0; i < geometry.rows.size(); i++) {
            int levelNumber = static_cast<int>(i) + 1;
            levels.push_back(levelNumber == 1
                                  ? dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[0])
                                  : dawnstar::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[i], items,
                                                                               monsterDb));
        }
        std::printf("generated %zu levels\n", levels.size());

        dawnstar::JavaRandom globalRng(20260912);

        // --- dropped-item auto-loot on arrival: full loot (all items
        // picked up, tile flag cleared) ---
        {
            WorldRegistry world(levels.size());
            int x = 0, y = 0;
            Check(FindWalkableApproachFromSouth(levels[1], &x, &y), "need an interior walkable tile on level 2");

            PlayerState p = dawnstar::PlayerCreation::CreateCharacter(0, "Test", charData, items, globalRng);
            int startingCount = p.inventoryCount;
            p.currentLevel = 2;
            p.tileX = x;
            p.tileY = y + 1;  // one step south of the target tile
            p.facing = 1;     // facing north: forward decreases tileY, landing on (x,y)
            p.coreStats[6] = 50;

            // A category-11 ("gift") item id, for the giftPointsFound
            // branch below.
            int giftItemId = -1;
            int itemCount = static_cast<int>(items.category.size());
            for (int id = 1; id <= itemCount; id++) {
                if (items.category[static_cast<size_t>(id - 1)] == 11) {
                    giftItemId = id;
                    break;
                }
            }
            Check(giftItemId > 0, "need a real category-11 item id to test the gift-points branch");

            GeneratedLevel& level = levels[1];
            std::array<uint8_t, 7> record = {static_cast<uint8_t>(x),
                                              static_cast<uint8_t>(y),
                                              static_cast<uint8_t>(giftItemId),
                                              0,
                                              7,
                                              0,
                                              0};  // flags byte 0 -> bit1 clear -> giftPointsFound SHOULD increment
            DungeonRuntime::AddDroppedItem(level, world, record);
            Check((level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] & 4) != 0,
                  "the dropped-item presence bit should be set before the move");

            int16_t giftBefore = p.giftPointsFound;
            bool moved = PlayerMovement::Move(p, 1, false, levels, world, items);
            Check(moved, "stepping onto the dropped-item tile should succeed");
            Check(p.inventoryCount == startingCount + 1, "the dropped item should be auto-added to inventory");
            Check(std::abs(static_cast<int>(p.inventoryItemIds[static_cast<size_t>(p.inventoryCount - 1)])) ==
                      giftItemId,
                  "the newly-added slot should hold the dropped item's id");
            Check(p.giftPointsFound > giftBefore,
                  "picking up a category-11 item with flags bit1 clear should increase giftPointsFound");

            auto remaining = DungeonRuntime::DroppedItemsAt(world, 1, x, y);
            Check(remaining.empty(), "the looted record should be removed from the registry");
            Check((level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] & 4) == 0,
                  "the tile's dropped-item presence bit should clear once every record there is looted");
        }

        // --- dropped-item auto-loot: a full inventory leaves the
        // record (and presence bit) in place ---
        {
            WorldRegistry world(levels.size());
            int x = 0, y = 0;
            Check(FindWalkableApproachFromSouth(levels[1], &x, &y), "need an interior walkable tile on level 2 (full-inventory case)");

            PlayerState p = dawnstar::PlayerCreation::CreateCharacter(0, "Test", charData, items, globalRng);
            p.currentLevel = 2;
            p.tileX = x;
            p.tileY = y + 1;
            p.facing = 1;
            p.coreStats[6] = 50;
            p.inventoryCount = 24;  // full

            GeneratedLevel& level = levels[1];
            std::array<uint8_t, 7> record = {static_cast<uint8_t>(x), static_cast<uint8_t>(y), 1, 0, 3, 0, 0};
            DungeonRuntime::AddDroppedItem(level, world, record);

            bool moved = PlayerMovement::Move(p, 1, false, levels, world, items);
            Check(moved, "stepping onto the tile should still succeed even with a full inventory");
            Check(p.inventoryCount == 24, "a full inventory must not gain a slot");

            auto remaining = DungeonRuntime::DroppedItemsAt(world, 1, x, y);
            Check(remaining.size() == 1, "an unlooted (inventory-full) record should stay in the registry");
            Check((level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] & 4) != 0,
                  "the presence bit should survive when not everything there was looted");
        }

        // --- instant-lethal tile (bit 8) really triggers
        // MarkCampAndReturnToTown(false) now ---
        {
            WorldRegistry world(levels.size());
            int x = 0, y = 0;
            Check(FindWalkableApproachFromSouth(levels[1], &x, &y), "need an interior walkable tile on level 2 (lethal-tile case)");

            GeneratedLevel& level = levels[1];
            level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] = static_cast<uint8_t>(
                level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] | 8);

            PlayerState p = dawnstar::PlayerCreation::CreateCharacter(0, "Test", charData, items, globalRng);
            p.currentLevel = 2;
            p.tileX = x;
            p.tileY = y + 1;
            p.facing = 1;
            p.coreStats[6] = 50;

            bool moved = PlayerMovement::Move(p, 1, false, levels, world, items);
            Check(moved, "stepping onto the lethal tile still counts as a successful move");
            Check(p.currentLevel == 1, "the lethal-tile trigger should return the player to the hub");
            Check(p.tileX == 13 && p.tileY == 6 && p.facing == 4,
                  "the lethal-tile trigger's ResetToHubPosition(true) should land at the alt-spawn point");
            Check(p.campLevel == 2 && p.campX == x && p.campY == y && p.campFacing == 1,
                  "the lethal-tile trigger should bookmark the tile the player actually stepped onto as the camp mark");
            // MarkCampAndReturnToTown itself sets suppressStrafeAdjust,
            // but Player.java's own move() unconditionally resets it to
            // false at the very end of every call (this port's Move()
            // does too) -- so it's never observable as true once Move()
            // returns; only a same-call strafe sequence (turn, step,
            // turn-back) ever reads it while true.
            Check(!p.suppressStrafeAdjust, "Move()'s own cleanup should reset suppressStrafeAdjust to false on return");

            level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] = static_cast<uint8_t>(
                level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] & ~static_cast<uint8_t>(8));
        }

        // --- roaming-monster cleanup on level change ---
        {
            WorldRegistry world(levels.size());
            int16_t spawnIdCounter = 0;

            int ex = 0, ey = 0;
            int crossDir = 0;
            for (int dir = 1; dir <= 4; dir++) {
                if (FindEdgeExit(levels[0], dir, &ex, &ey)) {
                    crossDir = dir;
                    break;
                }
            }
            Check(crossDir != 0, "need a real hub border exit to test level-change cleanup");

            if (crossDir != 0) {
                bool spawned = DungeonRuntime::TrySpawnMonsterNear(levels, world, 0, ex, ey, 41, globalRng, monsterDb,
                                                                    spawnIdCounter);
                Check(spawned, "forcing a type-41 spawn near the hub exit should succeed");

                PlayerState p = dawnstar::PlayerCreation::CreateCharacter(0, "Test", charData, items, globalRng);
                p.currentLevel = 1;
                p.tileX = ex;
                p.tileY = ey;
                p.facing = crossDir;
                p.coreStats[6] = 50;
                p.roamingSpecialMonsterPresent = true;

                size_t hubMonsterCountBefore = world.monsters[0].size();
                bool moved = PlayerMovement::Move(p, 1, false, levels, world, items);
                std::printf("  roaming-cleanup: crossed dir=%d moved=%d level %d->%d\n", crossDir, moved, 1,
                            p.currentLevel);

                if (moved && p.currentLevel != 1) {
                    Check(!p.roamingSpecialMonsterPresent,
                          "crossing into a new level should clear roamingSpecialMonsterPresent once the type-41 "
                          "monster is found");
                    Check(world.monsters[0].size() == hubMonsterCountBefore - 1,
                          "the type-41 monster should be removed from the LEAVING level's registry");
                } else {
                    std::printf("  (skipped: the hub exit wasn't actually walkable into the neighbor -- move did "
                                "not cross)\n");
                }
            }
        }

        // --- roaming-monster cleanup must NOT fire without an actual
        // level change (a plain turn) ---
        {
            WorldRegistry world(levels.size());
            int16_t spawnIdCounter = 100;
            int x = 0, y = 0;
            Check(FindInteriorWalkable(levels[1], &x, &y), "need an interior walkable tile on level 2 (no-cleanup case)");

            bool spawned =
                DungeonRuntime::TrySpawnMonsterNear(levels, world, 1, x, y, 41, globalRng, monsterDb, spawnIdCounter);
            Check(spawned, "forcing a type-41 spawn on level 2 should succeed");
            size_t countBefore = world.monsters[1].size();

            PlayerState p = dawnstar::PlayerCreation::CreateCharacter(0, "Test", charData, items, globalRng);
            p.currentLevel = 2;
            p.tileX = x;
            p.tileY = y;
            p.facing = 1;
            p.coreStats[6] = 50;
            p.roamingSpecialMonsterPresent = true;

            PlayerMovement::Move(p, 3, false, levels, world, items);  // turn right: never crosses a level
            Check(p.roamingSpecialMonsterPresent, "a plain turn must not clear roamingSpecialMonsterPresent");
            Check(world.monsters[1].size() == countBefore, "a plain turn must not touch the monster registry");
        }

        if (!g_ok) {
            std::printf("SOME CHECKS FAILED\n");
            return 1;
        }
        std::printf("all movement/registry-wiring checks passed\n");
        return 0;
    } catch (const std::exception& e) {
        std::printf("EXCEPTION: %s\n", e.what());
        return 2;
    }
}
