#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "assets/asset_root.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/Monster.java's static type
// table -- monstersin.dat (../../docs/ASSET_FORMATS.md): a u32 count, a
// name per monster *type*, then a 17-byte stat row per type. Byte-for-byte
// identical layout to dawnstar's own monstersin.dat. This is only the
// static per-type database Monster.loadTypes() builds -- not the
// per-instance Monster (spawn/AI/combat), which is gameplay logic for a
// later milestone, not data loading.
//
// Not all 17 stat columns are pinned down (../../docs/ASSET_FORMATS.md's
// "what's actually left" section): confirmed so far are col2 (damage-
// mitigation cap), col3 (base attack chance), col4 (base defense), col5
// (base attack power), col11 (inflicted-ailment id), col14 (starting/max
// HP), col15/16 (loot drop chance/table bonus). Stat() exposes the whole
// row rather than named accessors for just the confirmed columns, matching
// Monster.java's own generic stat(column) -- callers that need a specific
// column look it up the same way the Java source does.
struct MonsterDatabase {
    std::vector<std::string> typeName;
    std::vector<std::array<uint8_t, 17>> typeStats;

    int TypeCount() const { return static_cast<int>(typeName.size()); }

    // `typeId` is 1-based, matching Monster.java's `this.typeIndex - 1`
    // indexing convention throughout (e.g. stat(), typeNames[]).
    uint8_t Stat(int typeId, int column) const { return typeStats[typeId - 1][column]; }
    const std::string& TypeName(int typeId) const { return typeName[typeId - 1]; }

    static MonsterDatabase Load(const AssetRoot& assets);
};

}  // namespace stormhold
