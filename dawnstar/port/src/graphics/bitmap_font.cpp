#include "graphics/bitmap_font.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <cstdint>

namespace dawnstar {

namespace {

// Logical (GDI) pixel height requested from CreateFont -- tuned to fit
// comfortably inside every existing fixed row-height/line-spacing
// constant this port's UI code already uses (ui/screen.cpp's own
// kLineHeight=12, ui/boot_splash.cpp's 14px copyright-card line
// spacing, render/message_popup.cpp's 12px gap between its own two
// lines, render/minimap_renderer.cpp's ~10-15px compass-glyph
// clearance) -- those were already sized for a real font, not this
// port's old 7px-tall invented one, so they didn't need to change.
constexpr int kFontHeightPx = 10;

// A generously oversized scratch canvas every DrawString/StringWidth
// call reuses -- comfortably larger than any real string this port
// ever draws (menu titles, item names, popup lines), so no per-call
// resizing is needed. Text landing outside it is simply not measured/
// drawn, the same "silently drops what doesn't fit" behavior FillRect/
// Blit's own clipping already has elsewhere in this port.
constexpr int kScratchWidth = 400;
constexpr int kScratchHeight = 40;

// Lazily-constructed, process-lifetime GDI state: one memory DC, one
// 32bpp top-down DIB section selected into it (so its pixels are both
// GDI-drawable and directly readable by DrawString below), and one
// bold font selected into it. Never torn down -- a single persistent
// font/DC for a small game process's whole lifetime is the same
// "acceptable to just hold onto" call this port already makes for
// other process-lifetime resources (e.g. the launcher's own single
// window class registration).
class GdiFontContext {
public:
    static GdiFontContext& Instance() {
        static GdiFontContext instance;
        return instance;
    }

    HDC dc() const { return dc_; }
    const uint32_t* Bits() const { return static_cast<const uint32_t*>(bits_); }

    // Clears the scratch canvas to black and draws `text` at (0,0),
    // white-on-black -- so every channel of every resulting pixel
    // directly IS its own text coverage (0 = background, 255 = fully
    // inside a glyph stroke), with no color-fringing to account for
    // (ANTIALIASED_QUALITY below is plain grayscale anti-aliasing, not
    // ClearType's subpixel-color kind, which would assume a specific
    // physical subpixel layout this scaled-up virtual screen has no
    // equivalent of). Returns the real measured extent, clamped to the
    // scratch canvas's own size.
    SIZE RenderToScratch(const std::string& text) {
        RECT full{0, 0, kScratchWidth, kScratchHeight};
        FillRect(dc_, &full, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        SIZE extent{0, 0};
        if (!text.empty()) {
            GetTextExtentPoint32A(dc_, text.c_str(), static_cast<int>(text.size()), &extent);
            TextOutA(dc_, 0, 0, text.c_str(), static_cast<int>(text.size()));
            GdiFlush();
        }
        extent.cx = std::min<LONG>(extent.cx, kScratchWidth);
        extent.cy = std::min<LONG>(extent.cy, kScratchHeight);
        return extent;
    }

    SIZE Measure(const std::string& text) const {
        SIZE extent{0, 0};
        if (!text.empty()) GetTextExtentPoint32A(dc_, text.c_str(), static_cast<int>(text.size()), &extent);
        return extent;
    }

private:
    GdiFontContext() {
        dc_ = CreateCompatibleDC(nullptr);

        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = kScratchWidth;
        bmi.bmiHeader.biHeight = -kScratchHeight;  // negative = top-down, row 0 first
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bitmap_ = CreateDIBSection(dc_, &bmi, DIB_RGB_COLORS, &bits_, nullptr, 0);
        SelectObject(dc_, bitmap_);

        // "Arial Black" is a real, distinct heavy-weight font family
        // (not just FW_BOLD on regular Arial) bundled with Windows
        // since the Office/IE font packages of the late 90s/early 2000s
        // -- the closest broadly-available match to the KEmulator
        // screenshot's own very heavy stroke weight. GDI's font mapper
        // falls back to the closest installed FF_SWISS/FW_BLACK match
        // on a system that somehow lacks it, same graceful-degradation
        // behavior any named CreateFont call already has.
        font_ = CreateFontA(-kFontHeightPx, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, ANSI_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                            DEFAULT_PITCH | FF_SWISS, "Arial Black");
        SelectObject(dc_, font_);
        SetBkMode(dc_, OPAQUE);
        SetBkColor(dc_, RGB(0, 0, 0));
        SetTextColor(dc_, RGB(255, 255, 255));
    }

    // Deliberately no destructor -- see this class's own doc comment.
    GdiFontContext(const GdiFontContext&) = delete;
    GdiFontContext& operator=(const GdiFontContext&) = delete;

    HDC dc_ = nullptr;
    HBITMAP bitmap_ = nullptr;
    HFONT font_ = nullptr;
    void* bits_ = nullptr;
};

}  // namespace

int BitmapFont::CharWidth(char c) {
    const char s[2] = {c, '\0'};
    return StringWidth(std::string(s));
}

int BitmapFont::StringWidth(const std::string& text) {
    return static_cast<int>(GdiFontContext::Instance().Measure(text).cx);
}

void BitmapFont::DrawString(Backbuffer& bb, int x, int y, const std::string& text, uint16_t rgb565) {
    if (text.empty()) return;

    GdiFontContext& ctx = GdiFontContext::Instance();
    SIZE extent = ctx.RenderToScratch(text);
    const uint32_t* pixels = ctx.Bits();

    const uint8_t tr = static_cast<uint8_t>(((rgb565 >> 11) & 0x1F) * 255 / 31);
    const uint8_t tg = static_cast<uint8_t>(((rgb565 >> 5) & 0x3F) * 255 / 63);
    const uint8_t tb = static_cast<uint8_t>((rgb565 & 0x1F) * 255 / 31);

    for (int sy = 0; sy < extent.cy; sy++) {
        int dy = y + sy;
        if (dy < 0 || dy >= Backbuffer::kHeight) continue;
        for (int sx = 0; sx < extent.cx; sx++) {
            int dx = x + sx;
            if (dx < 0 || dx >= Backbuffer::kWidth) continue;

            // White-on-black source: every channel already equals this
            // pixel's own text coverage (0-255) -- see RenderToScratch's
            // own doc comment.
            const uint32_t px = pixels[sy * kScratchWidth + sx];
            const uint8_t coverage = static_cast<uint8_t>(px & 0xFF);
            if (coverage == 0) continue;

            if (coverage == 255) {
                bb.SetPixel(dx, dy, rgb565);
                continue;
            }

            const uint16_t existing = bb.Data()[static_cast<size_t>(dy) * Backbuffer::kWidth + dx];
            const uint8_t er = static_cast<uint8_t>(((existing >> 11) & 0x1F) * 255 / 31);
            const uint8_t eg = static_cast<uint8_t>(((existing >> 5) & 0x3F) * 255 / 63);
            const uint8_t eb = static_cast<uint8_t>((existing & 0x1F) * 255 / 31);

            const uint8_t r = static_cast<uint8_t>((tr * coverage + er * (255 - coverage)) / 255);
            const uint8_t g = static_cast<uint8_t>((tg * coverage + eg * (255 - coverage)) / 255);
            const uint8_t b = static_cast<uint8_t>((tb * coverage + eb * (255 - coverage)) / 255);
            bb.SetPixel(dx, dy, PackRGB565(r, g, b));
        }
    }
}

}  // namespace dawnstar
