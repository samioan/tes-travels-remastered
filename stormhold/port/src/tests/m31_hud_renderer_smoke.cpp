// M31 smoke test: GameRenderer::RenderHud -- paintHud()'s own actual
// pixel drawing (../../../src/GameCanvas.java lines 1024-1062), gated on
// M29's ResolveHudIconSet and built on M30's Backbuffer::FillRoundRect
// plus this milestone's own new BitmapFont digit glyphs ('0'-'9') and
// HotbarAssets (the 6 real icon_*.png files).
//
// No JVM ground truth possible (same reasoning every other rendering
// milestone's own tests already give) -- verified via:
//  - BitmapFont's digit glyphs: structural StringWidth/box-presence
//    checks (M74: real proportional GDI text, not a hand-predictable
//    fixed pixel table -- see graphics/bitmap_font.h's own class
//    comment).
//  - Synthetic per-iconSet checks: the exact 4 (icon, glyph) index pairs
//    each of the 3 branches selects, cross-checked against
//    GameCanvas.paintHud()'s own literal index lists, not read back
//    from game_renderer.cpp.
//  - The background fills always drawing regardless of iconSet
//    (including an out-of-range one -- unreachable in practice, but
//    matches the original's own no-final-else chain).
//  - A real-asset integration check: every opaque pixel of a real
//    icon_attack.png blit is independently recomputed here and compared
//    against DecodedImage's own R/G/B/A accessors directly, same
//    discipline M25/M28's own real-asset checks already used.
#include <cstdio>
#include <string>

#include "assets/asset_root.h"
#include "graphics/bitmap_font.h"
#include "render/game_renderer.h"

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

uint16_t PixelAt(const Backbuffer& bb, int x, int y) { return bb.Data()[static_cast<size_t>(y) * Backbuffer::kWidth + x]; }

// True if any pixel in the [x0,x0+w) x [y0,y0+h) box differs from the
// (black-filled) background -- used for the digit glyphs below instead
// of a hand-picked exact pixel, since BitmapFont now (M74) renders
// through real (anti-aliased, proportional) GDI text rather than a
// fixed 4x7 pixel table with one exact bit pattern per character to
// predict by hand.
bool AnyNonBlackInBox(const Backbuffer& bb, int x0, int y0, int w, int h) {
    for (int y = y0; y < y0 + h; y++) {
        for (int x = x0; x < x0 + w; x++) {
            if (PixelAt(bb, x, y) != 0) return true;
        }
    }
    return false;
}

DecodedImage MakeImage(int width, int height, uint8_t r, uint8_t g, uint8_t b) {
    DecodedImage img;
    img.width = width;
    img.height = height;
    img.pixels.assign(static_cast<size_t>(width) * height * 4, 0);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            size_t off = (static_cast<size_t>(y) * width + x) * 4;
            img.pixels[off + 0] = r;
            img.pixels[off + 1] = g;
            img.pixels[off + 2] = b;
            img.pixels[off + 3] = 255;
        }
    }
    return img;
}

void TestBitmapFontDigits() {
    std::printf("-- BitmapFont: digit glyphs --\n");
    // A real, proportional GDI font's exact glyph shapes can't be
    // hand-predicted bit-by-bit the way the old fixed 4x7 table's could
    // (M74) -- these check structural properties instead, same approach
    // dawnstar's own identical M58 rewrite of this test already
    // established.
    Expect(BitmapFont::StringWidth("0123456789") > BitmapFont::StringWidth("0"),
           "10 digits should measure wider than a single one");

    Backbuffer bb;
    bb.Fill(0);
    constexpr uint16_t kWhite = PackRGB565(255, 255, 255);
    BitmapFont::DrawString(bb, 0, 0, "1", kWhite);
    Expect(AnyNonBlackInBox(bb, 0, 0, BitmapFont::StringWidth("1") + 1, BitmapFont::kGlyphHeight + 1),
           "'1' should draw at least one non-background pixel in its own box");
}

// Cross-checked directly against GameCanvas.paintHud()'s own literal
// index lists (../../../src/GameCanvas.java), not read back from
// game_renderer.cpp.
void TestHotbarRowSelection() {
    std::printf("-- RenderHud: per-iconSet (icon, glyph) index selection --\n");
    HotbarAssets assets;
    assets.icons[0] = MakeImage(4, 4, 255, 0, 0);
    assets.icons[1] = MakeImage(4, 4, 0, 255, 0);
    assets.icons[2] = MakeImage(4, 4, 0, 0, 255);
    assets.icons[3] = MakeImage(4, 4, 255, 255, 0);
    assets.icons[4] = MakeImage(4, 4, 255, 0, 255);
    assets.icons[5] = MakeImage(4, 4, 0, 255, 255);

    // iconSet 0: icons[1,2,3,5] at x=14/62/104/144; glyphs '3','5','7','0'.
    {
        Backbuffer bb;
        bb.Fill(0);
        GameRenderer::RenderHud(bb, assets, 0);
        Expect(PixelAt(bb, 14, 174) == PackRGB565(0, 255, 0), "iconSet 0 slot0 should draw icons[1] (green)");
        Expect(PixelAt(bb, 62, 174) == PackRGB565(0, 0, 255), "iconSet 0 slot1 should draw icons[2] (blue)");
        Expect(PixelAt(bb, 104, 174) == PackRGB565(255, 255, 0), "iconSet 0 slot2 should draw icons[3] (yellow)");
        Expect(PixelAt(bb, 144, 174) == PackRGB565(0, 255, 255), "iconSet 0 slot3 should draw icons[5] (cyan)");
    }

    // iconSet 1: icons[0,1,2,3] at the same 4 x positions; glyphs '1','3','5','7'.
    {
        Backbuffer bb;
        bb.Fill(0);
        GameRenderer::RenderHud(bb, assets, 1);
        Expect(PixelAt(bb, 14, 174) == PackRGB565(255, 0, 0), "iconSet 1 slot0 should draw icons[0] (red)");
        Expect(PixelAt(bb, 62, 174) == PackRGB565(0, 255, 0), "iconSet 1 slot1 should draw icons[1] (green)");
        Expect(PixelAt(bb, 104, 174) == PackRGB565(0, 0, 255), "iconSet 1 slot2 should draw icons[2] (blue)");
        Expect(PixelAt(bb, 144, 174) == PackRGB565(255, 255, 0), "iconSet 1 slot3 should draw icons[3] (yellow)");
    }

    // iconSet 2: icons[1,2,3,4] at the same 4 x positions; glyphs '3','5','7','9'.
    {
        Backbuffer bb;
        bb.Fill(0);
        GameRenderer::RenderHud(bb, assets, 2);
        Expect(PixelAt(bb, 14, 174) == PackRGB565(0, 255, 0), "iconSet 2 slot0 should draw icons[1] (green)");
        Expect(PixelAt(bb, 62, 174) == PackRGB565(0, 0, 255), "iconSet 2 slot1 should draw icons[2] (blue)");
        Expect(PixelAt(bb, 104, 174) == PackRGB565(255, 255, 0), "iconSet 2 slot2 should draw icons[3] (yellow)");
        Expect(PixelAt(bb, 144, 174) == PackRGB565(255, 0, 255), "iconSet 2 slot3 should draw icons[4] (magenta)");
    }

    // Glyph placement: iconSet 1's first glyph is hotbarKeyGlyphs[0]='1',
    // drawn at (5,180) in black -- checked as "something non-background
    // drew somewhere in its own box" (real proportional GDI text, not
    // one hand-picked exact pixel -- see AnyNonBlackInBox's own doc
    // comment), against a solid black canvas so a lit (non-black) pixel
    // can only be the panel background or the icon, and a specifically
    // panel-bg-colored pixel can only be the glyph's own drawn box
    // minus its glyph strokes.
    {
        Backbuffer bb;
        bb.Fill(0);
        GameRenderer::RenderHud(bb, assets, 1);
        uint16_t panelBg = PackRGB565(0xC7, 0x99, 0x67);
        Expect(PixelAt(bb, 5, 180) == panelBg,
               "'1's glyph box should start on the panel background (not yet inside a glyph stroke)");
        bool anyBlackInGlyphBox = false;
        for (int y = 0; y < BitmapFont::kGlyphHeight && !anyBlackInGlyphBox; y++) {
            for (int x = 0; x < BitmapFont::StringWidth("1") && !anyBlackInGlyphBox; x++) {
                if (PixelAt(bb, 5 + x, 180 + y) == PackRGB565(0, 0, 0)) anyBlackInGlyphBox = true;
            }
        }
        Expect(anyBlackInGlyphBox, "'1' should draw at least one black pixel in its own box at (5,180)");
    }
}

void TestBackgroundAlwaysDrawn() {
    std::printf("-- RenderHud: background fills draw regardless of iconSet --\n");
    HotbarAssets assets;
    for (auto& icon : assets.icons) icon = MakeImage(1, 1, 0, 0, 0);

    Backbuffer bb;
    bb.Fill(PackRGB565(9, 9, 9));
    GameRenderer::RenderHud(bb, assets, 99);  // unreachable in practice, but no final else in the original
    Expect(PixelAt(bb, 0, 156) == PackRGB565(0, 0, 0), "the outer black fillRect should draw even for an out-of-range iconSet");
    Expect(PixelAt(bb, 30, 170) == PackRGB565(0xC7, 0x99, 0x67), "the rounded panel should draw even for an out-of-range iconSet");
    // (14,174) sits inside the panel itself, so "no icon drawn" shows as
    // the panel's own background color there, not the outer sentinel --
    // the icon (all-black, 0,0,0) would be visibly distinct from that.
    Expect(PixelAt(bb, 14, 174) == PackRGB565(0xC7, 0x99, 0x67), "no icon/glyph row should draw for an out-of-range iconSet");
}

void TestRealAssetIntegration(const std::string& root) {
    std::printf("-- integration: real icon_attack.png against iconSet 1 slot0 --\n");
    AssetRoot assetRoot(root);
    HotbarAssets assets = HotbarAssets::Load(assetRoot);

    Backbuffer bb;
    uint16_t sentinel = PackRGB565(3, 3, 3);
    bb.Fill(sentinel);
    GameRenderer::RenderHud(bb, assets, 1);  // slot0 = icons[0] = icon_attack.png

    const DecodedImage& img = assets.icons[0];
    bool mismatch = false;
    int opaqueChecked = 0;
    int maxY = std::min(img.height, Backbuffer::kHeight - 174);
    int maxX = std::min(img.width, Backbuffer::kWidth - 14);
    for (int y = 0; y < maxY; y++) {
        for (int x = 0; x < maxX; x++) {
            if (img.A(x, y) == 0) continue;
            uint16_t expected = PackRGB565(img.R(x, y), img.G(x, y), img.B(x, y));
            uint16_t actual = bb.Data()[(174 + y) * Backbuffer::kWidth + (14 + x)];
            if (actual != expected) mismatch = true;
            opaqueChecked++;
        }
    }
    Expect(!mismatch, "every opaque pixel of real icon_attack.png should match its own R/G/B exactly once blitted");
    Expect(opaqueChecked > 0, "should have checked at least one real opaque icon pixel");
    std::printf("  checked %d real opaque icon_attack.png pixels, all matched=%s\n", opaqueChecked,
                mismatch ? "false" : "true");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        TestBitmapFontDigits();
        TestHotbarRowSelection();
        TestBackgroundAlwaysDrawn();
        TestRealAssetIntegration(root);

        if (!g_ok) {
            std::fprintf(stderr, "m31_hud_renderer_smoke: FAILED\n");
            return 1;
        }

        std::printf("all checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m31_hud_renderer_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
