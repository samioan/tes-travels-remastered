#pragma once
#include <cstdint>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "monster/monster_state.h"
#include "player/player_state.h"
#include "util/java_random.h"

namespace stormhold {

// Renamed-source counterpart of the two combat entry points that need
// BOTH Player and Monster: ../../../src/Player.java's attack(Monster) and
// ../../../src/Monster.java's tick(Player, now). Kept in their own module
// (rather than folded into player/player_combat_stats.h or
// monster/monster_runtime.h) precisely so those two stay siblings with no
// dependency on each other -- only this module depends on both. Same
// split dawnstar's own port uses (its M15's combat/combat_resolution.h).
//
// Neither entry point here awards skill exp on a strong hit
// (Player.gainSkillExp()/Player.attack()'s activeWeaponSkillIndex() call,
// tick()'s player.defenseSkillIndex() call): no leveling system is ported
// yet (gainSkillExp/consumeLevelExp, confirmed to have a real cross-
// system coupling with Shop.clearQuestTurnInState() -- see player/
// player_combat_stats.h's own class header comment and
// ../../docs/PORT_ROADMAP.md's M13 "what's next" note). Flagged again at
// each specific omission point below rather than silently dropped, same
// discipline player/player_movement.h uses for ITS deferred side effects.
class CombatResolution {
public:
    // Player.attack(Monster target): rolls hit tier via
    // PlayerCombatStats::RollOutcome(attackAccuracy+diff, target's
    // defend-chance stat-diff), applies damage (weaponDamage vs. target's
    // armor stat, min 4, scaled by the target's type multiplier column)
    // via MonsterRuntime::TakeDamage, with an extra "effect 7" second
    // damage tick and an "effect 13" armor-reduction/"effect 10"
    // diff-bonus read of `target.scratch[]`. Sets player.lastCombatTargetId.
    // SIMPLIFIED: skips target.store() (no live per-level registry yet --
    // see monster/monster_runtime.h's class comment) and the skill-exp
    // award (see class comment above).
    static void PlayerAttack(PlayerState& player, MonsterState& target, const CharacterData& charData,
                              const ItemDatabase& items, const MonsterDatabase& monsterDb, JavaRandom& globalRng);

    // Monster.tick(Player, now): the monster's own attack-the-player AI
    // step. UNLIKE dawnstar's own MonsterTick, this does NOT internally
    // gate on an 800ms wind-up phase -- confirmed directly from
    // Monster.java, it resolves a full attack roll unconditionally every
    // call (see monster/monster_state.h's aiPhase field comment). Rolls
    // detection/attack (vs. the player's defenseSkillValue(true)/
    // baseEvasion()), then on a hit deals damage (vs. the player's
    // armorValue()) directly to player.coreStats[2] (HP), and on a tier-3
    // ("clean hit") result rolls a further 30% chance to inflict an
    // ailment matching the monster type's RAW column-11 id (setting
    // player.ailmentMask and, for ids 4/5, vampirismTimer/manaBurnTimer).
    // SIMPLIFIED: the ailment-2 ("swarm curse") side effect -- spawning 3
    // more monsters via Dungeon.spawnAmbushMonsters(3) -- is a no-op, no
    // live per-level monster registry exists to spawn into yet (same
    // class of gap as player/player_movement.h's deferred auto-loot); and
    // the skill-exp award on a successful player block (see class comment
    // above).
    static void MonsterTick(MonsterState& m, PlayerState& player, const CharacterData& charData,
                             const ItemDatabase& items, const MonsterDatabase& monsterDb, int64_t now,
                             JavaRandom& globalRng);
};

}  // namespace stormhold
