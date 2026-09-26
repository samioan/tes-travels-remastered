#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>

#include "assets/image.h"

namespace oblivion {

// The port's virtual screen: 0x00RRGGBB, 176x208 -- the Nokia 3650's canvas
// size (a port decision; the original asks the Canvas for its size and adds
// 25 rows, see Game.setScreenSize).
class Backbuffer {
public:
    static constexpr int kWidth = 176;
    static constexpr int kHeight = 208;

    Backbuffer() : pixels_(static_cast<size_t>(kWidth) * kHeight, 0) {}

    void Fill(uint32_t rgb) { std::fill(pixels_.begin(), pixels_.end(), rgb); }

    void FillRect(int x, int y, int w, int h, uint32_t rgb) {
        int x0 = std::max(x, 0), y0 = std::max(y, 0);
        int x1 = std::min(x + w, kWidth), y1 = std::min(y + h, kHeight);
        for (int yy = y0; yy < y1; yy++)
            for (int xx = x0; xx < x1; xx++) pixels_[static_cast<size_t>(yy) * kWidth + xx] = rgb;
    }

    // Copies the source rectangle (sx, sy, w, h) of `img` to (dx, dy), clipped
    // to the screen and to the clip rectangle [cx0,cx1) x [cy0,cy1). Pixels
    // with alpha < 128 are skipped (MIDP 1.0 style 1-bit transparency). With
    // `mirror` the rectangle is flipped horizontally.
    void Blit(const Image& img, int sx, int sy, int w, int h, int dx, int dy, bool mirror = false,
              int cx0 = 0, int cy0 = 0, int cx1 = kWidth, int cy1 = kHeight) {
        for (int j = 0; j < h; j++) {
            int y = dy + j;
            int srcY = sy + j;
            if (y < cy0 || y >= cy1 || y < 0 || y >= kHeight || srcY < 0 || srcY >= img.height) continue;
            for (int i = 0; i < w; i++) {
                int x = dx + i;
                if (x < cx0 || x >= cx1 || x < 0 || x >= kWidth) continue;
                int srcX = sx + (mirror ? w - 1 - i : i);
                if (srcX < 0 || srcX >= img.width) continue;
                uint32_t p = img.At(srcX, srcY);
                if ((p >> 24) < 128) continue;
                pixels_[static_cast<size_t>(y) * kWidth + x] = p & 0xFFFFFF;
            }
        }
    }

    const uint32_t* Data() const { return pixels_.data(); }

private:
    std::vector<uint32_t> pixels_;
};

}  // namespace oblivion
