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
//
// Widescreen: the buffer can be wider than the native 176 columns. A *view*
// narrows drawing to a column of it: every draw call is translated by the
// view's offset and clipped to it, so the HUD, menus and sprites -- all laid
// out for 176 columns -- draw unchanged into a centred 176-wide view while
// the 3D corridor fills the whole width.
class Backbuffer {
public:
    static constexpr int kWidth = 176;  // the native width: what every layout is written against
    static constexpr int kHeight = 208;
    static constexpr int kMaxWidth = 554;  // 24:9 at this height

    explicit Backbuffer(int width = kWidth) { Resize(width); }

    // Reallocates for a new width (clears to black) and resets the view.
    void Resize(int width) {
        // Even widths only: GDI wants each 16-bit row padded to 4 bytes, and the rows are
        // presented as stored -- an odd width would shear the picture.
        w_ = std::max(kWidth, std::min(kMaxWidth, width + (width & 1)));
        pixels_.assign(static_cast<size_t>(w_) * kHeight, 0);
        vx_ = 0;
        vw_ = w_;
    }

    int RealWidth() const { return w_; }
    // The width drawing code lays out against: the view's, not the buffer's.
    int Width() const { return vw_; }

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

    // Fills the columns beside the view with the view's own edge columns, row by row: a screen
    // that is a flat colour (title bar, body, soft-key bar) simply continues to the sides.
    void ExtendViewEdges(int y0 = 0, int y1 = kHeight) {
        for (int y = std::max(0, y0); y < std::min(kHeight, y1); y++) {
            uint16_t* row = &pixels_[static_cast<size_t>(y) * w_];
            std::fill(row, row + vx_, row[vx_]);
            std::fill(row + vx_ + vw_, row + w_, row[vx_ + vw_ - 1]);
        }
    }

    // View-relative read (0 outside the view).
    uint16_t GetPixel(int x, int y) const {
        if (x < 0 || x >= vw_ || y < 0 || y >= kHeight) return 0;
        return pixels_[static_cast<size_t>(y) * w_ + vx_ + x];
    }

    void Fill(uint16_t rgb565) { std::fill(pixels_.begin(), pixels_.end(), rgb565); }

    void SetPixel(int x, int y, uint16_t rgb565) {
        if (x < 0 || x >= vw_ || y < 0 || y >= kHeight) return;
        pixels_[static_cast<size_t>(y) * w_ + vx_ + x] = rgb565;
    }

    // Graphics.fillRect()'s counterpart -- clipped to the backbuffer. A
    // non-positive w/h simply draws nothing (MIDP's own fillRect leaves
    // negative widths/heights undefined; this port just no-ops them,
    // matching the empty-clip-range GameCanvas itself would effectively
    // see).
    void FillRect(int x, int y, int w, int h, uint16_t rgb565) {
        int x0 = std::max(x, 0);
        int y0 = std::max(y, 0);
        int x1 = std::min(x + w, vw_);
        int y1 = std::min(y + h, kHeight);
        for (int yy = y0; yy < y1; yy++) {
            for (int xx = x0; xx < x1; xx++) {
                pixels_[static_cast<size_t>(yy) * w_ + vx_ + xx] = rgb565;
            }
        }
    }

    // Graphics.fillRoundRect()'s counterpart -- M30's message-popup
    // background is this port's only real call site. Unlike FillRect/
    // Blit's own MIDP-precedent-bug preservation, there's no game-data
    // bug to reproduce here: `arcWidth`/`arcHeight` are plain call-site
    // constants (GameCanvas's own `g.fillRoundRect(96, 118, 75, 35, 5,
    // 5)`, no operator-precedence trap involved), so this is a
    // straightforward reimplementation of the documented MIDP
    // primitive -- each corner is cut to a quarter-ellipse of
    // `arcWidth`x`arcHeight`, tested via the standard normalized-
    // ellipse-distance formula.
    void FillRoundRect(int x, int y, int w, int h, int arcWidth, int arcHeight, uint16_t rgb565) {
        if (w <= 0 || h <= 0) return;
        double rx = arcWidth / 2.0;
        double ry = arcHeight / 2.0;
        int x0 = std::max(x, 0);
        int y0 = std::max(y, 0);
        int x1 = std::min(x + w, vw_);
        int y1 = std::min(y + h, kHeight);

        for (int yy = y0; yy < y1; yy++) {
            for (int xx = x0; xx < x1; xx++) {
                double dx = 0.0;
                double dy = 0.0;
                bool inCornerBox = false;
                if (xx < x + rx && yy < y + ry) {
                    dx = (x + rx) - xx - 0.5;
                    dy = (y + ry) - yy - 0.5;
                    inCornerBox = true;
                } else if (xx >= x + w - rx && yy < y + ry) {
                    dx = xx - (x + w - rx) + 0.5;
                    dy = (y + ry) - yy - 0.5;
                    inCornerBox = true;
                } else if (xx < x + rx && yy >= y + h - ry) {
                    dx = (x + rx) - xx - 0.5;
                    dy = yy - (y + h - ry) + 0.5;
                    inCornerBox = true;
                } else if (xx >= x + w - rx && yy >= y + h - ry) {
                    dx = xx - (x + w - rx) + 0.5;
                    dy = yy - (y + h - ry) + 0.5;
                    inCornerBox = true;
                }

                if (inCornerBox && rx > 0.0 && ry > 0.0) {
                    double nx = dx / rx;
                    double ny = dy / ry;
                    if (nx * nx + ny * ny > 1.0) continue;
                }

                pixels_[static_cast<size_t>(yy) * w_ + vx_ + xx] = rgb565;
            }
        }
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
        int x1 = std::min(clipX1, vw_);

        for (int sy = 0; sy < img.height; sy++) {
            int dy = y + sy;
            if (dy < 0 || dy >= kHeight) continue;
            for (int sx = 0; sx < img.width; sx++) {
                int dx = x + sx;
                if (dx < x0 || dx >= x1) continue;
                if (img.A(sx, sy) == 0) continue;
                pixels_[static_cast<size_t>(dy) * w_ + vx_ + dx] = PackRGB565(img.R(sx, sy), img.G(sx, sy),
                                                                             img.B(sx, sy));
            }
        }
    }

private:
    int w_ = kWidth;   // buffer width
    int vx_ = 0;       // view: first column
    int vw_ = kWidth;  // view: width
    std::vector<uint16_t> pixels_;
};

}  // namespace dawnstar
