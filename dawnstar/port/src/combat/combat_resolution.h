#pragma once
#include <cstdint>
#include <vector>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/spell_database.h"
#include "dungeon/dungeon_runtime.h"
#include "monster/monster_state.h"
#include "player/player_state.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

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

    // Player.castOnMonster(spellId, target): the third entry point that
    // needs both Player and Monster (case 14 -- "Blade Focus" or
    // similar -- literally calls back into PlayerAttack). Same
    // roll-then-apply shape as PlayerAttack/player/
    // player_spellcasting.h's CastOnSelf: rolls hit tier off the
    // caster's spell skill vs the target's evasion/defense stats, spends
    // Magicka scaled by the outcome, then a big per-spell-id switch of
    // direct damage (via MonsterRuntime::TakeDamage) and status effects
    // on `target` (target.scratch[]) or self-buffs
    // (player.effectDurations[]/tempArmorBonus). SIMPLIFIED: like
    // PlayerAttack, skips every target.store() call (see
    // monster/monster_runtime.h's class comment).
    static void CastOnMonster(PlayerState& player, MonsterState& target, const CharacterData& charData,
                               const ItemDatabase& items, const MonsterDatabase& monsterDb,
                               const SpellDatabase& spells, JavaRandom& globalRng);

    // Player.useItem(slot, target): the fourth entry point that needs
    // both Player and Monster -- its 97/98/99 instant-kill scrolls read
    // `target`'s stat(4)/stat(10) and, if the roll-free threshold check
    // passes, zero its hp directly (`target.hp = 0`, not TakeDamage --
    // ported exactly, bypassing whatever TakeDamage's own clamping does).
    // `target` is nullable, matching Java's `Monster target` (null when
    // called from ESGame's inventory screen with no monster targeted;
    // GameCanvas.targetMonster itself isn't ported, so the 1-arg
    // useItem(slot) overload that reads it isn't either -- callers pass
    // whatever target they have, or nullptr). Every other item id (87-96)
    // is Player-only: 87 (warp-to-camp-or-mark) delegates to
    // player/player_movement.h's HasCampMark/WarpToCampMark/
    // MarkCampAndReturnToTown (hence the `levels`/`world` parameters --
    // M23 gave MarkCampAndReturnToTown a live-registry dependency of its
    // own, for its roaming-monster cleanup), 88 to player/
    // player_spellcasting.h's CureRandomAilment (hence `globalRng`).
    // Every used item is consumed (removed from its slot) afterward
    // except 96 ("Safe Camping"), which the original deliberately leaves
    // in the inventory -- ported via the same `consume` flag the
    // original uses. SIMPLIFIED: like the other combat entry points,
    // skips target.store().
    static void UseItem(PlayerState& player, int slot, MonsterState* target, const ItemDatabase& items,
                         const MonsterDatabase& monsterDb, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                         JavaRandom& globalRng);
};

}  // namespace dawnstar
