// M43 smoke test: PlayerMovement::ChestAheadOfPlayer/PlayerInventory::
// CollectChestItem -- Player.chestAheadOfPlayer()/collectChestItem(),
// both new this session (mirroring MonsterInFront's established shape),
// needed for GameCanvas.checkChestAhead()/resolveInteractInput()'s own
// chest-opening half (M41).
#include <cstdio>
#include <map>
#include <string>

#include "assets/asset_root.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "player/player_inventory.h"
#include "player/player_movement.h"
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

GeneratedLevel MakeOpenLevel(int number, int width = 35, int height = 35) {
    GeneratedLevel level;
    level.number = number;
    level.width = width;
    level.height = height;
    level.tiles.assign(static_cast<size_t>(width), std::vector<uint8_t>(static_cast<size_t>(height), 0));
    level.populated = true;
    return level;
}

// M52: PlayerInventory::CollectChestItem now takes a GameAdvancement::
// LevelLookup too, for the same real GameAdvancement::OpenZone wiring
// player/player_movement.h's own CommitMove already exercises. A single-
// level stand-in (returns `level` regardless of the number asked for) is
// enough for every test below whose own item never crosses a gift-points
// threshold, since OpenZone is then never actually called --
// TestCollectChestItemGiftCategory (which deliberately DOES cross one)
// uses a real lazily-populated all-levels lookup instead, below.
GameAdvancement::LevelLookup SingleLevelLookup(GeneratedLevel& level) {
    return [&level](int) -> GeneratedLevel& { return level; };
}

void TestChestAheadOfPlayer() {
    std::printf("-- ChestAheadOfPlayer: found ahead, nothing ahead, no-neighbor edge case --\n");
    GeneratedLevel level = MakeOpenLevel(5);
    WorldRegistry world(37);

    std::array<int8_t, 8> record{};
    record[0] = 10;
    record[1] = 9;  // one tile north of the player below
    record[4] = 42;  // item id
    DungeonRuntime::StoreChest(level, world, record);

    std::map<int, GeneratedLevel> cache;
    cache[5] = level;
    PlayerMovement::LevelLookup lookup = [&](int n) -> GeneratedLevel& { return cache.at(n); };

    PlayerState p;
    p.currentLevel = 5;
    p.tileX = 10;
    p.tileY = 10;
    p.facing = 1;  // north

    auto found = PlayerMovement::ChestAheadOfPlayer(p, lookup, world);
    Expect(found.has_value(), "a chest directly ahead is found");
    if (found.has_value()) {
        Expect((*found)[4] == 42, "the returned record is the real stored one, not a copy with stale data");
    }

    p.facing = 2;  // south -- nothing there
    auto nothing = PlayerMovement::ChestAheadOfPlayer(p, lookup, world);
    Expect(!nothing.has_value(), "nothing ahead resolves to nullopt");

    // No-neighbor edge case: same believed-unreachable landmine
    // ComputeMoveTarget itself already guards by throwing, MonsterInFront
    // already catches gracefully (M38) -- ChestAheadOfPlayer does too.
    GeneratedLevel isolated = MakeOpenLevel(6, 3, 3);
    std::map<int, GeneratedLevel> cache2;
    cache2[6] = isolated;
    PlayerMovement::LevelLookup lookup2 = [&](int n) -> GeneratedLevel& { return cache2.at(n); };
    PlayerState edge;
    edge.currentLevel = 6;
    edge.tileX = 0;
    edge.tileY = 0;
    edge.facing = 4;  // west, straight off the edge, no neighbor configured
    auto edgeResult = PlayerMovement::ChestAheadOfPlayer(edge, lookup2, world);
    Expect(!edgeResult.has_value(), "the no-neighbor edge case resolves to nullopt gracefully, not an exception");
}

void TestCollectChestItemWithSpace(const ItemDatabase& items) {
    std::printf("-- CollectChestItem: inventory has space -- item added, chest removed --\n");
    GeneratedLevel level = MakeOpenLevel(5);
    WorldRegistry world(37);

    std::array<int8_t, 8> record{};
    record[0] = 10;
    record[1] = 10;
    record[2] = 99;  // will be stamped to 2 (confirmed dead byte) and ignored either way
    record[3] = 7;   // tier byte, unused by this path
    record[4] = 1;   // item id 1
    record[5] = 0;   // packedValue high byte
    record[6] = 5;   // packedValue low byte
    record[7] = 3;   // charge
    DungeonRuntime::StoreChest(level, world, record);

    PlayerState p;
    p.inventoryCount = 0;
    int16_t before = p.giftPointsFound;

    int result = PlayerInventory::CollectChestItem(p, record, items, level, world, SingleLevelLookup(level));
    Expect(result == 1, "a normal collect with inventory space returns 1");
    Expect(p.inventoryCount == 1, "the item was actually added to the inventory");
    Expect(p.inventoryItemIds[0] == 1, "the added item's id matches record[4]");
    Expect((level.tiles[10][10] & 0x10) == 0, "the chest's presence bit is cleared once removed");

    bool foundInRegistry = world.chests[static_cast<size_t>(level.number - 1)].count(PackTileKey(10, 10)) > 0;
    Expect(!foundInRegistry, "the chest is gone from the registry");

    if (items.category[0] != 11) {
        Expect(p.giftPointsFound == before, "item 1 isn't category 11 here, so no gift points should be granted");
    }
}

void TestCollectChestItemGiftCategory(const ItemDatabase& items) {
    std::printf("-- CollectChestItem: a real category-11 (gift) item grants gift points --\n");
    int giftItemId = -1;
    for (int i = 0; i < items.ItemCount(); i++) {
        if (items.category[static_cast<size_t>(i)] == 11) {
            giftItemId = i + 1;
            break;
        }
    }
    if (!Expect(giftItemId > 0, "at least one real category-11 item exists in itemsin.dat")) return;

    GeneratedLevel level = MakeOpenLevel(5);
    WorldRegistry world(37);
    std::array<int8_t, 8> record{};
    record[0] = 5;
    record[1] = 5;
    record[4] = static_cast<int8_t>(giftItemId);
    DungeonRuntime::StoreChest(level, world, record);

    PlayerState p;
    p.inventoryCount = 0;
    p.giftPointsFound = 0;

    // M52: a real gift item DOES cross a gift-points threshold here (any
    // category-11 item's own subtype is >= 9, i.e. at least zone 0's own
    // boundary), so GameAdvancement::OpenZone runs for real inside
    // CollectChestItem -- give it a lookup that can resolve ANY of the
    // 37 levels on demand, not just this test's own level 5, since which
    // zone actually opens depends on the real item's own subtype value.
    std::map<int, GeneratedLevel> allLevels;
    GameAdvancement::LevelLookup lazyLookup = [&allLevels](int n) -> GeneratedLevel& {
        auto it = allLevels.find(n);
        if (it != allLevels.end()) return it->second;
        return allLevels.emplace(n, MakeOpenLevel(n)).first->second;
    };

    int result = PlayerInventory::CollectChestItem(p, record, items, level, world, lazyLookup);
    Expect(result == 1, "collecting a real gift item with space succeeds");
    int expectedSubtype = items.subtype[static_cast<size_t>(giftItemId - 1)];
    Expect(p.giftPointsFound == expectedSubtype,
           "giftPointsFound increases by exactly the item's own subtype column");

    // Every real category-11 item's own subtype is >= 9 (GameAdvancement
    // ::Level's own bucket-0 upper bound), so SOME zone always opens --
    // `allLevels` only ever gets entries via `lazyLookup`'s own on-demand
    // creation, and the only caller that ever invokes `lazyLookup` in
    // this test is `GameAdvancement::OpenZone` itself (inside
    // CollectChestItem) -- so a non-empty, all-`populated` result here
    // confirms the M52 wiring fired for real, without this test needing
    // its own copy of GameAdvancement.cpp's own private zone table (see
    // m52_game_advancement_smoke.cpp for that exact-membership check).
    Expect(!allLevels.empty(), "OpenZone touched at least one level -- the M52 wiring fired");
    bool allOpened = true;
    for (const auto& [number, openedLevel] : allLevels) allOpened = allOpened && openedLevel.populated;
    Expect(allOpened, "every level OpenZone touched ended up populated");
}

void TestCollectChestItemNoSpace(const ItemDatabase& items) {
    std::printf("-- CollectChestItem: inventory full -- item auto-drops instead, chest still removed --\n");
    GeneratedLevel level = MakeOpenLevel(5);
    WorldRegistry world(37);

    std::array<int8_t, 8> record{};
    record[0] = 3;
    record[1] = 3;
    record[4] = 7;
    record[5] = 0;
    record[6] = 1;
    record[7] = 0;
    DungeonRuntime::StoreChest(level, world, record);

    PlayerState p;
    p.inventoryCount = 24;  // completely full

    int result = PlayerInventory::CollectChestItem(p, record, items, level, world, SingleLevelLookup(level));
    Expect(result == 0, "a full inventory returns 0");
    Expect(p.inventoryCount == 24, "the inventory itself is untouched");

    auto dropped = DungeonRuntime::FirstDroppedItemAt(world, level.number - 1, 3, 3);
    Expect(dropped.has_value(), "the item lands as a dropped-item record on the same tile instead");
    if (dropped.has_value()) {
        Expect((*dropped)[2] == 7, "the dropped record's item id matches the chest's own record[4]");
        Expect((*dropped)[6] == 1, "the dropped record's own presence byte (index 6) is stamped to 1");
    }

    bool foundInRegistry = world.chests[static_cast<size_t>(level.number - 1)].count(PackTileKey(3, 3)) > 0;
    Expect(!foundInRegistry, "the chest is still removed from the registry even on the full-inventory path");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assets(root);
        ItemDatabase items = ItemDatabase::Load(assets);

        TestChestAheadOfPlayer();
        TestCollectChestItemWithSpace(items);
        TestCollectChestItemGiftCategory(items);
        TestCollectChestItemNoSpace(items);

        if (!g_ok) {
            std::fprintf(stderr, "m43_chest_interaction_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m43_chest_interaction_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
