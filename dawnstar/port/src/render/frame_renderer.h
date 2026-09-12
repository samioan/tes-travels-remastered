#pragma once
#include "assets/decoded_image.h"
#include "assets/img_archive.h"
#include "graphics/backbuffer.h"
#include "render/corridor_render_plan.h"
#include "world/dungeon_view.h"

namespace dawnstar {

// The 5 wall/floor textures GameCanvas.paintCorridorWalls() draws with,
// decoded once up front (../../src/ESGame.java's own createImage() call
// sites: floor3.png/floorIce.png/wallsr.png/wallsi.png/gate.png).
struct FrameTextures {
    DecodedImage floor;
    DecodedImage floorIce;
    DecodedImage wall;
    DecodedImage wallIce;
    DecodedImage gate;

    static FrameTextures Load(const ImgArchive& archive);
};

// Renamed-source counterpart of GameCanvas.paintCorridorWalls()'s pixel
// output -- floor + wall/gate segments, composited onto a real
// Backbuffer via CorridorRenderPlan's draw-call list (M9) and stb_image-
// decoded real textures (M10). Out of scope, both because they need
// Player (not ported): the two Player.hasAilment-gated floor overrides
// (a solid dark rect, or nothing at all) -- this always takes the
// default "draw the floor texture" path -- and object/monster/chest/NPC
// sprites, the HUD, and the minimap (paintVisibleObjects/
// paintStatusBars/etc., GameCanvas.paintGameView's other calls).
class FrameRenderer {
public:
    static void Render(Backbuffer& bb, const FrameTextures& textures, const DungeonView& dungeon, int playerX,
                        int playerY, int facing, int dungeonNumber);
};

}  // namespace dawnstar
