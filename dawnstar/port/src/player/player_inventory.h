#pragma once
#include "assets/item_database.h"
#include "player/player_state.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Player.java's inventory
// slot/equip-state management: addInventoryItem()/removeInventorySlot()/
// unequipInventorySlot()/isEquipped()/canEquipOrUnequip()/equipItem()/
// equipLastPickedUpItem(). Split out of player/player_creation.h (which
// used to carry small private copies of AddInventoryItem/EquipItem/
// UnequipItemInSlot for grantStartingItems()) so player/
// player_spellcasting.h's castOnSelf (spell 6's "cure poison" scroll
// grant-and-equip) can reuse the same logic instead of a second
// duplicate copy; player_creation.cpp now calls these too.
//
// Deliberately NOT ported: dropInventoryItem() (needs a live Dungeon's
// dropped-item list, which the port doesn't model yet) and addGold()
// (a one-line field increment nothing here needs yet).
class PlayerInventory {
public:
    // Appends an item to the first free slot; false if inventoryCount
    // is already 24.
    static bool AddItem(PlayerState& p, int itemId, int spawnIdOrPacked, int charge);

    // True if inventoryItemIds[slot]'s sign marks it equipped (negative)
    // AND the item is actually equippable -- Player.java's isEquipped().
    static bool IsEquipped(const PlayerState& p, const ItemDatabase& items, int slot);

    // Gates the "Equip"/"Unequip" menu option: true for weapon (1-4),
    // armor (5-10), and "special weapon" (15) categories, checked via
    // Item.column(1,itemId) i.e. ItemDatabase::category directly (not
    // IsEquippable's equipSlot check) -- Player.java's
    // canEquipOrUnequip() keeps its own separate category switch, kept
    // faithful here too.
    static bool CanEquipOrUnequip(const PlayerState& p, const ItemDatabase& items, int slot);

    // Equips inventoryItemIds[slot] (flips it negative), auto-unequipping
    // whatever already occupies that equip slot if autoUnequipConflict.
    static bool Equip(PlayerState& p, const ItemDatabase& items, int slot, bool autoUnequipConflict);

    // Unequips slot in place (flips inventoryItemIds[slot] back
    // positive, clears the matching equippedItems[] entry) if it's
    // currently equipped (via IsEquipped, hence the ItemDatabase
    // parameter); a no-op otherwise.
    static void UnequipSlot(PlayerState& p, const ItemDatabase& items, int slot);

    // Unequips slot, then removes it, compacting later slots down --
    // Player.java's removeInventorySlot(). False if slot >= inventoryCount.
    static bool RemoveSlot(PlayerState& p, const ItemDatabase& items, int slot);

    // Equips the most-recently-added inventory item (inventoryCount-1).
    static bool EquipLastPickedUpItem(PlayerState& p, const ItemDatabase& items, bool autoUnequipConflict);
};

}  // namespace dawnstar
