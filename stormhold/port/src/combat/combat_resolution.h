#pragma once
#include <cstdint>
#include <vector>

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

    // GameCanvas.tickMonsterAI() (M37, phase-3 port; was decompiled/
    // e.java's b(long)) -- REPLACES this port's own earlier wrong
    // "tickStatusCountdowns_b" guess-name in ../../../src/GameCanvas.java.
    // The long-flagged, previously-unrecovered caller for Monster.tick()/
    // Monster.chase() (see ../../docs/ROADMAP.md, flagged since phase-3
    // M14/M15) -- finally found. For every monster registered on `level`
    // (must be the PLAYER's own current level -- caller's responsibility,
    // same trust-the-caller convention `MonsterTick`'s own `level`
    // parameter already uses): NOT adjacent to the player ->
    // `MonsterRuntime::Chase` one step (itself gated on IsWithinRange/
    // chaseCadence; `IsAdjacent`'s own non-adjacent call ALSO resets
    // `aiPhase` to 0, a confirmed side effect, see its own doc comment);
    // adjacent -> an 800ms wind-up: `aiPhase` 0->1 just starts the timer,
    // `aiPhase` 1 past 800ms resolves the FIRST real attack (`MonsterTick`)
    // and marks this call's own return true (the caller's cue to show the
    // "Creature attacks!" popup, `MSG_CREATURE_ATTACKS` priority 2 --
    // `../../../src/GameCanvas.java`'s own `ah`/`showMessage(ah,2)` call,
    // confirmed reproduced ONLY on this first wind-up-to-attack
    // transition), and any LATER 800ms-elapsed tick (`aiPhase` already
    // left at 2 by `MonsterTick`'s own previous call) resolves a repeat
    // attack with no further popup. Every branch stores the monster back
    // via `DungeonRuntime::StoreMonster` -- unlike `tickPerSecond()`'s own
    // confirmed dead-write loop (`player/player_combat_stats.h`'s own
    // `TickPerSecond`), this one really does persist, since the original
    // explicitly calls `store()` itself every time.
    //
    // Iterates a SNAPSHOT of `level`'s own registered spawnIds, not the
    // live map directly: `MonsterTick`'s own ailment-2 ("swarm curse")
    // branch can insert NEW monsters into this exact same registry mid-
    // loop (`DungeonRuntime::SpawnAmbushMonsters`) -- the real Hashtable/
    // Enumeration the original enumerates has the SAME hazard (a real,
    // confirmed original quirk, not introduced here), but Java's legacy
    // Enumeration merely has unspecified-but-non-crashing behavior for
    // it, while a `std::unordered_map` iterator invalidated by a
    // mid-loop insert is genuine undefined behavior -- strictly worse
    // than the original, not a faithful port of it. A monster removed
    // mid-loop (a future kill-on-attack path) is simply skipped if its
    // snapshotted spawnId no longer resolves.
    //
    // Returns true if a "Creature attacks!" popup should be shown THIS
    // call -- caller decides what to do with that (e.g. `render/
    // message_popup.h`'s own `MessagePopup::Show`), same "caller
    // supplies/owns state" pattern this port uses throughout; this
    // module still doesn't depend on `stormhold_render` at all.
    static bool TickMonstersOnLevel(WorldRegistry& world, GeneratedLevel& level, PlayerState& player,
                                     const CharacterData& charData, const ItemDatabase& items,
                                     const MonsterDatabase& monsterDb, std::vector<GeneratedLevel>& levels,
                                     int64_t now, JavaRandom& globalRng, JavaRandom& ambushRng,
                                     int16_t& spawnIdCounter);
};

}  // namespace stormhold
