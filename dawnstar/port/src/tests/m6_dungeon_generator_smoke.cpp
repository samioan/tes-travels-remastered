// M6 smoke test: DungeonGenerator against real Item/Monster/geometry data.
//
// Unlike M2-M5, there's no bit-exact JVM ground truth available for this
// one -- see world/dungeon_generator.h's header comment for why (ESGame's
// RegisteredMIDlet base class's static initializer throws the instant
// anything touching ESGame is actually *run* against the MIDP stub jars,
// not just compiled). So this checks strong internal self-consistency
// instead, matching the standard Phase 1's own hand-trace rename work
// used before any of this port existed: exactly 15 rooms carved, exactly
// 5 chests placed, every monster/chest position walkable and in-bounds,
// every rolled item id valid, special-room marking only on the 4
// documented levels (3/12/21/30), and the tile grid contains only
// documented bit values.
#include <cstdio>

#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "world/dungeon_generator.h"

namespace {

int CountBit(const dawnstar::GeneratedLevel& level, uint8_t bit) {
    int count = 0;
    for (const auto& row : level.tiles) {
        for (uint8_t t : row) {
            if (t & bit) count++;
        }
    }
    return count;
}

bool CheckLevel(const dawnstar::GeneratedLevel& level, const dawnstar::ItemDatabase& items,
                 const dawnstar::MonsterDatabase& monsters) {
    bool ok = true;

    if (level.monsters.size() != 15) {
        std::printf("  FAIL: expected 15 monster spawns, got %zu\n", level.monsters.size());
        ok = false;
    }
    if (level.chests.size() != 5) {
        std::printf("  FAIL: expected 5 chests, got %zu\n", level.chests.size());
        ok = false;
    }

    int guaranteedGiftCount = 0;
    for (const auto& chest : level.chests) {
        if (chest.x < 0 || chest.x >= level.width || chest.y < 0 || chest.y >= level.height) {
            std::printf("  FAIL: chest at (%d,%d) out of bounds\n", chest.x, chest.y);
            ok = false;
        }
        if (!(level.tiles[chest.x][chest.y] & 16)) {
            std::printf("  FAIL: chest at (%d,%d) tile missing bit 16\n", chest.x, chest.y);
            ok = false;
        }
        // Item.rollLoot()'s packed return value: the low byte is always
        // the real item id (1..ItemCount); a nonzero high byte only ever
        // gets attached downstream (DungeonGenerator.placeChests, this
        // code's model of it) when that low byte is exactly 86 -- it's
        // never a "the id is actually bigger than ItemCount" signal. See
        // ../../src/DungeonGenerator.java's placeChests().
        int realItemId = chest.itemId & 0xFF;
        if (realItemId < 1 || realItemId > items.ItemCount()) {
            std::printf("  FAIL: chest item id %d (packed 0x%x) out of range [1,%d]\n", realItemId,
                        chest.itemId, items.ItemCount());
            ok = false;
        }
        if (chest.guaranteedGift) guaranteedGiftCount++;
    }
    if (guaranteedGiftCount != 1) {
        std::printf("  FAIL: expected exactly 1 guaranteed-gift chest, got %d\n", guaranteedGiftCount);
        ok = false;
    }

    for (const auto& spawn : level.monsters) {
        if (spawn.x < 0 || spawn.x >= level.width || spawn.y < 0 || spawn.y >= level.height) {
            std::printf("  FAIL: monster at (%d,%d) out of bounds\n", spawn.x, spawn.y);
            ok = false;
        }
        if (!(level.tiles[spawn.x][spawn.y] & 2)) {
            std::printf("  FAIL: monster at (%d,%d) tile missing bit 2\n", spawn.x, spawn.y);
            ok = false;
        }
        if (spawn.monsterType < 1 || spawn.monsterType > monsters.TypeCount()) {
            std::printf("  FAIL: monster type %d out of range [1,%d]\n", spawn.monsterType,
                        monsters.TypeCount());
            ok = false;
        }
    }

    bool wantsSpecialShop = level.number == 3 || level.number == 12 || level.number == 21 || level.number == 30;
    bool hasSpecialShop = level.specialShopX >= 0;
    if (wantsSpecialShop != hasSpecialShop) {
        std::printf("  FAIL: level %d specialShop presence %d, expected %d\n", level.number, hasSpecialShop,
                    wantsSpecialShop);
        ok = false;
    }

    // Every tile value must be one of the documented bit combinations --
    // a stray value would mean the carving logic wrote something the
    // Java source never would.
    for (const auto& row : level.tiles) {
        for (uint8_t t : row) {
            uint8_t known = 1 | 2 | 4 | 8 | 16 | 32 | 64;
            if (t & ~known) {
                std::printf("  FAIL: tile value 0x%02x has undocumented bits set\n", t);
                ok = false;
            }
        }
    }

    std::printf("  level %d: tier=%d rooms(via monster spawns)=%zu chests=%zu wallTiles=%d doorTiles=%d\n",
                level.number, level.tier, level.monsters.size(), level.chests.size(), CountBit(level, 1),
                CountBit(level, 2));

    return ok;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";
    std::string datPath = root + "/datfiles.lmp";

    try {
        dawnstar::DatArchive archive(datPath);
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        dawnstar::MonsterDatabase monsters = dawnstar::MonsterDatabase::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);

        bool ok = true;

        dawnstar::GeneratedLevel hub = dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[0]);
        std::printf("hub (level 1): %dx%d, wallTiles=%d\n", hub.width, hub.height, CountBit(hub, 1));
        if (hub.width != 19 || hub.height != 19) {
            std::printf("  FAIL: hub level should be 19x19\n");
            ok = false;
        }

        // A spread of levels covering all 4 special-shop levels plus a
        // few ordinary ones.
        int testLevels[] = {2, 3, 12, 15, 21, 30, 37};
        for (int levelNumber : testLevels) {
            dawnstar::GeneratedLevel level =
                dawnstar::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[levelNumber - 1], items,
                                                           monsters);
            if (!CheckLevel(level, items, monsters)) ok = false;
        }

        if (!ok) {
            std::fprintf(stderr, "m6_dungeon_generator_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m6_dungeon_generator_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
