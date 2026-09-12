// M27 smoke test: VisibleObjectRenderer::PaintObjectAtPosition
// (render/visible_object_renderer.h) -- the closest-slot monster
// renderer, GameCanvas.paintObjectAtPosition()'s OBJECT_DRAW_TABLE/
// OBJECT_ICON_TABLE/OBJECT_EXTRA_FLAGS-driven sprite compositing plus
// drawSpriteFrame()'s multi-frame slicing, and the stairs-icon special
// case for monster types 41/42. Verified against the real extracted
// textures (M7/M10's own standard), hand-tracing each case's expected
// draws directly from the same tables' own values (independently
// re-derived here, not read back from visible_object_renderer.cpp).
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>

#include "assets/dat_archive.h"
#include "assets/decoded_image.h"
#include "assets/img_archive.h"
#include "assets/monster_image_names.h"
#include "graphics/backbuffer.h"
#include "render/visible_object_renderer.h"

namespace {

using dawnstar::Backbuffer;
using dawnstar::DecodedImage;
using dawnstar::PackRGB565;
using dawnstar::VisibleObjectRenderer;
using dawnstar::VisibleObjectTextures;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

// Finds the first opaque pixel within frame `frame` (of `frameCount`
// equal-width horizontal slices) of `img`, in the image's OWN
// coordinates.
bool FindFirstOpaquePixelInFrame(const DecodedImage& img, int frame, int frameCount, int* outX, int* outY) {
    int frameWidth = img.width / frameCount;
    int x0 = frame * frameWidth;
    int x1 = x0 + frameWidth;
    for (int y = 0; y < img.height; y++) {
        for (int x = x0; x < x1; x++) {
            if (img.A(x, y) != 0) {
                *outX = x;
                *outY = y;
                return true;
            }
        }
    }
    return false;
}

// Checks that frame `frame` of `img` was drawn at (drawX, drawY) --
// drawX/drawY are the *frame's own* on-screen anchor (matching
// DrawSpriteFrame's own `x, y` parameters), not the sheet's draw origin.
void CheckFrameAt(const Backbuffer& bb, int drawX, int drawY, const DecodedImage& img, int frame, int frameCount,
                   const char* label) {
    int sx = 0, sy = 0;
    bool found = FindFirstOpaquePixelInFrame(img, frame, frameCount, &sx, &sy);
    Check(found, "frame should have at least one opaque pixel to sample");
    if (!found) return;

    int frameWidth = img.width / frameCount;
    uint16_t expected = PackRGB565(img.R(sx, sy), img.G(sx, sy), img.B(sx, sy));
    int px = drawX + (sx - frame * frameWidth);
    int py = drawY + sy;
    uint16_t actual = bb.Data()[static_cast<size_t>(py) * Backbuffer::kWidth + px];
    if (actual != expected) {
        std::printf("  FAIL: %s: pixel at (%d,%d) expected 0x%04x, got 0x%04x\n", label, px, py, expected, actual);
        g_ok = false;
    }
}

bool RegionAllBackground(const Backbuffer& bb, int x, int y, int w, int h, uint16_t background) {
    for (int yy = y; yy < y + h; yy++) {
        for (int xx = x; xx < x + w; xx++) {
            if (bb.Data()[static_cast<size_t>(yy) * Backbuffer::kWidth + xx] != background) return false;
        }
    }
    return true;
}

Backbuffer RenderObjectAt(const VisibleObjectTextures& textures, int posCode, int frameOverride = -1) {
    Backbuffer bb;
    bb.Fill(0);
    VisibleObjectRenderer::PaintObjectAtPosition(bb, textures, posCode, frameOverride);
    return bb;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ImgArchive imageArchive(root + "/imgfiles.lmp");
        dawnstar::MonsterImageNames names = dawnstar::MonsterImageNames::Load(archive);
        VisibleObjectTextures textures = VisibleObjectTextures::Load(imageArchive, names);
        std::printf("loaded textures\n");

        // --- posCode 1 (bucket 0, type 1-5's row): iconA=objectSprites[0]
        // frame 0/2 at (43,48), iconB=objectSprites[1] frame 0/3 at
        // (66,55) -- no extras (OBJECT_EXTRA_FLAGS[0] is all-false) ---
        {
            Backbuffer bb = RenderObjectAt(textures, 1);
            CheckFrameAt(bb, 43, 48, textures.objectSprites[0], 0, 2, "posCode 1 iconA");
            CheckFrameAt(bb, 66, 55, textures.objectSprites[1], 0, 3, "posCode 1 iconB");
        }

        // --- posCode 4 (same bucket, but OBJECT_ICON_TABLE[3]={1,2} --
        // non-zero frames for both sprites -- and OBJECT_EXTRA_FLAGS[3]'s
        // extra0 is true: objectSprites[2] at (43,112) ---
        {
            Backbuffer bb = RenderObjectAt(textures, 4);
            CheckFrameAt(bb, 43, 48, textures.objectSprites[0], 1, 2, "posCode 4 iconA (frame 1)");
            CheckFrameAt(bb, 66, 55, textures.objectSprites[1], 2, 3, "posCode 4 iconB (frame 2)");
            CheckFrameAt(bb, 43, 112, textures.objectSprites[2], 0, 1, "posCode 4 extra0");
        }

        // --- posCode 26 (bucket 2, type 26-40's row): iconA only
        // (iconB=-1, no second sprite) = objectSprites[14] frame 0/3 at
        // (40,50); extra0 (objectSprites[16] at (49,79)) and extra2
        // (objectSprites[17] at (81,91)) both true, extra1 false ---
        {
            Backbuffer bb = RenderObjectAt(textures, 26);
            CheckFrameAt(bb, 40, 50, textures.objectSprites[14], 0, 3, "posCode 26 iconA");
            CheckFrameAt(bb, 49, 79, textures.objectSprites[16], 0, 1, "posCode 26 extra0");
            CheckFrameAt(bb, 81, 91, textures.objectSprites[17], 0, 1, "posCode 26 extra2");
            // extra1 is false for posCode 26 -- posCode 11 below covers
            // the "all extras false" case cleanly instead of risking an
            // ambiguous negative-space check here (extra1's own would-be
            // position sits inside iconA's already-drawn region).
        }

        // --- posCode 11 (bucket 3, type 11-25's row): iconA only,
        // objectSprites[20] frame 0/3 at (37,50) -- OBJECT_EXTRA_FLAGS[10]
        // is all-false, so nothing else should be drawn at all ---
        {
            Backbuffer bb = RenderObjectAt(textures, 11);
            CheckFrameAt(bb, 37, 50, textures.objectSprites[20], 0, 3, "posCode 11 iconA");
            Check(RegionAllBackground(bb, 0, 0, 37, Backbuffer::kHeight, 0),
                  "posCode 11 should draw nothing to the left of its own sprite (no extras/second icon)");
        }

        // --- posCode 41/42 (bucket 4): Monster.java's real "Gehenoth"/
        // "Gehenoth Thriceborn" types -- render the STAIRS icon instead
        // of any monster sprite at all (paintStairsIcon(posCode==41)),
        // a real, surprising behavior traced from the source rather than
        // assumed -- see PaintObjectAtPosition's own doc comment ---
        {
            Backbuffer bb = RenderObjectAt(textures, 41);
            CheckFrameAt(bb, 33, 48, textures.objectSprites[23], 0, 2, "posCode 41 (Gehenoth) draws stairs-up icon");
        }
        {
            Backbuffer bb = RenderObjectAt(textures, 42);
            CheckFrameAt(bb, 33, 48, textures.objectSprites[23], 1, 2,
                         "posCode 42 (Gehenoth Thriceborn) draws stairs-down icon");
        }

        if (g_ok) {
            std::printf("all paintObjectAtPosition checks passed\n");
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
