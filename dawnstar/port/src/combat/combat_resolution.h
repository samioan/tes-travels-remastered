#pragma once
#include <cstdint>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "monster/monster_state.h"
#include "player/player_state.h"
#include "util/java_random.h"

namespace dawnstar {

// Renamed-source counterpart of the two combat entry points that need
// BOTH Player and Monster: ../../../src/Player.java's attack(Monster)
// and ../../../src/Monster.java's tick(Player, now). Kept in their own
// module (rather than folded into player/player_combat_stats.h or
// monster/monster_runtime.h) precisely so those two stay siblings with
// no dependency on each other -- only this module depends on both. See
// docs/PORT_ROADMAP.md's M15 entry.
class CombatResolution {
public:
    // Player.attack(Monster target): rolls hit tier via
    // PlayerCombatStats::RollOutcome(attackAccuracy, target's evasion-ish
    // stat), applies damage (weaponDamage - target defense, min 4,
    // scaled by the target's type multiplier) via
    // MonsterRuntime::TakeDamage, and on a strong hit awards weapon-skill
    // exp. Sets player.combatTargetSpawnId. SIMPLIFIED: doesn't call
    // Monster's store() (no live per-level registry -- see
    // monster/monster_runtime.h's class comment).
    static void PlayerAttack(PlayerState& player, MonsterState& target, const CharacterData& charData,
                              const ItemDatabase& items, const MonsterDatabase& monsterDb, JavaRandom& globalRng);

    // Monster.tick(Player, now): the monster's own attack-the-player AI
    // tick (an 800ms "wind-up" then an action phase). Resolves detection
    // (vs the player's weaponSkillValue/baseEvasion), then an attack (vs
    // the player's armorValue) directly mutating `player`'s HP/
    // ailments/timers, and rolls a 30%-chance on-hit ailment matching the
    // monster type's column-11 id. Returns whether an attack landed.
    // SIMPLIFIED: the ailment==2 ("curse of hunger") side effect, which
    // spawns 3 more monsters via Dungeon.populateRandomMonsters(3), is a
    // no-op -- no live per-level monster registry exists to spawn into
    // yet (same class of gap as player/player_movement.h's deferred
    // roaming-monster cleanup).
    static bool MonsterTick(MonsterState& m, PlayerState& player, const CharacterData& charData,
                             const ItemDatabase& items, const MonsterDatabase& monsterDb, int64_t now,
                             JavaRandom& globalRng);
};

}  // namespace dawnstar
