#pragma once
#include <cstdint>

#include "world/dungeon_generator.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Dungeon.java's runtime tile
// query methods (tileAt()/sampleCorridorView()) -- as opposed to
// DungeonGenerator, which only builds a level's initial state.
//
// SIMPLIFIED: TileAt() does NOT do Dungeon.tileAt()'s cross-level
// neighbor stitching (following `neighbors[]` into a different
// GeneratedLevel when (x,y) falls outside this level's own bounds) --
// there is no multi-level "world" object yet to stitch into, only ever
// one GeneratedLevel at a time. Out-of-bounds queries return 1 (wall)
// unconditionally instead, which is wrong exactly at a level's edges
// (where the real game would show the neighboring level's own tiles, or
// a bare wall only if that edge truly has no neighbor) -- everywhere
// else it's identical. Revisit once a real multi-level world exists;
// GeneratedLevel already carries neighborNorth/East/South/West for that.
class DungeonView {
public:
    explicit DungeonView(const GeneratedLevel& level) : level_(level) {}

    // Dungeon.java's tileAt(int,int) -- see the class comment for the
    // one simplification versus the original.
    uint8_t TileAt(int x, int y) const {
        if (x < 0 || y < 0 || x >= level_.width || y >= level_.height) return 1;
        return level_.tiles[x][y];
    }

    // Dungeon.java's isWalkable(int,int) -- NOT the same rule as
    // Player.isWalkable(byte) (player/player_movement.h)! This one does
    // its own bounds check and additionally treats bit 8 (no-spawn
    // special room) as blocking -- used by Monster movement (monsters
    // can't wander into a no-spawn room), whereas Player.isWalkable is
    // tested against a tile the caller already knows is in-bounds and
    // doesn't care about bit 8 at all.
    bool IsWalkable(int x, int y) const {
        if (x < 0 || y < 0 || x >= level_.width || y >= level_.height) return false;
        uint8_t flags = level_.tiles[x][y];
        if ((flags & 1) != 0) return false;
        if ((flags & 2) != 0) return false;
        if ((flags & 8) != 0) return false;
        return (flags & 32) == 0;
    }

    // Dungeon.java's sampleCorridorView(): a widening-diamond sample of
    // tile bits ahead of (x,y) facing `direction` (1=N,2=E,3=S,4=W),
    // for the first-person corridor renderer's per-column occlusion
    // test. `out` rows widen 3,5,7,9,9 with distance. Ported verbatim.
    void SampleCorridorView(int x, int y, int direction, uint8_t out[9][5]) const {
        if (direction != 1 && direction != 3) {
            if (direction == 2 || direction == 4) {
                int sign = direction == 2 ? 1 : -1;

                out[0][0] = TileAt(x, y - sign);
                out[1][0] = TileAt(x, y);
                out[2][0] = TileAt(x, y + sign);
                int fx = x + sign;
                for (int i = 0; i < 5; i++) out[i][1] = TileAt(fx, y + (i - 2) * sign);
                fx = x + 2 * sign;
                for (int i = 0; i < 7; i++) out[i][2] = TileAt(fx, y + (i - 3) * sign);
                fx = x + 3 * sign;
                for (int i = 0; i < 9; i++) out[i][3] = TileAt(fx, y + (i - 4) * sign);
                fx = x + 4 * sign;
                for (int i = 0; i < 9; i++) out[i][4] = TileAt(fx, y + (i - 4) * sign);
            }
        } else {
            int sign = direction == 1 ? 1 : -1;

            out[0][0] = TileAt(x - sign, y);
            out[1][0] = TileAt(x, y);
            out[2][0] = TileAt(x + sign, y);
            int fy = y - sign;
            for (int i = 0; i < 5; i++) out[i][1] = TileAt(x + (i - 2) * sign, fy);
            fy = y - 2 * sign;
            for (int i = 0; i < 7; i++) out[i][2] = TileAt(x + (i - 3) * sign, fy);
            fy = y - 3 * sign;
            for (int i = 0; i < 9; i++) out[i][3] = TileAt(x + (i - 4) * sign, fy);
            fy = y - 4 * sign;
            for (int i = 0; i < 9; i++) out[i][4] = TileAt(x + (i - 4) * sign, fy);
        }
    }

    const GeneratedLevel& Level() const { return level_; }

private:
    const GeneratedLevel& level_;
};

}  // namespace dawnstar
