#pragma once
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/spell_database.h"
#include "dungeon/dungeon_runtime.h"
#include "player/player_state.h"
#include "world/dungeon_generator.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Player.java's inventory
// slot/equip-state management: addInventoryItem()/removeInventorySlot()/
// unequipInventorySlot()/isEquipped()/canEquipOrUnequip()/equipItem()/
// equipLastPickedUpItem()/canUseItem()/itemTooltip()/dropInventoryItem().
// Split out of player/player_creation.h (which used to carry small
// private copies of AddInventoryItem/EquipItem/UnequipItemInSlot for
// grantStartingItems()) so player/player_spellcasting.h's castOnSelf
// (spell 6's "cure poison" scroll grant-and-equip) can reuse the same
// logic instead of a second duplicate copy; player_creation.cpp now
// calls these too.
//
// dropInventoryItem() was deferred through M18-M40 (it needs a live
// Dungeon's dropped-item list, which didn't exist until M22/M24's
// DungeonRuntime/WorldRegistry) -- ported now, M41, alongside
// ItemTooltip(), for the real Inventory-item screen (ui/options_menu.h).
//
// Deliberately NOT ported: addGold() (a one-line field increment nothing
// here needs yet).
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

    // Gates the "Use" inventory-item menu option (M18): true only for
    // the 87-99 "gift"/special-consumable category (13) -- Player.java's
    // canUseItem(). The actual use logic (useItem()) needs a Monster
    // target for its instant-kill items (97/98/99), so it lives in
    // combat/combat_resolution.h instead, alongside PlayerAttack/
    // MonsterTick/CastOnMonster.
    static bool CanUseItem(const PlayerState& p, const ItemDatabase& items, int slot);

    // Item-info tooltip for `slot` (M41, ESGame's real Inventory-item
    // screen): name + a category-specific detail line -- weapon/armor
    // value (categories 1-4/5-10), a spell scroll's name + known status
    // (12), gift flavor text (13, via a hardcoded 13-entry table matching
    // Item.java's own `specialEffectText`), category 15's "special
    // weapon" value (needs PlayerCombatStats::SkillValue, hence
    // CharacterData), or just the plain name + category label for
    // everything else (11/14 and default) -- Player.java's itemTooltip().
    static std::string ItemTooltip(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items,
                                    const SpellDatabase& spells, int slot);

    // Drops the item in `slot` onto the player's current tile (via
    // DungeonRuntime::AddDroppedItem, packing the same 7-byte record
    // Player.java's own dropInventoryItem() builds) unless it's the
    // non-droppable StarFrost (id 101 -- a real quirk preserved as found,
    // see Player.java's own doc comment: StarFrost is granted elsewhere
    // as id 100, so this looks like either a second related item or a
    // transcription quirk in the original), then removes it from the
    // inventory either way (via RemoveSlot).
    static void DropInventoryItem(PlayerState& p, const ItemDatabase& items, std::vector<GeneratedLevel>& levels,
                                   WorldRegistry& world, int slot);
};

}  // namespace dawnstar
