#pragma once
#include <array>
#include <cstdint>

#include "assets/binary_reader.h"
#include "assets/binary_writer.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "monster/monster_state.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/Monster.java -- everything
// except tick() (the monster's attack-the-player AI) and attack()'s
// Monster-side reads (both live in combat/combat_resolution.h instead, so
// this module and player/player_combat_stats.h stay siblings with no
// dependency on each other; only the small combat-resolution module
// depends on both). Same split dawnstar's own port uses (its M15).
//
// IMPORTANT, deliberately-preserved distinction (confirmed directly from
// Monster.java, matches dawnstar's own finding): typeStats is read via TWO
// different conventions depending on which method -- stat(column) (masked
// `& 0xFF`, used by Player.attack()/combat_resolution.h's PlayerAttack)
// versus direct field access to `typeStats[...]` (RAW signed byte, used
// internally by Monster's own constructor/tick()/onDeath()).
// MonsterDatabase::Stat() (assets/monster_database.h) matches the masked
// reading; RawStat() matches the other one.
class MonsterRuntime {
public:
    // Monster.spawn(Random, Dungeon, forcedTypeIndex)'s type-selection
    // half only (the constructor call itself is Spawn(), below):
    // `forcedTypeIndex` >= 0 bypasses the roll and is returned as-is;
    // otherwise clamps `levelTier - 1` to [0,36] and rolls a 1-10
    // (RandomInt1Based -- ESGame.randomInt's own 1-based convention, see
    // util/java_random.h's naming-swap warning) rarity bucket (<=4:0,
    // <=7:1, <=9:2, else:3) into
    // DungeonGenerator::MonsterTypeForTierBucket.
    static int PickMonsterType(JavaRandom& rng, int levelTier, int forcedTypeIndex);

    // Monster(int spawnId, int typeIndex, int dungeonLevel)'s
    // constructor: hp copied RAW (not masked) from the type's max-HP
    // column (14). tileX/tileY are NOT set here -- the real Java ctor
    // doesn't set them either; callers place the monster afterward (see
    // world/dungeon_generator.h's GeneratedMonsterSpawn / M6's
    // PopulateLevel, which still inlines its own copy of this type-
    // selection rather than calling this function -- see
    // MonsterTypeForTierBucket's own doc comment for why).
    static MonsterState Spawn(int16_t spawnId, int typeIndex, int dungeonLevel, const MonsterDatabase& db);

    // The masked/unsigned reading -- Monster.stat(column)'s convention.
    static uint8_t Stat(const MonsterState& m, const MonsterDatabase& db, int column) {
        return db.Stat(m.typeIndex, column);
    }
    // The raw signed reading -- see the class comment.
    static int8_t RawStat(const MonsterState& m, const MonsterDatabase& db, int column) {
        return db.RawStat(m.typeIndex, column);
    }
    static const std::string& TypeName(const MonsterState& m, const MonsterDatabase& db) {
        return db.TypeName(m.typeIndex);
    }
    static bool IsUndead(const MonsterState& m) { return m.typeIndex >= 6 && m.typeIndex <= 8; }
    static void TakeDamage(MonsterState& m, int amount);

    // Monster.toBytes()/fromBytes(Shared|Into): the packed 28-byte record
    // ESGame.monsters[]'s spawnId-keyed hashtable stores (see store(),
    // wired for real since M16/M17's dungeon/dungeon_runtime.h) -- and
    // OnDeath()'s dropped-item record below needs the same packed-byte-
    // array shape, so it's worth having standalone regardless.
    static std::array<uint8_t, 28> ToBytes(const MonsterState& m);
    static MonsterState FromBytes(const std::array<uint8_t, 28>& in);

    // M20: Monster.readFrom(DataInputStream)/writeTo(DataOutputStream) --
    // deferred since M14 pending a BinaryWriter (assets/binary_writer.h,
    // added this milestone). **Confirmed, by reading both methods
    // side by side, to encode the EXACT SAME 28 fields in the EXACT SAME
    // order as toBytes()/fromBytes*() above** -- unlike dawnstar's own
    // Monster, where its own port's M15 entry found genuinely different
    // field sets between the two serializations, Stormhold's `readFrom`/
    // `writeTo` are just `toBytes()`/`fromBytesShared()`'s same 28 bytes
    // written through `DataInputStream`/`DataOutputStream` primitive
    // calls instead of manual bit-shifting into a `byte[]` -- a
    // consequence of THIS game's engine, not assumed to carry over from
    // dawnstar's. `ReadFrom`/`WriteTo` are still written as their own
    // direct field-by-field stream calls (not implemented in terms of
    // ToBytes/FromBytes) for line-for-line fidelity with Monster.java's
    // own two separate methods.
    static MonsterState ReadFrom(BinaryReader& in);
    static void WriteTo(BinaryWriter& out, const MonsterState& m);

    // Monster.move(direction): one step in compass direction 1=N/2=E/3=S/
    // 4=W. Blocked by out-of-bounds, a stairway tile (IsStairwayTile), or
    // Dungeon.isWalkable's rule -- CONFIRMED to be a DIFFERENT bit test
    // than PlayerMovement::IsWalkableTileBits (bits 1/2/32): Dungeon.
    // isWalkable also checks bit 8 (no-spawn/special), Player's own
    // isWalkableTileBits doesn't. Mutates the monster-occupied tile-bit
    // marker (bit 2) on `levels[m.dungeonLevel-1]` directly. Does NOT call
    // store() itself -- confirmed from Monster.java's own header comment,
    // a REAL engine difference from dawnstar's Monster.move (which does
    // call it), not a simplification this port is choosing.
    static bool Move(MonsterState& m, int dir, std::vector<GeneratedLevel>& levels);
    // The cascading N/S/W/E stairway-tile check, keyed off the level's
    // own up/down stairway direction fields -- same "only the highest-
    // priority direction ever matches" property dawnstar's own
    // isStairwayTile has (see dawnstar's PORT_ROADMAP.md M15). Preserved
    // byte-for-byte, not "fixed" into an order-independent check.
    static bool IsStairwayTile(const GeneratedLevel& level, int x, int y);

    // Monster.distanceTo(Player): manhattan distance to (targetX,targetY).
    static int DistanceTo(const MonsterState& m, int targetX, int targetY) {
        return JavaAbs(targetX - m.tileX) + JavaAbs(targetY - m.tileY);
    }
    // Monster.isWithinRange(Player): manhattan distance <= 3.
    static bool IsWithinRange(const MonsterState& m, int targetX, int targetY) {
        return DistanceTo(m, targetX, targetY) <= 3;
    }
    // Monster.isAdjacent(Player): manhattan distance == 1 -- has the side
    // effect of resetting aiPhase to idle (0) on any NON-adjacent call,
    // ported exactly (this is a real, if surprising, side effect of a
    // method whose name reads as a pure query).
    static bool IsAdjacent(MonsterState& m, int targetX, int targetY);

    // Monster.chase(Player): gated on IsWithinRange (distance <= 3) --
    // Stormhold's own version, UNLIKE dawnstar's Monster.chase(x,y) (which
    // takes no such internal gate; see dawnstar's src/Monster.java line
    // ~201), always checks range itself before doing anything with
    // chaseCadence. Confirmed directly from Monster.java: a real
    // Stormhold-specific divergence, not a transcription gap. Within
    // range, takes one step toward (targetX,targetY) only 1 call in 5
    // (chaseCadence 0..4), picking the larger-distance axis first (ties
    // broken via a coin flip -- ESGame.lingoRandomInt(2), the 0-based
    // RandomInt0Based convention, NOT RandomInt1Based), falling back to
    // the other axis if blocked. VOID, matching the original exactly --
    // unlike dawnstar's own Monster.chase(x,y), which returns whether it
    // stepped, Stormhold's chase(Player) returns nothing at all.
    static void Chase(MonsterState& m, int targetX, int targetY, std::vector<GeneratedLevel>& levels,
                       JavaRandom& globalRng);

    // Monster.onDeath(guaranteedDrop): rolls (or, if guaranteed, forces) a
    // death-loot drop from the type's loot-table row (columns 15/16, read
    // RAW -- see the class comment). Returns the raw 7-byte dropped-item
    // record Monster.java itself builds (tileX, tileY, itemIdLow,
    // spawnIdHigh, spawnIdLow, itemIdHigh, flags -- itemIdHigh only
    // meaningful when itemIdLow==86, see assets/item_database.h's
    // RollLoot doc comment) with `dropped=false` if nothing drops.
    // SIMPLIFIED: doesn't call Dungeon.addDroppedItem() -- no live
    // dropped-item registry exists yet (same gap player/player_movement.h
    // already flags for its own deferred auto-loot). `dropSpawnId`
    // substitutes for Item.nextSpawnId()'s global counter, same
    // simplification as player/player_creation.cpp's GrantStartingItems.
    struct DeathDrop {
        bool dropped = false;
        std::array<uint8_t, 7> record{};
    };
    static DeathDrop OnDeath(const MonsterState& m, const MonsterDatabase& monsterDb, const ItemDatabase& items,
                              int levelTier, bool guaranteedDrop, int16_t dropSpawnId, JavaRandom& globalRng);
};

}  // namespace stormhold
