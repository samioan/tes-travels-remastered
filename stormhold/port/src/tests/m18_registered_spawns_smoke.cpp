// M18 smoke test: DungeonRuntime::RegisterGeneratedSpawns against the
// real, full 37-level world -- the last piece of the 3-way registry split
// M16/M17 worked through (dawnstar's own M24 equivalent). No JVM ground
// truth is possible for generation itself (same reason as M6); this test
// instead cross-checks every registered record against the exact
// GeneratedMonsterSpawn/GeneratedChestSpawn data DungeonGenerator already
// produced, plus a real end-to-end integration check (removing a
// generation-registered monster via DungeonRuntime::RemoveMonster, not one
// this test spawned itself).
#include <cstdio>
#include <string>

#include "assets/asset_root.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "dungeon/dungeon_runtime.h"
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

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        stormhold::AssetRoot assets(root);
        stormhold::ItemDatabase items = stormhold::ItemDatabase::Load(assets);
        stormhold::MonsterDatabase monsters = stormhold::MonsterDatabase::Load(assets);
        stormhold::DungeonGeometry geometry = stormhold::DungeonGeometry::Load(assets);

        stormhold::WorldRegistry world(37);

        std::printf("-- the hub town (no room-monster/chest generation at all) --\n");
        stormhold::GeneratedLevel hub = stormhold::DungeonGenerator::BuildHubLevel(geometry.rows[0]);
        stormhold::DungeonRuntime::RegisterGeneratedSpawns(hub, world, monsters);
        Expect(world.monsters[0].empty(), "the hub should register zero monsters (no rooms are ever generated for it)");
        Expect(world.chests[0].empty(), "the hub should register zero chests");

        std::printf("-- every one of the 36 standard levels --\n");
        int totalMonsters = 0, totalChests = 0, extendedIdChests = 0, guaranteedGiftChests = 0;
        stormhold::MonsterState monsterToRemove;
        bool haveMonsterToRemove = false;
        int removeLevelIndex = -1;

        for (int levelNumber = 2; levelNumber <= 37; levelNumber++) {
            stormhold::GeneratedLevel level =
                stormhold::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[levelNumber - 1], items, monsters);
            int levelIndex = levelNumber - 1;

            stormhold::DungeonRuntime::RegisterGeneratedSpawns(level, world, monsters);

            Expect(world.monsters[static_cast<size_t>(levelIndex)].size() == level.monsters.size(),
                   "registered monster count should match the generated spawn count exactly");
            Expect(world.chests[static_cast<size_t>(levelIndex)].size() == level.chests.size(),
                   "registered chest count should match the generated spawn count exactly");

            for (const auto& spawn : level.monsters) {
                auto it = world.monsters[static_cast<size_t>(levelIndex)].find(static_cast<int16_t>(spawn.spawnId));
                bool found = it != world.monsters[static_cast<size_t>(levelIndex)].end();
                Expect(found, "every GeneratedMonsterSpawn should have a registry entry keyed by its own spawnId");
                if (!found) continue;

                stormhold::MonsterState m = stormhold::MonsterRuntime::FromBytes(it->second);
                Expect(m.spawnId == static_cast<int16_t>(spawn.spawnId), "registered spawnId should round-trip");
                Expect(m.typeIndex == static_cast<int8_t>(spawn.monsterType), "registered typeIndex should round-trip");
                Expect(m.tileX == static_cast<int8_t>(spawn.x) && m.tileY == static_cast<int8_t>(spawn.y),
                       "registered position should round-trip");
                Expect(m.currentHp == static_cast<int8_t>(spawn.hp), "registered currentHp should match the RAW column-14 read");
                Expect(m.dungeonLevel == static_cast<int8_t>(levelNumber), "registered dungeonLevel should match this level");
                Expect((level.tiles[static_cast<size_t>(spawn.x)][static_cast<size_t>(spawn.y)] & 2) != 0,
                       "the monster tile bit should already be set from generation -- RegisterGeneratedSpawns "
                       "must not need to set it itself");

                if (!haveMonsterToRemove) {
                    monsterToRemove = m;
                    removeLevelIndex = levelIndex;
                    haveMonsterToRemove = true;
                }
            }

            int guaranteedThisLevel = 0;
            for (const auto& chest : level.chests) {
                int32_t key = stormhold::PackTileKey(chest.x, chest.y);
                auto it = world.chests[static_cast<size_t>(levelIndex)].find(key);
                bool found = it != world.chests[static_cast<size_t>(levelIndex)].end();
                Expect(found, "every GeneratedChestSpawn should have a registry entry keyed by its own position");
                if (!found) continue;

                const std::array<int8_t, 8>& record = it->second;
                Expect(record[0] == static_cast<int8_t>(chest.x) && record[1] == static_cast<int8_t>(chest.y),
                       "registered chest position should round-trip");
                Expect(record[2] == 0, "byte 2 should be the confirmed always-zero on-disk value, never chest.guaranteedGift");
                Expect((record[3] & 0x3F) == static_cast<int8_t>(level.tier),
                       "byte 3's low 6 bits should carry the level's own tier");

                int low = record[4] & 0xFF;
                int high = record[7] & 0xFF;
                Expect(low == (chest.itemId & 0xFF), "the record's low byte should always match chest.itemId's own low byte");
                if (low == 86) {
                    extendedIdChests++;
                    Expect(high == ((chest.itemId >> 8) & 0xFF),
                           "when the low byte is 86, the high byte should carry chest.itemId's own high byte");
                } else {
                    Expect(high == 0, "when the low byte isn't 86, the high byte should stay 0 (Dungeon.placeChests()'s "
                                      "own literal `if (low == 86)` gate, unrelated to RollLoot's own packing rule)");
                }

                uint16_t spawnIdBack = static_cast<uint16_t>(((record[5] & 0xFF) << 8) | (record[6] & 0xFF));
                Expect(spawnIdBack == static_cast<uint16_t>(chest.spawnId), "registered chest spawnId should round-trip");

                Expect((level.tiles[static_cast<size_t>(chest.x)][static_cast<size_t>(chest.y)] & 16) != 0,
                       "the chest tile bit should already be set from generation");

                if (chest.guaranteedGift) guaranteedThisLevel++;
            }
            Expect(guaranteedThisLevel == 1, "exactly one guaranteed-gift chest per level (GeneratedChestSpawn's own "
                                              "flag, not the always-zero on-disk byte)");
            guaranteedGiftChests += guaranteedThisLevel;

            totalMonsters += static_cast<int>(level.monsters.size());
            totalChests += static_cast<int>(level.chests.size());
        }

        std::printf("  registered %d monsters and %d chests across 36 standard levels (%d extended-id chests, "
                     "%d guaranteed-gift chests)\n",
                    totalMonsters, totalChests, extendedIdChests, guaranteedGiftChests);
        Expect(guaranteedGiftChests == 36, "every one of the 36 standard levels should contribute exactly 1 guaranteed-gift chest");

        std::printf("-- integration: RemoveMonster against a REAL generation-registered monster --\n");
        Expect(haveMonsterToRemove, "at least one real generated monster should exist to test removal against");
        if (haveMonsterToRemove) {
            stormhold::GeneratedLevel removalLevel;
            removalLevel.number = removeLevelIndex + 1;
            removalLevel.width = removalLevel.height = 35;
            removalLevel.tiles.assign(35, std::vector<uint8_t>(35, 0));
            removalLevel.tiles[static_cast<size_t>(monsterToRemove.tileX)][static_cast<size_t>(monsterToRemove.tileY)] = 2;

            stormhold::DungeonRuntime::RemoveMonster(removalLevel, world, monsterToRemove.spawnId);
            Expect(world.monsters[static_cast<size_t>(removeLevelIndex)].count(monsterToRemove.spawnId) == 0,
                   "RemoveMonster should erase a real generation-registered monster, not just dynamically-spawned ones");
            Expect((removalLevel.tiles[static_cast<size_t>(monsterToRemove.tileX)]
                                       [static_cast<size_t>(monsterToRemove.tileY)] &
                    2) == 0,
                   "RemoveMonster should clear the monster-presence tile bit");
        }

        if (!g_ok) {
            std::fprintf(stderr, "m18_registered_spawns_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m18_registered_spawns_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
