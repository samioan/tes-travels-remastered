#pragma once
#include <cstdint>
#include <functional>
#include <vector>

#include "graphics/backbuffer.h"
#include "render/frame_renderer.h"
#include "world/dungeon_view.h"

namespace dawnstar {

// PC-only widescreen corridor: a real 3D raycast of the dungeon walls, drawn
// across the whole (wider) canvas. The original view is hand-drawn art for a
// 176-wide screen -- it has no pictures for the extra width -- so in widescreen
// the walls are projected instead, textured with the original wall art.
//
// The camera is fitted to that art: the sloped side wall in wallsr.png is an
// exact perspective of a plane half a tile from the eye, which gives the
// vanishing point, the horizon and the ceiling/floor heights (see the
// constants in wide_corridor.cpp). Each texture is recovered by un-projecting
// that slope, so bricks (and ice) keep the original look.

// A wall texture un-projected from the atlas into a flat picture: `u` runs
// along the wall, `v` from ceiling to floor. Pixels are RGB565; alpha[i] == 0
// marks a see-through pixel (the gate's bars). `valid` is false when the atlas is
// missing or empty.
struct WideTexture {
    int width = 0;
    int height = 0;
    double depthPerCopy = 0;  // projected depth one copy of the texture spans along a wall
    bool mirrorCopies = true; // lay copies out alternately mirrored (no seams) instead of repeated
    std::vector<uint16_t> pixels;
    std::vector<uint8_t> alpha;
    bool valid = false;

    // The nearest stretch of the atlas's sloped wall, straightened.
    static WideTexture BuildWall(const DecodedImage& atlas);
    // The portcullis: its front-face art as it is, repeated across the face.
    static WideTexture BuildGate(const DecodedImage& gate);
};

struct WideTextures {
    WideTexture wall, wallIce, gate;
    static WideTextures Build(const FrameTextures& textures);
    bool valid() const { return wall.valid && wallIce.valid && gate.valid; }
};

class WideCorridor {
public:
    // The tile bits at `right` tiles to the player's right and `ahead` tiles in
    // front of them (0, 0 = the player's own tile).
    using TileQuery = std::function<uint8_t(int right, int ahead)>;

    // A TileQuery over the dungeon map for a player on tile (x, y) facing `facing`
    // (1 = north, 2 = east, 3 = south, 4 = west): the same tile lookup the original
    // corridor view samples -- but over any distance, not just its fixed 9x5 window.
    // `view` must outlive the query.
    static TileQuery MakeTileQuery(const DungeonView& view, int x, int y, int facing);

    // Draws the 3D view into the top 156 rows of the whole buffer: `bb` must be
    // showing its full width (ResetView). `centerX` is the screen column of the
    // view's centre line (where the original view's vanishing point sits).
    // `dungeonNumber` picks the stone or ice walls and floor, as the original does.
    static void Render(Backbuffer& bb, int centerX, const FrameTextures& textures, const WideTextures& wide,
                       const TileQuery& tileAt, int dungeonNumber, bool blind, bool trollThirst);

    // The rows the 3D view occupies (the HUD panel starts below).
    static constexpr int kViewHeight = 156;
};

}  // namespace dawnstar
