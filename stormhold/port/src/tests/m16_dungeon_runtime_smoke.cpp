// M16 smoke test: DungeonRuntime's WorldRegistry + Dungeon.java's own
// registry-management methods, against both synthetic records and a real
// generated level (for SpawnAmbushMonsters, which needs real room
// rectangles).
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

stormhold::GeneratedLevel MakeBlankLevel(int number, int width = 35, int height = 35) {
    stormhold::GeneratedLevel level;
    level.number = number;
    level.width = width;
    level.height = height;
    level.tiles.assign(static_cast<size_t>(width), std::vector<uint8_t>(static_cast<size_t>(height), 0));
    return level;
}

void TestStoreAndRemoveMonster(const stormhold::MonsterDatabase& monsters) {
    std::printf("-- StoreMonster/RemoveMonster/MonsterAt --\n");
    stormhold::GeneratedLevel level = MakeBlankLevel(2);
    stormhold::WorldRegistry world(37);

    stormhold::MonsterState m = stormhold::MonsterRuntime::Spawn(5, 1, 2, monsters);
    m.tileX = 10;
    m.tileY = 12;
    level.tiles[10][12] |= 2;
    stormhold::DungeonRuntime::StoreMonster(world, m);

    Expect(world.monsters[1].count(5) == 1, "StoreMonster should register under the monster's own spawnId");

    auto found = stormhold::DungeonRuntime::MonsterAt(level, world, 10, 12);
    Expect(found.has_value() && found->spawnId == 5, "MonsterAt should find the monster at its own tile");

    auto notFound = stormhold::DungeonRuntime::MonsterAt(level, world, 0, 0);
    Expect(!notFound.has_value(), "MonsterAt should return nullopt where the monster-presence bit isn't set");

    level.tiles[10][12] |= 1;  // wall bit set too
    auto wallBlocked = stormhold::DungeonRuntime::MonsterAt(level, world, 10, 12);
    Expect(!wallBlocked.has_value(), "MonsterAt should return nullopt on a wall tile even if bit 2 is set");
    level.tiles[10][12] = static_cast<uint8_t>(level.tiles[10][12] & ~1);

    stormhold::DungeonRuntime::RemoveMonster(level, world, 5);
    Expect(world.monsters[1].count(5) == 0, "RemoveMonster should erase the registry entry");
    Expect((level.tiles[10][12] & 2) == 0, "RemoveMonster should clear the monster-presence tile bit");

    bool threw = false;
    try {
        stormhold::DungeonRuntime::RemoveMonster(level, world, 999);
    } catch (const std::exception&) {
        threw = true;
    }
    Expect(threw, "RemoveMonster on an unregistered spawnId should throw (the real killMonster() has a latent NPE here)");
}

void TestChestRegistry() {
    std::printf("-- StoreChest/RemoveChest --\n");
    stormhold::GeneratedLevel level = MakeBlankLevel(3);
    stormhold::WorldRegistry world(37);

    std::array<int8_t, 8> record = {7, 9, 0, 5, 42, 0, 12, 0};  // x=7,y=9,dead-byte=0,tier=5,itemIdLow=42,spawnIdHi=0,spawnIdLo=12,itemIdHigh=0
    stormhold::DungeonRuntime::StoreChest(level, world, record);
    Expect((level.tiles[7][9] & 16) != 0, "StoreChest should set the chest-presence tile bit");
    Expect(world.chests[2].count(stormhold::PackTileKey(7, 9)) == 1, "StoreChest should register under its position key");

    // A wall tile blocks removal even if bit 16 is (incorrectly) set.
    level.tiles[7][9] |= 1;
    stormhold::DungeonRuntime::RemoveChest(level, world, record);
    Expect(world.chests[2].count(stormhold::PackTileKey(7, 9)) == 1, "RemoveChest should refuse to remove on a wall tile");
    level.tiles[7][9] = static_cast<uint8_t>(level.tiles[7][9] & ~1);

    stormhold::DungeonRuntime::RemoveChest(level, world, record);
    Expect(world.chests[2].count(stormhold::PackTileKey(7, 9)) == 0, "RemoveChest should remove once the wall guard is clear");
    Expect((level.tiles[7][9] & 16) == 0, "RemoveChest should clear the chest-presence tile bit");

    // Removing again (bit 16 no longer set) should be a silent no-op, not a crash.
    stormhold::DungeonRuntime::RemoveChest(level, world, record);
}

void TestDroppedItemRegistry() {
    std::printf("-- AddDroppedItem/RemoveDroppedItem/CountDroppedItemsAt/FirstDroppedItemAt/DroppedItemsAt --\n");
    stormhold::GeneratedLevel level = MakeBlankLevel(4);
    stormhold::WorldRegistry world(37);

    std::array<int8_t, 7> recordA = {3, 3, 10, 0, 1, 0, 1};
    std::array<int8_t, 7> recordB = {3, 3, 20, 0, 2, 0, 1};  // same tile, different item
    stormhold::DungeonRuntime::AddDroppedItem(level, world, recordA);
    stormhold::DungeonRuntime::AddDroppedItem(level, world, recordB);

    Expect((level.tiles[3][3] & 4) != 0, "AddDroppedItem should set the dropped-item-presence tile bit");
    Expect(stormhold::DungeonRuntime::CountDroppedItemsAt(world, 3, 3, 3) == 2, "2 records at the same tile should both count");

    auto first = stormhold::DungeonRuntime::FirstDroppedItemAt(world, 3, 3, 3);
    Expect(first.has_value() && *first == recordA, "FirstDroppedItemAt should return the first-registered record");

    auto all = stormhold::DungeonRuntime::DroppedItemsAt(world, 3, 3, 3);
    Expect(all.size() == 2, "DroppedItemsAt should return both records");

    stormhold::DungeonRuntime::RemoveDroppedItem(level, world, recordA);
    Expect(stormhold::DungeonRuntime::CountDroppedItemsAt(world, 3, 3, 3) == 1, "removing one of two should leave one");
    Expect((level.tiles[3][3] & 4) != 0, "the dropped-item bit should stay set while one record remains");

    stormhold::DungeonRuntime::RemoveDroppedItem(level, world, recordB);
    Expect(stormhold::DungeonRuntime::CountDroppedItemsAt(world, 3, 3, 3) == 0, "removing the last should leave zero");
    Expect((level.tiles[3][3] & 4) == 0, "the dropped-item bit should clear once no records remain at that tile");
}

void TestRefreshTileFlags(const stormhold::MonsterDatabase& monsters) {
    std::printf("-- RefreshTileFlags rebuilds bits from the live registries --\n");
    stormhold::GeneratedLevel level = MakeBlankLevel(5);
    stormhold::WorldRegistry world(37);

    stormhold::MonsterState m = stormhold::MonsterRuntime::Spawn(9, 1, 5, monsters);
    m.tileX = 4;
    m.tileY = 4;
    world.monsters[4][9] = stormhold::MonsterRuntime::ToBytes(m);

    std::array<int8_t, 8> chest = {20, 21, 0, 1, 0, 0, 0, 0};
    world.chests[4][stormhold::PackTileKey(20, 21)] = chest;

    std::array<int8_t, 7> drop = {8, 8, 1, 0, 1, 0, 1};
    world.droppedItems[4].push_back(drop);

    Expect(level.tiles[4][4] == 0 && level.tiles[20][21] == 0 && level.tiles[8][8] == 0,
           "the level's tiles should start with no presence bits before refreshing");

    stormhold::DungeonRuntime::RefreshTileFlags(level, world);

    Expect((level.tiles[4][4] & 2) != 0, "RefreshTileFlags should set the monster bit from world.monsters");
    Expect((level.tiles[20][21] & 16) != 0, "RefreshTileFlags should set the chest bit from world.chests");
    Expect((level.tiles[8][8] & 4) != 0, "RefreshTileFlags should set the dropped-item bit from world.droppedItems");
}

void TestSpawnAmbushMonsters(const stormhold::DungeonGeometry& geometry, const stormhold::ItemDatabase& items,
                              const stormhold::MonsterDatabase& monsters) {
    std::printf("-- SpawnAmbushMonsters against a real generated level --\n");
    stormhold::GeneratedLevel level = stormhold::DungeonGenerator::PopulateLevel(2, geometry.rows[1], items, monsters);
    Expect(!level.rooms.empty(), "a real generated level should have a non-empty room list (added this milestone)");

    stormhold::WorldRegistry world(37);
    size_t before = world.monsters[1].size();

    stormhold::JavaRandom rng(777);
    int16_t spawnIdCounter = 1000;
    stormhold::DungeonRuntime::SpawnAmbushMonsters(level, world, 3, rng, monsters, spawnIdCounter);

    Expect(world.monsters[1].size() == before + 3, "SpawnAmbushMonsters(3) should register exactly 3 new monsters");
    Expect(spawnIdCounter == 1003, "spawnIdCounter should advance by exactly 3 (one per actually-placed monster)");

    for (const auto& [spawnId, record] : world.monsters[1]) {
        stormhold::MonsterState m = stormhold::MonsterRuntime::FromBytes(record);
        bool inBounds = m.tileX >= 0 && m.tileX < level.width && m.tileY >= 0 && m.tileY < level.height;
        Expect(inBounds, "every ambush-spawned monster should land in-bounds");
        if (inBounds) {
            Expect((level.tiles[static_cast<size_t>(m.tileX)][static_cast<size_t>(m.tileY)] & 2) != 0,
                   "every ambush-spawned monster's tile should carry the monster-presence bit");
        }

        bool inSomeRoom = false;
        for (const auto& room : level.rooms) {
            if (m.tileX >= room.x0 && m.tileX <= room.x1 && m.tileY >= room.y0 && m.tileY <= room.y1) {
                inSomeRoom = true;
                break;
            }
        }
        Expect(inSomeRoom, "every ambush-spawned monster should land inside SOME room's bounding box");
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        stormhold::AssetRoot assets(root);
        stormhold::ItemDatabase items = stormhold::ItemDatabase::Load(assets);
        stormhold::MonsterDatabase monsters = stormhold::MonsterDatabase::Load(assets);
        stormhold::DungeonGeometry geometry = stormhold::DungeonGeometry::Load(assets);

        TestStoreAndRemoveMonster(monsters);
        TestChestRegistry();
        TestDroppedItemRegistry();
        TestRefreshTileFlags(monsters);
        TestSpawnAmbushMonsters(geometry, items, monsters);

        if (!g_ok) {
            std::fprintf(stderr, "m16_dungeon_runtime_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m16_dungeon_runtime_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
