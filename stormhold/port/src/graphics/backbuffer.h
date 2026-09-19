#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>

#include "assets/decoded_image.h"
#include "assets/raw_image.h"

namespace stormhold {

constexpr uint16_t PackRGB565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

// RawImage's own decoded pixel format (see assets/raw_image.h's header
// comment): top nibble is an alpha TEST bit (0 = fully transparent, any
// other value = fully opaque -- real MIDP hardware this old has no partial
// alpha blending), the low 12 bits ARGB4444's own R/G/B nibbles. The literal
// `4444` GameCanvas passes as DirectGraphics.drawPixels()'s own `format`
// argument is Nokia UI API's TYPE_USHORT_4444_ARGB constant, which is what
// pins the nibble order (not independently re-derived here -- no way to
// render-compare against the original binary in this environment).
constexpr bool IsOpaquePixel(uint16_t argb4444) { return (argb4444 & 0xF000) != 0; }

constexpr uint16_t Argb4444ToRgb565(uint16_t argb4444) {
    uint8_t r4 = static_cast<uint8_t>((argb4444 >> 8) & 0xF);
    uint8_t g4 = static_cast<uint8_t>((argb4444 >> 4) & 0xF);
    uint8_t b4 = static_cast<uint8_t>(argb4444 & 0xF);
    // Replicate each 4-bit nibble into a full 8-bit channel (0xF -> 0xFF,
    // not 0xF0) before repacking to 5/6/5 -- the usual nibble-widening
    // idiom, not a recovered original constant (MIDP itself stayed in
    // 4444 space and never needed this conversion).
    return PackRGB565(static_cast<uint8_t>((r4 << 4) | r4), static_cast<uint8_t>((g4 << 4) | g4),
                       static_cast<uint8_t>((b4 << 4) | b4));
}

// The port's virtual screen -- see docs/PORT_ROADMAP.md's "176x208 virtual
// canvas" note: like dawnstar (same "ngame" engine, same MIDP Canvas
// resolution-agnostic design, same icon3650.png Nokia 3650 naming), this is
// a port decision, not a recovered original constant. RGB565, matching the
// natural 16bpp GDI presentation format.
class Backbuffer {
public:
    static constexpr int kWidth = 176;
    static constexpr int kHeight = 208;

    Backbuffer() : pixels_(static_cast<size_t>(kWidth) * kHeight, 0) {}

    void Fill(uint16_t rgb565) { std::fill(pixels_.begin(), pixels_.end(), rgb565); }

    void SetPixel(int x, int y, uint16_t rgb565) {
        if (x < 0 || x >= kWidth || y < 0 || y >= kHeight) return;
        pixels_[static_cast<size_t>(y) * kWidth + x] = rgb565;
    }

    void FillRect(int x, int y, int w, int h, uint16_t rgb565) {
        int x0 = std::max(x, 0);
        int y0 = std::max(y, 0);
        int x1 = std::min(x + w, kWidth);
        int y1 = std::min(y + h, kHeight);
        for (int yy = y0; yy < y1; yy++) {
            for (int xx = x0; xx < x1; xx++) {
                pixels_[static_cast<size_t>(yy) * kWidth + xx] = rgb565;
            }
        }
    }

    const uint16_t* Data() const { return pixels_.data(); }

    // M23: GameCanvas's own two RawImage-drawing idioms (see
    // ../src/GameCanvas.java's drawRawImageFull()/drawRawImageFrame(),
    // phase-3 M22) both reduce to the SAME operation once Graphics.setClip()
    // is modeled as an extra column-range argument instead of real
    // stateful clipping: draw the whole RawImage at (x, y) -- optionally
    // mirrored horizontally first (the DirectGraphics manipulation flag
    // 8192 = TRANS_MIRROR both drawWallSegment() and
    // renderWardenCompassIcon() use) -- alpha-tested (IsOpaquePixel()
    // above) and clipped to both the backbuffer itself and an additional
    // [clipX0, clipX1) column range.
    //
    // drawRawImageFull() (chests/dropped items/crystals): call with the
    // default full-width clip and mirrorX=false. drawRawImageFrame()
    // (monster/Warden sprites, and paintWalls()'s own wall segments via
    // render/corridor_render_plan.h's WallDrawCall): the caller passes
    // `x - frame * frameWidth` as this method's `x` (matching the
    // Java's own `x - frame*frameWidth` positioning trick exactly) and
    // [x, x + frameWidth) as the clip range, so only that one frame's
    // column slice of the (possibly multi-frame) spritesheet ends up
    // visible -- the rest is computed and discarded same as the
    // original, not specially cropped out first.
    void Blit(int x, int y, const RawImage& img, int clipX0 = 0, int clipX1 = kWidth, bool mirrorX = false) {
        int x0 = std::max(clipX0, 0);
        int x1 = std::min(clipX1, kWidth);

        for (int sy = 0; sy < img.height; sy++) {
            int dy = y + sy;
            if (dy < 0 || dy >= kHeight) continue;
            for (int sx = 0; sx < img.width; sx++) {
                int dx = x + sx;
                if (dx < x0 || dx >= x1) continue;
                int srcX = mirrorX ? (img.width - 1 - sx) : sx;
                uint16_t pixel = img.pixels[static_cast<size_t>(sy) * static_cast<size_t>(img.width) +
                                             static_cast<size_t>(srcX)];
                if (!IsOpaquePixel(pixel)) continue;
                pixels_[static_cast<size_t>(dy) * kWidth + static_cast<size_t>(dx)] = Argb4444ToRgb565(pixel);
            }
        }
    }

    // M24: the plain-PNG counterpart of Blit() above, for floorTexture/
    // wallTexture/effectImages/hotbarIcons -- GameCanvas's own plain
    // `Image` fields (confirmed real `.png` files via ESGame.java's own
    // asset-loading call sites, e.g. "floor3.png"/"newwallsnok.png"; see
    // assets/decoded_image.h's own header comment), decoded through
    // stb_image rather than M7's from-scratch RawImage/.cus decoder.
    // Alpha-tested only (A == 0 fully transparent, anything else fully
    // opaque) -- same "MIDP hardware this old has no partial alpha
    // blending" reasoning Blit() above and dawnstar's own identical
    // DecodedImage-Blit already use, not a simplification specific to
    // this overload.
    void Blit(int x, int y, const DecodedImage& img, int clipX0 = 0, int clipX1 = kWidth, bool mirrorX = false) {
        int x0 = std::max(clipX0, 0);
        int x1 = std::min(clipX1, kWidth);

        for (int sy = 0; sy < img.height; sy++) {
            int dy = y + sy;
            if (dy < 0 || dy >= kHeight) continue;
            for (int sx = 0; sx < img.width; sx++) {
                int dx = x + sx;
                if (dx < x0 || dx >= x1) continue;
                int srcX = mirrorX ? (img.width - 1 - sx) : sx;
                if (img.A(srcX, sy) == 0) continue;
                pixels_[static_cast<size_t>(dy) * kWidth + static_cast<size_t>(dx)] =
                    PackRGB565(img.R(srcX, sy), img.G(srcX, sy), img.B(srcX, sy));
            }
        }
    }

private:
    std::vector<uint16_t> pixels_;
};

}  // namespace stormhold
