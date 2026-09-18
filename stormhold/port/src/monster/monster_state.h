#pragma once
#include <array>
#include <cstdint>

namespace stormhold {

// Renamed-source counterpart of ../../../src/Monster.java's per-instance
// runtime state (as opposed to assets/monster_database.h, the static
// per-type stat table Monster.loadTypes() builds once at startup).
//
// Two confirmed Stormhold-specific divergences from dawnstar's own
// MonsterState (see Monster.java's own header comment, read directly
// rather than assumed to carry over): `spawnId` is a per-SPAWN unique
// counter value (Monster.nextSpawnId()'s static incrementing short), NOT
// a "monster type id" the way dawnstar's field of the same shape is
// named/used; and the live per-level registry this eventually feeds
// (ESGame.monsters[dungeonLevel-1], not modeled yet -- see
// monster_runtime.h's class comment) is keyed by `String.valueOf(spawnId)`
// alone, not a position key.
struct MonsterState {
    int16_t spawnId = 0;
    int8_t typeIndex = 0;
    int8_t currentHp = 0;
    int8_t tileX = 0;
    int8_t tileY = 0;
    // Monster.java's own field comment doesn't say more than the name
    // implies -- a collected/looted marker, unconfirmed further.
    bool unconfirmedFlag = false;
    int8_t dungeonLevel = 0;
    // 10 bytes of monster-type-specific combat/status scratch space --
    // Player.attack()'s reads of scratch[1]/[5]/[8] for specific ongoing-
    // effect magnitudes (see combat/combat_resolution.h's PlayerAttack).
    std::array<int8_t, 10> scratch{};
    // AI pacing: chase() takes an actual step only 1 time in 5 calls
    // (cycles 0..4) -- see MonsterRuntime::Chase.
    int8_t chaseCadence = 0;
    // AI phase: 0=idle/just noticed, 1=wound-up (waiting), 2=acted this
    // tick. UNLIKE dawnstar's own MonsterTick, Stormhold's tick() (see
    // combat/combat_resolution.h's MonsterTick doc comment) does NOT gate
    // itself on an 800ms wind-up using this field internally -- it sets
    // aiPhase=2 and resolves combat unconditionally on every call, only
    // ever leaving it at 1 afterward. Confirmed by reading tick() itself
    // directly: whatever caller decides WHEN to call tick() (presumably
    // in one of Stormhold's still-stubbed GameCanvas rendering/AI-scan
    // methods -- see ../../docs/ROADMAP.md's Phase 1 status) isn't
    // recovered yet, so the real wind-up gating this field's own name
    // implies can't be confirmed either way.
    int8_t aiPhase = 0;
    // Wall-clock ms timestamp (System.currentTimeMillis() in the
    // original) -- Monster.java's own field comment leaves its exact use
    // unconfirmed beyond being written by tick(); not read back by
    // tick() itself, only stored.
    int64_t unconfirmedTimestamp = 0;
};

}  // namespace stormhold
