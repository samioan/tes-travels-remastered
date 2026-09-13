#pragma once
#include <cstdint>
#include <vector>

#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "dungeon/dungeon_runtime.h"
#include "player/player_state.h"
#include "render/message_popup.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace dawnstar {

// Renamed-source counterpart of GameCanvas's camp system:
// dispatchTickActions()'s own campRequested branch (enterCampState()),
// plus run()'s own per-tick campState 1/2/3 state machine that sits
// right before dispatchTickActions() itself. Kept in its own module
// (the same reason combat/combat_tick.h and interact/interact_tick.h
// are) because it needs dawnstar_player (PlayerCamp::Rest), dawnstar_dungeon
// (DungeonRuntime::TrySpawnMonsterNear), AND dawnstar_render (the
// "Cannot Camp!"/"Rest disturbed!"/"Rest complete!" popups) all at
// once. See docs/PORT_ROADMAP.md's M35 entry.
class CampTick {
public:
    // GameCanvas's own campRequested dispatch branch (dispatchTickActions()'s
    // highest-priority action): if a monster is actively attacking, shows
    // "Cannot/Camp!" instead of entering camp. `monsterAttacking` is a
    // plain bool parameter here rather than something this method reads
    // off PlayerState itself -- since M36, main.cpp's own real call site
    // passes VisibleObjects::AnyMonsterAttacking's own result (itself a
    // persistent, one-tick-lagged local there, same shape as
    // hotbarContext -- see main.cpp's own doc comment on why); M32-M35
    // always passed a hardcoded false instead, back when nothing in this
    // port ever set it.
    //
    // GameCanvas.enterCampState(): campState 1 by default; 3 instead for
    // the rare scripted "disturbed" camp event (a 1-in-10 roll, gated on
    // character level > 3 and specialEncounterResolved not yet set); 2
    // (skip the interruption roll/wait entirely) if safeCampingBuff is
    // active OR the player is in the hub town (level 1) -- checked in
    // that exact order, so a hub-town camp with specialEncounterResolved
    // still false and level > 3 can roll into campState 3 first, only
    // to be immediately overridden back to 2 by the hub-town check right
    // after, matching the original's own sequential if-chain exactly
    // (not an early-exit if/else-if).
    static void TryEnterCamp(PlayerState& player, JavaRandom& globalRng, int64_t nowMs, int64_t& campStartTimeMs,
                             MessagePopupState& messagePopup, bool monsterAttacking);

    // GameCanvas.run()'s own per-tick campState 1/2/3 state machine,
    // the block immediately preceding dispatchTickActions() itself --
    // returns whether the REST of this tick's normal per-tick work
    // (dispatchTickActions/tickVisibleObjects/minimap refresh/message-
    // popup tick, all still gated behind main.cpp's own `if (runTick)`)
    // should run at all this tick. While actually camping (before the
    // relevant timer elapses) this returns false, freezing everything
    // else for the tick -- no attack/cast/cycle/interact/movement,
    // matching the original exactly (the entire dispatchTickActions()
    // call itself is skipped while runTick is false). The one tick a
    // camp cycle actually RESOLVES (interrupted or completed) returns
    // true, but also sets `suppressMoveThisTick` -- GameCanvas.
    // suppressMoveInput's own real effect for that same tick, reproduced
    // here as an out-parameter for main.cpp's own movement branch to
    // check, since this port has no pendingMoveDir queue for the
    // original's own `(pendingMoveDir != 0 || suppressMoveInput) &&
    // !suppressMoveInput` movement-commit condition to fold into --
    // simplifies to "commit only if a move is pending AND not
    // suppressed", which is exactly what this out-parameter lets
    // main.cpp check directly.
    //
    // `nextMonsterSpawnId` substitutes for Monster.nextSpawnIdCounter's
    // own global counter -- a DIFFERENT counter than Item.nextSpawnId()
    // (main.cpp's own `nextDropSpawnId`, M32): Monster.java's spawn()
    // and onDeath() genuinely read two distinct static counters, traced
    // directly from ../../../src/Monster.java (lines ~441 and ~461).
    // Starting it fresh at 1 here (rather than continuing from whatever
    // world generation's own per-level-local chest/monster spawnId
    // counters last used) is safe for the same reason M6's own
    // dungeon_generator.h doc comment already established: this port's
    // WorldRegistry keys monsters by POSITION, never by spawnId, so
    // spawnId collisions across a fresh live-spawn counter and
    // generation's own per-level counters are pure inert bookkeeping,
    // not a correctness issue.
    static bool TickCampState(PlayerState& player, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                              const ItemDatabase& items, const MonsterDatabase& monsterDb, JavaRandom& globalRng,
                              int64_t nowMs, int64_t& campStartTimeMs, int16_t& nextMonsterSpawnId,
                              MessagePopupState& messagePopup, bool& suppressMoveThisTick);
};

}  // namespace dawnstar
