#pragma once
#include <array>
#include <cstdint>
#include <vector>

#include "assets/binary_reader.h"
#include "assets/binary_writer.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "monster/monster_state.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Monster.java -- everything
// except tick() (the monster's attack-the-player AI) and attack()'s
// Monster-side reads (both live in combat/combat_resolution.h instead,
// so this module and player/player_combat_stats.h stay siblings with no
// dependency on each other; only the small combat-resolution module
// depends on both). See docs/PORT_ROADMAP.md's M15 entry.
//
// IMPORTANT, deliberately-preserved distinction found while transcribing
// this class: Monster.java's own methods read typeStats via TWO
// different conventions depending on which method -- stat(column)
// (masked `& 0xFF`, used by Player.attack(), i.e. combat_resolution.h's
// PlayerAttack) versus direct private-field access to `typeStats[...]`
// (raw signed byte, used internally by Monster's own tick()/onDeath()).
// These give genuinely different int values whenever a column's stored
// byte is >=128 as an unsigned reading. MonsterDatabase::Stat() (see
// assets/monster_database.h) always returns the masked/unsigned
// reading, matching stat()'s convention; RawStat() below matches the
// other one, and combat_resolution.h's MonsterTick uses RawStat for
// exactly the columns Monster.tick() reads directly.
class MonsterRuntime {
public:
    // Monster.spawn()'s type-selection: `forcedType` >= 0 (1-based type
    // id) bypasses the roll and is returned as-is; otherwise rolls a
    // tier-weighted bucket (Dungeon.MONSTER_TABLE via
    // DungeonGenerator::MonsterTypeForTierBucket).
    static int PickMonsterType(JavaRandom& rng, int difficultyTier, int forcedType);

    // Monster(int spawnId, int monsterType, int dungeonLevel)'s
    // constructor: hp copied raw (not masked) from the type's max-HP
    // column (14). x/y are NOT set here -- the real Java ctor doesn't
    // set them either; callers place the monster afterward (see
    // ../../../src/DungeonGenerator.java's room-monster loop / M6's
    // PopulateLevel, which still inlines its own copy of this
    // type-selection rather than calling this function -- see
    // world/dungeon_generator.h's MonsterTypeForTierBucket doc comment
    // for why).
    static MonsterState Spawn(int16_t spawnId, int monsterType, int dungeonLevel, const MonsterDatabase& db);

    // The masked/unsigned reading -- Monster.stat(column)'s convention.
    static uint8_t Stat(const MonsterState& m, const MonsterDatabase& db, int column) {
        return db.Stat(m.monsterType, column);
    }
    // The raw signed reading -- see the class comment.
    static int8_t RawStat(const MonsterState& m, const MonsterDatabase& db, int column) {
        return static_cast<int8_t>(db.Stat(m.monsterType, column));
    }
    static bool IsUndead(const MonsterState& m) { return m.monsterType >= 6 && m.monsterType <= 8; }
    static void TakeDamage(MonsterState& m, int amount);

    // Monster.toBytes()/fromBytes(): the packed 28-byte record
    // ESGame.monsters[]'s position-keyed hashtable stores (see store()
    // -- no live per-level registry exists in this port yet, so nothing
    // calls an equivalent of Store() yet; ToBytes/FromBytes are provided
    // standalone for whichever milestone adds one).
    static std::array<uint8_t, 28> ToBytes(const MonsterState& m);
    static MonsterState FromBytes(const std::array<uint8_t, 28>& in);

    // Monster.readFrom(DataInputStream)/writeTo(DataOutputStream): a
    // second, different (unpacked) serialization -- both are ported
    // since both are real, distinct formats in the source.
    static MonsterState ReadFrom(BinaryReader& in);
    static void WriteTo(BinaryWriter& out, const MonsterState& m);

    // Monster.move(direction): one step in dungeon direction 1-4
    // (N/E/S/W, matching Player's facing convention -- but NOTE the
    // fallthrough pairing is different from PlayerMovement's: here
    // direction 1 falls into 3's code, and 4 falls into 2's, whereas
    // Player's computeMoveTarget pairs 1 with 2). Mutates the
    // monster-occupied tile-bit marker (bit 2) on
    // `levels[m.dungeonLevel-1]`. Uses Dungeon.isWalkable's rule
    // (DungeonView::IsWalkable), NOT Player.isWalkable's -- they differ
    // (see world/dungeon_view.h). SIMPLIFIED: doesn't call store() (no
    // live registry -- see the class comment above).
    static bool Move(MonsterState& m, int direction, std::vector<GeneratedLevel>& levels);
    // True if (x,y) is one of this level's 4 fixed stairway tiles AND
    // this level actually has a stairway in that direction (monsters
    // don't use stairs).
    static bool IsStairwayTile(const GeneratedLevel& level, int x, int y);

    // Monster.chase(targetX,targetY): moves once every 5 ticks, toward
    // (targetX,targetY), picking the larger-distance axis first (ties
    // broken randomly via `globalRng`), falling back to the other axis
    // if blocked. Returns whether it actually stepped this call.
    static bool Chase(MonsterState& m, int targetX, int targetY, std::vector<GeneratedLevel>& levels,
                       JavaRandom& globalRng);

    // Monster.onDeath(guaranteed): rolls (or, if `guaranteed`, forces) a
    // death-loot drop from the type's loot-table row (columns 15/16,
    // read raw -- see the class comment). Returns the raw 7-byte
    // dropped-item record Monster.java itself builds (x, y, itemIdLow,
    // spawnIdHigh, spawnIdLow, itemIdHigh, flags -- itemIdHigh is only
    // meaningful when itemIdLow==86, see assets/item_database.h's
    // RollLoot doc comment) with `dropped=false` if nothing drops.
    // SIMPLIFIED: doesn't call Dungeon.addDroppedItem() -- no live
    // dropped-item registry exists yet (same gap as
    // player/player_movement.h's deferred auto-loot). `spawnId`
    // substitutes for Item.nextSpawnId()'s global counter, same
    // simplification as player/player_creation.cpp's GrantStartingItems.
    struct DeathDrop {
        bool dropped = false;
        std::array<uint8_t, 7> record{};
    };
    static DeathDrop OnDeath(const MonsterState& m, const MonsterDatabase& monsterDb, const ItemDatabase& items,
                             int levelTier, bool guaranteed, int16_t spawnId, JavaRandom& globalRng);
};

}  // namespace dawnstar
