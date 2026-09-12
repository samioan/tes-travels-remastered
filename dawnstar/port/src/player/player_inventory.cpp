#include "player/player_inventory.h"

#include <cstdlib>

namespace dawnstar {

namespace {

// Player.java's private unequipItemInSlot(equipSlot): finds whichever
// inventory slot currently occupies `equipSlot` and unequips it.
void UnequipItemInSlot(PlayerState& p, const ItemDatabase& items, int equipSlot) {
    for (int slot = 0; slot < p.inventoryCount; slot++) {
        int itemId = std::abs(static_cast<int>(p.inventoryItemIds[slot]));
        if (items.EquipSlotOf(itemId) == equipSlot) {
            PlayerInventory::UnequipSlot(p, items, slot);
        }
    }
}

}  // namespace

bool PlayerInventory::AddItem(PlayerState& p, int itemId, int spawnIdOrPacked, int charge) {
    if (p.inventoryCount >= 24) return false;
    p.inventoryItemIds[p.inventoryCount] = static_cast<int8_t>(itemId);
    int32_t packed = (spawnIdOrPacked << 16) + static_cast<int8_t>(charge);
    p.inventoryItemData[p.inventoryCount] = packed;
    p.inventoryCount++;
    return true;
}

bool PlayerInventory::IsEquipped(const PlayerState& p, const ItemDatabase& items, int slot) {
    int8_t itemId = p.inventoryItemIds[slot];
    if (!items.IsEquippable(std::abs(static_cast<int>(itemId)))) return false;
    return itemId < 0;
}

bool PlayerInventory::CanEquipOrUnequip(const PlayerState& p, const ItemDatabase& items, int slot) {
    int itemId = std::abs(static_cast<int>(p.inventoryItemIds[slot]));
    int8_t category = items.category[itemId - 1];
    switch (category) {
        case 1: case 2: case 3: case 4: case 5:
        case 6: case 7: case 8: case 9: case 10:
        case 15:
            return true;
        default:
            return false;
    }
}

bool PlayerInventory::Equip(PlayerState& p, const ItemDatabase& items, int slot, bool autoUnequipConflict) {
    int8_t itemId = p.inventoryItemIds[slot];
    if (itemId < 0) return false;
    if (!items.IsEquippable(itemId)) return false;

    int equipSlot = items.EquipSlotOf(itemId);
    if (p.equippedItems[equipSlot] != 0) {
        if (!autoUnequipConflict) return false;
        UnequipItemInSlot(p, items, equipSlot);
    }

    p.equippedItems[equipSlot] = itemId;
    p.inventoryItemIds[slot] = static_cast<int8_t>(-itemId);
    return true;
}

void PlayerInventory::UnequipSlot(PlayerState& p, const ItemDatabase& items, int slot) {
    if (!IsEquipped(p, items, slot)) return;
    if (slot < 0 || slot > 23) return;

    int8_t itemId = static_cast<int8_t>(std::abs(static_cast<int>(p.inventoryItemIds[slot])));
    p.inventoryItemIds[slot] = itemId;

    for (int i = 0; i < 7; i++) {
        if (p.equippedItems[i] == itemId) {
            p.equippedItems[i] = 0;
            break;
        }
    }
}

bool PlayerInventory::RemoveSlot(PlayerState& p, const ItemDatabase& items, int slot) {
    if (slot >= p.inventoryCount) return false;

    UnequipSlot(p, items, slot);
    p.inventoryItemIds[slot] = 0;

    for (int i = slot; i < p.inventoryCount - 1; i++) {
        p.inventoryItemIds[i] = p.inventoryItemIds[i + 1];
        p.inventoryItemData[i] = p.inventoryItemData[i + 1];
    }

    p.inventoryCount--;
    return true;
}

bool PlayerInventory::EquipLastPickedUpItem(PlayerState& p, const ItemDatabase& items, bool autoUnequipConflict) {
    int slot = p.inventoryCount - 1;
    return Equip(p, items, slot, autoUnequipConflict);
}

}  // namespace dawnstar
