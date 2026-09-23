#include "player/player_inventory.h"

#include <cmath>
#include <stdexcept>

namespace stormhold {

bool PlayerInventory::AddInventoryItemRaw(PlayerState& p, int itemId, int packedValue, int charge) {
    if (p.inventoryCount >= 24) return false;
    int slot = p.inventoryCount;
    p.inventoryItemIds[static_cast<size_t>(slot)] = static_cast<int8_t>(itemId);
    int32_t packed = (static_cast<int32_t>(packedValue) << 16) + static_cast<int8_t>(charge);
    p.inventoryItemData[static_cast<size_t>(slot)] = packed;
    p.inventoryCount++;
    return true;
}

void PlayerInventory::UnequipSlot(PlayerState& p, int slot, const ItemDatabase& items) {
    int8_t id = p.inventoryItemIds[static_cast<size_t>(slot)];
    bool isEquipped = items.IsEquipmentCategory(static_cast<int>(std::abs(id))) && id < 0;
    if (!isEquipped) return;

    id = static_cast<int8_t>(std::abs(id));
    p.inventoryItemIds[static_cast<size_t>(slot)] = id;
    for (int i = 0; i < 7; i++) {
        if (p.equippedItems[static_cast<size_t>(i)] == id) {
            p.equippedItems[static_cast<size_t>(i)] = 0;
            break;
        }
    }
}

namespace {
// Player.unequipMatchingCategory(equipSlot) -- private helper, only
// EquipItem's swap path calls this.
void UnequipMatchingEquipSlot(PlayerState& p, int equipSlotWanted, const ItemDatabase& items) {
    for (int i = 0; i < p.inventoryCount; i++) {
        int id = std::abs(p.inventoryItemIds[static_cast<size_t>(i)]);
        if (items.EquipSlotOf(id) == equipSlotWanted) {
            PlayerInventory::UnequipSlot(p, i, items);
        }
    }
}
}  // namespace

bool PlayerInventory::EquipItem(PlayerState& p, int slot, bool allowSwap, const ItemDatabase& items) {
    int8_t id = p.inventoryItemIds[static_cast<size_t>(slot)];
    if (id < 0) return false;
    if (!items.IsEquipmentCategory(id)) return false;

    int equipSlot = items.EquipSlotOf(id);
    if (p.equippedItems[static_cast<size_t>(equipSlot)] != 0) {
        if (!allowSwap) return false;
        UnequipMatchingEquipSlot(p, equipSlot, items);
    }

    p.equippedItems[static_cast<size_t>(equipSlot)] = id;
    p.inventoryItemIds[static_cast<size_t>(slot)] = static_cast<int8_t>(-std::abs(id));
    return true;
}

bool PlayerInventory::EquipLastPickedUpItem(PlayerState& p, bool allowSwap, const ItemDatabase& items) {
    int slot = p.inventoryCount - 1;
    return EquipItem(p, slot, allowSwap, items);
}

bool PlayerInventory::RemoveInventorySlot(PlayerState& p, int slot, const ItemDatabase& items) {
    if (slot < 0) {
        throw std::runtime_error("PlayerInventory::RemoveInventorySlot: negative slot -- the real "
                                  "removeInventorySlot() has no guard here either, so the original would "
                                  "throw ArrayIndexOutOfBoundsException reading this.H[-1]");
    }
    if (slot >= p.inventoryCount) return false;

    UnequipSlot(p, slot, items);
    p.inventoryItemIds[static_cast<size_t>(slot)] = 0;

    for (int i = slot; i < p.inventoryCount - 1; i++) {
        p.inventoryItemIds[static_cast<size_t>(i)] = p.inventoryItemIds[static_cast<size_t>(i + 1)];
        p.inventoryItemData[static_cast<size_t>(i)] = p.inventoryItemData[static_cast<size_t>(i + 1)];
    }

    p.inventoryCount--;
    return true;
}

int PlayerInventory::FindEquippedSlotForItem(const PlayerState& p, int itemId) {
    int8_t target = static_cast<int8_t>(-std::abs(itemId));
    for (int i = 0; i < p.inventoryCount; i++) {
        if (target == p.inventoryItemIds[static_cast<size_t>(i)]) return i;
    }
    return -1;
}

bool PlayerInventory::InitializeItemCharge(PlayerState& p, int slot, const ItemDatabase& items) {
    int id = std::abs(p.inventoryItemIds[static_cast<size_t>(slot)]);
    if (!items.IsEquipmentCategory(id)) return false;

    int32_t charge = 3;
    p.inventoryItemData[static_cast<size_t>(slot)] =
        (p.inventoryItemData[static_cast<size_t>(slot)] & ~static_cast<int32_t>(0xFF)) | charge;
    return true;
}

bool PlayerInventory::IsItemCharged(const PlayerState& p, int slot) {
    int8_t charge = static_cast<int8_t>(p.inventoryItemData[static_cast<size_t>(slot)] & 0xFF);
    return charge == 3;
}

bool PlayerInventory::TryPickUpItem(PlayerState& p, const std::array<int8_t, 7>& record) {
    int8_t itemId = record[2];
    // Sign-extending shift, matching Java's `(record[3] << 8) +
    // record[4]` on signed bytes exactly -- see this method's own
    // declaration comment / DropInventoryItem's header comment.
    int32_t packedValue = (static_cast<int32_t>(record[3]) << 8) + record[4];
    int8_t charge = record[5];
    return AddInventoryItemRaw(p, itemId, packedValue, charge);
}

std::optional<std::array<int8_t, 7>> PlayerInventory::DropInventoryItem(PlayerState& p, int slot,
                                                                         const ItemDatabase& items,
                                                                         GeneratedLevel& level, WorldRegistry& world) {
    int itemId = std::abs(p.inventoryItemIds[static_cast<size_t>(slot)]);
    if (itemId == 109) {
        RemoveInventorySlot(p, slot, items);
        return std::nullopt;
    }

    std::array<int8_t, 7> record{};
    record[0] = p.tileX;
    record[1] = p.tileY;
    record[2] = static_cast<int8_t>(itemId);
    record[5] = static_cast<int8_t>(p.inventoryItemData[static_cast<size_t>(slot)] & 0xFF);

    uint32_t extended = static_cast<uint32_t>(p.inventoryItemData[static_cast<size_t>(slot)]) >> 16 & 0xFFFFu;
    record[3] = static_cast<int8_t>(extended >> 8 & 0xFF);
    record[4] = static_cast<int8_t>(extended & 0xFF);
    record[6] = 3;

    // Player.dropInventoryItem()'s own `this.currentDungeon().
    // addDroppedItem(record)` call -- see this method's own declaration
    // comment.
    DungeonRuntime::AddDroppedItem(level, world, record);

    RemoveInventorySlot(p, slot, items);
    return record;
}

int PlayerInventory::CollectChestItem(PlayerState& p, std::array<int8_t, 8> record, const ItemDatabase& items,
                                       GeneratedLevel& level, WorldRegistry& world,
                                       const GameAdvancement::LevelLookup& levels) {
    // record[2] = 2 -- confirmed dead, not reproduced; see this method's
    // own declaration comment.
    if (p.inventoryCount < 24) {
        int8_t itemId = record[4];
        int32_t packedValue = (static_cast<int32_t>(record[5]) << 8) + record[6];
        int8_t charge = record[7];
        AddInventoryItemRaw(p, itemId, packedValue, charge);
        DungeonRuntime::RemoveChest(level, world, record);

        int itemIndex = itemId - 1;
        if (items.category[static_cast<size_t>(itemIndex)] == 11) {
            p.giftPointsFound =
                static_cast<int16_t>(p.giftPointsFound + items.subtype[static_cast<size_t>(itemIndex)]);
            // M52: see this method's own declaration comment.
            GameAdvancement::OpenZone(GameAdvancement::Level(p.giftPointsFound), levels);
        }

        return 1;
    }

    std::array<int8_t, 7> dropped{record[0], record[1], record[4], record[5], record[6], record[7], 1};
    DungeonRuntime::AddDroppedItem(level, world, dropped);
    DungeonRuntime::RemoveChest(level, world, record);
    return 0;
}

bool PlayerInventory::HasCampMark(const PlayerState& p) { return p.campLevel > 0; }

void PlayerInventory::MarkCampAndReturnToTown(PlayerState& p) {
    p.campLevel = p.currentLevel;
    p.campX = p.tileX;
    p.campY = p.tileY;
    p.campFacing = p.facing;

    // setHubSpawnPosition(true) -- the death/respawn hub point, (12, 14),
    // DISTINCT from character creation's (9, 10). See this method's own
    // header comment.
    p.currentLevel = p.pendingLevel = 1;
    p.tileX = p.pendingTileX = 12;
    p.tileY = p.pendingTileY = 14;
    p.facing = p.pendingFacing = 1;

    p.justMarkedCamp = true;
}

void PlayerInventory::WarpToCampMark(PlayerState& p) {
    p.currentLevel = p.pendingLevel = p.campLevel;
    p.tileX = p.pendingTileX = p.campX;
    p.tileY = p.pendingTileY = p.campY;
    p.justMarkedCamp = true;
}

bool PlayerInventory::IsSlotEquipped(const PlayerState& p, int slot, const ItemDatabase& items) {
    int8_t itemId = p.inventoryItemIds[static_cast<size_t>(slot)];
    if (!items.IsEquippable(static_cast<int>(std::abs(itemId)))) return false;
    return itemId < 0;
}

}  // namespace stormhold
