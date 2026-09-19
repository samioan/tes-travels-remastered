#pragma once
#include "dungeon/dungeon_runtime.h"
#include "graphics/backbuffer.h"
#include "render/corridor_assets.h"
#include "render/corridor_render_plan.h"

namespace stormhold {

// M25: the first real end-to-end pixel render this port has -- GameCanvas.
// paintWalls()'s corridor floor+wall pass (../../../src/GameCanvas.java
// lines 419-465), M21's CorridorRenderPlan selection logic and M23/M24's
// Backbuffer::Blit() compositors now actually wired together and drawing
// into a real Backbuffer, rather than each being verified in isolation.
//
// Every OTHER paint* method M22 transcribed (paintObjects/paintMonsters/
// paintStatusBars/paintHud/paintMinimap*/paintMessagePopup/
// paintFlashOverlays/paintUnknown_b) is deliberately still out of scope
// here -- each needs far more live state this port doesn't have yet
// (Player.visibleObjects, a populated WorldRegistry-backed monster/
// chest/dropped-item cache actually feeding a frame, hotbar/dialogue
// state, HP/Magicka/Fatigue bars). Same "primitive first, pipeline
// later" discipline M21/M23/M24 already established, just now applied
// to one whole vertical slice (selection logic + compositor + real
// pixels) instead of a single primitive.
class GameRenderer {
public:
    // GameCanvas.paintWalls(), transcribed directly from
    // CorridorRenderPlan::Plan()'s own output (the scan/selection logic
    // itself lives there, M21 -- this method only draws what it decided):
    //  - ailment-4 branch: a flat dark-red fill (`g.fillRect(0, 0,
    //    getWidth(), floorTexture.getHeight())`, color literal 10485760 =
    //    0xA00000 = RGB(160, 0, 0)) instead of the floor texture.
    //  - ailment-3 branch: neither this fill nor the floor tiles draw at
    //    all (see CorridorRenderPlanResult's own header comment).
    //  - otherwise: floorTexture tiled 5x, 36px apart, at (col*36, 0).
    //  - each WallDrawCall: wallTexture drawn at `x - frame*18` (frames
    //    0-7) or horizontally mirrored at `x - (frame-8)*18` (frames
    //    8-15), both shifted-then-clipped to `[x, x+18)` -- matching
    //    Blit()'s own frame-slicing idiom exactly (see backbuffer.h's own
    //    header comment on why `Blit()`'s `x` argument is the SHIFTED
    //    position, not the on-screen column).
    static void RenderCorridorView(Backbuffer& bb, const CorridorAssets& assets, const CorridorViewGrid& view,
                                    bool ailment3Active, bool ailment4Active);
};

}  // namespace stormhold
