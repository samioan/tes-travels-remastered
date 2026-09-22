// M29 smoke test: the minimap (dungeon/dungeon_runtime.h's
// SampleSquareView, render/minimap_renderer.h's MinimapSurface/
// MinimapRenderer) -- GameCanvas.sampleSquareView()/refreshMinimap()/
// paintMinimapGrid()/paintGameView()'s own compositing step. No JVM
// ground truth available (same reason as every prior milestone) --
// verified via:
//  - Pure arithmetic/primitive checks of MinimapSurface itself (no game
//    data needed).
//  - Two real, independently re-derived bugs in GameCanvas.
//    paintMinimapGrid's own background-fill/border-outline size
//    computation (a classic Java `<<` vs `+` precedence trap), proven
//    with a pre-dirtied surface so the exact reach of each buggy fill is
//    directly observable rather than indistinguishable from "untouched".
//  - DungeonRuntime::SampleSquareView against the real 37-level
//    generated world (M6/M24), using known real chest/no-spawn-room/
//    monster positions centered in the sample window (the center cell
//    always maps to the sampled position itself regardless of facing,
//    so no col/row math is needed to predict where to look).
//  - MinimapRenderer::Composite's ailment-3 visibility gate and its
//    zoomed-in-clipped vs. zoomed-out-full draw.
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "dungeon/dungeon_runtime.h"
#include "graphics/backbuffer.h"
#include "graphics/bitmap_font.h"
#include "monster/monster_runtime.h"
#include "player/player_state.h"
#include "render/minimap_renderer.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::Backbuffer;
using dawnstar::DungeonRuntime;
using dawnstar::GeneratedLevel;
using dawnstar::ItemDatabase;
using dawnstar::MinimapRenderer;
using dawnstar::MinimapSurface;
using dawnstar::MonsterDatabase;
using dawnstar::MonsterRuntime;
using dawnstar::MonsterState;
using dawnstar::PackRGB565;
using dawnstar::PlayerState;
using dawnstar::WorldRegistry;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

uint16_t PixelAt(const std::vector<uint16_t>& pixels, int x, int y) {
    return pixels[static_cast<size_t>(y) * MinimapSurface::kSize + x];
}

uint16_t BbPixelAt(const Backbuffer& bb, int x, int y) { return bb.Data()[static_cast<size_t>(y) * Backbuffer::kWidth + x]; }

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        ItemDatabase items = ItemDatabase::Load(archive);
        MonsterDatabase monsterDb = MonsterDatabase::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);

        std::vector<GeneratedLevel> levels;
        WorldRegistry world(geometry.rows.size());
        for (size_t i = 0; i < geometry.rows.size(); i++) {
            int levelNumber = static_cast<int>(i) + 1;
            levels.push_back(levelNumber == 1
                                  ? dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[0])
                                  : dawnstar::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[i], items,
                                                                               monsterDb));
            DungeonRuntime::RegisterGeneratedSpawns(levels.back(), world);
        }
        std::printf("generated + registered %zu levels\n", levels.size());

        constexpr uint16_t kWhite = PackRGB565(255, 255, 255);
        constexpr uint16_t kBlack = PackRGB565(0, 0, 0);
        constexpr uint16_t kMarker = PackRGB565(1, 2, 3);

        // --- A: MinimapSurface primitives, no game data needed ---
        {
            MinimapSurface s;
            s.FillRect(-10, -10, 30, 30, kWhite);
            Check(PixelAt(s.Pixels(), 0, 0) == kWhite, "FillRect should clip a negative-origin rect to the surface");
            Check(PixelAt(s.Pixels(), 19, 19) == kWhite, "FillRect's clipped rect should still cover its in-bounds part");
            Check(PixelAt(s.Pixels(), 20, 20) == 0, "FillRect should not paint past its own (clipped) extent");

            MinimapSurface s2;
            s2.DrawRect(2, 2, 10, 10, kWhite);
            Check(PixelAt(s2.Pixels(), 2, 2) == kWhite, "DrawRect's top-left corner");
            Check(PixelAt(s2.Pixels(), 12, 2) == kWhite, "DrawRect's top-right corner (inclusive: x+w)");
            Check(PixelAt(s2.Pixels(), 2, 12) == kWhite, "DrawRect's bottom-left corner (inclusive: y+h)");
            Check(PixelAt(s2.Pixels(), 12, 12) == kWhite, "DrawRect's bottom-right corner");
            Check(PixelAt(s2.Pixels(), 7, 7) == 0, "DrawRect should not fill its own interior");

            MinimapSurface s3;
            s3.FillRect(0, 0, MinimapSurface::kSize, MinimapSurface::kSize, kMarker);
            std::vector<uint16_t> before = s3.Pixels();
            // Java's `<<-1` on GameCanvas.paintMinimapGrid's real
            // base=87 (see minimap_renderer.cpp's own JavaShiftLeft doc
            // comment) masks the shift to `<<31`, which for an odd value
            // like 87 comes out to exactly Integer.MIN_VALUE --
            // independently re-derived here, not read back from the
            // implementation.
            s3.DrawRect(1, 1, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), kWhite);
            Check(s3.Pixels() == before, "DrawRect with an INT32_MIN dimension should be a true no-op");
        }

        // --- B: paintMinimapGrid's real fillRect/drawRect size bugs,
        // via a real Refresh() (border=1, zoomed-in path: gridSize=7,
        // cellSize=3, border=1 -> base=22) ---
        {
            MinimapSurface surf;
            surf.FillRect(0, 0, MinimapSurface::kSize, MinimapSurface::kSize, kMarker);

            PlayerState p;
            p.currentLevel = 2;
            p.tileX = levels[1].width / 2;
            p.tileY = levels[1].height / 2;
            p.facing = 1;
            p.minimapZoomedOut = false;

            MinimapRenderer::Refresh(surf, p, levels, world);
            const std::vector<uint16_t>& pixels = surf.Pixels();

            // Bug 1: `gridSize*cellSize+border << 1` parses (Java `<<`
            // binds looser than `+`) as `(22)<<1=44`, not the
            // "add border on both sides" 22*3+... i.e. NOT the 23
            // intended by the "+border" naming -- the background fill
            // really does reach (35,35) (inside the buggy 44x44 region,
            // outside any 23x23 "intended" one), erasing the pre-set
            // marker there, while (60,60) -- outside even the buggy
            // 44x44 -- stays untouched.
            Check(PixelAt(pixels, 35, 35) == kBlack,
                  "the oversized (44x44, not 22x22) background fill should reach (35,35)");
            Check(PixelAt(pixels, 60, 60) == kMarker,
                  "the oversized background fill is still bounded -- (60,60) should stay untouched");

            // Bug 2: the very next drawRect's size uses `<<0` (a no-op),
            // so the white outer border is exactly `base`=22 pixels
            // wide/tall (edges at x/y=0 and 22, DrawRect's own
            // inclusive-corner convention) instead of the "obviously
            // intended" 23 (gridSize*cellSize + 2*border) a correctly
            // doubled border would have produced.
            Check(PixelAt(pixels, 22, 10) == kWhite, "the buggy (22-wide) border's own right edge");
            Check(PixelAt(pixels, 23, 10) != kWhite,
                  "a correctly-computed 23-wide border would have its edge one pixel further -- confirms the "
                  "off-by-border shortfall");
        }

        // --- C: DungeonRuntime::SampleSquareView against real
        // generated data -- the sample's CENTER cell always maps back
        // to the sampled (x,y) itself regardless of `direction` (the
        // (col-half)/(row-half) terms are both 0 there), so centering
        // the sample directly on a known real position needs no
        // col/row/facing math to predict. ---
        {
            std::array<std::array<uint8_t, 17>, 17> grid{};

            // A real chest: tile bit 16 set (by DungeonGenerator itself,
            // per dungeon_runtime.h's own RegisterGeneratedSpawns doc
            // comment), bits 2/4/32 clear at a chest tile -- takes the
            // "else" branch (`value |= 4`), landing on the SAME visible
            // minimap category (bit 4, blue) sampleSquareView also uses
            // for tile bit 32 ("special room" marker) or an unseen
            // monster's own raw bit 2 -- a real, intentional
            // conflation, not something this test invents.
            size_t chestLevelIndex = 0;
            bool foundChest = false;
            const GeneratedLevel* chestLevel = nullptr;
            dawnstar::GeneratedChestSpawn chestSpawn;
            for (size_t i = 1; i < levels.size() && !foundChest; i++) {
                if (!levels[i].chests.empty()) {
                    chestLevelIndex = i;
                    chestLevel = &levels[i];
                    chestSpawn = levels[i].chests[0];
                    foundChest = true;
                }
            }
            Check(foundChest, "should find at least one real generated chest");
            if (foundChest) {
                DungeonRuntime::SampleSquareView(levels, static_cast<int>(chestLevelIndex), chestSpawn.x, chestSpawn.y,
                                                  1, 7, world, grid);
                Check(grid[3][3] == 4, "a real chest tile should sample as minimap category 4 (blue)");
                (void)chestLevel;
            }

            // A real "no-spawn special room" tile: bit 8 set, bits
            // 2/4/16/32 clear (world/dungeon_generator.cpp marks this on
            // every non-key level's own special room) -- takes the
            // `value = tile & 8` branch cleanly.
            bool foundNoSpawn = false;
            int nsX = 0, nsY = 0;
            size_t nsLevelIndex = 0;
            for (size_t i = 1; i < levels.size() && !foundNoSpawn; i++) {
                const GeneratedLevel& lvl = levels[i];
                for (int x = 0; x < lvl.width && !foundNoSpawn; x++) {
                    for (int y = 0; y < lvl.height && !foundNoSpawn; y++) {
                        uint8_t t = lvl.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)];
                        if ((t & 8) != 0 && (t & (2 | 4 | 16 | 32)) == 0) {
                            nsX = x;
                            nsY = y;
                            nsLevelIndex = i;
                            foundNoSpawn = true;
                        }
                    }
                }
            }
            Check(foundNoSpawn, "should find at least one real no-spawn special-room tile");
            if (foundNoSpawn) {
                DungeonRuntime::SampleSquareView(levels, static_cast<int>(nsLevelIndex), nsX, nsY, 1, 7, world, grid);
                Check(grid[3][3] == 8, "a real no-spawn special-room tile should sample as minimap category 8");
            }

            // A real, marked-seen monster: temporarily flip its own
            // registered record's `flag` (mirroring what M25's
            // MarkLooted does the first tick a monster becomes visible)
            // so sampleSquareView's own monster-flag check finds it.
            bool foundMonster = false;
            int mX = 0, mY = 0;
            size_t mLevelIndex = 0;
            for (size_t i = 1; i < levels.size() && !foundMonster; i++) {
                if (!levels[i].monsters.empty()) {
                    mLevelIndex = i;
                    mX = levels[i].monsters[0].x;
                    mY = levels[i].monsters[0].y;
                    foundMonster = true;
                }
            }
            Check(foundMonster, "should find at least one real generated monster");
            if (foundMonster) {
                auto& monsterMap = world.monsters[mLevelIndex];
                auto it = monsterMap.find(dawnstar::PackPosKey(mX, mY));
                Check(it != monsterMap.end(), "the chosen monster should be registered");
                if (it != monsterMap.end()) {
                    MonsterState m = MonsterRuntime::FromBytes(it->second);
                    m.flag = true;
                    it->second = MonsterRuntime::ToBytes(m);

                    DungeonRuntime::SampleSquareView(levels, static_cast<int>(mLevelIndex), mX, mY, 1, 7, world, grid);
                    Check((grid[3][3] & 2) != 0, "a real seen monster should sample with minimap bit 2 (red) set");
                }
            }
        }

        // --- D: MinimapRenderer::Composite -- the ailment-3 visibility
        // gate, and the zoomed-in-clipped vs. zoomed-out-full draw ---
        {
            MinimapSurface surf;
            surf.FillRect(0, 0, MinimapSurface::kSize, MinimapSurface::kSize, kMarker);

            PlayerState p;
            p.ailmentMask = static_cast<int8_t>(1 << (3 - 1));  // ailment 3 active
            Backbuffer bb;
            bb.Fill(0);
            MinimapRenderer::Composite(bb, surf, p);
            bool anyDrawn = false;
            for (int y = 0; y < Backbuffer::kHeight && !anyDrawn; y++) {
                for (int x = 0; x < Backbuffer::kWidth; x++) {
                    if (BbPixelAt(bb, x, y) != 0) {
                        anyDrawn = true;
                        break;
                    }
                }
            }
            Check(!anyDrawn, "ailment 3 active should hide the minimap entirely");

            p.ailmentMask = 0;
            p.minimapZoomedOut = false;
            Backbuffer bbZoomedIn;
            bbZoomedIn.Fill(0);
            MinimapRenderer::Composite(bbZoomedIn, surf, p);
            Check(BbPixelAt(bbZoomedIn, 10, 20) == kMarker, "zoomed-in composite's top-left corner (10,20)");
            Check(BbPixelAt(bbZoomedIn, 10 + 22, 20 + 22) == kMarker,
                  "zoomed-in composite's last visible pixel of its 23x23 clip window");
            Check(BbPixelAt(bbZoomedIn, 10 + 23, 20) == 0,
                  "zoomed-in composite should not draw past its own 23x23 clip window");

            p.minimapZoomedOut = true;
            Backbuffer bbZoomedOut;
            bbZoomedOut.Fill(0);
            MinimapRenderer::Composite(bbZoomedOut, surf, p);
            Check(BbPixelAt(bbZoomedOut, 15, 25) == kMarker, "zoomed-out composite's top-left corner (15,25)");
            Check(BbPixelAt(bbZoomedOut, 15 + 88, 25 + 88) == kMarker,
                  "zoomed-out composite should draw the surface's full 89x89 extent, unclipped");
        }

        // --- E: M55's compass glyph (COMPASS_GLYPHS[player.facing]) --
        // drawn at its own real fixed position in both zoom states, and
        // skipped along with the rest of the minimap under ailment 3.
        // Checked via BitmapFont's own StringWidth/glyph box rather than
        // a specific hand-picked lit pixel, since the exact glyph shape
        // is this port's own invented font, not something to hardcode a
        // pixel-perfect expectation for. ---
        {
            auto anyWhiteInBox = [](const Backbuffer& bb, int x0, int y0, uint16_t white) {
                for (int y = y0; y < y0 + dawnstar::BitmapFont::kGlyphHeight; y++) {
                    for (int x = x0; x < x0 + dawnstar::BitmapFont::kGlyphWidth; x++) {
                        if (BbPixelAt(bb, x, y) == white) return true;
                    }
                }
                return false;
            };

            MinimapSurface surf;  // left black -- isolates the glyph from the minimap image itself

            PlayerState p;
            p.facing = 2;  // 'E' -- COMPASS_GLYPHS[2]
            p.minimapZoomedOut = false;
            Backbuffer bbZoomedIn;
            bbZoomedIn.Fill(0);
            MinimapRenderer::Composite(bbZoomedIn, surf, p);
            Check(anyWhiteInBox(bbZoomedIn, 16, 10, kWhite), "zoomed-in: the compass glyph should draw at (16,10)");

            p.minimapZoomedOut = true;
            Backbuffer bbZoomedOut;
            bbZoomedOut.Fill(0);
            MinimapRenderer::Composite(bbZoomedOut, surf, p);
            Check(anyWhiteInBox(bbZoomedOut, 58, 10, kWhite),
                  "zoomed-out: the compass glyph should draw at its own (58,10) position, not (16,10)");
            Check(!anyWhiteInBox(bbZoomedOut, 16, 10, kWhite),
                  "zoomed-out: nothing should be drawn at the zoomed-in glyph position");

            p.ailmentMask = static_cast<int8_t>(1 << (3 - 1));  // ailment 3 active
            Backbuffer bbBlind;
            bbBlind.Fill(0);
            MinimapRenderer::Composite(bbBlind, surf, p);
            Check(!anyWhiteInBox(bbBlind, 58, 10, kWhite), "ailment 3 active should hide the compass glyph too");
        }

        if (g_ok) {
            std::printf("all minimap checks passed\n");
            return 0;
        } else {
            std::printf("SOME CHECKS FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("EXCEPTION: %s\n", e.what());
        return 2;
    }
}
