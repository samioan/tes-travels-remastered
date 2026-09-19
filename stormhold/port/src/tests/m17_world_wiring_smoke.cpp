// M17 smoke test: the wiring milestone itself -- player/player_movement.h's
// CommitMove now actually consuming M16's WorldRegistry for dropped-item
// auto-loot (including a real, confirmed bit-test asymmetry between the
// original's "one item on this tile" and "several items on this tile"
// branches), player/player_inventory.h's DropInventoryItem now actually
// registering into it, and combat/combat_resolution.h's target.store()/
// Dungeon.spawnAmbushMonsters() wiring (the latter two also get dedicated
// coverage added directly to m14_monster_combat_smoke.cpp, since that's
// where PlayerAttack/MonsterTick's own tests already live -- this file
// focuses on the movement/inventory side, the bigger NEW surface).
#include <cstdio>
#include <map>
#include <string>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/item_database.h"
#include "player/player_creation.h"
#include "player/player_inventory.h"
#include "player/player_movement.h"

namespace {

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

stormhold::GeneratedLevel MakeLevel(int number, int width = 35, int height = 35) {
    stormhold::GeneratedLevel level;
    level.number = number;
    level.width = width;
    level.height = height;
    level.tiles.assign(static_cast<size_t>(width), std::vector<uint8_t>(static_cast<size_t>(height), 0));
    return level;
}

// Finds two real item ids in category 11 ("gift") with distinct subtype
// values, so a test can tell which one's subtype actually got added to
// giftPointsFound.
struct GiftItems {
    int idA = -1, subtypeA = 0;
    int idB = -1, subtypeB = 0;
};

GiftItems FindTwoGiftItems(const stormhold::ItemDatabase& items) {
    GiftItems out;
    for (int id = 1; id <= items.ItemCount(); id++) {
        if (items.category[static_cast<size_t>(id - 1)] != 11) continue;
        if (out.idA < 0) {
            out.idA = id;
            out.subtypeA = items.subtype[static_cast<size_t>(id - 1)];
        } else if (out.idB < 0 && items.subtype[static_cast<size_t>(id - 1)] != out.subtypeA) {
            out.idB = id;
            out.subtypeB = items.subtype[static_cast<size_t>(id - 1)];
            break;
        }
    }
    return out;
}

void TestSingleItemNotPossessedBeforeGrantsGiftPoints(const stormhold::ItemDatabase& items) {
    std::printf("-- single dropped gift item, not possessed before -> giftPointsFound grows --\n");
    GiftItems gift = FindTwoGiftItems(items);
    Expect(gift.idA > 0, "real itemsin.dat should have at least one category-11 item");
    if (gift.idA < 0) return;

    stormhold::GeneratedLevel level = MakeLevel(5);
    stormhold::WorldRegistry world(37);
    stormhold::MonsterDatabase monsters{};
    stormhold::WardenState warden{};
    std::array<int8_t, 7> record = {10, 10, static_cast<int8_t>(gift.idA), 0, 0, 0, 1};  // bit2 clear: not possessed
    stormhold::DungeonRuntime::AddDroppedItem(level, world, record);

    std::map<int, stormhold::GeneratedLevel> cache;
    cache[5] = level;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& { return cache.at(n); };

    stormhold::PlayerState p;
    p.currentLevel = 5;
    p.tileX = 9;
    p.tileY = 10;
    p.facing = 2;  // east, straight onto the dropped item's tile
    p.coreStats[6] = 100;

    bool moved = stormhold::PlayerMovement::CommitMove(p, 1, lookup, world, items, monsters, warden);
    Expect(moved, "stepping onto the dropped item's tile should succeed");
    Expect(p.inventoryCount == 1, "the item should be auto-picked-up");
    Expect(p.giftPointsFound == gift.subtypeA, "giftPointsFound should grow by the item's own subtype value");
    Expect(world.droppedItems[4].empty(), "the registry entry should be removed after pickup");
    Expect((cache.at(5).tiles[10][10] & 4) == 0, "the dropped-item tile bit should clear once the last item is gone");
}

void TestSingleItemAlreadyPossessedSkipsGiftPoints(const stormhold::ItemDatabase& items) {
    std::printf("-- single dropped gift item, ALREADY possessed -> no giftPointsFound change (real asymmetry) --\n");
    GiftItems gift = FindTwoGiftItems(items);
    if (gift.idA < 0) return;

    stormhold::GeneratedLevel level = MakeLevel(6);
    stormhold::WorldRegistry world(37);
    stormhold::MonsterDatabase monsters{};
    stormhold::WardenState warden{};
    std::array<int8_t, 7> record = {10, 10, static_cast<int8_t>(gift.idA), 0, 0, 0, 3};  // bit2 SET: already possessed
    stormhold::DungeonRuntime::AddDroppedItem(level, world, record);

    std::map<int, stormhold::GeneratedLevel> cache;
    cache[6] = level;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& { return cache.at(n); };

    stormhold::PlayerState p;
    p.currentLevel = 6;
    p.tileX = 9;
    p.tileY = 10;
    p.facing = 2;
    p.coreStats[6] = 100;

    bool moved = stormhold::PlayerMovement::CommitMove(p, 1, lookup, world, items, monsters, warden);
    Expect(moved, "stepping onto the dropped item's tile should succeed");
    Expect(p.inventoryCount == 1, "the item should still be auto-picked-up regardless of the possessed flag");
    Expect(p.giftPointsFound == 0,
           "the SINGLE-item branch requires bit2 CLEAR to grant gift points -- an already-possessed item should "
           "NOT grow giftPointsFound here, confirmed asymmetric vs. the multi-item branch below");
}

void TestSingleItemLockedSetsPendingFlagAndSkipsPickup(const stormhold::ItemDatabase& items) {
    std::printf("-- single dropped item, LOCKED -> pendingLockedItemFlag, no pickup, early return --\n");
    stormhold::GeneratedLevel level = MakeLevel(7);
    stormhold::WorldRegistry world(37);
    stormhold::MonsterDatabase monsters{};
    stormhold::WardenState warden{};
    std::array<int8_t, 7> record = {10, 10, 1, 0, 0, 0, 5};  // bit4 (locked) | bit0
    stormhold::DungeonRuntime::AddDroppedItem(level, world, record);

    std::map<int, stormhold::GeneratedLevel> cache;
    cache[7] = level;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& { return cache.at(n); };

    stormhold::PlayerState p;
    p.currentLevel = 7;
    p.tileX = 9;
    p.tileY = 10;
    p.facing = 2;
    p.coreStats[6] = 100;

    bool moved = stormhold::PlayerMovement::CommitMove(p, 1, lookup, world, items, monsters, warden);
    Expect(moved, "CommitMove should still return true on a locked-item tile (the original's own early `return "
                  "true`, not a failed move)");
    Expect(p.pendingLockedItemFlag, "pendingLockedItemFlag should be set");
    Expect(p.inventoryCount == 0, "a locked item should NOT be auto-picked-up");
    Expect(world.droppedItems[6].size() == 1, "the locked item's registry entry should remain untouched");
    // The early return also means the auto-camp-mark check (isStep && bit
    // 8) never runs, even if the tile happened to carry bit 8 too -- not
    // separately exercised here, matches CommitMove's own control flow.
}

void TestMultiItemAsymmetryOppositeOfSingleItem(const stormhold::ItemDatabase& items) {
    std::printf("-- two dropped gift items on one tile -- confirmed OPPOSITE bit test vs. the single-item case --\n");
    GiftItems gift = FindTwoGiftItems(items);
    Expect(gift.idB > 0, "real itemsin.dat should have at least two category-11 items with distinct subtypes");
    if (gift.idB < 0) return;

    stormhold::GeneratedLevel level = MakeLevel(8);
    stormhold::WorldRegistry world(37);
    stormhold::MonsterDatabase monsters{};
    stormhold::WardenState warden{};
    // Item A: bit2 CLEAR ("not possessed before") -- per the MULTI-item
    // branch's own (record[6] & 2) != 0 condition, this one should NOT
    // grant gift points (opposite of the single-item branch's rule).
    std::array<int8_t, 7> recordA = {10, 10, static_cast<int8_t>(gift.idA), 0, 0, 0, 1};
    // Item B: bit2 SET ("already possessed") -- this one SHOULD grant
    // gift points here.
    std::array<int8_t, 7> recordB = {10, 10, static_cast<int8_t>(gift.idB), 0, 0, 0, 3};
    stormhold::DungeonRuntime::AddDroppedItem(level, world, recordA);
    stormhold::DungeonRuntime::AddDroppedItem(level, world, recordB);
    Expect(stormhold::DungeonRuntime::CountDroppedItemsAt(world, 7, 10, 10) == 2, "both records should be registered");

    std::map<int, stormhold::GeneratedLevel> cache;
    cache[8] = level;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& { return cache.at(n); };

    stormhold::PlayerState p;
    p.currentLevel = 8;
    p.tileX = 9;
    p.tileY = 10;
    p.facing = 2;
    p.coreStats[6] = 100;

    bool moved = stormhold::PlayerMovement::CommitMove(p, 1, lookup, world, items, monsters, warden);
    Expect(moved, "stepping onto a tile with 2 dropped items should succeed");
    Expect(p.inventoryCount == 2, "both items should be picked up");
    Expect(p.giftPointsFound == gift.subtypeB,
           "only item B's (bit2 SET) subtype should be added -- item A's (bit2 clear) should NOT contribute, "
           "the exact opposite rule from the single-item branch");
    Expect(world.droppedItems[7].empty(), "both registry entries should be removed after pickup");
}

void TestNonGiftItemPicksUpWithNoPointsChange(const stormhold::CharacterData& charData,
                                               const stormhold::ItemDatabase& items) {
    std::printf("-- non-gift item auto-loot: picked up, no giftPointsFound change --\n");
    stormhold::PlayerState p = stormhold::PlayerCreation::CreateCharacter(0, "Looter", 7, charData, items);
    int startCount = p.inventoryCount;
    p.currentLevel = 9;
    p.tileX = 9;
    p.tileY = 10;
    p.facing = 2;
    p.coreStats[6] = 100;

    stormhold::GeneratedLevel level = MakeLevel(9);
    stormhold::WorldRegistry world(37);
    stormhold::MonsterDatabase monsters{};
    stormhold::WardenState warden{};
    std::array<int8_t, 7> record = {10, 10, 1, 0, 0, 0, 1};  // item id 1 (Miner Pick), category != 11
    stormhold::DungeonRuntime::AddDroppedItem(level, world, record);

    std::map<int, stormhold::GeneratedLevel> cache;
    cache[9] = level;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& { return cache.at(n); };

    stormhold::PlayerMovement::CommitMove(p, 1, lookup, world, items, monsters, warden);
    Expect(p.inventoryCount == startCount + 1, "a non-gift item should still be auto-picked-up");
    Expect(p.giftPointsFound == 0, "a non-category-11 item should never affect giftPointsFound");
}

void TestAutoMarkCampOnTile() {
    std::printf("-- auto-camp-mark on a bit-8 tile --\n");
    stormhold::GeneratedLevel level = MakeLevel(10);
    level.tiles[10][10] = 8;  // bit 8: auto-camp-mark tile
    stormhold::WorldRegistry world(37);
    stormhold::MonsterDatabase monsters{};
    stormhold::WardenState warden{};
    stormhold::ItemDatabase items{};  // unused by this path -- no dropped item on this tile

    std::map<int, stormhold::GeneratedLevel> cache;
    cache[10] = level;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& { return cache.at(n); };

    stormhold::PlayerState p;
    p.currentLevel = 10;
    p.tileX = 9;
    p.tileY = 10;
    p.facing = 2;
    p.coreStats[6] = 100;

    bool moved = stormhold::PlayerMovement::CommitMove(p, 1, lookup, world, items, monsters, warden);
    Expect(moved, "stepping onto a bit-8 tile should succeed");
    Expect(p.campLevel == 10 && p.campX == 10 && p.campY == 10,
           "the camp bookmark should capture the JUST-STEPPED-ONTO tile (commitMove updates position BEFORE the "
           "auto-camp-mark check runs, matching the original's own control flow)");
    Expect(p.currentLevel == 1 && p.tileX == 12 && p.tileY == 14 && p.facing == 1,
           "should respawn at the hub's DEATH/RESPAWN point after an auto-camp-mark, same as a manual one");
    Expect(!p.justMarkedCamp, "autoMarkCampOnTile immediately clears justMarkedCamp back to false");
}

void TestDropThenWalkBackPicksItUpAgain(const stormhold::CharacterData& charData, const stormhold::ItemDatabase& items) {
    std::printf("-- DropInventoryItem then walking back onto the tile picks it back up (end-to-end) --\n");
    stormhold::PlayerState p = stormhold::PlayerCreation::CreateCharacter(0, "RoundTrip", 7, charData, items);
    p.currentLevel = 11;
    p.tileX = 15;
    p.tileY = 15;
    p.facing = 2;  // east
    p.coreStats[6] = 100;
    int startCount = p.inventoryCount;

    stormhold::GeneratedLevel level = MakeLevel(11);
    stormhold::WorldRegistry world(37);
    stormhold::MonsterDatabase monsters{};
    stormhold::WardenState warden{};

    auto dropped = stormhold::PlayerInventory::DropInventoryItem(p, 0, items, level, world);
    Expect(dropped.has_value(), "dropping slot 0 should produce a record");
    Expect(p.inventoryCount == startCount - 1, "dropping should remove the slot");
    Expect((level.tiles[15][15] & 4) != 0, "the drop should set the dropped-item tile bit at the player's position");
    Expect(world.droppedItems[10].size() == 1, "the drop should register into the level's own dropped-item list");

    std::map<int, stormhold::GeneratedLevel> cache;
    cache[11] = level;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& { return cache.at(n); };

    // Step away, then back.
    bool steppedAway = stormhold::PlayerMovement::CommitMove(p, 1, lookup, world, items, monsters, warden);
    Expect(steppedAway, "stepping away from the drop tile should succeed");
    Expect(p.inventoryCount == startCount - 1, "walking away should not pick anything up");

    bool steppedBack = stormhold::PlayerMovement::CommitMove(p, 2, lookup, world, items, monsters, warden);
    Expect(steppedBack, "stepping back onto the drop tile should succeed");
    Expect(p.inventoryCount == startCount, "walking back onto the drop tile should auto-pick the item back up");
    Expect(world.droppedItems[10].empty(), "the registry entry should be removed after re-pickup");
    Expect((cache.at(11).tiles[15][15] & 4) == 0, "the dropped-item tile bit should clear after re-pickup");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        stormhold::AssetRoot assets(root);
        stormhold::CharacterData charData = stormhold::CharacterData::Load(assets);
        stormhold::ItemDatabase items = stormhold::ItemDatabase::Load(assets);

        TestSingleItemNotPossessedBeforeGrantsGiftPoints(items);
        TestSingleItemAlreadyPossessedSkipsGiftPoints(items);
        TestSingleItemLockedSetsPendingFlagAndSkipsPickup(items);
        TestMultiItemAsymmetryOppositeOfSingleItem(items);
        TestNonGiftItemPicksUpWithNoPointsChange(charData, items);
        TestAutoMarkCampOnTile();
        TestDropThenWalkBackPicksItUpAgain(charData, items);

        if (!g_ok) {
            std::fprintf(stderr, "m17_world_wiring_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m17_world_wiring_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
