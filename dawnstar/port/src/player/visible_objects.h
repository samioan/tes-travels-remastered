#pragma once
#include <vector>

#include "dungeon/dungeon_runtime.h"
#include "player/player_state.h"
#include "world/dungeon_generator.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Player.java's
// tickVisibleObjects()/refreshVisibleObjects()/placeVisibleObject()/
// markLooted() -- the 13-slot "what's renderable at each 3D-view object
// slot this frame" cache GameCanvas.paintVisibleObjects() paints from.
// Deliberately the DATA-MODEL half only: this milestone produces
// PlayerState::visibleObjects, correctly populated every tick, with no
// pixel drawn anywhere -- the same split M9 (corridor wall-segment
// *selection*) made against M10 (actually drawing them). A later
// milestone's FrameRenderer addition is the pixel-drawing half (the
// icon/position tables in GameCanvas.paintVisibleObjects/
// paintObjectAtPosition/drawMonsterMid/Far/drawFarLootIcon/etc., plus
// their sprite sheets).
//
// A REAL, surprising finding, confirmed by tracing every read site of
// Monster.flag (`rec[6]` in the packed record) across ../../../src/:
// the only place that ever sets it true is this class's own MarkLooted
// (called immediately after ANY monster is newly placed into a visible
// slot, every single tick it stays in view) -- nothing ever sets it
// back to false. So despite the name ("flag"/Dungeon.java's own
// "unconfirmed exact meaning" note) and this port's earlier
// MonsterState::flag doc comment ("collected/looted marker"), it does
// NOT track combat/loot state at all -- it tracks "has the player ever
// seen this monster", and because Java's `rec` there is literally the
// same array reference sitting in the slot AND the registry
// (mutate-in-place aliasing), the very same tick that notices a monster
// also flags-and-renders it, forever after. GameCanvas's rendering gate
// (`rec[6] != 0`) is therefore true for essentially every monster that
// has ever been on screen, not just "attacking" ones -- ported exactly
// (MarkLooted below), not reinterpreted.
class VisibleObjects {
public:
    // Player.tickVisibleObjects(unused): rebuilds `p.visibleObjects`
    // from scratch for the current tick -- wall/occlusion slots first
    // (Refresh, below), then every monster/chest/dropped-item registered
    // on the current level plus its NPC(s), each via the same
    // facing-relative placement math (Place) placeVisibleObject uses.
    static void Tick(PlayerState& p, const std::vector<GeneratedLevel>& levels, WorldRegistry& world);

private:
    // Player.refreshVisibleObjects(): samples p.corridorView at the 13
    // slots' fixed forward/diagonal offsets, marking wall-blocked slots
    // directly and then propagating occlusion to whichever slots sit
    // behind them -- exact cascade order preserved (later checks can see
    // occlusion an earlier check just propagated, matching Java's
    // sequential re-reads of the same live array).
    static void Refresh(const PlayerState& p, std::array<VisibleSlot, 13>& slots);

    // Player.placeVisibleObject's facing-relative col/row math plus its
    // fixed col/row -> slot lookup, WITHOUT the assignment itself (the
    // original does both in one call) -- callers still need the target
    // slot's index afterward, for MarkLooted. Returns -1 if (objX,objY)
    // doesn't land on any of the 9 placeable slots, or if that slot is
    // already occupied.
    static int FindPlacementSlot(const PlayerState& p, const std::array<VisibleSlot, 13>& slots, int objX, int objY);

    // Player.markLooted(kind, obj): for a just-placed monster, sets its
    // "seen" flag (see the class comment above) both on the slot's own
    // copy and back into the live registry (Monster.store()'s
    // counterpart); for a dropped item, sets the same bit on the
    // slot's own copy only. SIMPLIFIED for dropped items: Java's `rec`
    // is the exact same array reference the registry's Vector holds, so
    // this mutation incidentally reaches the registry there too -- this
    // port's registry holds independent copies, so it doesn't, but
    // nothing anywhere else ever reads this particular bit (confirmed by
    // grep), so the divergence has no observable effect.
    static void MarkLooted(WorldRegistry& world, int levelIndex, VisibleSlot& slot);
};

}  // namespace dawnstar
