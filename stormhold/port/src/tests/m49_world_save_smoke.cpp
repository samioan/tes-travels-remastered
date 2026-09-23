// M49 smoke test: WorldSave::ToBytes/FromBytes (ESGame.
// writeAllLevelRegistries()/readPerLevelRecords(), the WorldRegistry half
// of the real save format).
#include <cstdio>
#include <string>

#include "assets/asset_root.h"
#include "assets/monster_database.h"
#include "dungeon/world_save.h"

namespace {

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

using namespace stormhold;

void TestEmptyRoundTrip() {
    std::printf("-- WorldSave: round-trips a fully empty WorldRegistry --\n");
    WorldRegistry world(37);
    std::vector<uint8_t> bytes = WorldSave::ToBytes(world);
    WorldRegistry back = WorldSave::FromBytes(bytes, 37);

    Expect(back.monsters.size() == 37 && back.chests.size() == 37 && back.droppedItems.size() == 37,
           "FromBytes rebuilds all 37 levels' worth of (empty) registries");
    bool allEmpty = true;
    for (size_t i = 0; i < 37; i++) {
        if (!back.monsters[i].empty() || !back.chests[i].empty() || !back.droppedItems[i].empty()) allEmpty = false;
    }
    Expect(allEmpty, "an empty WorldRegistry round-trips to an all-empty one");
}

void TestHubLevelSkippedForMonstersAndChests(const MonsterDatabase& monsterDb) {
    std::printf("-- WorldSave: the hub (index 0) is skipped for monsters/chests, included for dropped items --\n");

    WorldRegistry world(37);
    // Manually stuff a "monster"/"chest" into index 0 (the hub) -- real
    // gameplay never does this (M6/M18's own confirmed "hub has no
    // monster/chest spawns" finding), but this proves the FORMAT itself
    // truly skips index 0, not just that nothing ever populates it.
    MonsterState hubMonster = MonsterRuntime::Spawn(1, 1, /*dungeonLevel=*/1, monsterDb);
    world.monsters[0][1] = MonsterRuntime::ToBytes(hubMonster);
    world.chests[0][PackTileKey(5, 5)] = std::array<int8_t, 8>{5, 5, 0, 3, 0, 0, 1, 0};
    world.droppedItems[0].push_back(std::array<int8_t, 7>{5, 5, 3, 0, 1, 0, 3});

    std::vector<uint8_t> bytes = WorldSave::ToBytes(world);
    WorldRegistry back = WorldSave::FromBytes(bytes, 37);

    Expect(back.monsters[0].empty(), "index 0's monster is NOT round-tripped -- the format's own loop starts at 1");
    Expect(back.chests[0].empty(), "index 0's chest is NOT round-tripped either");
    Expect(back.droppedItems[0].size() == 1, "index 0's DROPPED ITEM IS round-tripped -- that loop starts at 0");
}

void TestRealDataRoundTrip(const MonsterDatabase& monsterDb) {
    std::printf("-- WorldSave: real monster/chest/dropped-item data round-trips exactly --\n");

    WorldRegistry world(37);

    // Level 2 (index 1): 2 real spawned monsters.
    MonsterState m1 = MonsterRuntime::Spawn(101, 3, /*dungeonLevel=*/2, monsterDb);
    m1.tileX = 5;
    m1.tileY = 7;
    m1.currentHp = 12;
    m1.scratch[3] = -7;
    m1.unconfirmedTimestamp = 1234567890123LL;
    DungeonRuntime::StoreMonster(world, m1);

    MonsterState m2 = MonsterRuntime::Spawn(102, 5, /*dungeonLevel=*/2, monsterDb);
    m2.tileX = 20;
    m2.tileY = 22;
    m2.aiPhase = 2;
    m2.chaseCadence = 4;
    DungeonRuntime::StoreMonster(world, m2);

    // Level 5 (index 4): a chest and 2 dropped items.
    std::array<int8_t, 8> chest{9, 11, 0, 3, 42, 0, 101, 0};
    world.chests[4][PackTileKey(9, 11)] = chest;

    world.droppedItems[4].push_back(std::array<int8_t, 7>{9, 11, 42, 0, 101, 0, 3});
    world.droppedItems[4].push_back(std::array<int8_t, 7>{9, 11, 17, -1, -50, 5, 1});  // negative bytes on purpose

    std::vector<uint8_t> bytes = WorldSave::ToBytes(world);
    WorldRegistry back = WorldSave::FromBytes(bytes, 37);

    Expect(back.monsters[1].size() == 2, "level 2's 2 monsters both round-trip");
    Expect(back.monsters[1].count(101) == 1 && back.monsters[1].count(102) == 1,
           "both round-tripped monsters keep their own spawnId key");

    MonsterState back1 = MonsterRuntime::FromBytes(back.monsters[1][101]);
    Expect(back1.tileX == 5 && back1.tileY == 7 && back1.currentHp == 12 && back1.scratch[3] == -7 &&
               back1.unconfirmedTimestamp == 1234567890123LL,
           "monster 101's full field set (including a negative scratch byte and a real 64-bit timestamp) "
           "round-trips exactly");

    MonsterState back2 = MonsterRuntime::FromBytes(back.monsters[1][102]);
    Expect(back2.aiPhase == 2 && back2.chaseCadence == 4, "monster 102's aiPhase/chaseCadence round-trip exactly");

    Expect(back.chests[4].size() == 1 && back.chests[4].count(PackTileKey(9, 11)) == 1,
           "the chest round-trips, keyed by its own tile position");
    Expect(back.chests[4][PackTileKey(9, 11)] == chest, "the chest's full 8-byte record round-trips exactly");

    Expect(back.droppedItems[4].size() == 2, "both dropped items on level 5 round-trip");
    Expect(back.droppedItems[4][0] == (std::array<int8_t, 7>{9, 11, 42, 0, 101, 0, 3}),
           "the first dropped item's record round-trips exactly");
    Expect(back.droppedItems[4][1] == (std::array<int8_t, 7>{9, 11, 17, -1, -50, 5, 1}),
           "the second dropped item's record round-trips exactly, INCLUDING negative bytes");

    // Every other level stayed empty.
    bool otherLevelsEmpty = true;
    for (size_t i = 1; i < 37; i++) {
        if (i == 1) continue;  // level 2
        if (!back.monsters[i].empty()) otherLevelsEmpty = false;
    }
    for (size_t i = 1; i < 37; i++) {
        if (i == 4) continue;  // level 5
        if (!back.chests[i].empty() || !back.droppedItems[i].empty()) otherLevelsEmpty = false;
    }
    Expect(otherLevelsEmpty, "every level not explicitly populated stays empty after the round trip");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assets(root);
        MonsterDatabase monsterDb = MonsterDatabase::Load(assets);

        TestEmptyRoundTrip();
        TestHubLevelSkippedForMonstersAndChests(monsterDb);
        TestRealDataRoundTrip(monsterDb);

        if (!g_ok) {
            std::fprintf(stderr, "m49_world_save_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m49_world_save_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
