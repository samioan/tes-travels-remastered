// M24 smoke test: DungeonRuntime::RegisterGeneratedSpawns
// (dungeon/dungeon_runtime.h) -- registering a level's already-generated
// pre-placed monster/chest spawns (world/dungeon_generator.h's
// GeneratedLevel::monsters/::chests) into the live WorldRegistry, the
// port's substitute for DungeonGenerator.java's populateLevel/
// placeChests directly calling Monster.spawn(...).store()/
// ESGame.chests[...].put(...) as part of generation itself. Same
// methodology as M6/M9/M11/M13-M23 (no JVM ground truth available).
#include <cstdio>
#include <string>
#include <vector>

#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "dungeon/dungeon_runtime.h"
#include "monster/monster_runtime.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::DungeonRuntime;
using dawnstar::GeneratedLevel;
using dawnstar::MonsterRuntime;
using dawnstar::MonsterState;
using dawnstar::PackPosKey;
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

        // Mirrors main.cpp's BuildWorld: generate, then register.
        std::vector<GeneratedLevel> levels;
        WorldRegistry world(geometry.rows.size());
        for (size_t i = 0; i < geometry.rows.size(); i++) {
            int levelNumber = static_cast<int>(i) + 1;
            levels.push_back(levelNumber == 1
                                  ? dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[0])
                                  : dawnstar::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[i], items,
                                                                               monsterDb));
            DungeonRuntime::RegisterGeneratedSpawns(levels.back(), world);
        }
        std::printf("generated + registered %zu levels\n", levels.size());

        // --- the hub town (level 1) has no room-monster/chest
        // generation at all (BuildHubLevel only marks the 5 shop
        // tiles) -- confirm registration is correctly a no-op there ---
        Check(levels[0].monsters.empty(), "hub level should have no generated monster spawns");
        Check(levels[0].chests.empty(), "hub level should have no generated chest spawns");
        Check(world.monsters[0].empty(), "hub level's registry should have no monsters either");
        Check(world.chests[0].empty(), "hub level's registry should have no chests either");

        // --- every standard level: registry count matches generated
        // count exactly, every generated spawn is registered under its
        // own position with the tile bit DungeonGenerator itself already
        // set, and round-trips back to the same data ---
        int totalMonsters = 0, totalChests = 0, totalGuaranteedGift = 0, extendedIdChestsSeen = 0;
        for (size_t i = 1; i < levels.size(); i++) {
            GeneratedLevel& level = levels[i];
            auto& registeredMonsters = world.monsters[i];
            auto& registeredChests = world.chests[i];

            Check(registeredMonsters.size() == level.monsters.size(),
                  "registered monster count should match the generated spawn count");
            Check(registeredChests.size() == level.chests.size(),
                  "registered chest count should match the generated spawn count (5 per level)");

            for (const auto& spawn : level.monsters) {
                auto it = registeredMonsters.find(PackPosKey(spawn.x, spawn.y));
                Check(it != registeredMonsters.end(), "each generated monster spawn should have a registry entry");
                if (it == registeredMonsters.end()) continue;

                MonsterState m = MonsterRuntime::FromBytes(it->second);
                Check(m.monsterType == spawn.monsterType, "registered monster type should match generation");
                Check(m.hp == static_cast<int8_t>(spawn.hp), "registered monster hp should match generation");
                Check(m.x == spawn.x && m.y == spawn.y, "registered monster position should match generation");
                Check(m.dungeonLevel == level.number, "registered monster dungeonLevel should be this level's number");
                Check(m.spawnId == spawn.spawnId, "registered monster spawnId should match generation");
                Check(!m.flag, "a freshly-registered monster should not be flagged");
                Check(m.moveCooldown == 0 && m.aiPhase == 0 && m.timestamp == 0,
                      "a freshly-registered monster's AI scratch state should be all-zero, matching Monster's own "
                      "constructor");

                uint8_t flags = level.tiles[static_cast<size_t>(spawn.x)][static_cast<size_t>(spawn.y)];
                Check((flags & 2) != 0,
                      "the generated monster's tile should already have the monster presence bit set (by "
                      "DungeonGenerator itself, not RegisterGeneratedSpawns)");
            }

            int guaranteedGiftCount = 0;
            for (const auto& chest : level.chests) {
                auto it = registeredChests.find(PackPosKey(chest.x, chest.y));
                Check(it != registeredChests.end(), "each generated chest spawn should have a registry entry");
                if (it == registeredChests.end()) continue;

                const auto& record = it->second;
                Check(record[0] == static_cast<uint8_t>(chest.x) && record[1] == static_cast<uint8_t>(chest.y),
                      "registered chest position should match generation");
                Check(record[2] == (chest.guaranteedGift ? 1 : 0),
                      "registered chest byte 2 should reflect guaranteedGift");
                Check(record[3] == static_cast<uint8_t>(level.tier), "registered chest byte 3 should be this level's tier");

                int reconstructedItemId = record[4];
                if (record[4] == 86) {
                    reconstructedItemId = (static_cast<int>(record[7]) << 8) | record[4];
                    extendedIdChestsSeen++;
                }
                Check(reconstructedItemId == chest.itemId,
                      "registered chest itemId bytes should reconstruct the generated itemId exactly");

                int reconstructedSpawnId = (static_cast<int>(record[5]) << 8) | record[6];
                Check(reconstructedSpawnId == chest.spawnId,
                      "registered chest spawnId bytes should reconstruct the generated spawnId exactly");

                uint8_t flags = level.tiles[static_cast<size_t>(chest.x)][static_cast<size_t>(chest.y)];
                Check((flags & 16) != 0,
                      "the generated chest's tile should already have the chest presence bit set (by "
                      "DungeonGenerator itself, not RegisterGeneratedSpawns)");

                if (chest.guaranteedGift) guaranteedGiftCount++;
            }
            Check(guaranteedGiftCount == 1, "exactly one chest per level should be the guaranteed-gift chest");

            totalMonsters += static_cast<int>(level.monsters.size());
            totalChests += static_cast<int>(level.chests.size());
            totalGuaranteedGift += guaranteedGiftCount;
        }
        std::printf("checked %d monsters / %d chests (%d guaranteed-gift) across 36 standard levels, %d chests hit "
                    "the extended-itemId (low byte 86) branch\n",
                    totalMonsters, totalChests, totalGuaranteedGift, extendedIdChestsSeen);

        // --- integration check: M22/M23's own machinery (RemoveMonster)
        // should work against a monster THIS milestone registered, not
        // just one DungeonRuntime itself spawned dynamically (as M22's
        // own smoke test only exercised) ---
        {
            const size_t kLevelIndex = 1;  // level 2
            GeneratedLevel& level = levels[kLevelIndex];
            Check(!level.monsters.empty(), "level 2 should have at least one generated monster to test removal on");
            if (!level.monsters.empty()) {
                const auto& spawn = level.monsters[0];
                size_t before = world.monsters[kLevelIndex].size();
                DungeonRuntime::RemoveMonster(level, world, spawn.x, spawn.y);
                Check(world.monsters[kLevelIndex].size() == before - 1,
                      "RemoveMonster should remove a pre-placed (not just dynamically-spawned) registered monster");
                uint8_t flags = level.tiles[static_cast<size_t>(spawn.x)][static_cast<size_t>(spawn.y)];
                Check((flags & 2) == 0, "RemoveMonster should clear the tile's monster presence bit");
            }
        }

        if (g_ok) {
            std::printf("all registered-spawns checks passed\n");
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
