#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>

namespace dawnstar {

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

private:
    std::vector<uint16_t> pixels_;
};

constexpr uint16_t PackRGB565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

}  // namespace dawnstar
