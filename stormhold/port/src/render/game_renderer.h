#pragma once
#include "assets/character_data.h"
#include "dungeon/dungeon_runtime.h"
#include "graphics/backbuffer.h"
#include "player/player_state.h"
#include "render/corridor_assets.h"
#include "render/corridor_render_plan.h"
#include "render/status_bar_plan.h"

namespace stormhold {

// M25: the first real end-to-end pixel render this port has -- GameCanvas.
// paintWalls()'s corridor floor+wall pass (../../../src/GameCanvas.java
// lines 419-465), M21's CorridorRenderPlan selection logic and M23/M24's
// Backbuffer::Blit() compositors now actually wired together and drawing
// into a real Backbuffer, rather than each being verified in isolation.
//
// M26: paintStatusBars() (lines 969-987) -- the HP/Magicka/Fatigue HUD
// bars, StatusBarPlan's own width computations (above) drawn as solid-
// color fillRects. The cheapest remaining paint method: entirely
// self-contained in PlayerState + CharacterData, no new asset loading at
// all (see StatusBarPlan's own header comment).
//
// Every OTHER paint* method M22 transcribed (paintObjects/paintMonsters/
// paintHud/paintMinimap*/paintMessagePopup/paintFlashOverlays/
// paintUnknown_b) is deliberately still out of scope here -- each needs
// far more live state this port doesn't have yet (Player.visibleObjects,
// a populated WorldRegistry-backed monster/chest/dropped-item cache
// actually feeding a frame, hotbar/dialogue state and hotbarIcons image
// assets, GameCanvas's own still-unmodeled UI flags). Same "primitive
// first, pipeline later" discipline M21/M23/M24 already established,
// just now applied to a second whole vertical slice instead of a single
// primitive.
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

    // GameCanvas.paintStatusBars(), transcribed directly from
    // StatusBarPlan::Plan()'s own output: three 40x7 yellow (0xFFFF00)
    // track rects at y=130/138/146, each with a 38x5 colored fill inset
    // by (1,1) -- red (0xFF0000) HP, green (0x00FF00) Magicka, blue
    // (0x0000FF) Fatigue -- drawn `plan.hpWidth`/`magickaWidth`/
    // `fatigueWidth` pixels wide.
    static void RenderStatusBars(Backbuffer& bb, const StatusBarPlan& plan);
};

}  // namespace stormhold
