#include "assets/dungeon_geometry.h"

namespace dawnstar {

DungeonGeometry DungeonGeometry::Load(DatArchive& archive) {
    DungeonGeometry geom;

    BinaryReader in = archive.OpenResource("geomin.dat");
    geom.rows.resize(37);
    for (auto& row : geom.rows) {
        row.north = in.ReadS8();
        row.east = in.ReadS8();
        row.south = in.ReadS8();
        row.west = in.ReadS8();
        row.stairsUpDir = in.ReadS8();
        row.stairsDownDir = in.ReadS8();
    }

    return geom;
}

}  // namespace dawnstar
