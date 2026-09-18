#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>

namespace stormhold {

constexpr uint16_t PackRGB565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

// The port's virtual screen -- see docs/PORT_ROADMAP.md's "176x208 virtual
// canvas" note: like dawnstar (same "ngame" engine, same MIDP Canvas
// resolution-agnostic design, same icon3650.png Nokia 3650 naming), this is
// a port decision, not a recovered original constant. RGB565, matching the
// natural 16bpp GDI presentation format.
//
// No Blit() yet -- this milestone doesn't decode PNGs yet (RawImage's own
// decoded short[] pixel format is a later milestone's job), so there's
// nothing to composite onto the backbuffer beyond flat fills.
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

private:
    std::vector<uint16_t> pixels_;
};

}  // namespace stormhold
