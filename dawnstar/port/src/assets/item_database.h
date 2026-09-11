#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "assets/dat_archive.h"

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

    static ItemDatabase Load(DatArchive& archive);
};

}  // namespace dawnstar
