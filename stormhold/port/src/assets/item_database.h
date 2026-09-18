#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "assets/asset_root.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/Item.java's static state --
// itemsin.dat + droppeditemsin.dat (../../docs/ASSET_FORMATS.md), loaded
// through an AssetRoot instead of Util.openResource. Struct-of-arrays
// layout kept 1:1 with Item.java rather than an array-of-structs Item
// type, since that's what the original data tables are and what
// Item.java's own callers (isEquippable(), equipSlotOf(), ...) index into
// directly. Data-only for now -- RollLoot()/RandomGiftItemOfSubtype()
// (Item.java's two RNG-driven loot-roll methods) are deferred to whichever
// later milestone actually needs dungeon generation, same as dawnstar's M2
// deferred them to its own M6.
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

    static ItemDatabase Load(const AssetRoot& assets);
};

}  // namespace stormhold
