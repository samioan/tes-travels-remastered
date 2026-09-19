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
        BitmapFont::DrawString(bb, kHotbarGlyphX[slot], 180, std::string(1, kHotbarKeyGlyphs[idx]),
                                PackRGB565(0, 0, 0));
    }
}

}  // namespace stormhold
