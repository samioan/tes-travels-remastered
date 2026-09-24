#include "render/game_renderer.h"

#include <string>

#include "graphics/bitmap_font.h"

namespace stormhold {

namespace {

// GameCanvas.hotbarKeyGlyphs = {'1','3','5','7','9','0'} -- confirmed
// against ../../../src/GameCanvas.java's own field declaration.
constexpr char kHotbarKeyGlyphs[6] = {'1', '3', '5', '7', '9', '0'};

// One row of paintHud()'s own 4 fixed (iconIdx, x) pairs -- the 3
// branches' exact index sets from GameCanvas.paintHud() (lines
// 1034-1061), y is always 174 for icons / 180 for glyphs.
struct HotbarRow {
    int iconIdx[4];
};

constexpr HotbarRow kHotbarRows[3] = {
    {{1, 2, 3, 5}},  // iconSet == 0
    {{0, 1, 2, 3}},  // iconSet == 1
    {{1, 2, 3, 4}},  // iconSet == 2
};

constexpr int kHotbarX[4] = {14, 62, 104, 144};
constexpr int kHotbarGlyphX[4] = {5, 53, 96, 135};

// GameCanvas.compassGlyphs = {'0','N','E','S','W'} -- confirmed against
// ../../../src/GameCanvas.java's own field declaration, indexed
// directly by `facing` (index 0 = '0' is dead in practice, since
// Player.facing is always 1-4 -- same assumption
// DungeonRuntime::RelativeViewOffset already makes for the same field).
constexpr char kCompassGlyphs[5] = {'0', 'N', 'E', 'S', 'W'};

}  // namespace

void GameRenderer::RenderCorridorView(Backbuffer& bb, const CorridorAssets& assets, const CorridorViewGrid& view,
                                       bool ailment3Active, bool ailment4Active) {
    CorridorRenderPlanResult plan = CorridorRenderPlan::Plan(view, ailment3Active, ailment4Active);

    if (plan.drawFloorFallbackFill) {
        bb.FillRect(0, 0, Backbuffer::kWidth, assets.floorTexture.height, PackRGB565(160, 0, 0));
    } else if (plan.drawFloorTiles) {
        for (int col = 0; col < 5; col++) {
            bb.Blit(col * 36, 0, assets.floorTexture);
        }
    }

    for (const WallDrawCall& seg : plan.wallSegments) {
        int clipX0 = seg.x;
        int clipX1 = seg.x + 18;
        if (seg.frame > 7) {
            int mirroredFrame = seg.frame - 8;
            bb.Blit(seg.x - mirroredFrame * 18, 0, assets.wallTexture, clipX0, clipX1, /*mirrorX=*/true);
        } else {
            bb.Blit(seg.x - seg.frame * 18, 0, assets.wallTexture, clipX0, clipX1, /*mirrorX=*/false);
        }
    }
}

void GameRenderer::RenderStatusBars(Backbuffer& bb, const StatusBarPlan& plan) {
    uint16_t track = PackRGB565(255, 255, 0);
    bb.FillRect(5, 130, 40, 7, track);
    bb.FillRect(5, 138, 40, 7, track);
    bb.FillRect(5, 146, 40, 7, track);

    bb.FillRect(6, 131, plan.hpWidth, 5, PackRGB565(255, 0, 0));
    bb.FillRect(6, 139, plan.magickaWidth, 5, PackRGB565(0, 255, 0));
    bb.FillRect(6, 147, plan.fatigueWidth, 5, PackRGB565(0, 0, 255));
}

void GameRenderer::RenderHud(Backbuffer& bb, const HotbarAssets& assets, int iconSet) {
    bb.FillRect(0, 156, Backbuffer::kWidth, 52, PackRGB565(0, 0, 0));
    bb.FillRoundRect(2, 158, Backbuffer::kWidth - 4, 48, 5, 5, PackRGB565(0xC7, 0x99, 0x67));

    if (iconSet < 0 || iconSet > 2) return;
    const HotbarRow& row = kHotbarRows[iconSet];

    for (int slot = 0; slot < 4; slot++) {
        int idx = row.iconIdx[slot];
        bb.Blit(kHotbarX[slot], 174, assets.icons[idx]);
        // paintHud(): smallFont (LatinPlain12).
        BitmapFont::DrawString(bb, kHotbarGlyphX[slot], 180, std::string(1, kHotbarKeyGlyphs[idx]),
                                PackRGB565(0, 0, 0), BitmapFont::Face::SmallPlain);
    }
}

void GameRenderer::RenderMinimapZoomedOut(Backbuffer& bb, const SquareViewGrid& grid, int facing) {
    // paintMinimapZoomedOut(): smallFont (LatinPlain12).
    BitmapFont::DrawString(bb, 16, 10, std::string(1, kCompassGlyphs[facing]), PackRGB565(255, 255, 255),
                           BitmapFont::Face::SmallPlain);
    bb.FillRect(10, 20, 23, 23, PackRGB565(0, 0, 0));
    DrawMinimapGrid(bb, 10, 20, 7, 3, 1, grid);
}

void GameRenderer::RenderMinimapNormal(Backbuffer& bb, const SquareViewGrid& grid, int facing) {
    // paintMinimapNormal(): minimapFont (LatinBold17).
    BitmapFont::DrawString(bb, 58, 10, std::string(1, kCompassGlyphs[facing]), PackRGB565(255, 255, 255),
                           BitmapFont::Face::LargeBold);
    bb.FillRect(15, 25, 89, 89, PackRGB565(0, 0, 0));
    DrawMinimapGrid(bb, 15, 25, 17, 5, 2, grid);
}

void GameRenderer::DrawMinimapGrid(Backbuffer& bb, int originX, int originY, int gridSize, int cellPx,
                                    int pixelOffset, const SquareViewGrid& grid) {
    int center = gridSize / 2;

    for (int row = 0; row < gridSize; row++) {
        int py = originY + pixelOffset + row * cellPx;
        for (int col = 0; col < gridSize; col++) {
            int px = originX + pixelOffset + col * cellPx;
            uint8_t cell = grid[static_cast<size_t>(col)][static_cast<size_t>(row)];

            if (cell == 1) {
                bb.FillRect(px, py, cellPx, cellPx, PackRGB565(0, 0, 0));
            } else if (cell == 0) {
                bb.FillRect(px, py, cellPx, cellPx, PackRGB565(255, 255, 255));
            } else if ((cell & 2) != 0) {
                bb.FillRect(px, py, cellPx, cellPx, PackRGB565(255, 0, 0));
            } else if ((cell & 4) != 0) {
                bb.FillRect(px, py, cellPx, cellPx, PackRGB565(0, 0, 255));
            } else if ((cell & 8) != 0) {
                bb.FillRect(px, py, cellPx, cellPx, PackRGB565(0xCC, 0x00, 0xFF));
            }

            if (row == center && col == center) {
                bb.FillRect(px, py, cellPx, cellPx, PackRGB565(0, 255, 0));
            }
        }
    }
}

}  // namespace stormhold
