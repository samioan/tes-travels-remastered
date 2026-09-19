// M25 smoke test: PlayerState::corridorView actually wired through
// PlayerMovement::CommitMove (via the new PlayerMovement::
// RefreshCorridorView), and GameRenderer::RenderCorridorView -- the first
// real end-to-end pixel render this port has, combining M21's
// CorridorRenderPlan selection logic with M23/M24's Backbuffer::Blit()
// compositors against a real Backbuffer, rather than each being verified
// only in isolation.
//
// Same "no JVM ground truth possible here" reasoning M6/M21's own tests
// already give for anything touching procedural generation -- verified
// via hand-built synthetic scenarios for exact pixel-level checks (the
// floor-tiling/fallback-fill/ailment-3-skip branches, using synthetic
// DecodedImages same as M24's own MakeImage helper), plus real
// floor3.png/newwallsnok.png blitted against a real M6-generated level
// for an integration check: every OPAQUE pixel a real wall segment or
// floor tile draws is independently recomputed here (position + mirror
// math, same formula render/game_renderer.cpp itself uses) and compared
// against DecodedImage's own R/G/B/A accessors directly -- the same
// "cross-check against the primitive's own rules" discipline M23/M24's
// real-asset tests already used, just one level up through GameRenderer
// instead of straight through Blit().
#include <algorithm>
#include <array>
#include <cstdio>
#include <map>
#include <string>

#include "assets/asset_root.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "dungeon/dungeon_runtime.h"
#include "player/player_movement.h"
#include "render/game_renderer.h"
#include "world/dungeon_generator.h"

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

GeneratedLevel MakeLevel(int number, int width = 35, int height = 35) {
    GeneratedLevel level;
    level.number = number;
    level.width = width;
    level.height = height;
    level.tiles.assign(static_cast<size_t>(width), std::vector<uint8_t>(static_cast<size_t>(height), 0));
    return level;
}

DecodedImage MakeImage(int width, int height, std::initializer_list<uint8_t> rgba) {
    DecodedImage img;
    img.width = width;
    img.height = height;
    img.pixels = rgba;
    return img;
}

void TestCommitMoveRefreshesCorridorView() {
    std::printf("-- CommitMove wires PlayerMovement::RefreshCorridorView --\n");
    GeneratedLevel level = MakeLevel(9);
    level.tiles[17][15] = 1;
    std::map<int, GeneratedLevel> cache;
    cache[9] = level;
    PlayerMovement::LevelLookup lookup = [&](int n) -> GeneratedLevel& { return cache.at(n); };
    DungeonRuntime::LevelLookup constLookup = [&](int n) -> const GeneratedLevel& { return cache.at(n); };
    WorldRegistry world(37);
    MonsterDatabase monsters{};
    WardenState warden{};
    ItemDatabase items{};

    PlayerState p;
    p.currentLevel = 9;
    p.tileX = 17;
    p.tileY = 17;
    p.facing = 1;
    p.coreStats[6] = 100;

    Expect(p.corridorView == (std::array<std::array<uint8_t, 5>, 9>{}),
           "a fresh PlayerState's corridorView should start all-zero");

    bool moved = PlayerMovement::CommitMove(p, 1, lookup, world, items, monsters, warden);
    Expect(moved, "a plain forward step in an open level should succeed");

    CorridorViewGrid expected = DungeonRuntime::SampleCorridorView(cache.at(9), p.tileX, p.tileY, p.facing, constLookup);
    Expect(p.corridorView == expected,
           "CommitMove should leave p.corridorView exactly matching a fresh SampleCorridorView at the landed "
           "position/facing");
}

void TestLockedItemEarlyReturnStillRefreshesCorridorView(const ItemDatabase& items) {
    std::printf("-- locked dropped item's early return still refreshes corridorView --\n");
    GeneratedLevel level = MakeLevel(10);
    WorldRegistry world(37);
    MonsterDatabase monsters{};
    WardenState warden{};
    std::array<int8_t, 7> record = {10, 10, 1, 0, 0, 0, 5};  // bit4 (locked) | bit0
    DungeonRuntime::AddDroppedItem(level, world, record);

    std::map<int, GeneratedLevel> cache;
    cache[10] = level;
    PlayerMovement::LevelLookup lookup = [&](int n) -> GeneratedLevel& { return cache.at(n); };
    DungeonRuntime::LevelLookup constLookup = [&](int n) -> const GeneratedLevel& { return cache.at(n); };

    PlayerState p;
    p.currentLevel = 10;
    p.tileX = 9;
    p.tileY = 10;
    p.facing = 2;
    p.coreStats[6] = 100;

    bool moved = PlayerMovement::CommitMove(p, 1, lookup, world, items, monsters, warden);
    Expect(moved, "CommitMove should still return true on the locked-item early return");
    Expect(p.pendingLockedItemFlag, "pendingLockedItemFlag should be set");

    CorridorViewGrid expected = DungeonRuntime::SampleCorridorView(cache.at(10), p.tileX, p.tileY, p.facing, constLookup);
    Expect(p.corridorView == expected,
           "the locked-item early return should ALSO have refreshed corridorView -- Player.commitMove()'s own "
           "refreshCorridorView() call happens before this early return, not skipped by it");
}

void TestRenderCorridorViewFloorTilingAndFallback() {
    std::printf("-- RenderCorridorView: synthetic floor tiling / fallback fill / ailment-3 skip --\n");
    CorridorAssets assets;
    assets.floorTexture = MakeImage(4, 2,
                                     {
                                         255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255,
                                         255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255,
                                     });
    assets.wallTexture = MakeImage(1, 1, {0, 0, 0, 0});  // never drawn -- openView below has no wall segments

    CorridorViewGrid openView{};  // all-open -- M21's own confirmed "zero wall segments" shape
    uint16_t sentinel = PackRGB565(9, 9, 9);

    {
        Backbuffer bb;
        bb.Fill(sentinel);
        GameRenderer::RenderCorridorView(bb, assets, openView, false, false);
        for (int col = 0; col < 5; col++) {
            Expect(bb.Data()[col * 36] == PackRGB565(255, 0, 0),
                   "each of the 5 floor tiles' own top-left pixel should be the floor color");
        }
        Expect(bb.Data()[10] == sentinel,
               "a column between two floor tiles (past the 4px-wide synthetic texture) should stay untouched");
    }

    {
        Backbuffer bb;
        bb.Fill(sentinel);
        GameRenderer::RenderCorridorView(bb, assets, openView, false, true);
        Expect(bb.Data()[5] == PackRGB565(160, 0, 0), "ailment 4 should fill with the dark-red fallback color (0xA00000)");
        Expect(bb.Data()[Backbuffer::kWidth + 5] == PackRGB565(160, 0, 0),
               "the fallback fill should cover every row within floorTexture.height (2 here)");
        Expect(bb.Data()[2 * Backbuffer::kWidth + 5] == sentinel,
               "the fallback fill should stop exactly at floorTexture.height, not cover the whole screen");
    }

    {
        Backbuffer bb;
        bb.Fill(sentinel);
        GameRenderer::RenderCorridorView(bb, assets, openView, true, false);
        bool allSentinel = true;
        for (int i = 0; i < Backbuffer::kWidth * Backbuffer::kHeight; i++) {
            if (bb.Data()[i] != sentinel) {
                allSentinel = false;
                break;
            }
        }
        Expect(allSentinel, "ailment 3 with an all-open view should leave the backbuffer completely untouched");
    }
}

void TestRealFloorTilingIntegration(const std::string& root) {
    std::printf("-- integration: real floor3.png tiling, no walls (synthetic open view) --\n");
    AssetRoot assetRoot(root);
    CorridorAssets assets = CorridorAssets::Load(assetRoot);
    CorridorViewGrid openView{};

    Backbuffer bb;
    uint16_t sentinel = PackRGB565(3, 3, 3);
    bb.Fill(sentinel);
    GameRenderer::RenderCorridorView(bb, assets, openView, false, false);

    bool mismatch = false;
    int checked = 0;
    int maxY = std::min(assets.floorTexture.height, Backbuffer::kHeight);
    for (int col = 0; col < 5; col++) {
        for (int y = 0; y < maxY; y++) {
            for (int x = 0; x < assets.floorTexture.width; x++) {
                int screenX = col * 36 + x;
                if (screenX >= Backbuffer::kWidth) continue;
                uint16_t expected = (assets.floorTexture.A(x, y) == 0)
                                         ? sentinel
                                         : PackRGB565(assets.floorTexture.R(x, y), assets.floorTexture.G(x, y),
                                                      assets.floorTexture.B(x, y));
                uint16_t actual = bb.Data()[y * Backbuffer::kWidth + screenX];
                if (actual != expected) mismatch = true;
                checked++;
            }
        }
    }
    Expect(!mismatch, "every floor-tile pixel across all 5 tiled copies should match floor3.png's own R/G/B exactly");
    Expect(checked > 0, "should have checked a nonzero number of real floor pixels");
    std::printf("  checked %d real floor pixels across 5 tiles, all matched=%s\n", checked, mismatch ? "false" : "true");
}

void TestRealWallSegmentIntegration(const std::string& root) {
    std::printf("-- integration: real newwallsnok.png against a real M6-generated level --\n");
    AssetRoot assetRoot(root);
    CorridorAssets assets = CorridorAssets::Load(assetRoot);
    ItemDatabase items = ItemDatabase::Load(assetRoot);
    MonsterDatabase monsters = MonsterDatabase::Load(assetRoot);
    DungeonGeometry geometry = DungeonGeometry::Load(assetRoot);

    GeneratedLevel level = DungeonGenerator::PopulateLevel(2, geometry.rows[1], items, monsters);
    std::map<int, GeneratedLevel> cache;
    cache[2] = level;
    DungeonRuntime::LevelLookup lookup = [&](int n) -> const GeneratedLevel& { return cache.at(n); };

    bool mismatch = false;
    bool sawPlainFrame = false;
    bool sawMirroredFrame = false;
    int opaqueChecked = 0;

    for (int facing = 1; facing <= 4; facing++) {
        CorridorViewGrid view = DungeonRuntime::SampleCorridorView(cache.at(2), 17, 5, facing, lookup);
        Backbuffer bb;
        bb.Fill(PackRGB565(3, 3, 3));
        GameRenderer::RenderCorridorView(bb, assets, view, false, false);

        CorridorRenderPlanResult plan = CorridorRenderPlan::Plan(view, false, false);
        int rowEnd = std::min(assets.wallTexture.height, Backbuffer::kHeight);
        for (const WallDrawCall& seg : plan.wallSegments) {
            bool mirrored = seg.frame > 7;
            int usedFrame = mirrored ? seg.frame - 8 : seg.frame;
            int blitOriginX = seg.x - usedFrame * 18;
            if (mirrored) {
                sawMirroredFrame = true;
            } else {
                sawPlainFrame = true;
            }

            int colStart = std::max(seg.x, 0);
            int colEnd = std::min(seg.x + 18, Backbuffer::kWidth);
            for (int col = colStart; col < colEnd; col++) {
                int sx = col - blitOriginX;
                int srcX = mirrored ? (assets.wallTexture.width - 1 - sx) : sx;
                if (srcX < 0 || srcX >= assets.wallTexture.width) continue;
                for (int row = 0; row < rowEnd; row++) {
                    if (assets.wallTexture.A(srcX, row) == 0) continue;
                    uint16_t expected =
                        PackRGB565(assets.wallTexture.R(srcX, row), assets.wallTexture.G(srcX, row), assets.wallTexture.B(srcX, row));
                    uint16_t actual = bb.Data()[row * Backbuffer::kWidth + col];
                    if (actual != expected) mismatch = true;
                    opaqueChecked++;
                }
            }
        }
    }

    Expect(!mismatch, "every real wall segment's opaque pixel should match newwallsnok.png's own R/G/B exactly");
    Expect(opaqueChecked > 0, "should have checked at least one real opaque wall pixel across all 4 facings");
    std::printf("  checked %d real opaque wall pixels across 4 facings (plain frame seen=%s, mirrored frame seen=%s)\n",
                opaqueChecked, sawPlainFrame ? "true" : "false", sawMirroredFrame ? "true" : "false");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        ItemDatabase items = ItemDatabase::Load(AssetRoot(root));

        TestCommitMoveRefreshesCorridorView();
        TestLockedItemEarlyReturnStillRefreshesCorridorView(items);
        TestRenderCorridorViewFloorTilingAndFallback();
        TestRealFloorTilingIntegration(root);
        TestRealWallSegmentIntegration(root);

        if (!g_ok) {
            std::fprintf(stderr, "m25_game_renderer_smoke: FAILED\n");
            return 1;
        }

        std::printf("all checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m25_game_renderer_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
