#pragma once
#include <vector>

#include "world/dungeon_view.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/GameCanvas.java's
// CORRIDOR_WALL_TABLE-driven corridor renderer (paintCorridorWalls()/
// drawWallSegment()/resolveWallFrame()) -- the wall-segment SELECTION
// logic only. This deliberately stops short of drawing actual pixels:
// this port has no PNG decoder yet (M7's ImgArchive only extracts raw,
// still-PNG-encoded byte blobs -- see docs/PORT_ROADMAP.md), so there's
// nothing to Blit() onto a Backbuffer with yet. What this produces is
// exactly the same sequence of "draw this texture at this x,y" decisions
// GameCanvas.drawWallSegment would have made -- as data, for a later
// milestone's real pixel renderer to consume directly once PNG decoding
// exists.
//
// Also out of scope here (both need Player, which isn't ported):
// floor/ceiling rendering (paintCorridorWalls' Player.hasAilment-gated
// branch -- draw the floor texture normally, or a solid dark rect for
// one specific ailment, or nothing for another), and object/monster/
// chest sprites (paintVisibleObjects). Just the wall/gate segments.
enum class WallTexture { kWall, kWallIce, kGate };

struct WallDrawCall {
    WallTexture texture = WallTexture::kWall;
    int x = 0;
    int y = 0;
};

class CorridorRenderPlan {
public:
    // `dungeonNumber` is the level number the player is standing on
    // (Dungeon.number) -- GameCanvas passes this straight through to
    // drawWallSegment as `wallType` for a bit-1 (real wall) hit; a bit-64
    // (edge marker) hit always passes -1 (gate) instead, regardless of
    // dungeonNumber. 1 draws the plain wall texture, anything else draws
    // the ice one (every non-hub level).
    static std::vector<WallDrawCall> Plan(const DungeonView& dungeon, int playerX, int playerY, int facing,
                                           int dungeonNumber);
};

}  // namespace dawnstar
