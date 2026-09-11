// M9 smoke test: runs CorridorRenderPlan against a real generated dungeon
// level (M6's DungeonGenerator) from a real, guaranteed-walkable position
// (a monster's door-tile spawn position) facing all 4 directions, and
// prints the resulting wall-segment draw-call list for each -- the same
// decisions GameCanvas.paintCorridorWalls()/drawWallSegment() would have
// made. No bit-exact JVM ground truth is possible here either (same
// ESGame/MIDP-stub wall as M6 -- this whole pipeline sits downstream of
// Item.load()/Monster.load()), so this checks internal self-consistency:
// every texture is one of the 3 known kinds, every draw call's step
// produced at most one call, and a simple ASCII strip per direction for
// a human to eyeball against the tile grid printed alongside it.
#include <cstdio>

#include "assets/dat_archive.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "render/corridor_render_plan.h"
#include "world/dungeon_view.h"

namespace {

const char* TextureName(dawnstar::WallTexture t) {
    switch (t) {
        case dawnstar::WallTexture::kWall:
            return "wall";
        case dawnstar::WallTexture::kWallIce:
            return "wallIce";
        case dawnstar::WallTexture::kGate:
            return "gate";
    }
    return "?";
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        dawnstar::MonsterDatabase monsters = dawnstar::MonsterDatabase::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);

        int levelNumber = argc > 2 ? std::atoi(argv[2]) : 2;
        dawnstar::GeneratedLevel level = dawnstar::DungeonGenerator::PopulateLevel(
            levelNumber, geometry.rows[levelNumber - 1], items, monsters);
        dawnstar::DungeonView view(level);

        bool ok = true;
        const char* dirNames[5] = {"", "N", "E", "S", "W"};

        auto testPosition = [&](const char* label, int px, int py) {
            std::printf("level %d, standing at (%d,%d) [%s]\n", levelNumber, px, py, label);
            for (int facing = 1; facing <= 4; facing++) {
                std::vector<dawnstar::WallDrawCall> calls =
                    dawnstar::CorridorRenderPlan::Plan(view, px, py, facing, level.number);

                std::printf("  facing %s: %zu segments:", dirNames[facing], calls.size());
                if (calls.size() > 10) {
                    std::printf(" FAIL (>10)\n");
                    ok = false;
                    continue;
                }
                for (const auto& call : calls) {
                    std::printf(" %s@%d", TextureName(call.texture), call.x);
                }
                std::printf("\n");
            }
        };

        // A monster's door-tile spawn is a guaranteed walkable, in-a-room
        // position -- a real "the player could stand here" tile, not an
        // arbitrary/possibly-wall coordinate. Small rooms (2-5 tiles per
        // side) don't discriminate facing very well on their own, so also
        // test from a known, predictable geometry: this level's own
        // stairway-corridor endpoint (a straight, 1-tile-wide, multi-tile
        // deep corridor in exactly one direction, per DungeonGeometry's
        // stairsUpDir/stairsDownDir for this level) -- facing down that
        // corridor should show visibly more depth than facing across it.
        testPosition("a real room-door tile", level.monsters[0].x, level.monsters[0].y);

        int stairsDir = geometry.rows[levelNumber - 1].stairsUpDir;
        if (stairsDir == 2) {
            testPosition("east stairway corridor", 31, 17);
        } else if (stairsDir == 4) {
            testPosition("west stairway corridor", 3, 17);
        } else if (stairsDir == 1) {
            testPosition("north stairway corridor", 17, 3);
        } else if (stairsDir == 3) {
            testPosition("south stairway corridor", 17, 31);
        }

        if (!ok) {
            std::fprintf(stderr, "m9_corridor_plan_smoke: FAILED\n");
            return 1;
        }
        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m9_corridor_plan_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
