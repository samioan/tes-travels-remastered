// M22 smoke test: DungeonRuntime (dungeon/dungeon_runtime.h) -- the live
// per-level monster/chest/dropped-item registry -- against the real
// 37-level generated world (M6's DungeonGenerator), the same methodology
// as M6/M9/M11/M13-M21 (no JVM ground truth available -- see
// player/player_movement.h's header comment for why that's still true
// here).
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "dungeon/dungeon_runtime.h"
#include "monster/monster_runtime.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"
#include "world/dungeon_view.h"

namespace {

using dawnstar::DungeonRuntime;
using dawnstar::DungeonView;
using dawnstar::GeneratedLevel;
using dawnstar::MonsterRuntime;
using dawnstar::MonsterState;
using dawnstar::WorldRegistry;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        dawnstar::MonsterDatabase monsterDb = dawnstar::MonsterDatabase::Load(archive);
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

        WorldRegistry world(levels.size());
        dawnstar::JavaRandom rng(4242);
        int16_t spawnIdCounter = 0;

        const int kLevelIndex = 1;  // level 2, a standard procedurally-generated level
        GeneratedLevel& level = levels[static_cast<size_t>(kLevelIndex)];

        // --- PopulateRandomMonsters: places exactly `count` monsters,
        // each on a genuinely walkable tile, each registered under a
        // key matching its own stored position ---
        const int kCount = 6;
        DungeonRuntime::PopulateRandomMonsters(levels, world, kLevelIndex, kCount, rng, monsterDb, spawnIdCounter);

        auto& monsters = world.monsters[static_cast<size_t>(kLevelIndex)];
        std::printf("populated %zu monsters (requested %d)\n", monsters.size(), kCount);
        Check(static_cast<int>(monsters.size()) == kCount, "should have registered exactly `count` monsters");

        int checkedTileBit = 0;
        for (const auto& [key, record] : monsters) {
            MonsterState m = MonsterRuntime::FromBytes(record);
            int expectedKey = dawnstar::PackPosKey(m.x, m.y);
            Check(key == expectedKey, "registry key should match the stored monster's own (x,y)");
            Check(m.dungeonLevel == level.number, "spawned monster's dungeonLevel should match the target level");
            uint8_t flags = level.tiles[static_cast<size_t>(m.x)][static_cast<size_t>(m.y)];
            Check((flags & 2) != 0, "spawned monster's tile should have the monster presence bit set");
            checkedTileBit++;
        }
        std::printf("checked %d spawned monsters' tile bits\n", checkedTileBit);

        // --- TrySpawnMonsterNear(41)/(42): the special "roaming"
        // monster ids bypass the tier roll and spawn exactly that type ---
        int ix = 0, iy = 0;
        bool foundInterior = false;
        for (int x = 1; x < level.width - 1 && !foundInterior; x++) {
            for (int y = 1; y < level.height - 1; y++) {
                if (DungeonView(levels, kLevelIndex).IsWalkable(x, y)) {
                    ix = x;
                    iy = y;
                    foundInterior = true;
                    break;
                }
            }
        }
        Check(foundInterior, "need an interior walkable tile to test forced spawns near it");

        if (foundInterior) {
            bool spawned41 = DungeonRuntime::TrySpawnMonsterNear(levels, world, kLevelIndex, ix, iy, 41, rng,
                                                                  monsterDb, spawnIdCounter);
            Check(spawned41, "forcing type 41 near a walkable interior tile should succeed");
            if (spawned41) {
                bool found = false;
                for (const auto& [key, record] : world.monsters[static_cast<size_t>(kLevelIndex)]) {
                    MonsterState m = MonsterRuntime::FromBytes(record);
                    if (m.monsterType == 41) found = true;
                }
                Check(found, "forcing type 41 should spawn exactly that type, bypassing the tier roll");
            }
        }

        // --- TrySpawnMonsterNear's surprising non-41/42 semantics: any
        // other "forcedTypeOrSentinel" value is used as a REPLACEMENT
        // TIER for a random roll, not a literal forced type -- verified
        // by reproducing the exact same roll independently ---
        {
            dawnstar::JavaRandom rngA(99999);
            dawnstar::JavaRandom rngB(99999);  // identical seed, independent instance

            int lx = 0, ly = 0;
            bool found = false;
            for (int x = 1; x < level.width - 1 && !found; x++) {
                for (int y = 1; y < level.height - 1; y++) {
                    if (DungeonView(levels, kLevelIndex).IsWalkable(x, y)) {
                        lx = x;
                        ly = y;
                        found = true;
                        break;
                    }
                }
            }
            Check(found, "need an interior walkable tile for the tier-substitution check");

            if (found) {
                // TrySpawnMonsterNear places the monster ADJACENT to
                // (lx,ly) (west/east/north/south/south+3), not
                // necessarily exactly at (lx,ly) -- so find whichever
                // key is new after the call, rather than assuming the
                // exact position.
                std::vector<int> keysBefore;
                for (const auto& [key, record] : world.monsters[static_cast<size_t>(kLevelIndex)]) {
                    (void)record;
                    keysBefore.push_back(key);
                }

                int16_t counterA = 500;
                bool spawnedA =
                    DungeonRuntime::TrySpawnMonsterNear(levels, world, kLevelIndex, lx, ly, 5, rngA, monsterDb, counterA);
                Check(spawnedA, "the tier-substitution spawn should succeed");

                // Independently reproduce what TrySpawnMonsterNear should
                // have rolled: PickMonsterType(rng, difficultyTier=5,
                // forcedType=-1) against a fresh, identically-seeded rng --
                // NOT PickMonsterType(rng, level.tier, 5), which is what a
                // naive reading of "forcedTypeOrSentinel=5" would suggest.
                int expectedType = MonsterRuntime::PickMonsterType(rngB, 5, -1);

                int actualType = -1;
                for (const auto& [key, record] : world.monsters[static_cast<size_t>(kLevelIndex)]) {
                    bool isNew = std::find(keysBefore.begin(), keysBefore.end(), key) == keysBefore.end();
                    if (isNew) actualType = MonsterRuntime::FromBytes(record).monsterType;
                }
                std::printf("tier-substitution: passed forcedTypeOrSentinel=5, expected rolled type=%d, actual=%d\n",
                            expectedType, actualType);
                Check(actualType == expectedType,
                      "a non-41/42 forcedTypeOrSentinel should be used as the roll's tier, not a literal forced type");
            }
        }

        // --- dropped items: Add/RemoveDroppedItem/DroppedItemsAt/
        // ClearDroppedItemFlag ---
        std::array<uint8_t, 7> dropRecord = {static_cast<uint8_t>(ix), static_cast<uint8_t>(iy), 5, 0, 7, 42, 3};
        DungeonRuntime::AddDroppedItem(level, world, dropRecord);
        Check((level.tiles[static_cast<size_t>(ix)][static_cast<size_t>(iy)] & 4) != 0,
              "AddDroppedItem should set the dropped-item presence bit");

        auto foundItems = DungeonRuntime::DroppedItemsAt(world, kLevelIndex, ix, iy);
        Check(foundItems.size() == 1 && foundItems[0] == dropRecord,
              "DroppedItemsAt should return the exact record just added");

        DungeonRuntime::RemoveDroppedItem(level, world, dropRecord);
        auto afterRemove = DungeonRuntime::DroppedItemsAt(world, kLevelIndex, ix, iy);
        Check(afterRemove.empty(), "RemoveDroppedItem should remove the record from the registry");
        Check((level.tiles[static_cast<size_t>(ix)][static_cast<size_t>(iy)] & 4) != 0,
              "RemoveDroppedItem should NOT itself clear the tile's presence bit -- ported exactly, a real gap "
              "the original only closes via a separate ClearDroppedItemFlag call");

        DungeonRuntime::ClearDroppedItemFlag(level, ix, iy);
        Check((level.tiles[static_cast<size_t>(ix)][static_cast<size_t>(iy)] & 4) == 0,
              "ClearDroppedItemFlag should clear the presence bit");

        // --- chests: RemoveChest's guard (wall bit blocks removal even
        // with the chest bit set; otherwise both the map entry and tile
        // bit clear) ---
        int cx = ix, cy = iy;
        std::array<uint8_t, 8> chestRecord = {static_cast<uint8_t>(cx), static_cast<uint8_t>(cy), 0, 3, 12, 0, 9, 0};
        world.chests[static_cast<size_t>(kLevelIndex)][dawnstar::PackPosKey(cx, cy)] = chestRecord;
        level.tiles[static_cast<size_t>(cx)][static_cast<size_t>(cy)] =
            static_cast<uint8_t>(level.tiles[static_cast<size_t>(cx)][static_cast<size_t>(cy)] | 16);

        uint8_t savedFlags = level.tiles[static_cast<size_t>(cx)][static_cast<size_t>(cy)];
        level.tiles[static_cast<size_t>(cx)][static_cast<size_t>(cy)] =
            static_cast<uint8_t>(savedFlags | 1);  // pretend it's (also) a wall tile
        DungeonRuntime::RemoveChest(level, world, chestRecord);
        Check(world.chests[static_cast<size_t>(kLevelIndex)].count(dawnstar::PackPosKey(cx, cy)) == 1,
              "RemoveChest should refuse to remove a chest on a wall tile");

        level.tiles[static_cast<size_t>(cx)][static_cast<size_t>(cy)] = savedFlags;  // clear the pretend wall bit
        DungeonRuntime::RemoveChest(level, world, chestRecord);
        Check(world.chests[static_cast<size_t>(kLevelIndex)].count(dawnstar::PackPosKey(cx, cy)) == 0,
              "RemoveChest should remove the chest once the wall guard no longer blocks it");
        Check((level.tiles[static_cast<size_t>(cx)][static_cast<size_t>(cy)] & 16) == 0,
              "RemoveChest should clear the chest presence bit");

        // --- RefreshTileFlags: rebuilds bits 2/4/16 from the live
        // registries alone, discarding any stale bits ---
        // Deliberately corrupt the tiles: set bit 2 somewhere with no
        // matching registry entry, then confirm RefreshTileFlags clears
        // it and re-derives every real bit from the registries.
        int stubX = ix == 1 ? 2 : 1, stubY = iy == 1 ? 2 : 1;
        level.tiles[static_cast<size_t>(stubX)][static_cast<size_t>(stubY)] = static_cast<uint8_t>(
            level.tiles[static_cast<size_t>(stubX)][static_cast<size_t>(stubY)] | 2 | 4 | 16);

        DungeonRuntime::RefreshTileFlags(level, world, kLevelIndex);

        Check((level.tiles[static_cast<size_t>(stubX)][static_cast<size_t>(stubY)] & (2 | 4 | 16)) == 0,
              "RefreshTileFlags should clear stale presence bits with no matching registry entry");

        int rebuiltMonsterBits = 0;
        for (const auto& [key, record] : world.monsters[static_cast<size_t>(kLevelIndex)]) {
            int x, y;
            dawnstar::UnpackPosKey(key, &x, &y);
            if ((level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] & 2) != 0) rebuiltMonsterBits++;
        }
        Check(rebuiltMonsterBits == static_cast<int>(world.monsters[static_cast<size_t>(kLevelIndex)].size()),
              "RefreshTileFlags should re-set the monster presence bit for every registered monster");

        if (g_ok) {
            std::printf("all dungeon-runtime checks passed\n");
            return 0;
        } else {
            std::printf("SOME CHECKS FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("EXCEPTION: %s\n", e.what());
        return 2;
    }
}
