// M63 smoke test (Stormhold M78): assets/gdr_font.h (the Symbian .gdr parser) and
// graphics/bitmap_font.h's device faces, against the real Nokia 3650 ROM
// fonts. Those are Nokia firmware and never committed, so this test SKIPS
// (passes) when port/assets/fonts/Ceurope.gdr isn't there -- CI, or a dev
// who hasn't copied their own in (see the repo .gitignore for where from).
//
// Usage: m63_device_font_smoke [fontDir] [out.ppm]
//   fontDir defaults to ../assets/fonts (build/ -> port/assets/fonts).
//   out.ppm, if given, receives a render of every face for eyeballing.
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "assets/gdr_font.h"
#include "graphics/backbuffer.h"
#include "graphics/bitmap_font.h"

namespace {

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

using namespace dawnstar;

bool HasInk(const GdrGlyph* g) {
    if (!g) return false;
    for (uint8_t b : g->bits) {
        if (b) return true;
    }
    return false;
}

void TestParser(const std::string& ceurope, const std::string& browser) {
    std::printf("-- GdrFont: parses the real stores --\n");
    std::vector<std::string> names = GdrFont::TypefaceNames(ceurope);
    Expect(names.size() == 8, "Ceurope.gdr lists 8 typefaces");
    const char* latin[] = {"LatinBold12", "LatinBold13", "LatinBold17", "LatinBold19", "LatinPlain12"};
    for (const char* want : latin) {
        bool found = false;
        for (const std::string& n : names) found = found || n == want;
        Expect(found, "Ceurope.gdr has every Latin typeface the MIDP runtime maps to");
    }

    struct Case {
        const char* file;
        const char* face;
        int cell;
    };
    const Case cases[] = {{"c", "LatinPlain12", 12}, {"c", "LatinBold12", 12}, {"c", "LatinBold13", 13},
                          {"c", "LatinBold17", 17}, {"b", "alpi17", 17}};
    for (const Case& c : cases) {
        GdrFont font;
        const std::string& path = c.file[0] == 'c' ? ceurope : browser;
        if (!Expect(font.Load(path, c.face), "each face the game uses loads")) continue;
        Expect(font.CellHeight() == c.cell, "cell height matches the typeface's own size");
        for (char ch : std::string("AZaz09!?'")) {
            Expect(HasInk(font.GetGlyph(static_cast<unsigned char>(ch))), "printable ASCII has real ink");
        }
        const GdrGlyph* space = font.GetGlyph(' ');
        Expect(space && space->advance > 0, "space advances the pen");
    }

    GdrFont missing;
    Expect(!missing.Load(ceurope, "NoSuchFace"), "an unknown typeface is a clean failure");
    Expect(!missing.Load(ceurope + ".nope", "LatinBold12"), "a missing file is a clean failure");
}

void TestFaces(const std::string& ceurope) {
    std::printf("-- BitmapFont: device faces --\n");
    Expect(BitmapFont::LoadDeviceFonts(ceurope), "LoadDeviceFonts reports the Latin faces loaded");
    Expect(BitmapFont::IsDeviceFace(BitmapFont::Face::LargeItalic), "Browsereur.gdr beside it gives the italic face");
    Expect(BitmapFont::LineHeight(BitmapFont::Face::SmallBold) == 12, "small = 12px (Font.getHeight)");
    Expect(BitmapFont::LineHeight(BitmapFont::Face::MediumBold) == 13, "medium = 13px");
    Expect(BitmapFont::LineHeight(BitmapFont::Face::LargeItalic) == 17, "large = 17px");
    Expect(BitmapFont::StringWidth("Stormhold", BitmapFont::Face::SmallPlain) <
               BitmapFont::StringWidth("Stormhold", BitmapFont::Face::LargeBold),
           "large text is wider than small");

    // 1-bit: every pixel written is exactly the pen colour.
    Backbuffer bb;
    bb.Fill(PackRGB565(0, 0, 0));
    const uint16_t pen = PackRGB565(255, 255, 0);
    BitmapFont::DrawString(bb, 4, 4, "New Game", pen, BitmapFont::Face::SmallBold);
    int ink = 0, other = 0;
    for (int i = 0; i < Backbuffer::kWidth * Backbuffer::kHeight; i++) {
        const uint16_t px = bb.Data()[i];
        if (px == pen) ink++;
        else if (px != 0) other++;
    }
    Expect(ink > 20, "text drew ink");
    Expect(other == 0, "no anti-aliased in-between colours -- the device fonts are 1-bit");
}

void WriteSpecimen(const std::string& outPath) {
    Backbuffer bb;
    bb.Fill(PackRGB565(0xAE, 0x68, 0x2E));
    bb.FillRect(0, 0, Backbuffer::kWidth, 14, PackRGB565(0, 0, 0));
    const std::string title = "Main Menu";
    BitmapFont::DrawString(bb, (Backbuffer::kWidth - BitmapFont::StringWidth(title, BitmapFont::Face::MediumBold)) / 2,
                           0, title, PackRGB565(255, 255, 255), BitmapFont::Face::MediumBold);
    const uint16_t yellow = PackRGB565(255, 255, 0);
    int y = 20;
    for (const char* line : {"New Game", "Continue Game", "Help", "Credits", "Exit"}) {
        BitmapFont::DrawString(bb, 15, y, line, yellow);
        y += 12;
    }
    BitmapFont::DrawString(bb, 15, y + 4, "Found Iron Dagger", yellow, BitmapFont::Face::SmallPlain);
    BitmapFont::DrawString(bb, 15, y + 20, "N E S W", yellow, BitmapFont::Face::LargeBold);
    BitmapFont::DrawString(bb, 15, y + 40, "You're Dead!", PackRGB565(255, 255, 255), BitmapFont::Face::LargeItalic);
    bb.FillRect(0, 190, Backbuffer::kWidth, 18, PackRGB565(255, 255, 255));
    BitmapFont::DrawString(bb, 10, 192, "Select", PackRGB565(0, 0, 0));
    const std::string back = "Back";
    BitmapFont::DrawString(bb, Backbuffer::kWidth - 10 - BitmapFont::StringWidth(back), 195, back, PackRGB565(0, 0, 0));

    FILE* f = nullptr;
    if (fopen_s(&f, outPath.c_str(), "wb") != 0 || !f) return;
    std::fprintf(f, "P6\n%d %d\n255\n", Backbuffer::kWidth, Backbuffer::kHeight);
    for (int i = 0; i < Backbuffer::kWidth * Backbuffer::kHeight; i++) {
        const uint16_t px = bb.Data()[i];
        const unsigned char rgb[3] = {static_cast<unsigned char>(((px >> 11) & 0x1F) * 255 / 31),
                                      static_cast<unsigned char>(((px >> 5) & 0x3F) * 255 / 63),
                                      static_cast<unsigned char>((px & 0x1F) * 255 / 31)};
        std::fwrite(rgb, 1, 3, f);
    }
    std::fclose(f);
    std::printf("wrote %s\n", outPath.c_str());
}

}  // namespace

int main(int argc, char** argv) {
    const std::filesystem::path dir = argc > 1 ? argv[1] : "../assets/fonts";
    const std::string ceurope = (dir / "Ceurope.gdr").string();
    const std::string browser = (dir / "Browsereur.gdr").string();
    if (!std::filesystem::exists(ceurope)) {
        std::printf("SKIPPED: no %s (Nokia firmware, not in the repo)\n", ceurope.c_str());
        return 0;
    }

    TestParser(ceurope, browser);
    TestFaces(ceurope);
    if (argc > 2) WriteSpecimen(argv[2]);

    if (!g_ok) {
        std::fprintf(stderr, "m63_device_font_smoke: FAILED self-consistency checks\n");
        return 1;
    }
    std::printf("all self-consistency checks passed\n");
    return 0;
}
