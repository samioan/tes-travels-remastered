#pragma once
#include <cstdint>
#include <functional>
#include <vector>

#include "graphics/backbuffer.h"
#include "dungeon/dungeon_runtime.h"
#include "render/corridor_assets.h"

namespace stormhold {

// PC-only widescreen corridor: a real 3D raycast of the dungeon walls, drawn
// across the whole (wider) canvas. The original view is hand-drawn art for a
// 176-wide screen -- it has no pictures for the extra width -- so in widescreen
// the walls are projected instead, textured with the original wall art.
//
// The camera is fitted to that art: the sloped side wall in newwallsnok.png is
// an exact perspective of a plane half a tile from the eye, which gives the
// vanishing point, the horizon and the ceiling/floor heights (see the
// constants in wide_corridor.cpp). The wall texture is recovered by
// un-projecting that slope, so bricks keep the original look.

// The wall texture, un-projected from the atlas into a flat picture: `u` runs
// along the wall, `v` from ceiling to floor. Pixels are RGB565; `valid` is false
// when the atlas is missing or empty.
struct WideWallTexture {
    int width = 0;
    int height = 0;
    std::vector<uint16_t> pixels;
    bool valid = false;

    static WideWallTexture Build(const DecodedImage& atlas);
};

class WideCorridor {
public:
    // Whether the cell `right` tiles to the player's right and `ahead` tiles in
    // front of them (0, 0 = the player's own tile) is a wall.
    using WallQuery = std::function<bool(int right, int ahead)>;

    // A WallQuery over the dungeon map for a player on tile (x, y) facing `facing`
    // (1 = north, 2 = east, 3 = south, 4 = west): the same tile lookup, and the
    // same wall test (bit 1), the original corridor view samples -- but over any
    // distance, not just its fixed 9x5 window. `level` must outlive the query.
    static WallQuery MakeWallQuery(const GeneratedLevel& level, const DungeonRuntime::LevelLookup& levels, int x,
                                   int y, int facing);

    // Draws the 3D view into the top 156 rows of the whole buffer: `bb` must be
    // showing its full width (ResetView). `centerX` is the screen column of the
    // view's centre line (where the original view's vanishing point sits).
    static void Render(Backbuffer& bb, int centerX, const CorridorAssets& assets, const WideWallTexture& wall,
                       const WallQuery& isWall, bool ailment3Active, bool ailment4Active);

    // The rows the 3D view occupies (the HUD panel starts below).
    static constexpr int kViewHeight = 156;
};

}  // namespace stormhold
