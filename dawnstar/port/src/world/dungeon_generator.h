#pragma once
#include <cstdint>
#include <vector>

#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "util/java_random.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/DungeonGenerator.java --
// procedurally builds one dungeon level's tile grid, monster placements,
// and chest placements. See ../../docs/PORT_ROADMAP.md's M6 entry for
// this milestone's verification story: unlike M2-M5, there is no
// bit-exact JVM ground truth available here (see that entry for why --
// short version: ESGame extends the shared RegisteredMIDlet, whose
// static initializer constructs real javax.microedition.lcdui.Command
// objects, and the MIDP *stub* jars this project's compile-check uses
// throw "API Stub has been used" the instant any of that is actually
// touched at runtime, not just compiled against -- so nothing that
// transitively references ESGame, including Item.load()/Monster.load(),
// can actually be *run* against these jars). Verified instead the same
// way Phase 1's own hand-trace rename work was: careful line-by-line
// transcription from ../../../src/DungeonGenerator.java, plus strong
// internal self-consistency checks (exactly 15 rooms, exactly 5 chests,
// every monster/chest position walkable and in-bounds, valid item ids,
// special-room marking only on the 4 documented levels) --
// see world/tests/m6_dungeon_generator_smoke.cpp.
//
// A monster spawned at a room's door position. Corresponds to a `Monster`
// this code doesn't otherwise model yet (no runtime Monster/save-state
// port exists) -- just enough to place it and know its type.
struct GeneratedMonsterSpawn {
    int x = 0;
    int y = 0;
    int monsterType = 0;
    int hp = 0;
};

// A chest placed in one of the level's 5 highest-(random-)weighted
// rooms. `guaranteedGift` is true only for the very first (the one
// DungeonGenerator.placeChests always rolls from Item's "gift" category
// via RandomGiftItemOfSubtype rather than a regular RollLoot pick).
struct GeneratedChestSpawn {
    int x = 0;
    int y = 0;
    bool guaranteedGift = false;
    int itemId = 0;
    int spawnId = 0;
};

struct GeneratedLevel {
    int number = 0;
    int tier = 0;
    int width = 0;
    int height = 0;
    // [x][y], same indexing as Dungeon.java's tiles[][]. Bit meanings
    // (see Dungeon.java/DungeonGenerator.java): 1=wall, 2=monster,
    // 4=dropped item, 8=no-spawn special room, 16=chest, 32=shopkeeper
    // special room, 64=edge marker (never written by generation itself).
    std::vector<std::vector<uint8_t>> tiles;
    std::vector<GeneratedMonsterSpawn> monsters;
    std::vector<GeneratedChestSpawn> chests;

    // Dungeon.java's neighbors[0..3] (north/east/south/west level ids,
    // <= 0 = no neighbor -- see assets/dungeon_geometry.h) plus
    // stairsUpDir/stairsDownDir, carried straight through from this
    // level's DungeonGeomRow. Not used by generation itself (M6), but
    // Dungeon.tileAt()'s cross-level lookup needs them -- stored here so
    // a later milestone's DungeonView doesn't have to re-derive them.
    int neighborNorth = 0;
    int neighborEast = 0;
    int neighborSouth = 0;
    int neighborWest = 0;
    int stairsUpDir = 0;
    int stairsDownDir = 0;

    // Set only on levels 3/12/21/30 -- DungeonGenerator.java writes these
    // directly into Shop.SHOP_X[5..8]/SHOP_Y[5..8] as a side effect
    // (this port has no Shop class yet, so they come back as plain
    // output fields instead of a static-array side effect). -1 means
    // "not one of those 4 levels" (or, in principle, that no room
    // qualified as the special room -- see PopulateLevel's comment).
    int specialShopX = -1;
    int specialShopY = -1;

    // Dungeon.java's `visited` -- set true by PlayerMovement::CommitMove
    // on arrival (M13, see player/player_movement.h). Not touched by
    // generation itself; starts false like a freshly-generated level.
    bool visited = false;
};

class DungeonGenerator {
public:
    // Standard 35x35 procedurally-generated level (levels 2-37).
    // `levelNumber` is 1-based, matching Dungeon.java's `number` field
    // and DIFFICULTY_TIER_LOOKUP's indexing convention.
    static GeneratedLevel PopulateLevel(int levelNumber, const DungeonGeomRow& geomRow,
                                         const ItemDatabase& items, const MonsterDatabase& monsters);

    // Level 1 (the hub town): fixed-size (19x19), hand-carved template,
    // no RNG involved -- DungeonGenerator.java's buildHubGrid().
    static GeneratedLevel BuildHubLevel(const DungeonGeomRow& geomRow);

    // Dungeon.initTier(): maps a level number to its "difficulty tier" --
    // NOT the level number itself, a permutation (see
    // Dungeon.DIFFICULTY_TIER_LOOKUP's doc comment in the Java source).
    static int InitTier(int levelNumber);

    // Dungeon.MONSTER_TABLE[tierIndex][bucket] (tierIndex = tier - 1,
    // bucket 0-3): a single public accessor onto the same table
    // PopulateLevel's own room-monster placement already reads
    // internally, so monster/monster_runtime.h's PickMonsterType (M15)
    // -- the port's counterpart of Monster.spawn(), which the real
    // DungeonGenerator.java actually calls rather than inlining this
    // dispatch itself -- has one place to read it from instead of a
    // second copy of the table. PopulateLevel's own inline copy of the
    // tier-roll/bucket arithmetic (not the table) is left as its own
    // small, harmless duplication of PickMonsterType's identical logic:
    // swapping it for an actual call would make dawnstar_world depend on
    // the monster module, which would need dawnstar_world back (for
    // DungeonView) -- not worth a library dependency cycle for 5 lines
    // of duplicated arithmetic over a single-sourced table.
    static int MonsterTypeForTierBucket(int tierIndex, int bucket);
};

}  // namespace dawnstar
