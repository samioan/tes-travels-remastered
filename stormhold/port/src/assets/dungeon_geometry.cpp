#include "assets/dungeon_geometry.h"

#include "assets/binary_reader.h"

namespace stormhold {

DungeonGeometry DungeonGeometry::Load(const AssetRoot& assets) {
    DungeonGeometry geom;

    std::ifstream stream = assets.OpenFile("geomin.dat");
    BinaryReader in(stream);

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

}  // namespace stormhold
