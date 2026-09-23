#pragma once
#include <array>
#include <cstdint>
#include <string>

#include "assets/character_data.h"
#include "assets/dungeon_names.h"
#include "assets/item_database.h"
#include "player/player_state.h"

namespace stormhold {

// GameCanvas's own `unconfirmed_s` instance field (a `long` death
// timestamp) -- session-level state, NOT on Player.java itself, same
// class-boundary precedent player/camp_state.h's own CampState (its
// `rollAtMs`) already established for GameCanvas-level fields this port
// doesn't fold into PlayerState.
//
// Unlike CampState, this state machine's actual STATE (alive/dead-
// waiting) is NOT modeled here at all -- it reuses PlayerState::facing
// directly (1=alive/normal, 2=just died this tick, 3=waiting out the 5s
// respawn window), the exact same field player/player_movement.h's own
// CommitMove/ComputeMoveTarget already drive for ordinary movement.
// Confirmed directly from ../../../src/GameCanvas.java's own run(): there
// is no separate "death state" field in the original either -- it really
// does overload `facing` this way, and run()'s own campState==1/campState
// ==2/facing!=1 dispatch is one shared else-if chain, so a player can
// never be simultaneously camping and dead in the original. This port's
// own Camping::Tick (player/camp_state.h) is NOT structurally exclusive
// with DeathSequence::Tick below the same way -- both are independent
// calls in main.cpp's tick loop -- but this is harmless: reaching HP<=0
// needs CombatResolution::TickMonstersOnLevel to actually deal damage,
// which (matching the original) only runs once already-established
// camp/death gating has cleared for the tick, so the two states still
// can't overlap in practice.
struct DeathState {
    int64_t deathAtMs = 0;
};

// What DeathSequence::Tick found this call, so the caller (main.cpp) can
// show the right popup and gate its own per-tick movement/attack/
// spellcast/monster-AI work without this module depending on
// stormhold_render -- same "game logic returns a signal, caller shows
// the message" pattern player/camp_state.h's CampTickResult and
// combat/spell_casting.h's Result already use.
enum class DeathTickResult {
    Alive,      // facing==1 -- an ordinary tick, caller runs normally.
    Waiting,    // facing==2 or 3 -- still waiting out the 5s window
                // (matches CampTickResult::StillWaiting's own role: the
                // caller's normal per-tick movement/attack/spellcast/
                // monster-AI work should be skipped, same as run()'s own
                // shouldRunTick==false gate).
    Respawned,  // the 5s window just elapsed -- a full respawn was just
                // applied THIS call.
};

class DeathSequence {
public:
    // GameCanvas.tickDeathAndRegen(now, deltaMs) -- confirmed sole real
    // caller: run()'s own `if (shouldRunTick)` block, i.e. only while
    // DeathSequence::Tick (below) is NOT currently returning Waiting (same
    // "StillWaiting means skip the caller's normal per-tick work" pattern
    // player/camp_state.h's own Camping::Tick/CampTickResult::StillWaiting
    // already established) -- the caller's job to gate this call
    // accordingly, this method does not check `p.facing` itself.
    //
    // Checks the player's CURRENT HP via
    // PlayerCombatStats::EffectiveStat(p, charData, 2): if it's already <=
    // 0, stamps `death.deathAtMs = now`, sets `p.facing = 2` (Tick's own
    // job to notice and advance that next call), and returns true so the
    // caller can clear its own "target monster" state (GameCanvas's own
    // `unconfirmed_aa` -- this port's `targetMonster`/`hudState.
    // unconfirmedAa` in main.cpp, kept there rather than duplicated here,
    // same "render/HUD state stays with its owner" boundary combat/
    // spell_casting.h's own Result enum already established). Otherwise
    // ticks passive Fatigue regen (PlayerCombatStats::TickFatigueRegen)
    // and returns false.
    //
    // **NOT modeled: `unconfirmed_at`** (an "an action was already
    // resolved this tick" gate the original puts on the fatigue-regen
    // branch, `if (!unconfirmed_at) tickFatigueRegen(...)`) -- same
    // confirmed, already-documented gap combat/spell_casting.h's own
    // ResolveSpellCastInput header comment flags for its own read of the
    // same field; fatigue regen here always runs unconditionally instead
    // of only on ticks where nothing else already acted.
    static bool TickDeathAndRegen(PlayerState& p, DeathState& death, const CharacterData& charData, int64_t now,
                                   int64_t deltaMs);

    // GameCanvas.run()'s own `facing != 1` dispatch branch (the death-
    // sequence counterpart of CampState's own campState==1/2 handling,
    // player/camp_state.h's Camping::Tick) -- checked EVERY tick, BEFORE
    // the caller's own shouldRunTick-gated work, the same position
    // Camping::Tick already occupies in main.cpp.
    //
    // Returns Alive when facing==1 (does nothing else). On facing==2,
    // advances it to 3 (the original's own transition-tick side effect,
    // `unconfirmed_ad = false; messagePriority = 0`, clearing whatever
    // message is currently shown, is NOT reproduced here -- already a
    // confirmed, documented gap: see render/message_popup.h's own Tick()
    // comment, which flags this exact reset, if under a slightly
    // mislabeled "camp-state transition" description -- corrected here:
    // it's this death-sequence transition, not a camp one) then, same as
    // the original's own unconditional post-transition check, tests the
    // 5s window immediately (in practice always still fresh the instant
    // facing becomes 2, since this only just got stamped THIS same real-
    // time tick by TickDeathAndRegen above). Returns Waiting while the
    // window hasn't elapsed.
    //
    // Once it has, applies the full respawn, matching run()'s own
    // sequence exactly: PlayerCreation::NormalizeToMaxStats(p.coreStats),
    // then strips every currently NON-equipped inventory slot (walking
    // backward through inventoryCount, matching the original's own
    // reverse loop exactly -- PlayerInventory::IsSlotEquipped/
    // RemoveInventorySlot), then PlayerCreation::RespawnAfterDeath(p)
    // (resetState(classIndex, true)), then resets `death.deathAtMs = 0`
    // and `p.facing = 1` (this last write is confirmed REDUNDANT --
    // RespawnAfterDeath's own setHubSpawnPosition(true) already sets
    // `p.facing = 1` as part of the hub-position write -- preserved
    // anyway, matching the original's own identical redundant
    // `this.facing = 1;` right after its own resetState(true) call).
    // Returns Respawned.
    //
    // **NOT modeled here, same "render/HUD state stays in main.cpp"
    // boundary as everything else in this class:** `Shop.
    // showSpecialGreeting`/`unconfirmed_v`/`unconfirmed_E` (refresh/UI-
    // only flags with no C++ port-side counterpart yet) -- the caller
    // should show a respawn-location message on a Respawned result
    // (see DeathSequence::RespawnMessageLines below), same "game logic
    // returns a signal, caller shows the message" pattern this whole
    // port uses.
    static DeathTickResult Tick(PlayerState& p, DeathState& death, const ItemDatabase& items, int64_t now);

    // GameCanvas.run()'s own dead-respawn message selection (byte-for-
    // byte the SAME 3-way choice `resolveMovementSideEffects()` also
    // makes for its own, separate, NOT-yet-wired level-crossing message --
    // see that method's own header comment in ../../../src/GameCanvas.java
    // for the cross-reference): `p.enteredNewLevelZone` -> "Warden's
    // Camp"; else `p.leftLevelZone` -> "Outer Camp" (confirmed permanently
    // dead by player/player_movement.h's own CommitMove finding -- M10 --
    // preserved here rather than deleted, same "port a confirmed-dead
    // branch faithfully" treatment this port already gives every other
    // one); else the CURRENT level's own display name (DungeonNames::
    // DisplayNames(p.currentLevel)).
    //
    // Meant to be called AFTER Tick() has already returned Respawned --
    // by that point `p.currentLevel` is always 1 (RespawnAfterDeath's own
    // setHubSpawnPosition(true)), so the "current level's display name"
    // branch in practice always resolves to the HUB's own dungnamesin.dat
    // row, UNLESS `enteredNewLevelZone` happens to still be true from
    // whatever the player's last real move was before dying -- neither
    // flag is touched by RespawnAfterDeath/resetState(true) at all
    // (confirmed by reading resetState() directly), so this genuinely can
    // read STALE pre-death state, not a bug on this port's side.
    static std::array<std::string, 2> RespawnMessageLines(const PlayerState& p, const DungeonNames& names);
};

}  // namespace stormhold
