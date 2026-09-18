#pragma once
#include <cstdint>
#include <functional>

#include "player/player_state.h"
#include "world/dungeon_generator.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/Player.java's core movement
// pipeline: computeMoveTarget() (facing-relative position math + cross-
// level boundary stitching, including the hub-town (19x19) <-> standard-
// level (35x35) recentering) + isWalkableTileBits() + commitMove() +
// move()'s turn/step/turn-back strafe wrapper.
//
// **Deliberately scoped to just position/facing/fatigue-cost mechanics,
// not the whole of commitMove()** -- that method also has several side
// effects this port can't model yet, since they all need persistent
// per-instance world state this port doesn't have (a real
// ESGame.dungeons[]/ESGame.monsters[]-equivalent game-session object):
// dropped-item pickup (giftPointsFound accumulation + a zone-opening
// trigger), clearing Shop.wardenPresent on any step, and the
// level-37-entry forced-respawn of the type-41 "roaming" monster. All
// three are noted again at their omission point in the .cpp rather than
// silently dropped. `refreshNearbyMonsterFlags()`/`refreshCorridorView()`
// (monster-registry/rendering side effects) are skipped the same way.
class PlayerMovement {
public:
    // Looks up a GeneratedLevel by its 1-based level number. This port has
    // no persistent world-state object yet (M6/M8's own precedent of
    // taking explicit GeneratedLevel arguments rather than inventing one)
    // -- the caller supplies whatever generated/cached levels are needed.
    // Returns a MUTABLE reference: CommitMove marks the target level
    // `visited` on a successful move (see GeneratedLevel::visited's own
    // header comment -- NOT the same concept as GeneratedLevel::populated).
    using LevelLookup = std::function<GeneratedLevel&(int levelNumber)>;

    // Player.computeMoveTarget(dir): pure position/facing math from `p`'s
    // current state. dir: 1=step forward, 2=step backward, 3=turn one way,
    // 4=turn the other way (no position change for a turn).
    //
    // Throws std::runtime_error if a step would cross a map edge with NO
    // neighbor level (a DungeonGeomRow neighbor <= 0). The real Java has
    // NO guard here at all -- it would throw ArrayIndexOutOfBoundsException
    // fetching a negative-indexed ESGame.dungeons[] slot. Believed
    // unreachable in real play (every walkable edge tile should border a
    // real neighbor, by construction of how stairways/corridors are
    // carved toward the level's own confirmed neighbors) but not proven
    // exhaustively across all 37 levels -- surfaced loudly here rather
    // than silently producing a nonsense pending position, same
    // discipline as AssetRoot::OpenFile/RawImage's palette-index guard.
    static void ComputeMoveTarget(PlayerState& p, int dir, const LevelLookup& levels);

    // Player.isWalkableTileBits(tileBits): not a wall (bit 1), not
    // blocked (bit 32, hub-town shop tiles), not monster-occupied (bit 2).
    static bool IsWalkableTileBits(uint8_t tileBits);

    // Player.commitMove(dir) -- see class header comment for what's
    // deliberately not modeled.
    static bool CommitMove(PlayerState& p, int dir, const LevelLookup& levels);

    // Player.move(dir, strafe): dir 1=forward/2=backward for a plain step;
    // when strafe is true, dir 3/4 are strafe-left/right (turn, step, turn
    // back). Unlike dawnstar's own Player.move (which guards the turn-back
    // step with a one-shot `suppressStrafeAdjust` flag -- see dawnstar's
    // own PlayerState comment), Stormhold's version has no such guard at
    // all: confirmed by reading move() directly, the turn-back always runs
    // unconditionally, and its return value OVERWRITES the sideways step's
    // own result (so a strafe move's return value reflects whether the
    // final turn-back succeeded -- which, since turning never fails
    // walkability, is almost always true -- not whether the actual step
    // landed). Preserved exactly; not a transcription gap on this port's
    // side, and dawnstar's own port preserves the identical overwrite
    // shape for its own equivalent method.
    static bool Move(PlayerState& p, int dir, bool strafe, const LevelLookup& levels);
};

}  // namespace stormhold
