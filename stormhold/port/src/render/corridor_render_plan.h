#pragma once
#include <cstdint>
#include <vector>

#include "dungeon/dungeon_runtime.h"

namespace stormhold {

// One 18px-wide wall column to draw, in on-screen left-to-right draw
// order. `frame` is GameCanvas.resolveWallFrame()'s own output: 0-7 draws
// the shared wallTexture spritesheet directly at that frame offset; 8-15
// draws the SAME spritesheet horizontally mirrored (see GameCanvas.
// drawWallSegment()'s own header comment) at frame-8. `x` is the screen
// column offset (a multiple of 18, 0..162).
struct WallDrawCall {
    int frame = 0;
    int x = 0;
};

struct CorridorRenderPlanResult {
    // GameCanvas.paintWalls()'s own ailment-gated floor branch: neither
    // flag set means "ailment 3 active, skip the floor entirely" (a
    // faithful port has nothing else to draw in that case either).
    bool drawFloorTiles = false;         // tile floorTexture 5x, 36px apart
    bool drawFloorFallbackFill = false;  // solid-color fillRect instead (ailment 4)
    // Only steps that actually found an occluding wall get an entry here
    // -- an open (no-wall-found) step draws nothing, matching the
    // original's own "no match in the 6-row scan -> draw nothing" result
    // exactly. In on-screen draw order (x ascending, 0..162).
    std::vector<WallDrawCall> wallSegments;
};

// Renamed-source counterpart of ../../../src/GameCanvas.java's
// paintWalls() (was decompiled/e.java's j(Graphics)) -- the corridor
// wall-segment SELECTION logic only, mirroring dawnstar's own equivalent
// port milestone's scope exactly ("deliberately stops short of drawing
// actual pixels" -- see docs/PORT_ROADMAP.md's M21 entry): this returns
// the same "draw this frame at this x" decisions the original would make,
// as data, for a later milestone's real pixel renderer (once this port
// has a Backbuffer::Blit()-equivalent) to consume directly.
//
// Confirmed, by reading paintWalls()/drawWallSegment() in full, to be
// architecturally SIMPLER than dawnstar's own paintCorridorWalls(): only
// ONE wall bit is ever tested (bit 1, plain wall -- no dawnstar-style
// bit-64 "gate/edge" branch exists here at all), and there is no
// per-dungeon-number ice/plain texture switch either (a single shared
// wallTexture/floorTexture pair, not dawnstar's 5 separate Image fields)
// -- both preserved exactly, not simplifications this port introduced.
//
// **Deliberately does NOT take a `PlayerState` or read `corridorView`
// from one** -- `PlayerState` has no `corridorView` field yet (the real
// `Player.refreshCorridorView()`/`Player.corridorView` aren't wired into
// `player/player_movement.h`'s `CommitMove` in this port), so the caller
// supplies an already-sampled `CorridorViewGrid` directly (`dungeon/
// dungeon_runtime.h`'s `DungeonRuntime::SampleCorridorView`, also new
// this milestone), same "caller supplies/owns world state" pattern used
// throughout this port. Wiring `SampleCorridorView` into `CommitMove`
// itself is left for whichever later milestone actually needs a live,
// always-current `PlayerState::corridorView` (e.g. the real pixel
// renderer, or `refreshVisibleObjectSlots()`'s own port) -- not invented
// early here.
class CorridorRenderPlan {
public:
    static CorridorRenderPlanResult Plan(const CorridorViewGrid& view, bool ailment3Active, bool ailment4Active);
};

}  // namespace stormhold
