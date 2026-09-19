// M12 smoke test: PlayerInventory's general-purpose inventory/equip/camp
// methods against real CharacterData/ItemDatabase.
#include <cstdio>
#include <cstdlib>
#include <string>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/item_database.h"
#include "player/player_creation.h"
#include "player/player_inventory.h"

namespace {

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

void TestRemoveAndReEquip(const stormhold::CharacterData& charData, const stormhold::ItemDatabase& items) {
    std::printf("-- remove/re-equip --\n");
    // Barbarian (class 0): Miner Pick (weapon, equip slot 0) + Padded
    // Cloth (armor, equip slot 1), both granted+equipped by creation.
    stormhold::PlayerState p = stormhold::PlayerCreation::CreateCharacter(0, "Test", 7, charData, items);
    int weaponEquipSlot = items.EquipSlotOf(1);
    int armorEquipSlot = items.EquipSlotOf(27);

    Expect(p.inventoryCount == 2, "should start with 2 items");
    bool removed = stormhold::PlayerInventory::RemoveInventorySlot(p, 0, items);
    Expect(removed, "removing slot 0 (the equipped weapon) should succeed");
    Expect(p.inventoryCount == 1, "inventoryCount should drop to 1 after removal");
    Expect(p.equippedItems[static_cast<size_t>(weaponEquipSlot)] == 0,
           "removing an equipped item should clear its equip slot");
    Expect(std::abs(p.inventoryItemIds[0]) == 27, "the armor should have compacted down into slot 0");
    Expect(p.equippedItems[static_cast<size_t>(armorEquipSlot)] == 27, "the armor should still be equipped");

    // Add the weapon back and equip it via EquipLastPickedUpItem.
    stormhold::PlayerInventory::AddInventoryItemRaw(p, 1, 7, 0);
    Expect(p.inventoryCount == 2, "inventoryCount should be back to 2");
    Expect(p.inventoryItemIds[1] == 1, "the re-added weapon should be un-equipped (positive id) initially");
    bool equipped = stormhold::PlayerInventory::EquipLastPickedUpItem(p, true, items);
    Expect(equipped, "EquipLastPickedUpItem should succeed");
    Expect(p.inventoryItemIds[1] == -1, "the weapon should now be equipped (negative id)");
    Expect(p.equippedItems[static_cast<size_t>(weaponEquipSlot)] == 1, "equippedItems should show the weapon again");
}

void TestItemCharge(const stormhold::CharacterData& charData, const stormhold::ItemDatabase& items) {
    std::printf("-- item charge --\n");
    stormhold::PlayerState p = stormhold::PlayerCreation::CreateCharacter(0, "Test", 7, charData, items);

    Expect(!stormhold::PlayerInventory::IsItemCharged(p, 0), "a freshly granted item should not start charged");
    bool charged = stormhold::PlayerInventory::InitializeItemCharge(p, 0, items);
    Expect(charged, "InitializeItemCharge should succeed on an equipment-category item");
    Expect(stormhold::PlayerInventory::IsItemCharged(p, 0), "the item should read as charged afterward");

    // A category-11 ("gift") item is NOT an equipment category -- find
    // one and confirm InitializeItemCharge correctly refuses it.
    int giftItemId = -1;
    for (int id = 1; id <= items.ItemCount(); id++) {
        if (items.category[static_cast<size_t>(id - 1)] == 11) {
            giftItemId = id;
            break;
        }
    }
    Expect(giftItemId != -1, "real itemsin.dat should have at least one category-11 item");
    stormhold::PlayerInventory::AddInventoryItemRaw(p, giftItemId, 1, 0);
    int giftSlot = p.inventoryCount - 1;
    bool giftCharged = stormhold::PlayerInventory::InitializeItemCharge(p, giftSlot, items);
    Expect(!giftCharged, "InitializeItemCharge should refuse a non-equipment-category item");
}

stormhold::GeneratedLevel MakeLevel(int number, int width = 35, int height = 35) {
    stormhold::GeneratedLevel level;
    level.number = number;
    level.width = width;
    level.height = height;
    level.tiles.assign(static_cast<size_t>(width), std::vector<uint8_t>(static_cast<size_t>(height), 0));
    return level;
}

void TestPickUpDropRoundTrip(const stormhold::CharacterData& charData, const stormhold::ItemDatabase& items) {
    std::printf("-- pick up / drop round trip --\n");
    stormhold::PlayerState p = stormhold::PlayerCreation::CreateCharacter(0, "Test", 7, charData, items);
    p.tileX = 5;
    p.tileY = 6;
    p.currentLevel = 5;
    stormhold::GeneratedLevel level = MakeLevel(5);
    stormhold::WorldRegistry world(37);

    auto record = stormhold::PlayerInventory::DropInventoryItem(p, 0, items, level, world);
    Expect(record.has_value(), "dropping a normal item should produce a record");
    Expect(p.inventoryCount == 1, "dropping should remove the slot");
    Expect((*record)[0] == 5 && (*record)[1] == 6, "the record should carry the player's tile position");
    Expect((*record)[2] == 1, "the record should carry the dropped item's id (Miner Pick = 1)");
    Expect((level.tiles[5][6] & 4) != 0, "M17: DropInventoryItem should register the drop into the world registry");
    Expect(world.droppedItems[4].size() == 1, "M17: the record should land in the level's own dropped-item list");

    stormhold::PlayerState fresh;
    bool pickedUp = stormhold::PlayerInventory::TryPickUpItem(fresh, *record);
    Expect(pickedUp, "picking the record back up should succeed");
    Expect(fresh.inventoryCount == 1 && fresh.inventoryItemIds[0] == 1,
           "picking up should restore the same item id into a fresh inventory");

    // Item 109 -- non-droppable special id -- should still remove the
    // slot but produce NO record at all (and never touch the registry).
    stormhold::PlayerState p2 = stormhold::PlayerCreation::CreateCharacter(0, "Test", 7, charData, items);
    stormhold::PlayerInventory::AddInventoryItemRaw(p2, 109, 1, 0);
    int slot109 = p2.inventoryCount - 1;
    auto record109 = stormhold::PlayerInventory::DropInventoryItem(p2, slot109, items, level, world);
    Expect(!record109.has_value(), "dropping item 109 should produce no record");
    Expect(p2.inventoryCount == 2, "dropping item 109 should still remove its slot");
    Expect(world.droppedItems[4].size() == 1, "dropping item 109 should NOT add a second world registry entry");
}

void TestSpawnIdSignExtensionQuirk(const stormhold::ItemDatabase& items) {
    std::printf("-- confirmed sign-extension quirk in the packed-value round trip (preserved, not fixed) --\n");
    stormhold::GeneratedLevel level = MakeLevel(6);
    stormhold::WorldRegistry world(37);

    // Normal-range packed value (< 32768, e.g. a realistic Item.nextSpawnId()
    // result): round-trips correctly.
    stormhold::PlayerState pNormal;
    stormhold::PlayerInventory::AddInventoryItemRaw(pNormal, /*itemId=*/1, /*packedValue=*/7, /*charge=*/0);
    auto recordNormal = stormhold::PlayerInventory::DropInventoryItem(pNormal, 0, items, level, world);
    stormhold::PlayerState freshNormal;
    stormhold::PlayerInventory::TryPickUpItem(freshNormal, *recordNormal);
    Expect((freshNormal.inventoryItemData[0] >> 16) == 7, "a normal packed value (7) should round-trip correctly");

    // A packed value >= 32768: the high byte's sign bit is set, so
    // dropInventoryItem/tryPickUpItem's shared 2-signed-byte packing
    // sign-extends on the way back in, reconstructing a NEGATIVE value
    // instead of the original one. Confirmed real Java `byte` arithmetic
    // (record[hi] << 8 sign-extends when record[hi] is a signed byte
    // >= 0x80), not a transcription slip on this port's side -- the same
    // shape as Player.collectChestItem()'s own `(record[5] << 8) +
    // record[6]` chest-value unpacking. Whether real gameplay can ever
    // actually produce a packed value in this range depends on every
    // producer across the whole codebase (Item.nextSpawnId() alone is
    // bounded to a Java `short`'s positive range under normal play, so
    // THAT specific producer wouldn't reach it) -- not exhaustively
    // traced here, out of this milestone's scope. What matters for this
    // milestone is that TryPickUpItem/DropInventoryItem reproduce the
    // mechanical behavior exactly, demonstrated directly below.
    stormhold::PlayerState pHigh;
    stormhold::PlayerInventory::AddInventoryItemRaw(pHigh, /*itemId=*/1, /*packedValue=*/40000, /*charge=*/0);
    auto recordHigh = stormhold::PlayerInventory::DropInventoryItem(pHigh, 0, items, level, world);
    stormhold::PlayerState freshHigh;
    stormhold::PlayerInventory::TryPickUpItem(freshHigh, *recordHigh);
    int32_t reconstructed = freshHigh.inventoryItemData[0] >> 16;
    Expect(reconstructed != 40000, "a packed value >= 32768 should NOT round-trip cleanly (confirmed quirk)");
    Expect(reconstructed < 0, "the reconstructed high word should come back negative (sign-extension)");
    std::printf("  packed value 40000 dropped and re-picked-up reconstructs as %d (confirmed quirk, not a bug here)\n",
                reconstructed);
}

void TestCampBookmark() {
    std::printf("-- camp bookmark --\n");
    stormhold::PlayerState p;
    p.currentLevel = 5;
    p.tileX = 10;
    p.tileY = 20;
    p.facing = 3;

    Expect(!stormhold::PlayerInventory::HasCampMark(p), "a fresh character should have no camp mark");

    stormhold::PlayerInventory::MarkCampAndReturnToTown(p);
    Expect(stormhold::PlayerInventory::HasCampMark(p), "should have a camp mark after MarkCampAndReturnToTown");
    Expect(p.campLevel == 5 && p.campX == 10 && p.campY == 20 && p.campFacing == 3,
           "the camp bookmark should capture the pre-call position");
    Expect(p.currentLevel == 1 && p.tileX == 12 && p.tileY == 14 && p.facing == 1,
           "should respawn at the DEATH/RESPAWN hub point (12, 14), not character creation's (9, 10)");
    Expect(p.justMarkedCamp, "justMarkedCamp should be true");

    p.currentLevel = 9;
    p.tileX = 1;
    p.tileY = 1;
    p.facing = 4;
    p.justMarkedCamp = false;
    stormhold::PlayerInventory::WarpToCampMark(p);
    Expect(p.currentLevel == 5 && p.tileX == 10 && p.tileY == 20, "warping back should restore the camp position");
    Expect(p.facing == 4, "WarpToCampMark should NOT touch facing at all (confirmed by reading the whole method)");
    Expect(p.justMarkedCamp, "justMarkedCamp should be true again after warping");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        stormhold::AssetRoot assets(root);
        stormhold::CharacterData charData = stormhold::CharacterData::Load(assets);
        stormhold::ItemDatabase items = stormhold::ItemDatabase::Load(assets);

        TestRemoveAndReEquip(charData, items);
        TestItemCharge(charData, items);
        TestPickUpDropRoundTrip(charData, items);
        TestSpawnIdSignExtensionQuirk(items);
        TestCampBookmark();

        if (!g_ok) {
            std::fprintf(stderr, "m12_player_inventory_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m12_player_inventory_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
