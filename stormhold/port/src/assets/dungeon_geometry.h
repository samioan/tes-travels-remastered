#pragma once
#include <cstdint>
#include <vector>

#include "assets/asset_root.h"

namespace stormhold {

// One geomin.dat row: the static level-connectivity data for one of the 37
// dungeon levels (../../docs/ASSET_FORMATS.md), renamed-source counterpart
// of ESGame.java's loadDungeonGeometryRows()/Dungeon.java's `neighbors[]`/
// `stairsUpDir`/`stairsDownDir`.
//
// stairsUpDir/stairsDownDir are compass direction codes (1=N, 2=E, 3=S,
// 4=W -- same convention as neighbors[]), NOT fixed tile coordinates.
// **This differs from dawnstar**: Stormhold's Dungeon.generate() calls
// carveStairwell(direction) to place each stairwell procedurally as part
// of level generation, so there's no fixed per-direction tile position to
// hardcode here the way dawnstar's DungeonGeomRow documents. That
// procedural placement is dungeon-generation logic for a later milestone,
// not represented by this struct.
//
// Fields are SIGNED (Dungeon.java declares `neighbors` as `byte[]`, read
// via readByte()): "no connection"/"no stairway in that direction" is any
// value <= 0, not just literal 0 -- Dungeon.tileAt's own boundary check is
// `neighborId <= 0` (confirmed at all 4 cardinal branches, ../src/
// Dungeon.java around line 1119). Same convention as dawnstar's own
// geomin.dat.
struct DungeonGeomRow {
    // Level ids of the level reachable by walking off each cardinal edge
    // of this level, <= 0 = no connection. Index order matches
    // ESGame.geomRows[level][0..3] exactly.
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

    static DungeonGeometry Load(const AssetRoot& assets);
};

}  // namespace stormhold
