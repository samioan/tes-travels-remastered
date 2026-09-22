#pragma once
#include "assets/decoded_image.h"
#include "assets/img_archive.h"
#include "graphics/backbuffer.h"
#include "player/player_state.h"
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
// decoded real textures (M10). M54 added the two Player.hasAilment-gated
// floor overrides the original's own opening `if (!hasAilment(3))
// {ailment 4 ? solid rect : floor texture}` chain makes: Blind (ailment
// 3) skips the floor entirely (the black `bb.Fill(0)` below already
// shows through, matching `g.setColor(0); g.fillRect(...)`'s own
// pre-paintCorridorWalls background); Troll Thirst (ailment 4, checked
// only when NOT also Blind) replaces it with a solid dark-red rect
// (`g.setColor(10485760)`) sized to the FIXED (non-ice) floor texture's
// own height, `floorTexture.getHeight()` -- not whichever texture the
// dungeon-dependent normal path would have used. Still out of scope:
// object/monster/chest/NPC sprites, the HUD, and the minimap
// (paintVisibleObjects/paintStatusBars/etc., GameCanvas.paintGameView's
// other calls).
class FrameRenderer {
public:
    static void Render(Backbuffer& bb, const FrameTextures& textures, const DungeonView& dungeon, int playerX,
                        int playerY, int facing, int dungeonNumber, const PlayerState& player);
};

}  // namespace dawnstar
