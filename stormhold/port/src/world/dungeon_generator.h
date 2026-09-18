#pragma once
#include <cstdint>
#include <vector>

#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "util/java_random.h"

namespace stormhold {

// Renamed-source counterpart of the room-carving/corridor-connection/
// monster-placement/chest-placement algorithm in `../../../src/
// Dungeon.java` -- named `DungeonGenerator` here for easy cross-reference
// with dawnstar's own port, even though **in the real source this is NOT a
// separate class**: `Dungeon.java`'s own header comment confirms Stormhold
// has no `DungeonGenerator.java` at all (`decompiled/c.java`, the only
// other candidate, is a thin FullCanvas UI delegator, unrelated) --
// generation is fused directly onto the same `Dungeon` instance that holds
// live per-level state. See ../../docs/PORT_ROADMAP.md's M6 entry for this
// milestone's verification story: like dawnstar's own M6, there is no
// bit-exact JVM ground truth available here (`ESGame` extends the shared
// `RegisteredMIDlet`, whose static initializer touches real
// `javax.microedition.lcdui.Command` objects the MIDP *stub* jars throw on
// at runtime -- anything transitively referencing `ESGame`, including
// `Item.load()`/`Monster.loadTypes()`, can't actually be *run* against
// them). Verified instead via careful line-by-line transcription plus
// strong internal self-consistency checks -- see
// src/tests/m6_dungeon_generator_smoke.cpp.
//
// A monster spawned at a room's door position. Corresponds to a `Monster`
// this code doesn't otherwise model yet (no runtime Monster/save-state
// port exists) -- just enough to place it and know its type/starting HP.
struct GeneratedMonsterSpawn {
    int x = 0;
    int y = 0;
    int monsterType = 0;
    // Monster's real constructor reads `typeStats[typeIndex-1][14]` as a
    // RAW signed byte, not through the masked `stat()` accessor (see
    // ../../src/Monster.java's own constructor and its header comment on
    // stat() vs. raw reads) -- MonsterDatabase::RawStat() matches that.
    int hp = 0;
    // Item.nextSpawnId()-shaped counter -- like GeneratedChestSpawn::
    // spawnId below, this is a per-level-local counter (1..room count)
    // rather than the original's single counter shared across all 37
    // levels generated in one pass. Nothing reads it yet, so this can't
    // observably diverge -- same reasoning as dawnstar's own M6 note.
    int spawnId = 0;
};

// A chest placed in one of the level's 5 highest-(random-)weighted rooms.
// `guaranteedGift` is true only for the very first (the one
// Dungeon.placeChests always rolls from Item's "gift" category via
// RandomGiftItemOfSubtype rather than a regular RollLoot pick).
//
// **A real, confirmed-dead field in the original NOT modeled here:**
// Dungeon.placeChests() writes a per-chest byte meant to carry this same
// flag (`record[2] = first ? 1 : 0`), but `first` is flipped to `false`
// earlier in the SAME iteration (inside the `if (first) {...; first =
// false;}` branch above it) -- so that write always evaluates to `0`,
// for every chest including the real gift one. Confirmed dead: the only
// consumer of a chest record (`Player.collectChestItem()`) never reads
// byte 2 at all (it unconditionally overwrites it, then only reads bytes
// 4-7 for the item id/value/flags). `guaranteedGift` below models the
// real game LOGIC (chest 0 really does get the gift-subtype roll), not
// this always-zero on-disk byte -- this port doesn't model the raw 8-byte
// chest record at all yet, only its semantic content, so there's no wrong
// value to preserve here. Revisit if a future save-format milestone needs
// the literal byte layout.
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
    // (Dungeon.java's isWalkable()/storeChest()/addDroppedItem()): 1=wall,
    // 2=monster, 4=dropped item, 8=no-spawn/special (see below), 16=chest,
    // 32=blocked (hub-town shop tiles only -- see BuildHubLevel).
    std::vector<std::vector<uint8_t>> tiles;
    std::vector<GeneratedMonsterSpawn> monsters;
    std::vector<GeneratedChestSpawn> chests;

    // Dungeon.java's neighbors[0..3] (north/east/south/west level ids,
    // <= 0 = no neighbor -- see assets/dungeon_geometry.h) plus
    // stairsUpDir/stairsDownDir, carried straight through from this
    // level's DungeonGeomRow.
    int neighborNorth = 0;
    int neighborEast = 0;
    int neighborSouth = 0;
    int neighborWest = 0;
    int stairsUpDir = 0;
    int stairsDownDir = 0;

    // Dungeon.java's `populated` flag -- **a deliberate simplification**,
    // corrected while porting M10 (player movement, which is the first
    // real reader of this field): in the real game, the hub's own
    // constructor sets `populated = true` unconditionally the instant it
    // runs, but a STANDARD level's `populated` is never touched by
    // `generate()`/`allocateAndGenerate()` at all -- it's set externally,
    // and only, by `ESGame.checkOpenAndPopulateDungeons()`/
    // `openAndPopulateAllUpTo()`, a progressive "open this zone's levels
    // once giftPointsFound crosses a threshold" world-state system this
    // port doesn't model yet (see docs/PORT_ROADMAP.md). Since this port
    // has no other path to a generated level at all (nothing calls
    // `PopulateLevel`/`BuildHubLevel` except deliberately, unlike the real
    // game which can hold not-yet-generated `Dungeon` objects for levels
    // the player hasn't reached), both generator functions set this
    // `true` on their own output -- "was generated by this port's own
    // generator" stands in for "populated" until a real zone-opening
    // milestone exists to model the distinction for real. Named
    // `populated` (not `visited`, this field's original M6-era name --
    // renamed for the same reason while fixing this) to match Dungeon
    // .java's own field directly, avoiding "has the PLAYER been here"
    // confusion with a name this field never actually meant.
    bool populated = true;

    // Dungeon.java's OWN `unconfirmedH` field, added while porting M10
    // (player movement) -- a SEPARATE, genuinely distinct concept from
    // `populated` above, easy to conflate since both are "has this level
    // been touched" flags. Player.java's own header comment confirms
    // `unconfirmedH` has a real write site after all: PlayerMovement::
    // CommitMove sets it `true` on every successful move INTO a level --
    // it's Dungeon.java's own dawnstar-equivalent "visited" flag (has the
    // PLAYER physically been here), not whether this level's tile data
    // has been generated at all. Starts false; nothing in this port reads
    // it yet (no minimap/fog-of-war milestone exists), same "producer
    // exists, no confirmed consumer yet" situation Player.java's own
    // header comment leaves it in.
    bool visited = false;
};

class DungeonGenerator {
public:
    // Standard 35x35 procedurally-generated level (levels 2-37).
    // `levelNumber` is 1-based, matching Dungeon.java's `levelNumber`
    // field and DIFFICULTY_TIER_LOOKUP's indexing convention.
    static GeneratedLevel PopulateLevel(int levelNumber, const DungeonGeomRow& geomRow,
                                         const ItemDatabase& items, const MonsterDatabase& monsters);

    // Level 1 (the hub town): fixed-size (19x19), hand-carved template, no
    // RNG involved -- ESGame.java's buildHubTileTemplate(). Unlike
    // dawnstar, Stormhold has no "special shopkeeper room" mechanic inside
    // procedurally-generated levels at all -- all 6-7 shop NPCs (`Shop.
    // SHOP_X`/`SHOP_Y`, 6 fixed + 1 conditional "Warden") live in the hub
    // town only, confirmed by `Shop.java`'s own 7-entry position arrays
    // and by `Dungeon.generate()` having no level-number-gated branch the
    // way dawnstar's own generator does. Only the 6 always-present shops
    // are marked here (bit 32) -- the Warden mechanic (`Shop.
    // wardenPresent`) isn't ported yet.
    static GeneratedLevel BuildHubLevel(const DungeonGeomRow& geomRow);

    // Dungeon.initTier(): maps a level number to its "difficulty tier" --
    // NOT the level number itself, a permutation
    // (DIFFICULTY_TIER_LOOKUP).
    static int InitTier(int levelNumber);

    // Dungeon.MONSTER_TYPE_BY_TIER[zone][bucket] (zone = tier - 1, bucket
    // 0-3): a single public accessor onto the same table PopulateLevel's
    // own room-monster placement already reads internally, so a later
    // milestone's runtime monster-spawn code has one place to read it
    // from instead of a second copy of the table.
    static int MonsterTypeForTierBucket(int tierIndex, int bucket);
};

}  // namespace stormhold
