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

    // M43: Player.java's grantStarFrostItem() -- the "Reveal Traitor"
    // quiz's correct-guess award. Sets starFrostBonusActive (SkillValue's
    // flat +4, M14), takes one spawn id from `nextItemSpawnId` (main.cpp's
    // own nextDropSpawnId, Item.nextSpawnId()'s stand-in -- handed out
    // with the same post-increment-from-1 convention combat_tick.cpp's
    // own death-drop call site already established, so the id the original
    // would hand out and this one agree on the first call: Java's
    // nextSpawnId() returns ++nextSpawnId from a 0 start = 1), then tries
    // AddItem(100 /* StarFrost */, spawnId, 0). If the inventory is full,
    // evicts to make room: a slot holding item id 87 ("Warp to camp",
    // Player.java's own check -- the loop breaks on the FIRST one it
    // sees, before even considering any cheaper item, preserved exactly)
    // wins outright; otherwise the non-equipped slot with the lowest
    // positive Item.column(5,...) sell price is evicted, then the add is
    // retried with the SAME spawnId. The original's "Still can't add
    // StarFrost" println tail isn't ported (no stdout channel any module
    // here uses).
    //
    // A real edge guarded defensively rather than ported literally: when
    // every slot is equipped (or zero-priced), Player.java's evictSlot
    // stays -1 and its removeInventorySlot(-1) would throw
    // ArrayIndexOutOfBoundsException -- unreachable in practice (at most
    // ~7 of 24 slots can be equipped, and every real inventory holds
    // positive-value loot); guarded here instead, the same
    // "C++ has no exceptions safety net for an out-of-bounds index"
    // precedent player_movement.h's own no-neighbor-edge guard set. The
    // retry add then simply fails again and the item is not granted --
    // starFrostBonusActive stays set either way, exactly as in the
    // original (it's set before the first add attempt).
    static void GrantStarFrostItem(PlayerState& p, const ItemDatabase& items, int16_t& nextItemSpawnId);

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
