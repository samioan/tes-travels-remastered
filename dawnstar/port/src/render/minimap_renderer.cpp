#include "render/minimap_renderer.h"

#include "graphics/bitmap_font.h"
#include "player/player_combat_stats.h"

namespace dawnstar {

void MinimapSurface::SetPixel(int x, int y, uint16_t rgb565) {
    if (x < 0 || x >= kSize || y < 0 || y >= kSize) return;
    pixels_[static_cast<size_t>(y) * kSize + x] = rgb565;
}

void MinimapSurface::FillRect(int x, int y, int w, int h, uint16_t rgb565) {
    int x0 = std::max(x, 0);
    int y0 = std::max(y, 0);
    int x1 = std::min(x + w, kSize);
    int y1 = std::min(y + h, kSize);
    for (int yy = y0; yy < y1; yy++) {
        for (int xx = x0; xx < x1; xx++) {
            pixels_[static_cast<size_t>(yy) * kSize + xx] = rgb565;
        }
    }
}

void MinimapSurface::DrawRect(int x, int y, int w, int h, uint16_t rgb565) {
    if (w <= 0 || h <= 0) return;
    for (int xx = x; xx <= x + w; xx++) {
        SetPixel(xx, y, rgb565);
        SetPixel(xx, y + h, rgb565);
    }
    for (int yy = y; yy <= y + h; yy++) {
        SetPixel(x, yy, rgb565);
        SetPixel(x + w, yy, rgb565);
    }
}

namespace {

constexpr uint16_t kBlack = PackRGB565(0, 0, 0);
constexpr uint16_t kWhite = PackRGB565(255, 255, 255);
constexpr uint16_t kGreen = PackRGB565(0, 255, 0);
constexpr uint16_t kRed = PackRGB565(255, 0, 0);
constexpr uint16_t kBlue = PackRGB565(0, 0, 255);
constexpr uint16_t kMagenta = PackRGB565(204, 0, 255);

// GameCanvas.COMPASS_GLYPHS, index 0 unused (never reached -- Player's
// own facing is always 1-4).
constexpr char kCompassGlyphs[5] = {'0', 'N', 'E', 'S', 'W'};

// Java's int `<<` operator masks its right-hand operand to the low 5
// bits (JLS 15.19) rather than rejecting or sign-extending an
// out-of-range shift count -- so `<<-1`, which appears literally in
// GameCanvas.paintMinimapGrid's border==2-only inner drawRect call, is
// `<< 31`, not a right-shift or a no-op. Reproduced with an unsigned
// intermediate since C++ gives undefined behavior for a negative/
// out-of-range shift count where Java gives a well-defined (if bizarre)
// wraparound.
int32_t JavaShiftLeft(int32_t value, int32_t amount) {
    return static_cast<int32_t>(static_cast<uint32_t>(value) << (amount & 31));
}

// GameCanvas.paintMinimapGrid(): fills the grid's background/border and
// then its gridSize x gridSize colored squares from `grid` (M29's
// SampleSquareView output).
//
// TWO real, faithfully-preserved bugs, both a consequence of the same
// Java operator-precedence trap (`<<` binds looser than `+`, so
// `a + b << c` parses as `(a + b) << c`, not `a + (b << c)`):
//  1. The background fillRect's size is computed as
//     `gridSize*cellSize+border << 1` -- intended (going by the
//     "border" name and the drawRect right below it) to add `border` on
//     BOTH sides, i.e. `gridSize*cellSize + 2*border`, but actually
//     evaluates to `(gridSize*cellSize+border) << 1`, exactly DOUBLE
//     the intended size (44 instead of 23 when zoomed in; 174 instead
//     of 89 when zoomed out). MinimapSurface::FillRect clips to the
//     89x89 surface same as a real MIDP Image's Graphics would, which
//     makes this bug's real effect depend entirely on which zoom level
//     is active: zoomed IN, 44 is still well under 89, so the fill
//     really does reach a visibly larger area than the "intended" 23
//     would have (proven by this file's own smoke test, which
//     pre-dirties the surface and confirms (35,35) -- inside the buggy
//     reach, outside the intended one -- gets erased while (60,60)
//     doesn't). Zoomed OUT, though, this bug is completely
//     UNOBSERVABLE: both the buggy value (174) and the "intended"
//     correct one (17*5+2*2=89) meet or exceed the 89px surface, so
//     both converge to "fill the entire surface" regardless -- a case
//     where a real bug happens to produce a result indistinguishable
//     from having fixed it.
//  2. The very next drawRect's size uses `<< 0` (a no-op shift) instead
//     of doubling `border` either -- so the white outer border is drawn
//     exactly `border` pixels short of the correct
//     `gridSize*cellSize + 2*border`, i.e. it's missing its right/
//     bottom edge's second border-width. Ported as the exact (off-by-
//     `border`) rectangle the original draws, not the "obviously
//     intended" correctly-bordered one.
// A third real oddity (border==2 only, i.e. only in zoomed-out mode):
// the inner drawRect's own size is computed with a literal `<<-1` in
// the source -- see JavaShiftLeft's own doc comment above. For the one
// real value this ever executes with (base=87, see below), that comes
// out to Integer.MIN_VALUE, which MinimapSurface::DrawRect's own
// non-positive-dimension guard turns into a no-op. Genuinely
// undefined/platform-dependent MIDP behavior for a rectangle this
// deranged in the first place (no real device's Graphics.drawRect
// contract covers it) -- a no-op is a reasonable stand-in for "produces
// nothing anyone would call a real rendered rectangle" rather than an
// attempt to reproduce one specific vendor's undefined behavior.
void PaintMinimapGrid(MinimapSurface& surface, int border, int origin, int gridSize, int cellSize,
                      const std::array<std::array<uint8_t, 17>, 17>& grid) {
    int base = gridSize * cellSize + border;
    surface.FillRect(0, 0, base << 1, base << 1, kBlack);
    surface.DrawRect(0, 0, base, base, kWhite);
    if (border == 2) {
        int huge = JavaShiftLeft(base, -1);
        surface.DrawRect(1, 1, huge, huge, kWhite);
    }

    int center = gridSize / 2;
    for (int row = 0; row < gridSize; row++) {
        int y = origin + row * cellSize;
        for (int col = 0; col < gridSize; col++) {
            int x = border + col * cellSize;
            if (row == center && col == center) {
                surface.FillRect(x, y, cellSize, cellSize, kGreen);
                continue;
            }

            uint8_t tile = grid[static_cast<size_t>(col)][static_cast<size_t>(row)];
            if (tile == 1) continue;  // wall -- draws nothing, leaves the black background
            if (tile == 0) {
                surface.FillRect(x, y, cellSize, cellSize, kWhite);
            } else if ((tile & 2) != 0) {
                surface.FillRect(x, y, cellSize, cellSize, kRed);
            } else if ((tile & 4) != 0) {
                surface.FillRect(x, y, cellSize, cellSize, kBlue);
            } else if ((tile & 8) != 0) {
                surface.FillRect(x, y, cellSize, cellSize, kMagenta);
            }
        }
    }
}

}  // namespace

void MinimapRenderer::Refresh(MinimapSurface& surface, PlayerState& p, const std::vector<GeneratedLevel>& levels,
                               const WorldRegistry& world) {
    p.minimapDirty = false;

    std::array<std::array<uint8_t, 17>, 17> grid{};
    int levelIndex = p.currentLevel - 1;
    if (!p.minimapZoomedOut) {
        DungeonRuntime::SampleSquareView(levels, levelIndex, p.tileX, p.tileY, p.facing, 7, world, grid);
        PaintMinimapGrid(surface, 1, 1, 7, 3, grid);
    } else {
        DungeonRuntime::SampleSquareView(levels, levelIndex, p.tileX, p.tileY, p.facing, 17, world, grid);
        PaintMinimapGrid(surface, 2, 2, 17, 5, grid);
    }
}

void MinimapRenderer::Composite(Backbuffer& bb, const MinimapSurface& surface, const PlayerState& p) {
    if (PlayerCombatStats::HasAilment(p, 3)) return;

    int facing = p.facing;
    if (facing >= 1 && facing <= 4) {
        std::string glyph(1, kCompassGlyphs[facing]);
        if (!p.minimapZoomedOut) {
            BitmapFont::DrawString(bb, 16, 10, glyph, kWhite, BitmapFont::Face::SmallPlain);
        } else {
            BitmapFont::DrawString(bb, 58, 10, glyph, kWhite, BitmapFont::Face::LargeBold);
        }
    }

    const std::vector<uint16_t>& pixels = surface.Pixels();
    if (!p.minimapZoomedOut) {
        // g.setClip(10, 20, 23, 23) before drawImage(minimapImage, 10, 20, 20)
        // (anchor 20 = TOP|LEFT -- (10,20) is the image's own top-left
        // corner on screen): only the surface's own top-left 23x23
        // corner is ever visible.
        for (int sy = 0; sy < 23 && sy < MinimapSurface::kSize; sy++) {
            for (int sx = 0; sx < 23 && sx < MinimapSurface::kSize; sx++) {
                bb.SetPixel(10 + sx, 20 + sy, pixels[static_cast<size_t>(sy) * MinimapSurface::kSize + sx]);
            }
        }
    } else {
        // drawImage(minimapImage, 15, 25, 20): no clip set for this
        // branch -- the full 89x89 surface is drawn.
        for (int sy = 0; sy < MinimapSurface::kSize; sy++) {
            for (int sx = 0; sx < MinimapSurface::kSize; sx++) {
                bb.SetPixel(15 + sx, 25 + sy, pixels[static_cast<size_t>(sy) * MinimapSurface::kSize + sx]);
            }
        }
    }
}

}  // namespace dawnstar
