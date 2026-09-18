// M6 smoke test: DungeonGenerator against real Item/Monster/geometry data.
//
// Unlike M2-M5, there's no bit-exact JVM ground truth available for this
// one -- see world/dungeon_generator.h's header comment for why (ESGame's
// RegisteredMIDlet base class's static initializer throws the instant
// anything touching ESGame is actually *run* against the MIDP stub jars,
// not just compiled). So this checks strong internal self-consistency
// instead, matching the standard Phase 1's own hand-trace rename work used
// before any of this port existed: exactly 15 rooms carved, exactly 5
// chests placed, every monster/chest position walkable and in-bounds,
// every rolled item id valid, exactly one bit-8 "special" tile per level
// (no level-number-gated shop-room branch here, unlike dawnstar), level
// 37's forced monster type 41, and the tile grid contains only documented
// bit values.
#include <cstdio>

#include "assets/asset_root.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "world/dungeon_generator.h"

namespace {

int CountBit(const stormhold::GeneratedLevel& level, uint8_t bit) {
    int count = 0;
    for (const auto& row : level.tiles) {
        for (uint8_t t : row) {
            if (t & bit) count++;
        }
    }
    return count;
}

bool CheckLevel(const stormhold::GeneratedLevel& level, const stormhold::ItemDatabase& items,
                 const stormhold::MonsterDatabase& monsters) {
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
        // gets attached when the rolled rarity column is 1. See
        // ../../src/Item.java's rollLoot().
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

    for (size_t i = 0; i < level.monsters.size(); ++i) {
        const auto& spawn = level.monsters[i];
        if (spawn.x < 0 || spawn.x >= level.width || spawn.y < 0 || spawn.y >= level.height) {
            std::printf("  FAIL: monster at (%d,%d) out of bounds\n", spawn.x, spawn.y);
            ok = false;
        }
        if (!(level.tiles[spawn.x][spawn.y] & 2)) {
            std::printf("  FAIL: monster at (%d,%d) tile missing bit 2\n", spawn.x, spawn.y);
            ok = false;
        }
        if (spawn.monsterType < 1 || spawn.monsterType > monsters.TypeCount()) {
            std::printf("  FAIL: monster type %d out of range [1,%d]\n", spawn.monsterType, monsters.TypeCount());
            ok = false;
        }
        // Level 37's LAST room is a forced monster type 41 (Dungeon.java's
        // own class header comment -- NOT dawnstar's 42).
        if (level.number == 37 && i == level.monsters.size() - 1 && spawn.monsterType != 41) {
            std::printf("  FAIL: level 37's last room should be forced monster type 41, got %d\n",
                        spawn.monsterType);
            ok = false;
        }
    }

    // Unlike dawnstar, there's no level-number-gated "special shopkeeper
    // room" branch here -- every level marks exactly one bit-8 tile
    // (Dungeon.generate()'s own unconditional `markedCenter` logic), and
    // no level ever sets bit 32 during generation (only BuildHubLevel's
    // fixed hub-town shop tiles do).
    int specialTiles = CountBit(level, 8);
    if (specialTiles != 1) {
        std::printf("  FAIL: level %d expected exactly 1 bit-8 special tile, got %d\n", level.number, specialTiles);
        ok = false;
    }
    if (CountBit(level, 32) != 0) {
        std::printf("  FAIL: level %d should have no bit-32 tiles from generation\n", level.number);
        ok = false;
    }

    // Every tile value must be one of the documented bit combinations --
    // a stray value would mean the carving logic wrote something the Java
    // source never would.
    for (const auto& row : level.tiles) {
        for (uint8_t t : row) {
            uint8_t known = 1 | 2 | 4 | 8 | 16 | 32;
            if (t & ~known) {
                std::printf("  FAIL: tile value 0x%02x has undocumented bits set\n", t);
                ok = false;
            }
        }
    }

    std::printf("  level %d: tier=%d monsters=%zu chests=%zu wallTiles=%d doorTiles=%d\n", level.number, level.tier,
                level.monsters.size(), level.chests.size(), CountBit(level, 1), CountBit(level, 2));

    return ok;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        stormhold::AssetRoot assets(root);
        stormhold::ItemDatabase items = stormhold::ItemDatabase::Load(assets);
        stormhold::MonsterDatabase monsters = stormhold::MonsterDatabase::Load(assets);
        stormhold::DungeonGeometry geometry = stormhold::DungeonGeometry::Load(assets);

        bool ok = true;

        stormhold::GeneratedLevel hub = stormhold::DungeonGenerator::BuildHubLevel(geometry.rows[0]);
        std::printf("hub (level 1): %dx%d, wallTiles=%d, shopTiles(bit32)=%d\n", hub.width, hub.height,
                    CountBit(hub, 1), CountBit(hub, 32));
        if (hub.width != 19 || hub.height != 19) {
            std::printf("  FAIL: hub level should be 19x19\n");
            ok = false;
        }
        if (CountBit(hub, 32) != 6) {
            std::printf("  FAIL: hub level should have 6 shop tiles marked, got %d\n", CountBit(hub, 32));
            ok = false;
        }

        // A spread of levels, including 37 (the forced-monster-type-41
        // level).
        int testLevels[] = {2, 3, 12, 15, 21, 30, 37};
        for (int levelNumber : testLevels) {
            stormhold::GeneratedLevel level = stormhold::DungeonGenerator::PopulateLevel(
                levelNumber, geometry.rows[levelNumber - 1], items, monsters);
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
