// M32 smoke test: DungeonRuntime::SampleSquareView -- the shared
// implementation behind Dungeon.sampleSquareView7()/17() (the minimap's
// own populate step, GameCanvas's now-confirmed populateMinimapGrid()/
// populateVisibleGrid(), ../../../src/GameCanvas.java), reverse-engineered
// and transcribed directly from decompiled/i.java's own byte[][]
// a(int,int,int,int,byte[][]) this session.
//
// No JVM ground truth possible (same reasoning every other rendering/
// selection milestone's own tests already give) -- verified via:
//  - Wall/special-bit sampling (TileAt's & 1, falling back to & 8),
//    cross-checked against TileAt() called directly at the same
//    computed offset, same discipline M25's own real-integration checks
//    already use.
//  - Monster/chest/dropped-item overlay bits (2/4), for BOTH the
//    facing==1/3 and facing==2/4 index-math branches, each independently
//    re-derived from decompiled/i.java's own literal formulas, not read
//    back from dungeon_runtime.cpp.
//  - The dropped-item visibility gate (d[6]&1) and the monster
//    unconfirmedFlag gate.
//  - The hub-town (levelNumber==1) branch: HubMinimapMarkers overlay,
//    the Warden-slot (index 6) skip-unless-present gate, and a nullptr
//    hubMarkers skipping the overlay entirely without crashing.
#include <cstdio>
#include <map>

#include "dungeon/dungeon_runtime.h"

namespace {

using namespace stormhold;

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

GeneratedLevel MakeOpenLevel(int number, int width = 35, int height = 35) {
    GeneratedLevel level;
    level.number = number;
    level.width = width;
    level.height = height;
    level.tiles.assign(static_cast<size_t>(width), std::vector<uint8_t>(static_cast<size_t>(height), 0));
    level.populated = true;
    return level;
}

void TestWallBitSampling() {
    std::printf("-- SampleSquareView: wall/special-bit sampling matches TileAt directly --\n");
    GeneratedLevel level = MakeOpenLevel(5);
    level.tiles[17][15] = 1;   // a wall tile
    level.tiles[18][15] = 8;   // a "special" (bit 8) tile, no wall bit
    level.tiles[19][15] = 9;   // both bits set -- wall bit wins (matches original: & 1 checked first)
    std::map<int, GeneratedLevel> cache;
    cache[5] = level;
    DungeonRuntime::LevelLookup lookup = [&](int n) -> const GeneratedLevel& { return cache.at(n); };
    WorldRegistry world(37);

    // facing 1 (east), centered at (17,15) -- so the wall tile lands
    // dead center (col=half,row=half), the bit-8 tile one column east,
    // the both-bits tile two columns east.
    SquareViewGrid grid = DungeonRuntime::SampleSquareView(level, world, 17, 15, 1, 7, lookup);
    Expect(grid.size() == 7 && grid[0].size() == 7, "a size=7 request should produce a 7x7 grid");
    Expect(grid[3][3] == 1, "the wall tile should sample as bit 1 at dead center");
    Expect(grid[4][3] == 8, "a bit-8-only tile should sample as 8 (the & 1 -> 0 -> & 8 fallback)");
    Expect(grid[5][3] == 1, "a tile with BOTH bits set should sample as 1 -- & 1 is checked first and wins");
    Expect(grid[0][0] == 0, "a plain open tile should sample as 0");
}

void TestMonsterOverlayBothFacingBranches() {
    std::printf("-- SampleSquareView: monster overlay (bit 2), both index-math branches --\n");
    GeneratedLevel level = MakeOpenLevel(6);
    std::map<int, GeneratedLevel> cache;
    cache[6] = level;
    DungeonRuntime::LevelLookup lookup = [&](int n) -> const GeneratedLevel& { return cache.at(n); };
    WorldRegistry world(37);

    // Hand-built rather than MonsterRuntime::Spawn() -- Spawn() only
    // needs a real MonsterDatabase to read starting HP from, which this
    // test (position/flag overlay only) doesn't care about at all.
    MonsterState m;
    m.spawnId = 1;
    m.dungeonLevel = 6;
    m.tileX = 12;
    m.tileY = 12;
    m.unconfirmedFlag = true;
    DungeonRuntime::StoreMonster(world, m);

    // facing 1 (step=+1): col = (mx-x)+half, row = (my-y)+half.
    {
        SquareViewGrid grid = DungeonRuntime::SampleSquareView(level, world, 10, 12, 1, 7, lookup);
        Expect((grid[2 + 3][0 + 3] & 2) != 0, "facing 1: monster 2 tiles east should land at col=half+2,row=half");
    }
    // facing 2 (step=+1): col = (my-y)+half, row = half-(mx-x).
    {
        SquareViewGrid grid = DungeonRuntime::SampleSquareView(level, world, 10, 10, 2, 7, lookup);
        // dx=(12-10)=2, dy=(12-10)=2 -> col=2+3=5, row=3-2=1
        Expect((grid[5][1] & 2) != 0, "facing 2: monster overlay should land at the facing-2 index formula's own position");
    }

    // unconfirmedFlag == false should suppress the overlay entirely.
    m.unconfirmedFlag = false;
    WorldRegistry world2(37);
    DungeonRuntime::StoreMonster(world2, m);
    SquareViewGrid grid2 = DungeonRuntime::SampleSquareView(level, world2, 10, 12, 1, 7, lookup);
    Expect((grid2[5][3] & 2) == 0, "a monster with unconfirmedFlag == false should NOT be drawn");
}

void TestChestAndDroppedItemOverlay() {
    std::printf("-- SampleSquareView: chest (bit 4) and dropped-item (bit 4, visibility-gated) overlay --\n");
    GeneratedLevel level = MakeOpenLevel(7);
    std::map<int, GeneratedLevel> cache;
    cache[7] = level;
    DungeonRuntime::LevelLookup lookup = [&](int n) -> const GeneratedLevel& { return cache.at(n); };
    WorldRegistry world(37);

    std::array<int8_t, 8> chest = {13, 12, 0, 5, 0, 0, 1, 0};  // x=13,y=12
    DungeonRuntime::StoreChest(level, world, chest);

    std::array<int8_t, 7> visibleItem = {11, 12, 0, 0, 0, 0, 1};    // bit0 of [6] set -- visible
    std::array<int8_t, 7> hiddenItem = {9, 12, 0, 0, 0, 0, 0};      // bit0 clear -- NOT visible
    DungeonRuntime::AddDroppedItem(level, world, visibleItem);
    DungeonRuntime::AddDroppedItem(level, world, hiddenItem);

    // Player at (10,12), facing 1 (step=+1, half=3): col = (obj.x-10)+3.
    SquareViewGrid grid = DungeonRuntime::SampleSquareView(level, world, 10, 12, 1, 7, lookup);
    Expect((grid[6][3] & 4) != 0, "the chest (dx=+3) should draw at its own facing-1 offset");
    Expect((grid[4][3] & 4) != 0, "the VISIBLE dropped item (dx=+1) should draw at its own facing-1 offset");
    Expect((grid[2][3] & 4) == 0, "the HIDDEN (visibility bit clear) dropped item (dx=-1) should NOT draw");
}

void TestHubTownMarkerOverlay() {
    std::printf("-- SampleSquareView: hub-town (levelNumber==1) HubMinimapMarkers overlay --\n");
    GeneratedLevel hub = MakeOpenLevel(1, 19, 19);
    std::map<int, GeneratedLevel> cache;
    cache[1] = hub;
    DungeonRuntime::LevelLookup lookup = [&](int n) -> const GeneratedLevel& { return cache.at(n); };
    WorldRegistry world(37);

    HubMinimapMarkers markers;
    markers.questRewardClaimable[0] = true;  // shop 0: (12, 3)

    // Centered on shop 0's own position -- lands dead center regardless
    // of facing, easiest to check unambiguously.
    SquareViewGrid grid = DungeonRuntime::SampleSquareView(hub, world, kShopMarkerX[0], kShopMarkerY[0], 1, 7,
                                                            lookup, &markers);
    Expect((grid[3][3] & 4) != 0, "shop 0's own marker should draw at dead center when questRewardClaimable[0]");

    // Varus (index 6) should be skipped unless wardenPresent, even with
    // questRewardClaimable[6] set.
    HubMinimapMarkers varusMarkers;
    varusMarkers.questRewardClaimable[6] = true;
    varusMarkers.wardenPresent = false;
    SquareViewGrid gridNoWarden = DungeonRuntime::SampleSquareView(hub, world, kShopMarkerX[6], kShopMarkerY[6], 1, 7,
                                                                    lookup, &varusMarkers);
    Expect((gridNoWarden[3][3] & 4) == 0, "Varus's own marker should be skipped while wardenPresent is false");

    varusMarkers.wardenPresent = true;
    SquareViewGrid gridWithWarden = DungeonRuntime::SampleSquareView(hub, world, kShopMarkerX[6], kShopMarkerY[6], 1,
                                                                      7, lookup, &varusMarkers);
    Expect((gridWithWarden[3][3] & 4) != 0, "Varus's own marker should draw once wardenPresent is true");

    // A null hubMarkers should skip the overlay entirely (still samples
    // wall bits fine) rather than crash -- this port's own documented
    // "no live Shop model yet" gap.
    SquareViewGrid gridNoMarkers =
        DungeonRuntime::SampleSquareView(hub, world, kShopMarkerX[0], kShopMarkerY[0], 1, 7, lookup, nullptr);
    Expect((gridNoMarkers[3][3] & 4) == 0, "a null hubMarkers should draw no markers at all, without crashing");
}

}  // namespace

int main() {
    TestWallBitSampling();
    TestMonsterOverlayBothFacingBranches();
    TestChestAndDroppedItemOverlay();
    TestHubTownMarkerOverlay();

    if (!g_ok) {
        std::fprintf(stderr, "m32_minimap_sample_smoke: FAILED\n");
        return 1;
    }

    std::printf("all checks passed\n");
    return 0;
}
