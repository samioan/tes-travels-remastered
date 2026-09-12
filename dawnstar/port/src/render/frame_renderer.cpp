#include "render/frame_renderer.h"

namespace dawnstar {

FrameTextures FrameTextures::Load(const ImgArchive& archive) {
    FrameTextures t;
    t.floor = DecodedImage::FromPng(archive.Data("floor3.png"));
    t.floorIce = DecodedImage::FromPng(archive.Data("floorIce.png"));
    t.wall = DecodedImage::FromPng(archive.Data("wallsr.png"));
    t.wallIce = DecodedImage::FromPng(archive.Data("wallsi.png"));
    t.gate = DecodedImage::FromPng(archive.Data("gate.png"));
    return t;
}

void FrameRenderer::Render(Backbuffer& bb, const FrameTextures& textures, const DungeonView& dungeon,
                           int playerX, int playerY, int facing, int dungeonNumber) {
    // g.setColor(0); g.fillRect(0, 0, screenWidth, screenHeight) --
    // GameCanvas.paintGameView()'s own opening lines, before
    // paintCorridorWalls() is even called.
    bb.Fill(0);

    // paintCorridorWalls()'s floor loop: 5 columns of the (dungeon-
    // number-dependent) floor texture, default (no-ailment) path only.
    const DecodedImage& floorTex = dungeonNumber != 1 ? textures.floorIce : textures.floor;
    for (int col = 0; col < 5; col++) {
        bb.Blit(col * 36, 0, floorTex);
    }

    std::vector<WallDrawCall> calls = CorridorRenderPlan::Plan(dungeon, playerX, playerY, facing, dungeonNumber);
    for (const WallDrawCall& call : calls) {
        const DecodedImage& tex = call.texture == WallTexture::kGate
                                       ? textures.gate
                                       : (call.texture == WallTexture::kWallIce ? textures.wallIce : textures.wall);
        bb.Blit(call.x, call.y, tex, call.clipX, call.clipX + 18);
    }
}

}  // namespace dawnstar
