#include "render/frame_renderer.h"

#include "player/player_combat_stats.h"

namespace dawnstar {

namespace {

// paintCorridorWalls()'s own g.setColor(10485760) -- the Troll Thirst
// floor override.
constexpr uint16_t kTrollThirstFloorColor = PackRGB565((10485760 >> 16) & 0xFF, (10485760 >> 8) & 0xFF, 10485760 & 0xFF);

}  // namespace

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
                           int playerX, int playerY, int facing, int dungeonNumber, const PlayerState& player) {
    // g.setColor(0); g.fillRect(0, 0, screenWidth, screenHeight) --
    // GameCanvas.paintGameView()'s own opening lines, before
    // paintCorridorWalls() is even called.
    bb.Fill(0);

    // paintCorridorWalls()'s own `if (!hasAilment(3)) {...}` floor
    // block -- M54. Blind (ailment 3) skips it outright, leaving the
    // black fill above showing through; Troll Thirst (ailment 4, only
    // checked when NOT Blind) draws a solid dark-red rect sized to the
    // FIXED (non-ice) floor texture's own height, not whichever texture
    // the normal path below would have used; otherwise the normal
    // per-column (dungeon-number-dependent) floor texture tiling.
    if (!PlayerCombatStats::HasAilment(player, 3)) {
        if (PlayerCombatStats::HasAilment(player, 4)) {
            bb.FillRect(0, 0, Backbuffer::kWidth, textures.floor.height, kTrollThirstFloorColor);
        } else {
            const DecodedImage& floorTex = dungeonNumber != 1 ? textures.floorIce : textures.floor;
            for (int col = 0; col < 5; col++) {
                bb.Blit(col * 36, 0, floorTex);
            }
        }
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
