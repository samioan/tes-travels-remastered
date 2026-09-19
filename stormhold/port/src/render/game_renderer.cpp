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

}  // namespace stormhold
