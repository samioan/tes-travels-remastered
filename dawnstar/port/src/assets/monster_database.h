#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "assets/dat_archive.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Monster.java's static type
// table -- monstersin.dat (../../docs/ASSET_FORMATS.md): a count, a name
// per monster *type*, then a 17-byte stat row per type. This is only the
// static per-type database Monster.load() builds -- not the per-instance
// Monster (spawn/AI/combat), which is gameplay logic for a later
// milestone, not data loading.
//
// Not all 17 stat columns are pinned down (../../docs/CLASS_MAP.md's
// Monster section): confirmed so far are col2 (detection-range related),
// col4 (base defense/to-hit offset), col5 (attack stat), col11 (on-hit
// status-effect id, 1-8), col14 (max HP), col15/16 (death-drop chance and
// loot-table row). Stat() exposes the whole row rather than named
// accessors for just the confirmed columns, matching Monster.java's own
// generic stat(column) -- callers that need a specific column look it up
// the same way the Java source does.
struct MonsterDatabase {
    std::vector<std::string> typeName;
    std::vector<std::array<uint8_t, 17>> typeStats;

    int TypeCount() const { return static_cast<int>(typeName.size()); }

    // `typeId` is 1-based, matching Monster.java's `this.monsterType - 1`
    // indexing convention throughout (e.g. stat(), typeName()).
    uint8_t Stat(int typeId, int column) const { return typeStats[typeId - 1][column]; }
    const std::string& TypeName(int typeId) const { return typeName[typeId - 1]; }

    static MonsterDatabase Load(DatArchive& archive);
};

}  // namespace dawnstar
