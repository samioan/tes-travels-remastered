// M33 smoke test: GameRenderer::RenderMinimapZoomedOut/RenderMinimapNormal
// -- paintMinimapZoomedOut()/paintMinimapNormal()'s own actual pixel
// drawing (../../../src/GameCanvas.java lines 1229-1289), fed by M32's
// SampleSquareView output. Closes out the "paintMinimap*() needs pixels"
// gap M32 deliberately deferred.
//
// No JVM ground truth possible (same reasoning every other rendering
// milestone's own tests already give) -- verified via:
//  - The compass glyph (BitmapFont, at each method's own fixed position)
//    and black backdrop, for both zoom levels.
//  - DrawMinimapGrid's own per-byte-value color selection (1/0/bit2/
//    bit4/bit8), including the "both bit2 and bit4 set draws red, not
//    blue" if/else-if precedence GameCanvas.drawMinimapGrid() itself
//    checks in that exact order.
//  - The dead-center green player marker overriding whatever the
//    underlying cell's own color was.
//  - Both real call sites' own fixed geometry (origin/cellPx/
//    pixelOffset), cross-checked against the Java's own literal
//    arguments directly, not read back from game_renderer.cpp.
#include <cstdio>

#include "graphics/bitmap_font.h"
#include "render/game_renderer.h"

namespace {

using namespace stormhold;

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

uint16_t PixelAt(const Backbuffer& bb, int x, int y) { return bb.Data()[static_cast<size_t>(y) * Backbuffer::kWidth + x]; }

// True if any pixel in the glyph-sized [x0,x0+kGlyphWidth) x
// [y0,y0+kGlyphHeight) box matches `targetColor` -- used for the compass
// glyph below instead of a hand-picked exact pixel, since BitmapFont now
// (M74) renders through real (anti-aliased, proportional) GDI text
// rather than a fixed 4x7 pixel table with one exact bit pattern per
// character to predict by hand.
bool AnyPixelInGlyphBox(const Backbuffer& bb, int x0, int y0, uint16_t targetColor) {
    for (int y = y0; y < y0 + BitmapFont::kGlyphHeight; y++) {
        for (int x = x0; x < x0 + BitmapFont::kGlyphWidth; x++) {
            if (PixelAt(bb, x, y) == targetColor) return true;
        }
    }
    return false;
}

SquareViewGrid MakeGrid(int size) { return SquareViewGrid(static_cast<size_t>(size), std::vector<uint8_t>(static_cast<size_t>(size), 0)); }

void TestZoomedOutCompassAndBackdrop() {
    std::printf("-- RenderMinimapZoomedOut: compass glyph + black backdrop --\n");
    Backbuffer bb;
    bb.Fill(PackRGB565(9, 9, 9));
    SquareViewGrid grid = MakeGrid(7);
    GameRenderer::RenderMinimapZoomedOut(bb, grid, 2);  // facing 2 = 'E'

    // COMPASS_GLYPHS[2] = 'E', drawn white at (16,10).
    Expect(AnyPixelInGlyphBox(bb, 16, 10, PackRGB565(255, 255, 255)),
           "the compass glyph 'E' should draw at least one white pixel in its own box");

    // Backdrop: fillRect(10,20,23,23), black.
    Expect(PixelAt(bb, 10, 20) == PackRGB565(0, 0, 0), "the zoomed-out backdrop's own top-left pixel should be black");
    Expect(PixelAt(bb, 0, 0) == PackRGB565(9, 9, 9), "well outside the 23x23 backdrop should stay untouched");
}

void TestNormalCompassAndBackdrop() {
    std::printf("-- RenderMinimapNormal: compass glyph + black backdrop --\n");
    Backbuffer bb;
    bb.Fill(PackRGB565(9, 9, 9));
    SquareViewGrid grid = MakeGrid(17);
    GameRenderer::RenderMinimapNormal(bb, grid, 4);  // facing 4 = 'W'

    // COMPASS_GLYPHS[4] = 'W', drawn white at (58,10).
    Expect(AnyPixelInGlyphBox(bb, 58, 10, PackRGB565(255, 255, 255)),
           "the compass glyph 'W' should draw at least one white pixel in its own box");

    // Backdrop: fillRect(15,25,89,89), black.
    Expect(PixelAt(bb, 15, 25) == PackRGB565(0, 0, 0), "the normal backdrop's own top-left pixel should be black");
    Expect(PixelAt(bb, 15 + 88, 25 + 88) == PackRGB565(0, 0, 0), "the normal backdrop's own bottom-right pixel should be black");
    Expect(PixelAt(bb, 15 + 89, 25) == PackRGB565(9, 9, 9), "one pixel past the 89x89 backdrop should stay untouched");
}

void TestGridCellColorsAndPrecedence() {
    std::printf("-- RenderMinimapZoomedOut: per-cell color selection + if/else-if precedence --\n");
    SquareViewGrid grid = MakeGrid(7);
    grid[0][0] = 1;         // wall -- black
    grid[1][0] = 0;         // floor -- white
    grid[2][0] = 2;         // bit2 only -- red
    grid[3][0] = 4;         // bit4 only -- blue
    grid[4][0] = 8;         // bit8 only -- purple (0xCC00FF)
    grid[5][0] = 2 | 4;     // BOTH bit2 and bit4 -- red wins (checked first)

    Backbuffer bb;
    bb.Fill(PackRGB565(9, 9, 9));
    GameRenderer::RenderMinimapZoomedOut(bb, grid, 1);

    // Cells are drawn at origin (10,20) + pixelOffset 1 + col*3 -- row 0.
    int py = 20 + 1 + 0 * 3;
    Expect(PixelAt(bb, 10 + 1 + 0 * 3, py) == PackRGB565(0, 0, 0), "grid[0][0]==1 should draw black (wall)");
    Expect(PixelAt(bb, 10 + 1 + 1 * 3, py) == PackRGB565(255, 255, 255), "grid[1][0]==0 should draw white (floor)");
    Expect(PixelAt(bb, 10 + 1 + 2 * 3, py) == PackRGB565(255, 0, 0), "grid[2][0]==2 should draw red (bit2)");
    Expect(PixelAt(bb, 10 + 1 + 3 * 3, py) == PackRGB565(0, 0, 255), "grid[3][0]==4 should draw blue (bit4)");
    Expect(PixelAt(bb, 10 + 1 + 4 * 3, py) == PackRGB565(0xCC, 0x00, 0xFF), "grid[4][0]==8 should draw purple (bit8, 0xCC00FF)");
    Expect(PixelAt(bb, 10 + 1 + 5 * 3, py) == PackRGB565(255, 0, 0), "grid[5][0]==6 (bit2+bit4) should draw red -- bit2 wins, checked first");
}

void TestDeadCenterPlayerMarker() {
    std::printf("-- RenderMinimapZoomedOut: dead-center green player marker overrides the cell color --\n");
    SquareViewGrid grid = MakeGrid(7);
    grid[3][3] = 1;  // the dead-center cell (7/2 = 3) would otherwise be black (wall)

    Backbuffer bb;
    bb.Fill(0);
    GameRenderer::RenderMinimapZoomedOut(bb, grid, 1);

    int px = 10 + 1 + 3 * 3;
    int py = 20 + 1 + 3 * 3;
    Expect(PixelAt(bb, px, py) == PackRGB565(0, 255, 0),
           "the dead-center cell should draw green regardless of its own underlying wall/floor color");
}

}  // namespace

int main() {
    TestZoomedOutCompassAndBackdrop();
    TestNormalCompassAndBackdrop();
    TestGridCellColorsAndPrecedence();
    TestDeadCenterPlayerMarker();

    if (!g_ok) {
        std::fprintf(stderr, "m33_minimap_renderer_smoke: FAILED\n");
        return 1;
    }

    std::printf("all checks passed\n");
    return 0;
}
