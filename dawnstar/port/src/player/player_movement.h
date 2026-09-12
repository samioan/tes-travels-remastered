#pragma once
#include <cstdint>
#include <vector>

#include "player/player_state.h"
#include "world/dungeon_generator.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Player.java's movement and
// hub/camp positioning: computeMoveTarget()/commitMove()/move()/
// isWalkable()/hasCampMark()/resetToHubPosition()/
// markCampAndReturnToTown()/warpToCampMark() (the latter four added in
// M18 for combat/combat_resolution.h's UseItem -- see
// docs/PORT_ROADMAP.md). `levels` mirrors ESGame.dungeons[] -- one
// GeneratedLevel per dungeon level, indexed by levelNumber-1 (see
// world/dungeon_generator.h).
//
// SIMPLIFIED versus the original (each is a real behavioral gap, not
// just an implementation detail -- see docs/PORT_ROADMAP.md's M13 entry):
//  - No dropped-item auto-loot on arrival (commitMove's dropped-items
//    block) -- there's no live dropped-item registry yet, only
//    GeneratedLevel's one-time generation-time chest/monster lists.
//  - No instant-lethal-tile (tile bit 8) camp-mark-and-return-to-town
//    trigger -- no camp/town-return system ported yet. Position/facing
//    still commit normally; only that side effect is skipped.
//  - No "remove roaming gehen on level change" cleanup in
//    ComputeMoveTarget -- no live per-level monster-instance registry to
//    search/remove from yet. roamingSpecialMonsterPresent is left
//    untouched by movement.
//  - Every level in `levels` is treated as always populated
//    (Dungeon.java's lazy per-level population flag has no equivalent
//    here -- this port always generates every level it holds upfront).
//  - computeMoveTarget's own out-of-bounds neighbor-level lookup (used
//    to recenter the coordinate perpendicular to the crossed edge) is
//    NOT guarded in the original against a <=0 neighbor -- it relies on
//    the implicit level-design invariant that no walkable tile ever
//    borders a "no neighbor" edge, and would throw
//    ArrayIndexOutOfBoundsException in Java if that invariant were ever
//    violated. This port guards it anyway (skipping the recentering it
//    would have computed) since C++ has no equivalent safety net for an
//    out-of-bounds vector index; CommitMove's own `level <= 0` check
//    still rejects the move either way, exactly as the original does.
class PlayerMovement {
public:
    // direction: 1=forward, 2=backward, 3=turn right, 4=turn left.
    // `strafe`+3/4 sidesteps (turn, step, turn back) instead of turning
    // in place. Returns whether anything actually committed (a
    // successful step OR turn).
    static bool Move(PlayerState& p, int direction, bool strafe, std::vector<GeneratedLevel>& levels);

    // Wall(bit0)/blocked(bit5)/monster(bit1) test for a move target tile.
    static bool IsWalkable(uint8_t tileBits);

    // Player.java's hasCampMark(): whether markCampAndReturnToTown has
    // ever bookmarked a camp point.
    static bool HasCampMark(const PlayerState& p) { return p.campLevel > 0; }

    // Player.java's resetToHubPosition(altSpawn): repositions to one of
    // two fixed level-1 entry points (the normal spawn, or the
    // "returning from camp" alt-spawn just inside the hub's door) and
    // refreshes the corridor view. SIMPLIFIED: skips the roaming-
    // special-monster cleanup (no live per-level monster registry, same
    // gap as Move()'s class comment) and the chest/NPC-visibility
    // refresh calls (rendering, not ported).
    static void ResetToHubPosition(PlayerState& p, bool altSpawn, const std::vector<GeneratedLevel>& levels);

    // Player.java's markCampAndReturnToTown(skipMark): bookmarks the
    // current position (unless skipMark, a path never actually
    // exercised in the original either -- always called with false) and
    // returns to the hub via ResetToHubPosition(true).
    static void MarkCampAndReturnToTown(PlayerState& p, bool skipMark, const std::vector<GeneratedLevel>& levels);

    // Player.java's warpToCampMark(): warps to the bookmarked camp
    // point. SIMPLIFIED: skips the chest/NPC-visibility refresh calls,
    // same as ResetToHubPosition.
    static void WarpToCampMark(PlayerState& p, const std::vector<GeneratedLevel>& levels);

private:
    struct PendingMove {
        int level = 0;
        int tileX = 0;
        int tileY = 0;
        int facing = 0;
        bool levelChanged = false;
    };

    static PendingMove ComputeMoveTarget(const PlayerState& p, int direction,
                                          const std::vector<GeneratedLevel>& levels);
    // `outLevelChanged` mirrors Player.java's `this.levelChanged` field
    // -- move()'s strafe handling reads and restores it around the
    // turn-back step, so it has to survive across sequential
    // CommitMove calls within one Move() rather than being purely local.
    static bool CommitMove(PlayerState& p, int direction, std::vector<GeneratedLevel>& levels, bool& outLevelChanged);
    static void RefreshCorridorView(PlayerState& p, const std::vector<GeneratedLevel>& levels);
};

}  // namespace dawnstar
