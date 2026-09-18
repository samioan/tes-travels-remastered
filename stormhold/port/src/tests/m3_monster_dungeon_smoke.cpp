// M3 smoke test: loads the real monstersin.dat (MonsterDatabase) and
// geomin.dat (DungeonGeometry) via AssetRoot and prints enough to check by
// hand against ../../src/Monster.java/Dungeon.java and
// docs/ASSET_FORMATS.md.
#include <cstdio>

#include "assets/asset_root.h"
#include "assets/dungeon_geometry.h"
#include "assets/monster_database.h"

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        stormhold::AssetRoot assets(root);

        stormhold::MonsterDatabase monsters = stormhold::MonsterDatabase::Load(assets);
        std::printf("monster types: %d\n", monsters.TypeCount());
        for (int t = 1; t <= 3 && t <= monsters.TypeCount(); ++t) {
            std::printf("  type[%d]: %-16s maxHp=col14:%d attack=col5:%d dropChance=col15:%d lootRow=col16:%d\n",
                        t, monsters.TypeName(t).c_str(), monsters.Stat(t, 14), monsters.Stat(t, 5),
                        monsters.Stat(t, 15), monsters.Stat(t, 16));
        }

        stormhold::DungeonGeometry geom = stormhold::DungeonGeometry::Load(assets);
        std::printf("dungeon geometry rows: %zu\n", geom.rows.size());
        for (int lvl = 1; lvl <= 3 && lvl <= static_cast<int>(geom.rows.size()); ++lvl) {
            const stormhold::DungeonGeomRow& r = geom.rows[lvl - 1];
            std::printf("  level[%d]: N=%d E=%d S=%d W=%d stairsUp=%d stairsDown=%d\n", lvl, r.north,
                        r.east, r.south, r.west, r.stairsUpDir, r.stairsDownDir);
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m3_monster_dungeon_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
