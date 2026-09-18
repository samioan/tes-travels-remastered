#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "assets/asset_root.h"
#include "util/java_random.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/Item.java's static state --
// itemsin.dat + droppeditemsin.dat (../../docs/ASSET_FORMATS.md), loaded
// through an AssetRoot instead of Util.openResource. Struct-of-arrays
// layout kept 1:1 with Item.java rather than an array-of-structs Item
// type, since that's what the original data tables are and what
// Item.java's own callers (isEquippable(), equipSlotOf(), ...) index into
// directly.
struct ItemDatabase {
    std::vector<std::string> categoryNames;

    std::vector<std::string> name;
    std::vector<int8_t> category;
    std::vector<int8_t> subtype;
    std::vector<int8_t> questFlags;
    std::vector<int16_t> buyPrice;
    std::vector<int16_t> sellPrice;
    std::vector<int8_t> equipSlot;

    // droppeditemsin.dat: [depthRow][col] loot table.
    std::vector<std::vector<int8_t>> lootTable;

    int ItemCount() const { return static_cast<int>(name.size()); }

    // Item.java's isEquippable()/equipSlotOf() -- itemId is 1-based,
    // matching Item.java's own index0(itemId) = itemId - 1 convention.
    bool IsEquippable(int itemId) const { return equipSlot[itemId - 1] != -1; }
    int EquipSlotOf(int itemId) const { return equipSlot[itemId - 1]; }

    // Item.java's isEquipmentCategory() -- a DISTINCT gate from
    // IsEquippable() above (that one checks the equipSlot column; this one
    // checks category is 1-10). See ../../../src/Player.java's
    // equipItem()'s own header comment for the real bug found (and fixed
    // there, not here) from once conflating the two.
    bool IsEquipmentCategory(int itemId) const {
        int8_t cat = category[itemId - 1];
        return cat >= 1 && cat <= 10;
    }

    // Item.java's randomGiftItemOfSubtype()/rollLoot() -- the two
    // RNG-driven loot-roll methods Dungeon.placeChests() calls. M2
    // deferred these (data-only struct); M6 (dungeon generation) is the
    // first real caller, so they land here now, same order dawnstar's own
    // port added them in.
    //
    // Picks a random item id from category 11 ("gift") whose subtype
    // matches `subtypeWanted`.
    int RandomGiftItemOfSubtype(JavaRandom& rng, int subtypeWanted) const;

    // Rolls a loot-table item id for a monster/chest drop at dungeon
    // `depth`, weighted toward rarer rows for higher `bonusRolls` (best of
    // `bonusRolls` percentile samples). Returns a plain item id, or a
    // 2-byte extended id packed as (highByte<<8)|lowByte when the rolled
    // rarity column is 1 (../src/Item.java's own condition -- NOT "low
    // byte == 86", despite that being how dawnstar's own doc comment
    // paraphrases the identical check on its side).
    int RollLoot(JavaRandom& rng, int depth, int bonusRolls) const;

    static ItemDatabase Load(const AssetRoot& assets);
};

}  // namespace stormhold
