#include "render/game_renderer.h"

namespace stormhold {

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

}  // namespace stormhold
