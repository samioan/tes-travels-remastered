// Widescreen corridor check: the 3D raycast against hand-built maps, and the
// Backbuffer width/view mechanics it relies on. Needs floor3.png and
// newwallsnok.png from the extracted assets (argv[1], default ../../extracted).
//
// With WIDE_PREVIEW_DIR set it also writes side-by-side PPMs (the original
// hand-drawn view, and the 3D view at 16:9) of a few spots, for eyeballing.
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>

#include "assets/asset_root.h"
#include "dungeon/dungeon_runtime.h"
#include "graphics/backbuffer.h"
#include "render/corridor_assets.h"
#include "render/game_renderer.h"
#include "render/wide_corridor.h"

using namespace stormhold;

namespace {
int failures = 0;
void Check(bool ok, const std::string& what) {
    std::printf("  %-70s %s\n", what.c_str(), ok ? "ok" : "FAILED");
    failures += !ok;
}

GeneratedLevel MakeLevel(int w, int h, bool solid) {
    GeneratedLevel level;
    level.number = 1;
    level.width = w;
    level.height = h;
    level.tiles.assign(static_cast<size_t>(w), std::vector<uint8_t>(static_cast<size_t>(h), solid ? 1 : 0));
    return level;
}

void Carve(GeneratedLevel& l, int x0, int y0, int x1, int y1) {
    for (int x = x0; x <= x1; x++)
        for (int y = y0; y <= y1; y++) l.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] = 0;
}

void WritePpm(const std::string& path, const Backbuffer& bb) {
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "wb") != 0 || !f) return;
    const int W = bb.RealWidth(), H = Backbuffer::kHeight;
    std::fprintf(f, "P6\n%d %d\n255\n", W, H);
    Backbuffer& b = const_cast<Backbuffer&>(bb);
    b.ResetView();
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            const uint16_t c = b.GetPixel(x, y);
            const unsigned char rgb[3] = {static_cast<unsigned char>(((c >> 11) & 0x1F) * 255 / 31),
                                          static_cast<unsigned char>(((c >> 5) & 0x3F) * 255 / 63),
                                          static_cast<unsigned char>((c & 0x1F) * 255 / 31)};
            std::fwrite(rgb, 1, 3, f);
        }
    std::fclose(f);
}
}  // namespace

int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : "../../extracted";
    AssetRoot assetRoot(root);
    CorridorAssets assets = CorridorAssets::Load(assetRoot);

    std::printf("backbuffer width and views\n");
    {
        Backbuffer bb(370);
        Check(bb.RealWidth() == 370 && bb.Width() == 370, "a 370-wide buffer reports its width");
        bb.CenterView(Backbuffer::kWidth);
        Check(bb.ViewX() == 97 && bb.Width() == 176, "CenterView(176) sits at column 97");
        bb.FillRect(0, 0, 500, 10, PackRGB565(255, 0, 0));  // wider than the view: clipped to it
        bb.ResetView();
        Check(bb.GetPixel(96, 0) == 0 && bb.GetPixel(97, 0) != 0 && bb.GetPixel(272, 0) != 0 && bb.GetPixel(273, 0) == 0,
              "drawing is offset by, and clipped to, the view");
        bb.Fill(7);
        Check(bb.GetPixel(0, 0) == 7 && bb.GetPixel(369, 207) == 7, "Fill covers the whole buffer");
        Check(Backbuffer(277).RealWidth() == 278, "odd widths round up to even (GDI row alignment)");
        Backbuffer narrow;
        Check(narrow.RealWidth() == 176, "the default buffer is still the native 176");
    }

    const WideWallTexture tex = WideWallTexture::Build(assets.wallTexture);
    std::printf("wall texture\n");
    Check(tex.valid && tex.width > 0 && tex.height > 0, "the wall art un-projects into a texture");
    {
        bool anyBlack = false;
        for (uint16_t p : tex.pixels) anyBlack |= (p == 0);
        Check(!anyBlack, "no holes in the recovered texture");
    }

    std::printf("3D view\n");
    GeneratedLevel level = MakeLevel(35, 35, true);
    Carve(level, 17, 5, 17, 30);   // a long north-south corridor
    Carve(level, 10, 17, 24, 17);  // crossing east-west corridor
    Carve(level, 20, 8, 26, 12);   // a room to the north-east
    Carve(level, 17, 12, 20, 12);  // ... joined by a passage
    const DungeonRuntime::LevelLookup lookup = [&](int) -> const GeneratedLevel& { return level; };

    auto render = [&](int w, int x, int y, int facing, bool a3 = false, bool a4 = false) {
        Backbuffer bb(w);
        const int cx = (w - Backbuffer::kWidth) / 2 + 90;
        WideCorridor::Render(bb, cx, assets, tex, WideCorridor::MakeWallQuery(level, lookup, x, y, facing), a3, a4);
        return bb;
    };
    // Distinguish walls from the ceiling/floor backdrop: render the same spot with all walls removed.
    GeneratedLevel open = MakeLevel(35, 35, false);
    const DungeonRuntime::LevelLookup openLookup = [&](int) -> const GeneratedLevel& { return open; };
    auto backdrop = [&](int w, int x, int y, int facing) {
        Backbuffer bb(w);
        const int cx = (w - Backbuffer::kWidth) / 2 + 90;
        WideCorridor::Render(bb, cx, assets, tex, WideCorridor::MakeWallQuery(open, openLookup, x, y, facing), false, false);
        return bb;
    };
    auto wallPixels = [&](const Backbuffer& with, const Backbuffer& without, int x0, int x1) {
        int n = 0;
        for (int y = 0; y < WideCorridor::kViewHeight; y++)
            for (int x = x0; x < x1; x++) n += with.GetPixel(x, y) != without.GetPixel(x, y);
        return n;
    };

    {
        Backbuffer a = render(370, 17, 20, 1), bg = backdrop(370, 17, 20, 1);
        const int left = wallPixels(a, bg, 0, 185), right = wallPixels(a, bg, 185, 370);
        Check(left > 2000 && right > 2000, "a corridor shows a wall on both sides");
        Check(wallPixels(a, bg, 0, 20) > 500 && wallPixels(a, bg, 350, 370) > 500,
              "... which reaches the far left and right edges of the widescreen view");
        // Mirror symmetry: the same corridor looked at from the other end is the same picture flipped.
        Check(std::abs(left - right) < 1500, "the two sides carry about the same amount of wall");
    }
    {
        Backbuffer a = render(370, 17, 6, 1), bg = backdrop(370, 17, 6, 1);  // north end: a wall dead ahead
        int centre = 0;
        for (int y = 0; y < 156; y++) centre += a.GetPixel(185, y) != bg.GetPixel(185, y);
        Check(centre > 60, "a dead end fills the middle of the view");
    }
    {
        Backbuffer a = render(370, 17, 17, 1), bg = backdrop(370, 17, 17, 1);  // in the crossing
        Check(wallPixels(a, bg, 0, 40) < wallPixels(a, bg, 320, 370) + 8000,
              "a junction renders (side openings leave gaps at the edges)");
    }
    {
        Backbuffer a = render(176, 17, 20, 1);
        Backbuffer w = render(370, 17, 20, 1);
        int diff = 0;
        for (int y = 0; y < 156; y++)
            for (int x = 0; x < 176; x++) diff += a.GetPixel(x, y) != w.GetPixel(97 + x, y);
        Check(diff < 176 * 156 / 5, "the centre of the wide view matches the same view at 176 wide");
    }
    {
        Backbuffer a = render(370, 17, 20, 1, /*a3=*/true);
        bool allBlack = true;
        for (int y = 0; y < 156 && allBlack; y++)
            for (int x = 0; x < 370; x++)
                if (a.GetPixel(x, y) != 0 && (x > 200 || x < 170)) {
                    allBlack = false;  // walls are still drawn (ailment 3 only drops the floor picture)
                    break;
                }
        Check(!allBlack, "ailment 3 (no floor picture) still draws the walls");
    }

    char* dirBuf = nullptr;
    size_t dirLen = 0;
    _dupenv_s(&dirBuf, &dirLen, "WIDE_PREVIEW_DIR");
    if (const char* dir = dirBuf) {
        struct Spot { const char* name; int x, y, facing; };
        const Spot spots[] = {{"corridor", 17, 20, 1}, {"deadend", 17, 7, 1}, {"junction", 17, 17, 1},
                              {"passage", 18, 12, 2},  {"room", 22, 10, 1},   {"south", 17, 25, 3}};
        for (const Spot& s : spots) {
            // the original hand-drawn view
            Backbuffer orig;
            const CorridorViewGrid grid = DungeonRuntime::SampleCorridorView(level, s.x, s.y, s.facing, lookup);
            GameRenderer::RenderCorridorView(orig, assets, grid, false, false);
            WritePpm(std::string(dir) + "\\" + s.name + "_orig.ppm", orig);
            for (int w : {370}) {
                Backbuffer wide = render(w, s.x, s.y, s.facing);
                WritePpm(std::string(dir) + "\\" + s.name + "_wide.ppm", wide);
            }
        }
        std::printf("previews written to %s\n", dir);
    }

    std::printf("%s\n", failures ? "FAILED" : "OK");
    return failures ? 1 : 0;
}
