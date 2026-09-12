#pragma once
#include <cstdint>
#include <vector>

#include "assets/item_database.h"
#include "dungeon/dungeon_runtime.h"
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
// world/dungeon_generator.h). `world` (M22's dungeon/dungeon_runtime.h)
// is the live per-level monster/dropped-item registry Move()/
// CommitMove() now actually read and write -- this is the third module
// (after combat/ and dungeon/ itself) that needs two of the existing
// sibling modules at once, here player + dungeon (which itself already
// depends on world + monster); no cycle results, since neither world/
// nor monster/ depends back on player/.
//
// SIMPLIFIED versus the original (each is a real behavioral gap, not
// just an implementation detail -- see docs/PORT_ROADMAP.md's M13/M23
// entries):
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
//  - MarkCampAndReturnToTown/ResetToHubPosition/WarpToCampMark (M18)
//    still skip the chest/NPC-visibility refresh calls (rendering, not
//    ported) -- unrelated to M23's registry wiring below.
//
// M23 closed the three gaps M13/M18 had left open pending a live
// registry: CommitMove's dropped-item auto-loot-on-arrival now really
// runs (PlayerInventory::AddItem into the first free slot, removing the
// looted record from `world` and clearing the tile's presence bit once
// every record there is gone); CommitMove's instant-lethal-tile (bit 8)
// case now really calls MarkCampAndReturnToTown(false) instead of only
// committing position/facing; and ComputeMoveTarget's "remove roaming
// gehen on level change" cleanup now really searches the LEAVING
// level's live monster registry for type 41 and removes it via
// DungeonRuntime::RemoveMonster, matching Player.java's own use of
// `this.currentLevel` (the old level, not the new one) there.
class PlayerMovement {
public:
    // direction: 1=forward, 2=backward, 3=turn right, 4=turn left.
    // `strafe`+3/4 sidesteps (turn, step, turn back) instead of turning
    // in place. Returns whether anything actually committed (a
    // successful step OR turn). `world`/`items` are M23's addition, for
    // CommitMove's now-real dropped-item auto-loot and
    // ComputeMoveTarget's now-real roaming-monster cleanup below.
    static bool Move(PlayerState& p, int direction, bool strafe, std::vector<GeneratedLevel>& levels,
                      WorldRegistry& world, const ItemDatabase& items);

    // Wall(bit0)/blocked(bit5)/monster(bit1) test for a move target tile.
    static bool IsWalkable(uint8_t tileBits);

    // Player.java's hasCampMark(): whether markCampAndReturnToTown has
    // ever bookmarked a camp point.
    static bool HasCampMark(const PlayerState& p) { return p.campLevel > 0; }

    // Player.java's resetToHubPosition(altSpawn): repositions to one of
    // two fixed level-1 entry points (the normal spawn, or the
    // "returning from camp" alt-spawn just inside the hub's door),
    // cleans up the type-41 "roaming" special monster on the level
    // being left if one is still tracked as alive (M23 -- shares
    // CleanupRoamingMonsterIfPresent with ComputeMoveTarget below), and
    // refreshes the corridor view. SIMPLIFIED: skips the chest/NPC-
    // visibility refresh calls (rendering, not ported).
    static void ResetToHubPosition(PlayerState& p, bool altSpawn, std::vector<GeneratedLevel>& levels,
                                    WorldRegistry& world);

    // Player.java's markCampAndReturnToTown(skipMark): bookmarks the
    // current position (unless skipMark, a path never actually
    // exercised in the original either -- always called with false) and
    // returns to the hub via ResetToHubPosition(true).
    static void MarkCampAndReturnToTown(PlayerState& p, bool skipMark, std::vector<GeneratedLevel>& levels,
                                         WorldRegistry& world);

    // Player.java's warpToCampMark(): warps to the bookmarked camp
    // point. No roaming-monster cleanup in the original here (only
    // ResetToHubPosition/ComputeMoveTarget have it). SIMPLIFIED: skips
    // the chest/NPC-visibility refresh calls, same as ResetToHubPosition.
    static void WarpToCampMark(PlayerState& p, const std::vector<GeneratedLevel>& levels);

private:
    struct PendingMove {
        int level = 0;
        int tileX = 0;
        int tileY = 0;
        int facing = 0;
        bool levelChanged = false;
    };

    // Shared by ComputeMoveTarget and ResetToHubPosition (both of
    // Player.java's own real call sites for this exact block): if
    // p.roamingSpecialMonsterPresent, searches `world`'s registry for
    // the LEAVING level (p.currentLevel, read before either caller
    // updates it) for a type-41 monster and removes it via
    // DungeonRuntime::RemoveMonster, clearing the flag. The original's
    // "Remove roaming gehen failed" console message on a flag left set
    // isn't ported (no stdout channel any other module uses for this).
    static void CleanupRoamingMonsterIfPresent(PlayerState& p, std::vector<GeneratedLevel>& levels,
                                                WorldRegistry& world);

    static PendingMove ComputeMoveTarget(PlayerState& p, int direction, std::vector<GeneratedLevel>& levels,
                                          WorldRegistry& world);
    // `outLevelChanged` mirrors Player.java's `this.levelChanged` field
    // -- move()'s strafe handling reads and restores it around the
    // turn-back step, so it has to survive across sequential
    // CommitMove calls within one Move() rather than being purely local.
    static bool CommitMove(PlayerState& p, int direction, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                            const ItemDatabase& items, bool& outLevelChanged);
    static void RefreshCorridorView(PlayerState& p, const std::vector<GeneratedLevel>& levels);
};

}  // namespace dawnstar
