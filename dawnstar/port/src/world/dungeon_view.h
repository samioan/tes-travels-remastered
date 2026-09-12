#pragma once
#include <cstdint>
#include <vector>

#include "world/dungeon_generator.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Dungeon.java's runtime tile
// query methods (tileAt()/isWalkable()/sampleCorridorView()) -- as
// opposed to DungeonGenerator, which only builds a level's initial
// state.
//
// M19: TileAt() now does Dungeon.tileAt()'s real cross-level neighbor
// stitching (following `neighbor{North,East,South,West}` into a
// different GeneratedLevel when (x,y) falls outside this level's own
// bounds, including the hub town's width/height-mismatch recentering
// and its "edge marker" sentinel tile), superseding the M9 stand-in
// that unconditionally returned 1 (wall) at every level edge. Every
// caller already held the full `levels` vector (Move/Chase/
// RefreshCorridorView all take it), so this only changes how a
// DungeonView is constructed, not who can reach one.
//
// SIMPLIFIED: Dungeon.java's `populated` flag (a lazy per-level
// generation marker Dungeon.tileAt() consults via
// `neighbor.populated ? neighbor.tiles[x][y] : 1`) has no equivalent
// here -- same established simplification as player/player_movement.h's
// class comment: this port always generates every level it holds
// upfront, so every entry in `levels` is always "populated" and that
// ternary's true branch is the only one ever taken.
class DungeonView {
public:
    // `levelIndex` is levelNumber-1, matching `levels`' own indexing
    // convention (world/dungeon_generator.h).
    DungeonView(const std::vector<GeneratedLevel>& levels, int levelIndex)
        : levels_(levels), levelIndex_(levelIndex) {}

    // Dungeon.java's tileAt(int,int): returns 1 ("wall") for an
    // out-of-bounds request with no neighbor in that direction, or 64
    // ("edge marker") for the exact boundary tile one step before a
    // size-mismatched neighbor (only ever the hub town on one side,
    // since it alone is 19x19 against every other level's 35x35).
    uint8_t TileAt(int x, int y) const {
        const GeneratedLevel& level = levels_[static_cast<size_t>(levelIndex_)];
        const GeneratedLevel* neighbor = nullptr;
        int nx = x;
        int ny = y;
        bool atEdgeMarker = false;

        if (x < 0) {
            int neighborLevel = level.neighborWest;
            if (neighborLevel <= 0) return 1;
            neighbor = &levels_[static_cast<size_t>(neighborLevel - 1)];
            nx = neighbor->width + x;
            if (neighborLevel == 1 || level.number == 1) {
                ny = y + (neighbor->height - level.height) / 2;
                if (nx == neighbor->width - 2) atEdgeMarker = true;
            }
        } else if (x >= level.width) {
            int neighborLevel = level.neighborEast;
            if (neighborLevel <= 0) return 1;
            neighbor = &levels_[static_cast<size_t>(neighborLevel - 1)];
            nx = x - level.width;
            if (neighborLevel == 1 || level.number == 1) {
                ny = y + (neighbor->height - level.height) / 2;
                if (nx == 1) atEdgeMarker = true;
            }
        } else if (y < 0) {
            int neighborLevel = level.neighborNorth;
            if (neighborLevel <= 0) return 1;
            neighbor = &levels_[static_cast<size_t>(neighborLevel - 1)];
            ny = neighbor->height + y;
            if (neighborLevel == 1 || level.number == 1) {
                nx = x + (neighbor->width - level.width) / 2;
                if (ny == neighbor->height - 2) atEdgeMarker = true;
            }
        } else if (y >= level.height) {
            int neighborLevel = level.neighborSouth;
            if (neighborLevel <= 0) return 1;
            neighbor = &levels_[static_cast<size_t>(neighborLevel - 1)];
            ny = y - level.height;
            if (neighborLevel == 1 || level.number == 1) {
                nx = x + (neighbor->width - level.width) / 2;
                if (ny == 1) atEdgeMarker = true;
            }
        }

        if (neighbor != nullptr) {
            if (nx < 0 || nx >= neighbor->width) return 1;
            if (ny >= 0 && ny < neighbor->height) {
                if (atEdgeMarker && neighbor->tiles[static_cast<size_t>(nx)][static_cast<size_t>(ny)] == 0) return 64;
                // neighbor->populated is always true here -- see class comment.
                return neighbor->tiles[static_cast<size_t>(nx)][static_cast<size_t>(ny)];
            }
            return 1;
        }

        return level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)];
    }

    // Dungeon.java's isWalkable(int,int) -- NOT the same rule as
    // Player.isWalkable(byte) (player/player_movement.h)! This one does
    // its own bounds check (no cross-level stitching -- neither does the
    // original) and additionally treats bit 8 (no-spawn special room) as
    // blocking -- used by Monster movement (monsters can't wander into a
    // no-spawn room), whereas Player.isWalkable is tested against a tile
    // the caller already knows is in-bounds and doesn't care about bit 8
    // at all.
    bool IsWalkable(int x, int y) const {
        const GeneratedLevel& level = levels_[static_cast<size_t>(levelIndex_)];
        if (x < 0 || y < 0 || x >= level.width || y >= level.height) return false;
        uint8_t flags = level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)];
        if ((flags & 1) != 0) return false;
        if ((flags & 2) != 0) return false;
        if ((flags & 8) != 0) return false;
        return (flags & 32) == 0;
    }

    // Dungeon.java's sampleCorridorView(): a widening-diamond sample of
    // tile bits ahead of (x,y) facing `direction` (1=N,2=E,3=S,4=W),
    // for the first-person corridor renderer's per-column occlusion
    // test. `out` rows widen 3,5,7,9,9 with distance. Ported verbatim,
    // now via the cross-level-aware TileAt above.
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

    const GeneratedLevel& Level() const { return levels_[static_cast<size_t>(levelIndex_)]; }

private:
    const std::vector<GeneratedLevel>& levels_;
    int levelIndex_;
};

}  // namespace dawnstar
