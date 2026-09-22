// M31 smoke test: HotbarRenderer (render/hotbar_renderer.h) -- the
// bottom hotbar panel: GameCanvas.computeHotbarContext()'s pure
// 0/1/2 logic, plus paintHotbar()'s panel background + 4 digit/icon
// pairs per context. Verified against the real extracted "icons.png"/
// "panel.png" textures (M7/M10/M26's own "actually look at the real
// pixels" standard), plus the bitmap font's own newly-added digit
// glyphs (graphics/bitmap_font.h, extended this milestone).
//
// No JVM ground truth is available (same reason as every prior
// milestone) -- ComputeHotbarContext's 3-way branch and the exact pixel
// coordinates below are independently re-derived from
// ../../../src/GameCanvas.java's own computeHotbarContext()/
// paintHotbar()/drawHotbarIcon(), not read back from hotbar_renderer.cpp.
#include <cstdint>
#include <cstdio>
#include <string>

#include "assets/decoded_image.h"
#include "assets/img_archive.h"
#include "graphics/backbuffer.h"
#include "graphics/bitmap_font.h"
#include "render/hotbar_renderer.h"

namespace {

using dawnstar::Backbuffer;
using dawnstar::DecodedImage;
using dawnstar::HotbarRenderer;
using dawnstar::HotbarTextures;
using dawnstar::PackRGB565;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

uint16_t PixelAt(const Backbuffer& bb, int x, int y) { return bb.Data()[static_cast<size_t>(y) * Backbuffer::kWidth + x]; }

// True if any pixel in the [x0,x0+w) x [y0,y0+h) box differs from the
// (black-filled) background -- used for the digit glyphs below instead
// of a hand-picked exact pixel, since BitmapFont now (M58) renders
// through real (anti-aliased, proportional) GDI text rather than a
// fixed 4x7 pixel table with one exact bit pattern per character to
// predict by hand (same "check the glyph drew somewhere in its own
// box" approach M55's own compass-glyph test already established).
bool AnyNonBlackInBox(const Backbuffer& bb, int x0, int y0, int w, int h) {
    for (int y = y0; y < y0 + h; y++) {
        for (int x = x0; x < x0 + w; x++) {
            if (PixelAt(bb, x, y) != 0) return true;
        }
    }
    return false;
}

// Finds the first opaque pixel within icon frame `iconIdx`'s own 30-wide
// column range of `sheet` (icons.png is 9 such frames, tiled
// horizontally) -- a distinctive sample point to confirm the RIGHT
// frame landed at the expected screen offset, not just "some sprite
// pixel somewhere".
bool FindOpaqueInFrame(const DecodedImage& sheet, int iconIdx, int* outSx, int* outSy) {
    int x0 = iconIdx * 30;
    for (int y = 0; y < sheet.height; y++) {
        for (int x = x0; x < x0 + 30; x++) {
            if (sheet.A(x, y) != 0) {
                *outSx = x;
                *outSy = y;
                return true;
            }
        }
    }
    return false;
}

void CheckIconAt(const Backbuffer& bb, const DecodedImage& sheet, int iconIdx, int drawX, int drawY,
                  const char* label) {
    int sx = 0, sy = 0;
    bool found = FindOpaqueInFrame(sheet, iconIdx, &sx, &sy);
    Check(found, "icon frame should have at least one opaque pixel to sample");
    if (!found) return;
    uint16_t expected = PackRGB565(sheet.R(sx, sy), sheet.G(sx, sy), sheet.B(sx, sy));
    int px = drawX + (sx - iconIdx * 30);
    int py = drawY + sy;
    uint16_t actual = PixelAt(bb, px, py);
    if (actual != expected) {
        std::printf("  FAIL: %s: pixel at (%d,%d) expected 0x%04x, got 0x%04x\n", label, px, py, expected, actual);
        g_ok = false;
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::ImgArchive imageArchive(root + "/imgfiles.lmp");
        HotbarTextures textures = HotbarTextures::Load(imageArchive);
        Check(textures.icons.width == 270 && textures.icons.height == 24, "icons.png should be the real 270x24 sheet");
        Check(textures.panel.width == 176 && textures.panel.height == 52, "panel.png should be the real 176x52 image");

        // --- A: ComputeHotbarContext ---
        {
            Check(HotbarRenderer::ComputeHotbarContext(false, false, -1) == 0, "no monster/chest/npc -> context 0");
            Check(HotbarRenderer::ComputeHotbarContext(false, true, -1) == 2, "chest in sight -> context 2");
            Check(HotbarRenderer::ComputeHotbarContext(false, false, 3) == 2, "npc in sight -> context 2");
            // monsterTargeted wins over chest/npc, matching the original's
            // own if/else (not an "and" of all three conditions).
            Check(HotbarRenderer::ComputeHotbarContext(true, true, 3) == 1,
                  "monsterTargeted should take priority over chest/npc -> context 1");
            Check(HotbarRenderer::ComputeHotbarContext(true, false, -1) == 1, "monsterTargeted alone -> context 1");
        }

        // --- B: Paint(context=0) -- explore hotbar ---
        {
            Backbuffer bb;
            bb.Fill(0);
            HotbarRenderer::Paint(bb, textures, 0);

            // Panel background: sample its own first real opaque pixel.
            int psx = 0, psy = 0;
            bool found = false;
            for (int y = 0; y < textures.panel.height && !found; y++) {
                for (int x = 0; x < textures.panel.width; x++) {
                    if (textures.panel.A(x, y) != 0) {
                        psx = x;
                        psy = y;
                        found = true;
                        break;
                    }
                }
            }
            Check(found, "panel.png should have at least one opaque pixel");
            if (found) {
                uint16_t expected = PackRGB565(textures.panel.R(psx, psy), textures.panel.G(psx, psy),
                                                textures.panel.B(psx, psy));
                Check(PixelAt(bb, psx, 156 + psy) == expected, "panel background should be blitted at (0,156)");
            }

            // Context 0's own 4 icons: idx 1,2,3,5 at (13,53,93,133)/164.
            CheckIconAt(bb, textures.icons, 1, 13, 164, "context 0 icon slot 1 (idx 1) at (13,164)");
            CheckIconAt(bb, textures.icons, 2, 53, 164, "context 0 icon slot 2 (idx 2) at (53,164)");
            CheckIconAt(bb, textures.icons, 3, 93, 164, "context 0 icon slot 3 (idx 3) at (93,164)");
            CheckIconAt(bb, textures.icons, 5, 133, 164, "context 0 icon slot 4 (idx 5) at (133,164)");

            // Context 0's own digit chars: '3','5','7','0' (HOTBAR_DIGIT_CHARS
            // indices 1,2,3,5), drawn at (25,190) white / (26,191) black
            // shadow. Checked as "something non-background drew somewhere
            // in the glyph's own box" rather than one exact hand-picked
            // pixel -- see AnyNonBlackInBox's own doc comment.
            Check(AnyNonBlackInBox(bb, 25, 190, dawnstar::BitmapFont::kGlyphWidth + 1,
                                   dawnstar::BitmapFont::kGlyphHeight + 1),
                  "context 0's first digit ('3') should draw somewhere in its own glyph box");
        }

        // --- C: Paint(context=1) -- combat hotbar ---
        {
            Backbuffer bb;
            bb.Fill(0);
            HotbarRenderer::Paint(bb, textures, 1);
            // Context 1's own 4 icons: idx 0,1,2,3 at the same 4 x-positions.
            CheckIconAt(bb, textures.icons, 0, 13, 164, "context 1 icon slot 1 (idx 0) at (13,164)");
            CheckIconAt(bb, textures.icons, 3, 133, 164, "context 1 icon slot 4 (idx 3) at (133,164)");
            // First digit is '1' (HOTBAR_DIGIT_CHARS[0]), same (25,190)
            // position and same box-presence check as context 0's above.
            Check(AnyNonBlackInBox(bb, 25, 190, dawnstar::BitmapFont::kGlyphWidth + 1,
                                   dawnstar::BitmapFont::kGlyphHeight + 1),
                  "context 1's first digit ('1') should draw somewhere in its own glyph box");
        }

        // --- D: Paint(context=2) -- interact hotbar ---
        {
            Backbuffer bb;
            bb.Fill(0);
            HotbarRenderer::Paint(bb, textures, 2);
            // Context 2's own 4 icons: idx 1,2,3,4 at the same 4 x-positions
            // -- differs from context 0 only in the LAST icon (4 vs 5).
            CheckIconAt(bb, textures.icons, 4, 133, 164, "context 2 icon slot 4 (idx 4) at (133,164)");
        }

        if (g_ok) {
            std::printf("all hotbar-renderer checks passed\n");
            return 0;
        } else {
            std::printf("SOME CHECKS FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("EXCEPTION: %s\n", e.what());
        return 1;
    }
}
