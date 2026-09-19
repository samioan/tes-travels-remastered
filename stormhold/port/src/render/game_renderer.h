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
// M33: paintMinimapZoomedOut()/paintMinimapNormal() (lines 1229-1250) --
// both minimap zoom levels, fed by M32's SampleSquareView output.
//
// paintObjects()/paintMonsters() are ALREADY ported (M28), just not on
// this class -- see render/visible_object_renderer.h's own
// RenderObjects/RenderMonsters. paintMessagePopup() likewise lives on
// its own render/message_popup.h (M30), not here. Only
// paintFlashOverlays()/paintUnknown_b() remain unported pixel-wise,
// both still gated on live state (this class's own still-unmodeled
// tick-loop flags) this port doesn't have wired up yet. Same
// "primitive first, pipeline later" discipline M21/M23/M24 already
// established, just now applied to whole vertical slices instead of a
// single primitive.
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

    // GameCanvas.paintMinimapZoomedOut(): the compass glyph (M32's
    // `compassGlyphs[facing]`, white, at (16,10)), a black 23x23
    // backdrop at (10,20), then the 7x7 `grid` (M32's
    // `SampleSquareView(..., 7, ...)` output) via the shared
    // `DrawMinimapGrid` helper below (origin (10,20), 3px cells, 1px
    // offset). `grid` must be exactly 7x7 -- the caller's own job to
    // supply the right `SampleSquareView` output, same "caller supplies/
    // owns state" pattern this port already uses throughout.
    static void RenderMinimapZoomedOut(Backbuffer& bb, const SquareViewGrid& grid, int facing);

    // GameCanvas.paintMinimapNormal(): same shape as
    // RenderMinimapZoomedOut above, larger scale -- compass glyph at
    // (58,10), a black 89x89 backdrop at (15,25), then the 17x17 `grid`
    // (M32's `SampleSquareView(..., 17, ...)` output) at origin
    // (15,25), 5px cells, 2px offset.
    static void RenderMinimapNormal(Backbuffer& bb, const SquareViewGrid& grid, int facing);

private:
    // GameCanvas.drawMinimapGrid(): shared by both methods above.
    // `gridSize`x`gridSize` cells of `cellPx` pixels, each colored by
    // its own byte value -- 1=black(wall), 0=white(floor), else bit 2=
    // red, bit 4=blue, bit 8=purple (0xCC00FF -- decimal literal 13369599
    // decoded directly, not independently re-derived; GameCanvas.java's
    // own header comment calls this "cyan-ish" from a first, less
    // certain pass, not corrected here since the actual RGB bytes are
    // what matters, not the color-name guess) -- checked in that exact
    // order, matching the original's own if/else-if chain (a byte with
    // both bit 2 and bit 4 set draws red, never blue). The player's own
    // dead-center cell (row==col==gridSize/2) draws green (0x00FF00)
    // OVER whatever the grid's own cell color was, unconditionally.
    static void DrawMinimapGrid(Backbuffer& bb, int originX, int originY, int gridSize, int cellPx, int pixelOffset,
                                 const SquareViewGrid& grid);
};

}  // namespace stormhold
