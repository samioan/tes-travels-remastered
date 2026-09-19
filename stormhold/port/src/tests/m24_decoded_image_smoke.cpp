// M24 smoke test: DecodedImage (stb_image-decoded PNG) plus
// Backbuffer::Blit(DecodedImage) -- the plain-Image counterpart of M23's
// RawImage compositor, for GameCanvas's floorTexture/wallTexture/
// effectImages/hotbarIcons fields. Resolves phase-3 M21's own open question
// (whether floorTexture/wallTexture load from a .cus file or a plain
// MIDP-native Image resource): grepping ESGame.java's own asset-loading
// call sites confirms real .png files ("floor3.png"/"newwallsnok.png"),
// not M7's RawImage format at all.
//
// Same "no render-compare against real hardware possible here" reasoning
// M6/M7/M23's own tests already give -- this checks strong internal
// self-consistency instead: real .png files this game actually ships
// decode to sane, non-empty RGBA8 buffers; a synthetic image's alpha
// test/RGB565 conversion/clip-range/mirroring behave identically to M23's
// RawImage-based Blit(); and every pixel of a real decoded PNG blitted onto
// a backbuffer matches DecodedImage's own R/G/B/A accessors exactly.
#include <cstdio>
#include <cstdlib>
#include <string>

#include "assets/asset_root.h"
#include "assets/decoded_image.h"
#include "graphics/backbuffer.h"

namespace {

using stormhold::AssetRoot;
using stormhold::Backbuffer;
using stormhold::DecodedImage;
using stormhold::PackRGB565;

int g_failures = 0;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_failures++;
    }
}

DecodedImage MakeImage(int width, int height, std::initializer_list<uint8_t> rgba) {
    DecodedImage img;
    img.width = width;
    img.height = height;
    img.pixels = rgba;
    return img;
}

void TestBasicBlitAndAlphaTest() {
    std::printf("TestBasicBlitAndAlphaTest:\n");
    // 2x2: opaque red, transparent, opaque green, opaque blue.
    DecodedImage img = MakeImage(2, 2,
                                  {
                                      255, 0, 0, 255,    // (0,0) opaque red
                                      12, 34, 56, 0,     // (1,0) transparent -- must be skipped
                                      0, 255, 0, 255,    // (0,1) opaque green
                                      0, 0, 255, 255,    // (1,1) opaque blue
                                  });

    Backbuffer bb;
    uint16_t background = PackRGB565(1, 2, 3);
    bb.Fill(background);
    bb.Blit(10, 20, img);

    Check(bb.Data()[20 * Backbuffer::kWidth + 10] == PackRGB565(255, 0, 0), "(10,20) drawn as opaque red");
    Check(bb.Data()[20 * Backbuffer::kWidth + 11] == background, "(11,20) transparent pixel left background untouched");
    Check(bb.Data()[21 * Backbuffer::kWidth + 10] == PackRGB565(0, 255, 0), "(10,21) drawn as opaque green");
    Check(bb.Data()[21 * Backbuffer::kWidth + 11] == PackRGB565(0, 0, 255), "(11,21) drawn as opaque blue");
    std::printf("  ok\n");
}

// GameCanvas.drawWallSegment() (M21) uses this SAME "shift the whole
// spritesheet left by frame*frameWidth, then clip to one frame's column"
// idiom for wallTexture -- now confirmed a plain Image/.png, not RawImage,
// so it needs this DecodedImage overload of Blit(), not M23's.
void TestFrameSlicingIdiom() {
    std::printf("TestFrameSlicingIdiom:\n");
    const int frameWidth = 4;
    DecodedImage img = MakeImage(frameWidth * 2, 1,
                                  {
                                      255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255,  // frame 0: red
                                      0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,  // frame 1: green
                                  });

    Backbuffer bb;
    uint16_t background = PackRGB565(9, 9, 9);
    bb.Fill(background);

    int screenX = 40;
    int frame = 1;
    bb.Blit(screenX - frame * frameWidth, 3, img, screenX, screenX + frameWidth);

    for (int x = 0; x < frameWidth; x++) {
        Check(bb.Data()[3 * Backbuffer::kWidth + (screenX + x)] == PackRGB565(0, 255, 0), "frame-1 column shows green");
    }
    Check(bb.Data()[3 * Backbuffer::kWidth + (screenX - 1)] == background, "column left of clip range untouched");
    Check(bb.Data()[3 * Backbuffer::kWidth + (screenX + frameWidth)] == background,
          "column right of clip range untouched");
    std::printf("  ok\n");
}

void TestMirror() {
    std::printf("TestMirror:\n");
    DecodedImage img = MakeImage(3, 1,
                                  {
                                      255, 0, 0, 255,  // red
                                      0, 255, 0, 255,  // green
                                      0, 0, 255, 255,  // blue
                                  });

    Backbuffer bb;
    bb.Blit(0, 0, img, 0, Backbuffer::kWidth, /*mirrorX=*/true);

    Check(bb.Data()[0] == PackRGB565(0, 0, 255), "mirrored column 0 shows the source's last pixel (blue)");
    Check(bb.Data()[1] == PackRGB565(0, 255, 0), "mirrored column 1 shows the source's middle pixel (green)");
    Check(bb.Data()[2] == PackRGB565(255, 0, 0), "mirrored column 2 shows the source's first pixel (red)");
    std::printf("  ok\n");
}

// Real .png files GameCanvas's own plain-Image fields load (confirmed via
// ESGame.java's literal filenames). Checks decode succeeds with sane
// dimensions, and that a full Blit() onto a filled backbuffer matches
// DecodedImage's own R/G/B/A accessors pixel-by-pixel.
void TestRealPngIntegration(const std::string& assetsRoot) {
    std::printf("TestRealPngIntegration:\n");
    AssetRoot assets(assetsRoot);

    const char* names[] = {"floor3.png", "newwallsnok.png", "blood1.png", "icon_camp.png"};
    for (const char* name : names) {
        DecodedImage img = DecodedImage::Load(assets, name);
        Check(img.width > 0 && img.height > 0, "decoded with positive dimensions");
        Check(img.pixels.size() == static_cast<size_t>(img.width) * static_cast<size_t>(img.height) * 4,
              "pixel buffer size == width*height*4");
        std::printf("  %-18s %4dx%-4d\n", name, img.width, img.height);
    }

    // wallTexture (newwallsnok.png) is drawn 18px-per-frame by
    // drawWallSegment() (M21/M22) -- a real structural cross-check that its
    // width is an exact multiple of 18, same "confirm the recovered
    // constant against real data" discipline M6/M7's own tests use.
    DecodedImage wall = DecodedImage::Load(assets, "newwallsnok.png");
    Check(wall.width % 18 == 0, "newwallsnok.png width is an exact multiple of the 18px wall-frame width");

    DecodedImage icon = DecodedImage::Load(assets, "icon_camp.png");
    Backbuffer bb;
    uint16_t background = PackRGB565(11, 11, 11);
    bb.Fill(background);
    int originX = 5;
    int originY = 5;
    bb.Blit(originX, originY, icon);

    bool mismatch = false;
    int opaqueChecked = 0;
    int transparentChecked = 0;
    for (int y = 0; y < icon.height; y++) {
        for (int x = 0; x < icon.width; x++) {
            uint16_t drawn = bb.Data()[(originY + y) * Backbuffer::kWidth + (originX + x)];
            if (icon.A(x, y) != 0) {
                opaqueChecked++;
                if (drawn != PackRGB565(icon.R(x, y), icon.G(x, y), icon.B(x, y))) mismatch = true;
            } else {
                transparentChecked++;
                if (drawn != background) mismatch = true;
            }
        }
    }
    Check(!mismatch, "every pixel of icon_camp.png matches Blit()'s own alpha-test/copy rules");
    std::printf("  icon_camp.png: opaque=%d transparent=%d, all matched=%s\n", opaqueChecked, transparentChecked,
                mismatch ? "false" : "true");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    TestBasicBlitAndAlphaTest();
    TestFrameSlicingIdiom();
    TestMirror();

    try {
        TestRealPngIntegration(root);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m24_decoded_image_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    if (g_failures > 0) {
        std::fprintf(stderr, "m24_decoded_image_smoke: FAILED (%d check(s))\n", g_failures);
        return 1;
    }

    std::printf("all checks passed\n");
    return 0;
}
