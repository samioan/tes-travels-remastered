#include "render/corridor_render_plan.h"

namespace dawnstar {

namespace {

// GameCanvas.java's CORRIDOR_WALL_TABLE: wall-segment draw commands for
// the 5 possible forward-visibility steps x 6 candidate offsets to test
// x {cmd, column, dx, dy}. cmd feeds ResolveWallFrame (12 = pass the
// column through unchanged, 11 = mirror it) -- the exact geometric
// meaning of the two command codes isn't fully pinned down in the Java
// source either, preserved exactly as found there.
const int kCorridorWallTable[5][6][4] = {
    {{12, 0, 0, 1}, {11, 0, -1, 1}, {12, 1, -1, 2}, {12, 2, -1, 3}, {11, 2, -2, 3}, {12, 3, -2, 4}},
    {{12, 0, 0, 1}, {12, 1, 0, 2}, {11, 1, -1, 2}, {12, 2, -1, 3}, {12, 3, -1, 4}, {12, 3, -1, 4}},
    {{12, 0, 0, 1}, {12, 1, 0, 2}, {12, 2, 0, 3}, {11, 2, -1, 3}, {12, 3, -1, 4}, {12, 3, -1, 4}},
    {{12, 0, 0, 1}, {12, 1, 0, 2}, {12, 2, 0, 3}, {12, 3, 0, 4}, {11, 3, -1, 4}, {11, 3, -1, 4}},
    {{12, 0, 0, 1}, {12, 1, 0, 2}, {12, 2, 0, 3}, {12, 3, 0, 4}, {12, 3, 0, 4}, {12, 3, 0, 4}},
};

// Per-corridor-depth-column x-offset for wall/gate sprite placement.
const int kWallDrawOffsets[12] = {0, 0, 36, 72, 90, 108, 126, 144, 158, 176, 194, 212};

// Resolves a CORRIDOR_WALL_TABLE {cmd, column} pair plus scan side (-1
// forward, 1 mirrored) to the actual wall-frame column index used by
// kWallDrawOffsets.
int ResolveWallFrame(int cmd, int column, int side) {
    if (cmd == 12) return column;
    return side == -1 ? 8 + column : 7 - column;
}

// Draws one 18px-wide wall/gate column: wallType is the dungeon number
// (>1 draws the ice wall texture, ==1 draws the regular one) or -1 for a
// gate/transition tile. wallDrawnNear/wallDrawnMid suppress a duplicate
// draw when the forward and mirrored corridor scans land on the same
// segment -- ported verbatim from GameCanvas.drawWallSegment(), which
// every call actually reaches (there is no "skip the draw entirely"
// path; the dedup toggle only changes *where* the segment is drawn).
WallDrawCall DrawWallSegment(int frame, int x, int wallType, bool& wallDrawnNear, bool& wallDrawnMid) {
    WallDrawCall call;
    call.y = wallType == -1 ? 8 : 0;
    call.texture = wallType == -1 ? WallTexture::kGate : (wallType != 1 ? WallTexture::kWallIce : WallTexture::kWall);

    if (frame != 0 && frame != 1) {
        wallDrawnNear = false;
        if (frame == 2) {
            if (wallDrawnMid) {
                wallDrawnMid = false;
                call.x = x - kWallDrawOffsets[frame] - 18;
                return call;
            }
            wallDrawnMid = true;
        }
    } else {
        wallDrawnMid = false;
        if (wallDrawnNear) {
            wallDrawnNear = false;
            call.x = x - kWallDrawOffsets[frame] - 18;
            return call;
        }
        wallDrawnNear = true;
    }

    call.x = x - kWallDrawOffsets[frame];
    return call;
}

}  // namespace

std::vector<WallDrawCall> CorridorRenderPlan::Plan(const DungeonView& dungeon, int playerX, int playerY,
                                                    int facing, int dungeonNumber) {
    uint8_t view[9][5];
    dungeon.SampleCorridorView(playerX, playerY, facing, view);

    // Player.tileAt(dx,dy)'s exact re-centering formula over corridorView.
    auto tileAt = [&view](int dx, int dy) -> uint8_t {
        return dy < 4 ? view[dx + dy + 1][dy] : view[dx + dy][dy];
    };

    std::vector<WallDrawCall> calls;
    bool wallDrawnNear = false;
    bool wallDrawnMid = false;

    for (int step = 0; step < 5; step++) {
        int x = step * 18;
        for (int row = 0; row < 6; row++) {
            int cmd = kCorridorWallTable[step][row][0];
            int column = kCorridorWallTable[step][row][1];
            int dx = kCorridorWallTable[step][row][2];
            int dy = kCorridorWallTable[step][row][3];
            uint8_t tile = tileAt(dx, dy);

            if (tile & 1) {
                int frame = ResolveWallFrame(cmd, column, -1);
                calls.push_back(DrawWallSegment(frame, x, dungeonNumber, wallDrawnNear, wallDrawnMid));
                break;
            }
            if (tile & 64) {
                int frame = ResolveWallFrame(cmd, column, -1);
                calls.push_back(DrawWallSegment(frame, x, -1, wallDrawnNear, wallDrawnMid));
                break;
            }
        }
    }

    for (int step = 5; step < 10; step++) {
        int x = step * 18;
        for (int row = 0; row < 6; row++) {
            int cmd = kCorridorWallTable[9 - step][row][0];
            int column = kCorridorWallTable[9 - step][row][1];
            int dx = -kCorridorWallTable[9 - step][row][2];
            int dy = kCorridorWallTable[9 - step][row][3];
            uint8_t tile = tileAt(dx, dy);

            if (tile & 1) {
                int frame = ResolveWallFrame(cmd, column, 1);
                calls.push_back(DrawWallSegment(frame, x, dungeonNumber, wallDrawnNear, wallDrawnMid));
                break;
            }
            if (tile & 64) {
                int frame = ResolveWallFrame(cmd, column, 1);
                calls.push_back(DrawWallSegment(frame, x, -1, wallDrawnNear, wallDrawnMid));
                break;
            }
        }
    }

    return calls;
}

}  // namespace dawnstar
