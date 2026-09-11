#pragma once
#include <cstdint>
#include <vector>

#include "assets/dat_archive.h"

namespace dawnstar {

// One geomin.dat row: the static level-connectivity data for one of the
// 37 dungeon levels (../../docs/ASSET_FORMATS.md), renamed-source
// counterpart of DungeonGenerator.java's loadGeomRows()/Dungeon.java's
// neighbors[]/stairsUpDir/stairsDownDir.
//
// stairsUpDir/stairsDownDir are compass direction codes (1=N, 2=E, 3=S,
// 4=W -- same convention as neighbors[] and Player's facing), NOT tile
// coordinates despite the field names suggesting one: they say which edge
// of the level has an internal stairway, at that direction's fixed tile
// position ((17,5)=N, (30,17)=E, (17,30)=S, (5,17)=W). See
// Dungeon.tileAt/Monster.isStairwayTile in ../../src/.
//
// Fields are SIGNED (Dungeon.java's `neighbors` is `byte[]`, read via
// readByte()): "no connection"/"no stairway in that direction" is any
// value <= 0, not just literal 0 -- Dungeon.tileAt's boundary check is
// `neighborLevel <= 0`, and the real data actually uses -1 (0xFF) for
// "none" (e.g. level 1, the hub town, has no stairs; the docs/
// ASSET_FORMATS.md "0 = none" note undersells this -- worth tightening
// there too). Reading these as unsigned would print 255 instead of -1 and
// obscure that every "none" check in the source is a <= 0 comparison.
struct DungeonGeomRow {
    // Level ids of the level reachable by walking off each cardinal edge
    // of this level, <= 0 = no connection. Index order matches
    // DungeonGenerator.java's geomRows[level][0..3] exactly.
    int8_t north = 0;
    int8_t east = 0;
    int8_t south = 0;
    int8_t west = 0;
    int8_t stairsUpDir = 0;
    int8_t stairsDownDir = 0;
};

struct DungeonGeometry {
    // 37 rows, one per dungeon level (levels are 1-based in the rest of
    // the codebase -- rows[levelNumber - 1]).
    std::vector<DungeonGeomRow> rows;

    static DungeonGeometry Load(DatArchive& archive);
};

}  // namespace dawnstar
