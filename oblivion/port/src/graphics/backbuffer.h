#pragma once
#include <algorithm>
#include <climits>
#include <cstdint>
#include <vector>

#include "assets/image.h"

namespace oblivion {

// The port's virtual screen: 0x00RRGGBB, always 208 rows tall (the Nokia 3650's
// canvas height -- a port decision; the original asks the Canvas for its size
// and adds 25 rows, see Game.setScreenSize) and, by default, 176 wide. The
// width can grow for widescreen: sprites and HUD keep their native pixel size,
// only more of the world shows at the sides.
//
// A *view* narrows drawing to a column of the buffer: every draw call is
// translated by the view's offset and clipped to it, and Width() reports the
// view's width. Screens that were laid out for 176 columns (menus, text, the
// inventory) draw into a centred 176-wide view, so nothing is stretched.
class Backbuffer {
public:
    static constexpr int kWidth = 176;   // the original canvas width (the narrowest buffer)
    static constexpr int kHeight = 208;
    static constexpr int kMaxWidth = 554;  // 24:9 at this height

    explicit Backbuffer(int width = kWidth) { Resize(width); }

    // Reallocates for a new width (clears to black) and resets the view.
    void Resize(int width) {
        w_ = std::max(kWidth, std::min(kMaxWidth, width));
        pixels_.assign(static_cast<size_t>(w_) * kHeight, 0);
        vx_ = 0;
        vw_ = w_;
    }

    int RealWidth() const { return w_; }
    // The width drawing code should lay out against: the view's, not the buffer's.
    int Width() const { return vw_; }
    static constexpr int Height() { return kHeight; }

    // Draw into the column [x, x + width) of the buffer; coordinates become relative to it.
    void SetView(int x, int width) {
        vx_ = std::max(0, std::min(x, w_ - 1));
        vw_ = std::max(1, std::min(width, w_ - vx_));
    }
    void ResetView() {
        vx_ = 0;
        vw_ = w_;
    }
    // A view of `width` columns centred in the buffer.
    void CenterView(int width) { SetView((w_ - width) / 2, width); }
    int ViewX() const { return vx_; }

    // Fills the whole buffer, view or not (the bars beside a centred screen take the colour too).
    void Fill(uint32_t rgb) { std::fill(pixels_.begin(), pixels_.end(), rgb); }

    void FillRect(int x, int y, int w, int h, uint32_t rgb) {
        int x0 = std::max(x, 0), y0 = std::max(y, 0);
        int x1 = std::min(x + w, vw_), y1 = std::min(y + h, kHeight);
        for (int yy = y0; yy < y1; yy++)
            for (int xx = x0; xx < x1; xx++) pixels_[static_cast<size_t>(yy) * w_ + vx_ + xx] = rgb;
    }

    // Graphics.drawRect: outline of a (w+1) x (h+1) box, like MIDP.
    void DrawRect(int x, int y, int w, int h, uint32_t rgb) {
        for (int i = 0; i <= w; i++) {
            SetPixel(x + i, y, rgb);
            SetPixel(x + i, y + h, rgb);
        }
        for (int j = 0; j <= h; j++) {
            SetPixel(x, y + j, rgb);
            SetPixel(x + w, y + j, rgb);
        }
    }

    // Copies the source rectangle (sx, sy, w, h) of `img` to (dx, dy), clipped
    // to the view and to the clip rectangle [cx0,cx1) x [cy0,cy1). Pixels
    // with alpha < 128 are skipped (MIDP 1.0 style 1-bit transparency). With
    // `mirror` the rectangle is flipped horizontally.
    void Blit(const Image& img, int sx, int sy, int w, int h, int dx, int dy, bool mirror = false,
              int cx0 = 0, int cy0 = 0, int cx1 = INT_MAX, int cy1 = INT_MAX) {
        cx1 = std::min(cx1, vw_);
        cy1 = std::min(cy1, kHeight);
        for (int j = 0; j < h; j++) {
            int y = dy + j;
            int srcY = sy + j;
            if (y < cy0 || y >= cy1 || y < 0 || srcY < 0 || srcY >= img.height) continue;
            for (int i = 0; i < w; i++) {
                int x = dx + i;
                if (x < cx0 || x >= cx1 || x < 0) continue;
                int srcX = sx + (mirror ? w - 1 - i : i);
                if (srcX < 0 || srcX >= img.width) continue;
                uint32_t p = img.At(srcX, srcY);
                if ((p >> 24) < 128) continue;
                pixels_[static_cast<size_t>(y) * w_ + vx_ + x] = p & 0xFFFFFF;
            }
        }
    }

    // Copies all of `src` (its whole buffer) into the top-left of the current view.
    void CopyIn(const Backbuffer& src) {
        const int cols = std::min(src.w_, vw_);
        for (int y = 0; y < kHeight; y++)
            for (int x = 0; x < cols; x++)
                pixels_[static_cast<size_t>(y) * w_ + vx_ + x] = src.pixels_[static_cast<size_t>(y) * src.w_ + x];
    }

    void SetPixel(int x, int y, uint32_t rgb) {
        if (x >= 0 && y >= 0 && x < vw_ && y < kHeight) pixels_[static_cast<size_t>(y) * w_ + vx_ + x] = rgb;
    }

    // The whole buffer, RealWidth() x kHeight, row-major.
    const uint32_t* Data() const { return pixels_.data(); }

private:
    int w_ = kWidth, vx_ = 0, vw_ = kWidth;
    std::vector<uint32_t> pixels_;
};

}  // namespace oblivion
