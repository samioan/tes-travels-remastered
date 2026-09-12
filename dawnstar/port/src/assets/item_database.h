#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "assets/dat_archive.h"
#include "util/java_random.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Item.java's static state --
// itemsin.dat + droppeditemsin.dat (../../docs/ASSET_FORMATS.md), loaded
// through a DatArchive instead of ESGame.getResource(). Struct-of-arrays
// layout kept 1:1 with Item.java rather than an array-of-structs Item
// type, since that's what the original data tables are and what
// Item.java's own callers (column(), isEquippable(), ...) index into
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

    // Item.java's randomGiftItemOfSubtype()/rollLoot() -- the two loot-
    // roll methods DungeonGenerator's placeChests() calls. Kept as
    // methods on the data they roll against, same as the Java source.
    //
    // Picks a random item id from category 11 ("gift") whose subtype
    // matches `subtypeWanted`.
    int RandomGiftItemOfSubtype(JavaRandom& rng, int subtypeWanted) const;

    // Rolls a loot-table item id for a monster/chest drop at dungeon
    // `depth`, weighted toward rarer rows for higher `bonusRolls` (best
    // of `bonusRolls` percentile samples). Returns a plain item id, or a
    // 2-byte extended id packed as (highByte<<8)|lowByte when the low
    // byte is 86 (a reserved "extended id follows" marker).
    int RollLoot(JavaRandom& rng, int depth, int bonusRolls) const;

    static ItemDatabase Load(DatArchive& archive);
};

}  // namespace dawnstar
