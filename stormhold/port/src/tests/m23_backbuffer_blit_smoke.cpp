// M23 smoke test: Backbuffer::Blit() -- the alpha-tested/clipped/optionally
// mirrored RawImage compositor GameCanvas's own drawRawImageFull()/
// drawRawImageFrame() (phase-3 M22, see ../src/GameCanvas.java) both reduce
// to. See graphics/backbuffer.h's own doc comment for the exact
// correspondence.
//
// No JVM/original-binary ground truth is possible for pixel-level rendering
// output (same reason M6/M7's own tests give for not having one -- there's
// no way to render-compare against the real Nokia hardware/emulator from
// here), so this checks strong internal self-consistency instead: known
// synthetic ARGB4444 inputs produce the exact expected RGB565 outputs and
// alpha-test skips, clipping restricts exactly the claimed column range (the
// "draw the whole image, then clip" idiom GameCanvas.drawRawImageFrame()
// itself uses for frame-slicing a spritesheet), mirroring reverses column
// order, out-of-bounds positions don't crash, and a real M7-confirmed .cus
// sprite's every opaque pixel round-trips through the exact same
// IsOpaquePixel()/Argb4444ToRgb565() helpers Blit() itself uses internally.
#include <cstdio>
#include <cstdlib>
#include <string>

#include "assets/asset_root.h"
#include "assets/raw_image.h"
#include "graphics/backbuffer.h"

namespace {

using stormhold::Argb4444ToRgb565;
using stormhold::Backbuffer;
using stormhold::IsOpaquePixel;
using stormhold::PackRGB565;
using stormhold::RawImage;

int g_failures = 0;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_failures++;
    }
}

RawImage MakeImage(int width, int height, std::initializer_list<uint16_t> pixels) {
    RawImage img;
    img.width = width;
    img.height = height;
    img.hasTransparency = true;
    img.pixels = pixels;
    return img;
}

// Fully opaque red/green/blue (ARGB4444: alpha nibble 0xF, then R/G/B
// nibbles) plus one fully transparent pixel, 2x2.
void TestBasicBlitAndAlphaTest() {
    std::printf("TestBasicBlitAndAlphaTest:\n");
    RawImage img = MakeImage(2, 2,
                              {
                                  0xFF00,  // (0,0) opaque red    (R=0xF,G=0,B=0)
                                  0x0ABC,  // (1,0) transparent (alpha nibble 0) -- must be skipped
                                  0xF0F0,  // (0,1) opaque green  (R=0,G=0xF,B=0)
                                  0xF00F,  // (1,1) opaque blue   (R=0,G=0,B=0xF)
                              });

    Backbuffer bb;
    uint16_t background = PackRGB565(1, 2, 3);
    bb.Fill(background);
    bb.Blit(10, 20, img);

    uint16_t expectedRed = Argb4444ToRgb565(0xFF00);
    uint16_t expectedGreen = Argb4444ToRgb565(0xF0F0);
    uint16_t expectedBlue = Argb4444ToRgb565(0xF00F);
    Check(expectedRed == PackRGB565(0xFF, 0, 0), "0xFF00 converts to full-intensity red");
    Check(expectedGreen == PackRGB565(0, 0xFF, 0), "0xF0F0 converts to full-intensity green");
    Check(expectedBlue == PackRGB565(0, 0, 0xFF), "0xF00F converts to full-intensity blue");

    Check(bb.Data()[20 * Backbuffer::kWidth + 10] == expectedRed, "(10,20) drawn as opaque red");
    Check(bb.Data()[20 * Backbuffer::kWidth + 11] == background, "(11,20) transparent pixel left background untouched");
    Check(bb.Data()[21 * Backbuffer::kWidth + 10] == expectedGreen, "(10,21) drawn as opaque green");
    Check(bb.Data()[21 * Backbuffer::kWidth + 11] == expectedBlue, "(11,21) drawn as opaque blue");

    Check(!IsOpaquePixel(0x0ABC), "IsOpaquePixel treats alpha nibble 0 as transparent");
    Check(IsOpaquePixel(0xF000), "IsOpaquePixel treats any non-zero alpha nibble as opaque");
    std::printf("  ok\n");
}

// Models GameCanvas.drawRawImageFrame()'s own idiom directly: a 3-frame,
// 4px-wide-per-frame spritesheet (12x1), blitting frame 1 at screen x=50 by
// calling Blit(50 - 1*4, y, img, /*clipX0=*/50, /*clipX1=*/54) -- exactly
// the `x - frame*frameWidth` positioning plus [x, x+frameWidth) clip
// GameCanvas's own drawRawImageFrame() (M22) computes.
void TestFrameSlicingIdiom() {
    std::printf("TestFrameSlicingIdiom:\n");
    const int frameWidth = 4;
    RawImage img = MakeImage(frameWidth * 3, 1,
                              {
                                  0xFF00, 0xFF00, 0xFF00, 0xFF00,  // frame 0: red
                                  0xF0F0, 0xF0F0, 0xF0F0, 0xF0F0,  // frame 1: green
                                  0xF00F, 0xF00F, 0xF00F, 0xF00F,  // frame 2: blue
                              });

    Backbuffer bb;
    uint16_t background = PackRGB565(9, 9, 9);
    bb.Fill(background);

    int screenX = 50;
    int frame = 1;
    bb.Blit(screenX - frame * frameWidth, 5, img, screenX, screenX + frameWidth);

    uint16_t expectedGreen = Argb4444ToRgb565(0xF0F0);
    for (int x = 0; x < frameWidth; x++) {
        Check(bb.Data()[5 * Backbuffer::kWidth + (screenX + x)] == expectedGreen, "frame-1 column shows green");
    }
    // Columns just outside the clip range must be untouched, even though
    // the (mirrored/shifted) source image logically extends there too --
    // this is the whole point of the "draw everything, then clip" idiom.
    Check(bb.Data()[5 * Backbuffer::kWidth + (screenX - 1)] == background, "column left of clip range untouched");
    Check(bb.Data()[5 * Backbuffer::kWidth + (screenX + frameWidth)] == background,
          "column right of clip range untouched");
    std::printf("  ok\n");
}

// GameCanvas.drawWallSegment()/renderWardenCompassIcon() (M21/M22) both use
// DirectGraphics manipulation flag 8192 (TRANS_MIRROR) to draw a second,
// horizontally-flipped copy of the same spritesheet rather than storing a
// separate mirrored image.
void TestMirror() {
    std::printf("TestMirror:\n");
    RawImage img = MakeImage(3, 1,
                              {
                                  0xFF00,  // red
                                  0xF0F0,  // green
                                  0xF00F,  // blue
                              });

    Backbuffer bb;
    bb.Blit(0, 0, img, 0, Backbuffer::kWidth, /*mirrorX=*/true);

    Check(bb.Data()[0] == Argb4444ToRgb565(0xF00F), "mirrored column 0 shows the source's last pixel (blue)");
    Check(bb.Data()[1] == Argb4444ToRgb565(0xF0F0), "mirrored column 1 shows the source's middle pixel (green)");
    Check(bb.Data()[2] == Argb4444ToRgb565(0xFF00), "mirrored column 2 shows the source's first pixel (red)");
    std::printf("  ok\n");
}

// Blitting off every edge of the backbuffer must not crash or corrupt
// in-bounds pixels.
void TestOutOfBoundsSafety() {
    std::printf("TestOutOfBoundsSafety:\n");
    RawImage img = MakeImage(4, 4,
                              {
                                  0xFF00, 0xFF00, 0xFF00, 0xFF00, 0xFF00, 0xFF00, 0xFF00, 0xFF00,
                                  0xFF00, 0xFF00, 0xFF00, 0xFF00, 0xFF00, 0xFF00, 0xFF00, 0xFF00,
                              });

    Backbuffer bb;
    uint16_t background = PackRGB565(4, 4, 4);
    bb.Fill(background);

    bb.Blit(-2, -2, img);
    bb.Blit(Backbuffer::kWidth - 2, 0, img);
    bb.Blit(0, Backbuffer::kHeight - 2, img);
    bb.Blit(Backbuffer::kWidth - 2, Backbuffer::kHeight - 2, img);
    bb.Blit(-1000, -1000, img);
    bb.Blit(1000, 1000, img);

    // The top-left 2x2 corner of each edge-straddling blit should still
    // have landed correctly.
    Check(bb.Data()[0] == Argb4444ToRgb565(0xFF00), "blit straddling the top-left corner still drew (0,0)");
    Check(bb.Data()[(Backbuffer::kHeight - 1) * Backbuffer::kWidth + (Backbuffer::kWidth - 1)] ==
              Argb4444ToRgb565(0xFF00),
          "blit straddling the bottom-right corner still drew the last pixel");
    std::printf("  ok (no crash)\n");
}

// Cross-checks Blit() against a real M7-confirmed .cus sprite: every pixel
// IsOpaquePixel() calls opaque gets drawn as Argb4444ToRgb565() would
// convert it, every transparent pixel is skipped, at the exact expected
// position.
void TestRealSpriteIntegration(const std::string& assetsRoot) {
    std::printf("TestRealSpriteIntegration:\n");
    stormhold::AssetRoot assets(assetsRoot);
    RawImage img = RawImage::Load(assets, "chestnearclosed.cus");
    Check(img.width > 0 && img.height > 0, "chestnearclosed.cus decoded with positive dimensions");

    Backbuffer bb;
    uint16_t background = PackRGB565(7, 7, 7);
    bb.Fill(background);
    int originX = 20;
    int originY = 30;
    bb.Blit(originX, originY, img);

    int opaqueChecked = 0;
    int transparentChecked = 0;
    bool mismatch = false;
    for (int y = 0; y < img.height; y++) {
        for (int x = 0; x < img.width; x++) {
            uint16_t src = img.pixels[static_cast<size_t>(y) * static_cast<size_t>(img.width) +
                                       static_cast<size_t>(x)];
            uint16_t drawn = bb.Data()[(originY + y) * Backbuffer::kWidth + (originX + x)];
            if (IsOpaquePixel(src)) {
                opaqueChecked++;
                if (drawn != Argb4444ToRgb565(src)) mismatch = true;
            } else {
                transparentChecked++;
                if (drawn != background) mismatch = true;
            }
        }
    }

    Check(!mismatch, "every pixel of a real sprite matches Blit()'s own conversion/alpha-test rules");
    std::printf("  chestnearclosed.cus: %dx%d, opaque=%d transparent=%d, all matched=%s\n", img.width, img.height,
                opaqueChecked, transparentChecked, mismatch ? "false" : "true");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    TestBasicBlitAndAlphaTest();
    TestFrameSlicingIdiom();
    TestMirror();
    TestOutOfBoundsSafety();

    try {
        TestRealSpriteIntegration(root);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m23_backbuffer_blit_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    if (g_failures > 0) {
        std::fprintf(stderr, "m23_backbuffer_blit_smoke: FAILED (%d check(s))\n", g_failures);
        return 1;
    }

    std::printf("all checks passed\n");
    return 0;
}
