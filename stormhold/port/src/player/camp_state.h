#pragma once
#include <cstdint>

#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "dungeon/dungeon_runtime.h"
#include "player/player_state.h"
#include "util/java_random.h"

namespace stormhold {

// GameCanvas's own `campState`/`campRollAt` instance fields -- NOT on
// Player.java at all (Player has no camping fields of its own), so this
// lives as its own small struct rather than folding into PlayerState,
// same class-boundary precedent `render/hud_state.h`'s `HudState` and
// `render/message_popup.h`'s `MessagePopupState` already established for
// GameCanvas-level UI/session state that isn't part of the real save
// format either.
//
// Confirmed directly from ../../../src/GameCanvas.java: the state
// machine itself lives in run() (transcribed long before M41/M42
// existed), gated by `campState`/`campRollAt`; `startCampOrRest()` (M41,
// was decompiled/e.java's `a(long)`) is its only confirmed trigger.
struct CampState {
    // 0=not camping, 1=rolling for interruption (2.5s window), 2=safe/
    // undisturbed wait (5s window) -- see Tick()'s own header comment.
    uint8_t state = 0;
    int64_t rollAtMs = 0;
};

// What Tick() found this call, so the caller (main.cpp) can show the
// right popup without CampState itself depending on stormhold_render
// (same "game logic returns a signal, the caller shows the message"
// pattern combat/combat_resolution.h's TickMonstersOnLevel/MonsterTick
// already use).
enum class CampTickResult {
    NotCamping,    // state was already 0 -- a completely ordinary tick.
    StillWaiting,  // still rolling/waiting -- no window elapsed yet.
    Disturbed,     // the 2.5s roll failed: rest interrupted, an ambush
                   // monster spawned, partial (2/3) recovery applied.
    Complete,      // the 5s safe wait finished: full recovery applied.
};

class Camping {
public:
    // GameCanvas.startCampOrRest(now) (M41, was decompiled/e.java's
    // a(long)): state=1 by default, upgraded straight to 2 (skip the
    // interruption roll) when the player has Player.safeCampingBuff
    // active OR is standing in the hub town (currentLevel==1).
    static void Start(CampState& camp, const PlayerState& player, int64_t now);

    // GameCanvas.rollCampInterrupted() (M37, already ported): a flat
    // 1-in-10 chance.
    static bool RollInterrupted(JavaRandom& rng);

    // GameCanvas.run()'s own campState==1/2 handling (transcribed long
    // before this struct existed): while `state==1`, waits 2.5s then
    // rolls RollInterrupted() -- on a hit, resets to state 0,
    // spawns an ambush monster next to the player (DungeonRuntime::
    // SpawnAmbushMonsterNearPlayer), and applies a 2/3 (partial) rest
    // recovery (PlayerCombatStats::ApplyRestRecovery(false)); on a
    // miss, advances to state 2 (no recovery yet, no message). While
    // `state==2`, waits 5s then resets to state 0 and applies a FULL
    // rest recovery (ApplyRestRecovery(true)). Returns NotCamping
    // immediately (does nothing else) when `state==0` -- the caller's
    // own normal per-tick movement/attack/etc. handling should run
    // exactly when this does NOT return StillWaiting (matching run()'s
    // own `shouldRunTick` gate: the original also blocks input for the
    // full 2.5s/5s WINDOW, not just the instant a threshold crosses).
    static CampTickResult Tick(CampState& camp, PlayerState& player, GeneratedLevel& level, WorldRegistry& world,
                                const ItemDatabase& items, const MonsterDatabase& monsterDb, JavaRandom& rng,
                                int16_t& spawnIdCounter, int64_t now);
};

}  // namespace stormhold
