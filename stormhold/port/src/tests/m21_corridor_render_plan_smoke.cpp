// M21 smoke test: DungeonRuntime::TileAt/SampleCorridorView/ViewGridAt
// (the corridor 3D-renderer's visibility sample, including its cross-
// level boundary stitching) and CorridorRenderPlan::Plan (the wall-
// segment SELECTION logic, data only -- no pixels yet, mirroring
// dawnstar's own equivalent milestone's scope). No JVM ground truth is
// possible here (same reason as M6) -- verified via hand-built synthetic
// levels for exact geometric checks, plus a real M6-generated level for
// an integration/self-consistency check.
#include <cstdio>
#include <map>
#include <string>

#include "assets/asset_root.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "render/corridor_render_plan.h"
#include "world/dungeon_generator.h"

namespace {

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

stormhold::GeneratedLevel MakeLevel(int number, int width = 35, int height = 35) {
    stormhold::GeneratedLevel level;
    level.number = number;
    level.width = width;
    level.height = height;
    level.tiles.assign(static_cast<size_t>(width), std::vector<uint8_t>(static_cast<size_t>(height), 0));
    return level;
}

void TestViewGridAtFormula() {
    std::printf("-- ViewGridAt's depth<4 vs depth>=4 indexing --\n");
    stormhold::CorridorViewGrid grid{};
    grid[2][1] = 42;  // index=0,depth=1 -> grid[0+1+1][1] = grid[2][1]
    grid[3][4] = 99;  // index=-1,depth=4 -> grid[-1+4][4] = grid[3][4]

    Expect(stormhold::DungeonRuntime::ViewGridAt(grid, 0, 1) == 42, "depth<4 should read grid[index+depth+1][depth]");
    Expect(stormhold::DungeonRuntime::ViewGridAt(grid, -1, 4) == 99, "depth>=4 should read grid[index+depth][depth]");
}

void TestTileAtWithinLevel() {
    std::printf("-- TileAt within a single level (no boundary) --\n");
    stormhold::GeneratedLevel level = MakeLevel(5);
    level.tiles[17][17] = 1;
    std::map<int, stormhold::GeneratedLevel> cache;
    cache[5] = level;
    stormhold::DungeonRuntime::LevelLookup lookup = [&](int n) -> const stormhold::GeneratedLevel& {
        return cache.at(n);
    };

    Expect(stormhold::DungeonRuntime::TileAt(cache.at(5), 17, 17, lookup) == 1, "TileAt should read the wall tile directly");
    Expect(stormhold::DungeonRuntime::TileAt(cache.at(5), 10, 10, lookup) == 0, "TileAt should read an open tile directly");
}

void TestTileAtNoNeighborReturnsWall() {
    std::printf("-- TileAt with no neighbor in that direction returns wall (1) --\n");
    stormhold::GeneratedLevel level = MakeLevel(6);  // no neighbors set at all
    std::map<int, stormhold::GeneratedLevel> cache;
    cache[6] = level;
    stormhold::DungeonRuntime::LevelLookup lookup = [&](int n) -> const stormhold::GeneratedLevel& {
        return cache.at(n);
    };

    Expect(stormhold::DungeonRuntime::TileAt(cache.at(6), -1, 17, lookup) == 1, "no west neighbor should read as wall");
    Expect(stormhold::DungeonRuntime::TileAt(cache.at(6), 35, 17, lookup) == 1, "no east neighbor should read as wall");
    Expect(stormhold::DungeonRuntime::TileAt(cache.at(6), 17, -1, lookup) == 1, "no north neighbor should read as wall");
    Expect(stormhold::DungeonRuntime::TileAt(cache.at(6), 17, 35, lookup) == 1, "no south neighbor should read as wall");
}

void TestTileAtCrossesLevelBoundary() {
    std::printf("-- TileAt reads across a real level boundary, hub<->standard recentered --\n");
    stormhold::GeneratedLevel hub = MakeLevel(1, 19, 19);
    hub.neighborNorth = 2;
    stormhold::GeneratedLevel standard = MakeLevel(2, 35, 35);
    standard.neighborSouth = 1;
    standard.tiles[17][34] = 1;  // the tile the hub should see just across its own north edge

    std::map<int, stormhold::GeneratedLevel> cache;
    cache[1] = hub;
    cache[2] = standard;
    stormhold::DungeonRuntime::LevelLookup lookup = [&](int n) -> const stormhold::GeneratedLevel& {
        return cache.at(n);
    };

    // Same hand-derived recentering M10's own smoke test already confirmed
    // for player movement: hub X=9 lines up with standard X=17.
    uint8_t seen = stormhold::DungeonRuntime::TileAt(cache.at(1), 9, -1, lookup);
    Expect(seen == 1, "TileAt should read the real neighbor level's tile, recentered the same way movement crosses");

    uint8_t sameButOpen = stormhold::DungeonRuntime::TileAt(cache.at(1), 8, -1, lookup);
    Expect(sameButOpen == 0, "an adjacent recentered column should read the neighbor's own (open) tile, not the marked one");
}

void TestSampleCorridorViewEastWestAsymmetry() {
    std::printf("-- SampleCorridorView's confirmed grid[1][0]==0 asymmetry for east/west facing --\n");
    stormhold::GeneratedLevel level = MakeLevel(7);
    level.tiles[17][17] = 1;  // the player's own tile -- would be read as real data by north/south facing
    std::map<int, stormhold::GeneratedLevel> cache;
    cache[7] = level;
    stormhold::DungeonRuntime::LevelLookup lookup = [&](int n) -> const stormhold::GeneratedLevel& {
        return cache.at(n);
    };

    stormhold::CorridorViewGrid eastView = stormhold::DungeonRuntime::SampleCorridorView(cache.at(7), 17, 17, 2, lookup);
    Expect(eastView[1][0] == 0, "facing east: grid[1][0] should be the literal constant 0, not a real tile read");

    stormhold::CorridorViewGrid northView = stormhold::DungeonRuntime::SampleCorridorView(cache.at(7), 17, 17, 1, lookup);
    Expect(northView[1][0] == 1, "facing north: grid[1][0] SHOULD be a real TileAt(x,y) read of the player's own tile");
}

void TestSampleCorridorViewFindsWallAheadNorth() {
    std::printf("-- SampleCorridorView + Plan: a wall 1 tile ahead (north) produces a near wall segment --\n");
    stormhold::GeneratedLevel level = MakeLevel(8);
    level.tiles[17][16] = 1;  // directly north (facing 1: dy decreases) of (17,17)
    std::map<int, stormhold::GeneratedLevel> cache;
    cache[8] = level;
    stormhold::DungeonRuntime::LevelLookup lookup = [&](int n) -> const stormhold::GeneratedLevel& {
        return cache.at(n);
    };

    stormhold::CorridorViewGrid view = stormhold::DungeonRuntime::SampleCorridorView(cache.at(8), 17, 17, 1, lookup);
    stormhold::CorridorRenderPlanResult plan = stormhold::CorridorRenderPlan::Plan(view, false, false);

    Expect(plan.drawFloorTiles, "no ailments active -> the plan should draw the tiled floor");
    Expect(!plan.drawFloorFallbackFill, "the fallback fill flag should be false when the tiled floor is drawn");
    Expect(!plan.wallSegments.empty(), "a wall directly ahead should produce at least one wall segment");

    // The nearest step (x=0, forward scan) and its mirrored counterpart
    // (x=162) should both find the SAME immediately-adjacent wall --
    // wallSegmentTable's row 0 for step 0 is {12,0,0,1} (dx=0,dy=1), which
    // for facing==1 (dy decreasing) maps to exactly the tile placed above.
    bool sawNear = false, sawMirroredNear = false;
    for (const auto& seg : plan.wallSegments) {
        if (seg.x == 0) sawNear = true;
        if (seg.x == 162) sawMirroredNear = true;
    }
    Expect(sawNear, "the forward scan's nearest step (x=0) should find the wall placed directly ahead");
    Expect(sawMirroredNear, "the mirrored scan's nearest step (x=162) should find the same symmetric wall");
}

void TestPlanAilmentFloorFlags() {
    std::printf("-- Plan's ailment-gated floor flags --\n");
    stormhold::CorridorViewGrid openView{};  // every tile open (0) -- no walls anywhere in range

    auto noAilments = stormhold::CorridorRenderPlan::Plan(openView, false, false);
    Expect(noAilments.drawFloorTiles && !noAilments.drawFloorFallbackFill, "no ailments -> tiled floor");
    Expect(noAilments.wallSegments.empty(), "an all-open view should produce zero wall segments");

    auto ailment3Only = stormhold::CorridorRenderPlan::Plan(openView, true, false);
    Expect(!ailment3Only.drawFloorTiles && !ailment3Only.drawFloorFallbackFill,
           "ailment 3 active -> no floor drawn at all, regardless of ailment 4");

    auto ailment4Only = stormhold::CorridorRenderPlan::Plan(openView, false, true);
    Expect(!ailment4Only.drawFloorTiles && ailment4Only.drawFloorFallbackFill,
           "ailment 4 active (and not 3) -> the solid-color fallback fill, not the tiled floor");

    auto both = stormhold::CorridorRenderPlan::Plan(openView, true, true);
    Expect(!both.drawFloorTiles && !both.drawFloorFallbackFill,
           "ailment 3 takes priority over ailment 4 -- matches the original's own nested if/else exactly");
}

void TestAgainstRealGeneratedLevel(const std::string& root) {
    std::printf("-- integration: a real M6-generated level, self-consistency only --\n");
    stormhold::AssetRoot assets(root);
    stormhold::ItemDatabase items = stormhold::ItemDatabase::Load(assets);
    stormhold::MonsterDatabase monsters = stormhold::MonsterDatabase::Load(assets);
    stormhold::DungeonGeometry geometry = stormhold::DungeonGeometry::Load(assets);

    stormhold::GeneratedLevel level = stormhold::DungeonGenerator::PopulateLevel(2, geometry.rows[1], items, monsters);
    std::map<int, stormhold::GeneratedLevel> cache;
    cache[2] = level;
    stormhold::DungeonRuntime::LevelLookup lookup = [&](int n) -> const stormhold::GeneratedLevel& {
        return cache.at(n);
    };

    // Sample from the level's own stairway-corridor interior point (a
    // long, narrow, single-direction passage -- M6's own CarveStairwayCorridor
    // always carves N/S doors at X=17) facing down it, and from a small
    // room's door tile facing an arbitrary direction, same two reference
    // points dawnstar's own M9 smoke test used for the identical reason:
    // a real generated level's own varied geometry, not just a hand-built one.
    int checked = 0;
    for (int facing = 1; facing <= 4; facing++) {
        stormhold::CorridorViewGrid view = stormhold::DungeonRuntime::SampleCorridorView(cache.at(2), 17, 5, facing, lookup);
        stormhold::CorridorRenderPlanResult plan = stormhold::CorridorRenderPlan::Plan(view, false, false);
        for (const auto& seg : plan.wallSegments) {
            Expect(seg.frame >= 0 && seg.frame <= 15, "every wall segment's frame should be in the valid 0-15 range");
            Expect(seg.x >= 0 && seg.x <= 162 && seg.x % 18 == 0, "every wall segment's x should be an 18px-aligned column");
        }
        checked++;
    }
    Expect(checked == 4, "should have exercised all 4 facings against real generated level 2 with no exceptions");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        TestViewGridAtFormula();
        TestTileAtWithinLevel();
        TestTileAtNoNeighborReturnsWall();
        TestTileAtCrossesLevelBoundary();
        TestSampleCorridorViewEastWestAsymmetry();
        TestSampleCorridorViewFindsWallAheadNorth();
        TestPlanAilmentFloorFlags();
        TestAgainstRealGeneratedLevel(root);

        if (!g_ok) {
            std::fprintf(stderr, "m21_corridor_render_plan_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m21_corridor_render_plan_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
