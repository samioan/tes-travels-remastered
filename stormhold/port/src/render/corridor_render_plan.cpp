#include "render/corridor_render_plan.h"

namespace stormhold {

namespace {

// GameCanvas.wallSegmentTable (was decompiled/e.java's `n`, a confirmed
// 5x6x4 shape matching dawnstar's own CORRIDOR_WALL_TABLE) -- {cmd,
// column, dx, dy} per candidate offset, 6 candidates per forward-
// visibility step (0-4). cmd is fed into ResolveWallFrame (12 = pass the
// column through unchanged, 11 = mirror it) -- the exact geometric
// meaning of the two command codes isn't pinned down further (same open
// question dawnstar's own table comment already flags for its identical
// shape), transcribed verbatim.
const int kWallSegmentTable[5][6][4] = {
    {{12, 0, 0, 1}, {11, 0, -1, 1}, {12, 1, -1, 2}, {12, 2, -1, 3}, {11, 2, -2, 3}, {12, 3, -2, 4}},
    {{12, 0, 0, 1}, {12, 1, 0, 2}, {11, 1, -1, 2}, {12, 2, -1, 3}, {12, 3, -1, 4}, {12, 3, -1, 4}},
    {{12, 0, 0, 1}, {12, 1, 0, 2}, {12, 2, 0, 3}, {11, 2, -1, 3}, {12, 3, -1, 4}, {12, 3, -1, 4}},
    {{12, 0, 0, 1}, {12, 1, 0, 2}, {12, 2, 0, 3}, {12, 3, 0, 4}, {11, 3, -1, 4}, {11, 3, -1, 4}},
    {{12, 0, 0, 1}, {12, 1, 0, 2}, {12, 2, 0, 3}, {12, 3, 0, 4}, {12, 3, 0, 4}, {12, 3, 0, 4}},
};

// GameCanvas.resolveWallFrame(cmd, column, side).
int ResolveWallFrame(int cmd, int column, int side) {
    if (cmd == 12) return column;
    return side == -1 ? 8 + column : 7 - column;
}

}  // namespace

CorridorRenderPlanResult CorridorRenderPlan::Plan(const CorridorViewGrid& view, bool ailment3Active,
                                                   bool ailment4Active) {
    CorridorRenderPlanResult result;

    if (!ailment3Active) {
        if (ailment4Active) {
            result.drawFloorFallbackFill = true;
        } else {
            result.drawFloorTiles = true;
        }
    }

    for (int step = 0; step < 5; step++) {
        int x = step * 18;
        for (int row = 0; row < 6; row++) {
            int cmd = kWallSegmentTable[step][row][0];
            int column = kWallSegmentTable[step][row][1];
            int dx = kWallSegmentTable[step][row][2];
            int dy = kWallSegmentTable[step][row][3];
            if (DungeonRuntime::ViewGridAt(view, dx, dy) & 0x01) {
                int frame = ResolveWallFrame(cmd, column, -1);
                result.wallSegments.push_back({frame, x});
                break;
            }
        }
    }

    for (int step = 5; step < 10; step++) {
        int x = step * 18;
        for (int row = 0; row < 6; row++) {
            int cmd = kWallSegmentTable[9 - step][row][0];
            int column = kWallSegmentTable[9 - step][row][1];
            int dx = -kWallSegmentTable[9 - step][row][2];
            int dy = kWallSegmentTable[9 - step][row][3];
            if (DungeonRuntime::ViewGridAt(view, dx, dy) & 0x01) {
                int frame = ResolveWallFrame(cmd, column, 1);
                result.wallSegments.push_back({frame, x});
                break;
            }
        }
    }

    return result;
}

}  // namespace stormhold
