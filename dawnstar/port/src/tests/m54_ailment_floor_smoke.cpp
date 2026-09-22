// M54 smoke test: FrameRenderer::Render's now-real ailment-gated floor
// rendering -- GameCanvas.paintCorridorWalls()'s own opening `if
// (!hasAilment(3)) { hasAilment(4) ? solid rect : floor texture }`
// chain, previously always taking the no-ailment path (see
// docs/PORT_ROADMAP.md's M54 entry).
//
// No JVM ground truth (same reason as every prior milestone). Each
// ailment state is checked pixel-for-pixel against an INDEPENDENTLY
// built expected frame: the wall/gate segments come from
// CorridorRenderPlan::Plan directly (the same "known-good, unrelated to
// this milestone" reuse M48/M50's own tests already established) with
// their own Blit calls reproduced by hand here, not by calling
// FrameRenderer itself; the floor region is built by hand from
// GameCanvas.paintCorridorWalls()'s own literal color/rect values.
#include <cstdio>
#include <string>
#include <vector>

#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/img_archive.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "player/player_state.h"
#include "render/corridor_render_plan.h"
#include "render/frame_renderer.h"
#include "world/dungeon_generator.h"
#include "world/dungeon_view.h"

namespace {

using dawnstar::Backbuffer;
using dawnstar::CorridorRenderPlan;
using dawnstar::DecodedImage;
using dawnstar::FrameRenderer;
using dawnstar::FrameTextures;
using dawnstar::GeneratedLevel;
using dawnstar::PlayerState;
using dawnstar::WallDrawCall;
using dawnstar::WallTexture;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

bool SameFrame(const Backbuffer& a, const Backbuffer& b) {
    for (int i = 0; i < Backbuffer::kWidth * Backbuffer::kHeight; i++) {
        if (a.Data()[i] != b.Data()[i]) return false;
    }
    return true;
}

// Reproduces FrameRenderer::Render's own wall-compositing loop exactly,
// starting from whatever `bb` already holds (the caller lays down the
// floor/background first) -- the same "independently placed Blit calls"
// technique m48/m50's own tests already use, just for the wall segments
// instead of a splash/HUD.
void BlitWalls(Backbuffer& bb, const FrameTextures& textures, const std::vector<WallDrawCall>& calls) {
    for (const WallDrawCall& call : calls) {
        const DecodedImage& tex = call.texture == WallTexture::kGate
                                       ? textures.gate
                                       : (call.texture == WallTexture::kWallIce ? textures.wallIce : textures.wall);
        bb.Blit(call.x, call.y, tex, call.clipX, call.clipX + 18);
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        dawnstar::MonsterDatabase monsters = dawnstar::MonsterDatabase::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);
        dawnstar::ImgArchive images(root + "/imgfiles.lmp");
        FrameTextures textures = FrameTextures::Load(images);

        // A real non-hub level (so the ice floor texture is the one in
        // play, matching this test's own dungeonNumber != 1 case), same
        // "stand at the stairway corridor tile" positioning m10's own
        // test uses -- neighbors neutralized since this test only ever
        // holds this one level.
        int levelNumber = 2;
        GeneratedLevel level =
            dawnstar::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[levelNumber - 1], items, monsters);
        level.neighborNorth = level.neighborEast = level.neighborSouth = level.neighborWest = 0;
        std::vector<GeneratedLevel> levels{level};
        dawnstar::DungeonView view(levels, 0);

        int stairsDir = geometry.rows[levelNumber - 1].stairsUpDir;
        int px = 17, py = 17, facing = 1;
        if (stairsDir == 2) {
            px = 31;
            facing = 2;
        } else if (stairsDir == 4) {
            px = 3;
            facing = 4;
        } else if (stairsDir == 1) {
            py = 3;
            facing = 1;
        } else if (stairsDir == 3) {
            py = 31;
            facing = 3;
        } else {
            px = level.monsters[0].x;
            py = level.monsters[0].y;
            facing = 1;
        }

        std::vector<WallDrawCall> calls = CorridorRenderPlan::Plan(view, px, py, facing, level.number);
        Check(!calls.empty(), "setup: a real corridor position should produce at least one wall segment");

        const uint16_t black = dawnstar::PackRGB565(0, 0, 0);
        const uint16_t trollThirstColor = dawnstar::PackRGB565((10485760 >> 16) & 0xFF, (10485760 >> 8) & 0xFF,
                                                                 10485760 & 0xFF);

        // --- A: no ailment -- the normal per-column ice-floor tiling
        // (level.number != 1) ---
        {
            PlayerState p;  // ailmentMask == 0
            Backbuffer got;
            FrameRenderer::Render(got, textures, view, px, py, facing, level.number, p);

            Backbuffer want;
            want.Fill(black);
            for (int col = 0; col < 5; col++) want.Blit(col * 36, 0, textures.floorIce);
            BlitWalls(want, textures, calls);
            Check(SameFrame(got, want), "no ailment: normal floor-ice tiling + the real wall segments");
        }

        // --- B: Blind (ailment 3) -- no floor at all, walls unaffected ---
        {
            PlayerState p;
            p.ailmentMask = 1 << 2;  // ailment 3 (1-based) -> bit 2
            Backbuffer got;
            FrameRenderer::Render(got, textures, view, px, py, facing, level.number, p);

            Backbuffer want;
            want.Fill(black);  // the floor block is skipped outright -- black shows through
            BlitWalls(want, textures, calls);
            Check(SameFrame(got, want), "Blind: no floor drawn at all, walls still render");
        }

        // --- C: Troll Thirst (ailment 4), NOT Blind -- a solid dark-red
        // rect sized to the FIXED (non-ice) floor texture's own height ---
        {
            PlayerState p;
            p.ailmentMask = 1 << 3;  // ailment 4 -> bit 3
            Backbuffer got;
            FrameRenderer::Render(got, textures, view, px, py, facing, level.number, p);

            Backbuffer want;
            want.Fill(black);
            want.FillRect(0, 0, Backbuffer::kWidth, textures.floor.height, trollThirstColor);
            BlitWalls(want, textures, calls);
            Check(SameFrame(got, want),
                  "Troll Thirst: a solid dark-red rect sized to the FIXED floor texture's height, not the ice one");
        }

        // --- D: BOTH Blind and Troll Thirst -- Blind wins (the original's
        // own `if (!hasAilment(3)) { if (hasAilment(4)) ... }` never even
        // reaches the ailment-4 check once ailment 3 is set) ---
        {
            PlayerState p;
            p.ailmentMask = static_cast<int8_t>((1 << 2) | (1 << 3));
            Backbuffer got;
            FrameRenderer::Render(got, textures, view, px, py, facing, level.number, p);

            Backbuffer want;
            want.Fill(black);
            BlitWalls(want, textures, calls);
            Check(SameFrame(got, want), "Blind + Troll Thirst together: Blind takes priority, still no floor");
        }

        // --- E: the hub town (level 1) uses the plain (non-ice) floor
        // texture in the no-ailment case -- confirms the ailment gating
        // doesn't disturb the existing dungeon-number selection. ---
        {
            GeneratedLevel hub = dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[0]);
            hub.neighborNorth = hub.neighborEast = hub.neighborSouth = hub.neighborWest = 0;
            std::vector<GeneratedLevel> hubLevels{hub};
            dawnstar::DungeonView hubView(hubLevels, 0);
            int hpx = 2, hpy = 9, hfacing = 2;  // same position m10's own test uses for the hub
            std::vector<WallDrawCall> hubCalls = CorridorRenderPlan::Plan(hubView, hpx, hpy, hfacing, hub.number);

            PlayerState p;
            Backbuffer got;
            FrameRenderer::Render(got, textures, hubView, hpx, hpy, hfacing, hub.number, p);

            Backbuffer want;
            want.Fill(black);
            for (int col = 0; col < 5; col++) want.Blit(col * 36, 0, textures.floor);
            BlitWalls(want, textures, hubCalls);
            Check(SameFrame(got, want), "the hub town's own no-ailment floor should still use the plain (non-ice) texture");
        }

        if (g_ok) {
            std::printf("ailment_floor_smoke: all checks passed\n");
            return 0;
        } else {
            std::printf("ailment_floor_smoke: FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("ailment_floor_smoke: exception: %s\n", e.what());
        return 2;
    }
}
