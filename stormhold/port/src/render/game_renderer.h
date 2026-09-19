#pragma once
#include "assets/character_data.h"
#include "dungeon/dungeon_runtime.h"
#include "graphics/backbuffer.h"
#include "player/player_state.h"
#include "render/corridor_assets.h"
#include "render/corridor_render_plan.h"
#include "render/hotbar_assets.h"
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
// M31: paintHud() (lines 1024-1062) -- the bottom hotbar panel, gated on
// M29's ResolveHudIconSet and drawn with M30/M31's own new BitmapFont
// digit glyphs plus a new HotbarAssets icon bundle (render/
// hotbar_assets.h).
//
// Every OTHER paint* method M22 transcribed (paintObjects/paintMonsters/
// paintMinimap*/paintMessagePopup [rendered separately, render/
// message_popup.h, M30]/paintFlashOverlays/paintUnknown_b) is
// deliberately still out of scope here -- each needs far more live
// state this port doesn't have yet (Player.visibleObjects actually fed
// by a live tick loop, GameCanvas's own still-unmodeled UI flags, the
// still-untranscribed minimap-populate methods). Same "primitive first,
// pipeline later" discipline M21/M23/M24 already established, just now
// applied to a second whole vertical slice instead of a single
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

    // GameCanvas.paintHud(), transcribed directly: a black `fillRect(0,
    // 156, 176, 52)` background, then a `fillRoundRect(2, 158, 172, 48,
    // 5, 5)` panel (color 13080935 = 0xC79967, the same popup-background
    // color render/message_popup.cpp's own `kPopupBg` uses), then --
    // gated on `iconSet` (M29's `ResolveHudIconSet`, a plain parameter
    // here rather than recomputed inline, same "caller supplies/owns
    // state" pattern this port already uses throughout) -- 4 icons at
    // fixed x positions (14/62/104/144, y=174) plus 4 black digit glyphs
    // just above them (x=5/53/96/135, y=180), whichever 4 of
    // `hotbarIcons[0..5]`/`hotbarKeyGlyphs[0..5]` that iconSet selects
    // (../../../src/GameCanvas.java lines 1024-1062 -- the 3 branches'
    // exact index sets, not independently re-derived). An out-of-range
    // iconSet (unreachable -- ResolveHudIconSet only ever returns 0/1/2)
    // draws just the two background fills, matching the original's own
    // if/else-if chain with no final else. `hotbarActionSet` (also
    // written here in the original, read back by keyPressed()'s numeric-
    // hotkey dispatch) is NOT modeled -- input handling remains out of
    // scope, same gap M29's own header comment already flagged.
    static void RenderHud(Backbuffer& bb, const HotbarAssets& assets, int iconSet);
};

}  // namespace stormhold
