#include "graphics/text.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>

#include "assets/gdr_font.h"

namespace oblivion {
namespace Text {

namespace {

constexpr int kFaceCount = 4;

GdrFont& Slot(Face f) {
    static GdrFont slots[kFaceCount];
    return slots[static_cast<int>(f)];
}

// The font DrawString uses for `face`, or nullptr for the GDI stand-in.
const GdrFont* Device(Face face) {
    const GdrFont& f = Slot(face);
    if (f.IsLoaded()) return &f;
    if (face == Face::MediumPlain && Slot(Face::SmallPlain).IsLoaded()) return &Slot(Face::SmallPlain);
    return nullptr;
}

const GdrGlyph* Glyph(const GdrFont& font, char c) {
    const GdrGlyph* g = font.GetGlyph(static_cast<unsigned char>(c));
    return g ? g : font.GetGlyph('?');
}

// ---- GDI stand-in: 1-bit Arial, used only when no ROM fonts are present. ----

constexpr int kScratchW = 400, kScratchH = 24;

struct Gdi {
    HDC dc = nullptr;
    void* bits = nullptr;
    HFONT fonts[kFaceCount] = {};

    Gdi() {
        dc = CreateCompatibleDC(nullptr);
        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = kScratchW;
        bmi.bmiHeader.biHeight = -kScratchH;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        SelectObject(dc, CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0));
        const int heights[kFaceCount] = {10, 11, 10, 15};
        const int weights[kFaceCount] = {FW_NORMAL, FW_NORMAL, FW_BOLD, FW_BOLD};
        for (int i = 0; i < kFaceCount; i++)
            fonts[i] = CreateFontA(-heights[i], 0, 0, 0, weights[i], FALSE, FALSE, FALSE, ANSI_CHARSET,
                                   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
                                   DEFAULT_PITCH | FF_SWISS, "Arial");
        SetBkMode(dc, OPAQUE);
        SetBkColor(dc, RGB(0, 0, 0));
        SetTextColor(dc, RGB(255, 255, 255));
    }
};

Gdi& GdiCtx() {
    static Gdi g;
    return g;
}

SIZE GdiMeasure(const std::string& text, Face face) {
    Gdi& g = GdiCtx();
    SelectObject(g.dc, g.fonts[static_cast<int>(face)]);
    SIZE s{0, 0};
    if (!text.empty()) GetTextExtentPoint32A(g.dc, text.c_str(), static_cast<int>(text.size()), &s);
    return s;
}

void GdiDraw(Backbuffer& bb, int x, int y, const std::string& text, uint32_t rgb, Face face) {
    if (text.empty()) return;
    Gdi& g = GdiCtx();
    SIZE ext = GdiMeasure(text, face);
    RECT all{0, 0, kScratchW, kScratchH};
    FillRect(g.dc, &all, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    TextOutA(g.dc, 0, 0, text.c_str(), static_cast<int>(text.size()));
    GdiFlush();
    const uint32_t* px = static_cast<const uint32_t*>(g.bits);
    for (int sy = 0; sy < std::min<int>(ext.cy, kScratchH); sy++)
        for (int sx = 0; sx < std::min<int>(ext.cx, kScratchW); sx++)
            if ((px[sy * kScratchW + sx] & 0xFF) >= 128) bb.SetPixel(x + sx, y + sy, rgb);
}

}  // namespace

bool LoadDeviceFonts(const std::string& dir) {
    namespace fs = std::filesystem;
    std::error_code ec;
    std::string ceurope, browser;
    for (const fs::directory_entry& e : fs::directory_iterator(dir, ec)) {
        std::string name = e.path().filename().string();
        for (char& c : name) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (name == "ceurope.gdr") ceurope = e.path().string();
        else if (name == "browsereur.gdr") browser = e.path().string();
    }
    bool latin = !ceurope.empty();
    if (latin) {
        latin &= Slot(Face::SmallPlain).Load(ceurope, "LatinPlain12");
        latin &= Slot(Face::SmallBold).Load(ceurope, "LatinBold12");
        latin &= Slot(Face::LargeBold).Load(ceurope, "LatinBold17");
    }
    if (!browser.empty()) Slot(Face::MediumPlain).Load(browser, "Alp13");
    std::printf("Text: device fonts from %s: %s\n", dir.c_str(), latin ? "loaded" : "not found (using GDI stand-in)");
    return latin;
}

bool IsDeviceFace(Face face) { return Slot(face).IsLoaded(); }

int StringWidth(const std::string& text, Face face) {
    if (const GdrFont* f = Device(face)) {
        int w = 0;
        for (char c : text)
            if (const GdrGlyph* g = Glyph(*f, c)) w += g->advance;
        return w;
    }
    return static_cast<int>(GdiMeasure(text, face).cx);
}

int SubstringWidth(const std::string& text, size_t offset, size_t length, Face face) {
    if (offset >= text.size()) return 0;
    return StringWidth(text.substr(offset, length), face);
}

int LineHeight(Face face) {
    if (const GdrFont* f = Device(face)) return f->CellHeight();
    return static_cast<int>(GdiMeasure("Ag", face).cy);
}

void DrawString(Backbuffer& bb, int x, int y, const std::string& text, uint32_t rgb, Face face) {
    const GdrFont* f = Device(face);
    if (!f) {
        GdiDraw(bb, x, y, text, rgb, face);
        return;
    }
    // 1-bit, like the device: ink pixels take the pen colour, the rest is untouched.
    const int baseline = y + f->Ascent();
    int pen = x;
    for (char c : text) {
        const GdrGlyph* g = Glyph(*f, c);
        if (!g) continue;
        const int left = pen + g->leftBearing, top = baseline - g->ascent;
        for (int row = 0; row < g->height; row++)
            for (int col = 0; col < g->width; col++)
                if (g->bits[static_cast<size_t>(row) * g->width + col]) bb.SetPixel(left + col, top + row, rgb);
        pen += g->advance;
    }
}

}  // namespace Text
}  // namespace oblivion
