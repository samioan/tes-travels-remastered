#pragma once
#include <cstdint>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "dungeon/dungeon_runtime.h"
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
// Both entry points now award skill exp on a strong hit via
// player/player_leveling.h's PlayerLeveling::GainSkillExp (M15) --
// resolving the one gap M13/M14 both had to flag and defer. Note that
// awarding the exp is as far as either entry point goes: NEITHER calls
// PlayerLeveling::TryRankUpSkills itself (the real Player.attack()/
// Monster.tick() don't either -- rank-up checking is the caller's own
// separate per-tick concern in the original, not part of either combat
// method), so a caller driving a real combat loop needs to call
// TryRankUpSkills (and eventually ApplyLevelUpAttributeChoices) itself
// on its own cadence.
//
// M17: both entry points now wire the `target.store()`/
// `Dungeon.spawnAmbushMonsters()` real registry side effects M14 had to
// flag and defer, now that M16's `WorldRegistry`/`DungeonRuntime` exist
// -- see each method's own declaration comment.
class CombatResolution {
public:
    // Player.attack(Monster target): rolls hit tier via
    // PlayerCombatStats::RollOutcome(attackAccuracy+diff, target's
    // defend-chance stat-diff), applies damage (weaponDamage vs. target's
    // armor stat, min 4, scaled by the target's type multiplier column)
    // via MonsterRuntime::TakeDamage, with an extra "effect 7" second
    // damage tick and an "effect 13" armor-reduction/"effect 10"
    // diff-bonus read of `target.scratch[]`. Sets player.lastCombatTargetId,
    // and on a tier>=2 hit awards ActiveWeaponSkillIndex() exp via
    // PlayerLeveling::GainSkillExp. M17: `target.store()` -- registers
    // `target`'s post-damage bytes into `world` via
    // `DungeonRuntime::StoreMonster` -- now actually runs, keyed by
    // `target.dungeonLevel`/`target.spawnId` same as the real store();
    // caller must ensure `target` was already loaded from (or is about
    // to be written back into) that same `world`.
    static void PlayerAttack(PlayerState& player, MonsterState& target, const CharacterData& charData,
                              const ItemDatabase& items, const MonsterDatabase& monsterDb, JavaRandom& globalRng,
                              WorldRegistry& world);

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
    // On a successful player block (the defender's own roll also hit),
    // awards DefenseSkillIndex() exp via PlayerLeveling::GainSkillExp.
    //
    // M17: the ailment-2 ("swarm curse") side effect now really spawns 3
    // extra monsters via `DungeonRuntime::SpawnAmbushMonsters(level, world,
    // 3, ambushRng, monsterDb, spawnIdCounter)` -- `level` MUST be `m`'s
    // OWN level (`ESGame.dungeons[m.dungeonLevel - 1]` in the original;
    // caller's responsibility, same trust-the-caller convention as
    // `player/player_movement.h`'s LevelLookup). `ambushRng` is a
    // SEPARATE RNG stream from `globalRng` on purpose: the real
    // `Dungeon.spawnAmbushMonsters()` draws from that Dungeon INSTANCE's
    // own persisted `this.rng` (the same per-level generator RNG object
    // `Dungeon.generate()` seeded once and keeps advancing across the
    // level's whole lifetime), never `ESGame`'s shared roll RNG that
    // `globalRng` here stands in for -- this port has no persisted
    // per-level RNG object of its own yet (`GeneratedLevel` doesn't carry
    // one), so the caller supplies whichever `JavaRandom` stands in for
    // that level's own stream, same "caller supplies/owns world state"
    // pattern as everywhere else in this port. `spawnIdCounter` is the
    // same caller-owned local-counter stand-in `DungeonRuntime::
    // SpawnAmbushMonsters` itself already documents.
    static void MonsterTick(MonsterState& m, PlayerState& player, const CharacterData& charData,
                             const ItemDatabase& items, const MonsterDatabase& monsterDb, int64_t now,
                             JavaRandom& globalRng, GeneratedLevel& level, WorldRegistry& world,
                             JavaRandom& ambushRng, int16_t& spawnIdCounter);
};

}  // namespace stormhold
