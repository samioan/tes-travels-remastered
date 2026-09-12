#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

#include "assets/decoded_image.h"

namespace dawnstar {

constexpr uint16_t PackRGB565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

// The port's virtual screen -- see docs/PORT_ROADMAP.md's "176x208 virtual
// canvas" note: unlike shadowkey-decomp's confirmed-hardware 176x208, this
// is a port decision (no single confirmed target device), not a recovered
// original constant. RGB565, matching the natural 16bpp GDI presentation
// format -- the original MIDP Canvas's real bit depth was never pinned
// down and isn't load-bearing here the way it was for Shadowkey's palette
// data.
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

    const uint16_t* Data() const { return pixels_.data(); }

    // Straight opaque-pixel copy of a DecodedImage at (x, y), clipped to
    // both the backbuffer and an additional [clipX0, clipX1) column
    // range -- GameCanvas.drawWallSegment()'s `g.setClip(x, 0, 18,
    // screenHeight)` before every wall/gate drawImage() call (see
    // render/corridor_render_plan.h's WallDrawCall::clipX). Any pixel
    // with alpha 0 is treated as fully transparent and skipped, anything
    // else as fully opaque -- MIDP's own PNG support on real hardware
    // this old is binary-transparency-only (no alpha blending), so this
    // matches rather than under- or over-simplifying it.
    void Blit(int x, int y, const DecodedImage& img, int clipX0 = 0,
              int clipX1 = std::numeric_limits<int>::max()) {
        int x0 = std::max(clipX0, 0);
        int x1 = std::min(clipX1, kWidth);

        for (int sy = 0; sy < img.height; sy++) {
            int dy = y + sy;
            if (dy < 0 || dy >= kHeight) continue;
            for (int sx = 0; sx < img.width; sx++) {
                int dx = x + sx;
                if (dx < x0 || dx >= x1) continue;
                if (img.A(sx, sy) == 0) continue;
                pixels_[static_cast<size_t>(dy) * kWidth + dx] = PackRGB565(img.R(sx, sy), img.G(sx, sy),
                                                                             img.B(sx, sy));
            }
        }
    }

private:
    std::vector<uint16_t> pixels_;
};

}  // namespace dawnstar
