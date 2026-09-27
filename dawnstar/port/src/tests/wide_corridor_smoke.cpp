// Widescreen corridor check: the 3D raycast against hand-built maps, and the
// Backbuffer width/view mechanics it relies on. Needs imgfiles.lmp from the
// extracted assets (argv[1], default ../../extracted).
//
// With WIDE_PREVIEW_DIR set it also writes side-by-side PPMs (the original
// hand-drawn view, and the 3D view at 16:9) of a few spots, for eyeballing.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "assets/img_archive.h"
#include "graphics/backbuffer.h"
#include "player/player_state.h"
#include "render/frame_renderer.h"
#include "render/wide_corridor.h"
#include "world/dungeon_generator.h"
#include "world/dungeon_view.h"

using namespace dawnstar;

namespace {
int failures = 0;
void Check(bool ok, const std::string& what) {
    std::printf("  %-70s %s\n", what.c_str(), ok ? "ok" : "FAILED");
    failures += !ok;
}

GeneratedLevel MakeLevel(int number, bool solid) {
    GeneratedLevel level;
    level.number = number;
    level.width = 35;
    level.height = 35;
    level.tiles.assign(35, std::vector<uint8_t>(35, solid ? 1 : 0));
    return level;
}

void Carve(GeneratedLevel& l, int x0, int y0, int x1, int y1) {
    for (int x = x0; x <= x1; x++)
        for (int y = y0; y <= y1; y++) l.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] = 0;
}

void WritePpm(const std::string& path, Backbuffer& bb) {
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "wb") != 0 || !f) return;
    const int W = bb.RealWidth(), H = Backbuffer::kHeight;
    std::fprintf(f, "P6\n%d %d\n255\n", W, H);
    bb.ResetView();
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            const uint16_t c = bb.GetPixel(x, y);
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
    ImgArchive images(root + "/imgfiles.lmp");
    const FrameTextures textures = FrameTextures::Load(images);
    const WideTextures wide = WideTextures::Build(textures);

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
        bb.CenterView(Backbuffer::kWidth);
        bb.FillRect(0, 20, 176, 5, PackRGB565(0, 255, 0));
        bb.ExtendViewEdges();
        bb.ResetView();
        Check(bb.GetPixel(0, 22) == PackRGB565(0, 255, 0) && bb.GetPixel(369, 22) == PackRGB565(0, 255, 0),
              "ExtendViewEdges continues the view's edge columns to the sides");
        Check(Backbuffer(277).RealWidth() == 278, "odd widths round up to even (GDI row alignment)");
        Backbuffer narrow;
        Check(narrow.RealWidth() == 176, "the default buffer is still the native 176");
    }

    std::printf("wall textures\n");
    Check(wide.valid(), "the wall, ice and gate art all convert to textures");
    {
        bool anyBlack = false;
        for (uint16_t p : wide.wall.pixels) anyBlack |= (p == 0);
        Check(!anyBlack, "no holes in the recovered stone texture");
    }

    std::printf("3D view\n");
    GeneratedLevel level = MakeLevel(2, true);
    Carve(level, 17, 5, 17, 30);   // a long north-south corridor
    Carve(level, 10, 17, 24, 17);  // crossing east-west corridor
    Carve(level, 20, 8, 26, 12);   // a room to the north-east
    Carve(level, 17, 12, 20, 12);  // ... joined by a passage
    level.tiles[17][9] = 64;       // a gate across the corridor
    std::vector<GeneratedLevel> levels{level};
    DungeonView view(levels, 0);
    GeneratedLevel openLevel = MakeLevel(2, false);
    std::vector<GeneratedLevel> openLevels{openLevel};
    DungeonView openView(openLevels, 0);

    auto render = [&](const DungeonView& v, int w, int x, int y, int facing, int dungeon, bool blind = false) {
        Backbuffer bb(w);
        const int cx = (w - Backbuffer::kWidth) / 2 + 90;
        WideCorridor::Render(bb, cx, textures, wide, WideCorridor::MakeTileQuery(v, x, y, facing), dungeon, blind, false);
        return bb;
    };
    auto wallPixels = [&](const Backbuffer& with, const Backbuffer& without, int x0, int x1) {
        int n = 0;
        for (int y = 0; y < WideCorridor::kViewHeight; y++)
            for (int x = x0; x < x1; x++) n += with.GetPixel(x, y) != without.GetPixel(x, y);
        return n;
    };

    for (int dungeon : {1, 2}) {
        std::printf(" (%s walls)\n", dungeon == 1 ? "stone" : "ice");
        Backbuffer a = render(view, 370, 17, 20, 1, dungeon), bg = render(openView, 370, 17, 20, 1, dungeon);
        const int left = wallPixels(a, bg, 0, 185), right = wallPixels(a, bg, 185, 370);
        Check(left > 2000 && right > 2000, "a corridor shows a wall on both sides");
        Check(wallPixels(a, bg, 0, 20) > 500 && wallPixels(a, bg, 350, 370) > 500,
              "... which reaches the far left and right edges of the widescreen view");
        Check(std::abs(left - right) < 1500, "the two sides carry about the same amount of wall");
    }
    {
        Backbuffer a = render(view, 370, 17, 6, 1, 2), bg = render(openView, 370, 17, 6, 1, 2);  // north end: a wall dead ahead
        int centre = 0;
        for (int y = 0; y < 156; y++) centre += a.GetPixel(185, y) != bg.GetPixel(185, y);
        Check(centre > 60, "a dead end fills the middle of the view");
    }
    {
        Backbuffer a = render(view, 370, 17, 12, 1, 2), bg = render(openView, 370, 17, 12, 1, 2);  // a gate 3 tiles ahead
        int centre = 0;
        for (int y = 0; y < 156; y++) centre += a.GetPixel(187, y) != bg.GetPixel(187, y);
        Check(centre > 20, "a gate ahead is drawn across the corridor");
    }
    {
        Backbuffer a = render(view, 176, 17, 20, 1, 2);
        Backbuffer w = render(view, 370, 17, 20, 1, 2);
        int diff = 0;
        for (int y = 0; y < 156; y++)
            for (int x = 0; x < 176; x++) diff += a.GetPixel(x, y) != w.GetPixel(97 + x, y);
        Check(diff < 176 * 156 / 5, "the centre of the wide view matches the same view at 176 wide");
    }
    {
        Backbuffer a = render(view, 370, 17, 20, 1, 2, /*blind=*/true);
        int lit = 0;
        for (int y = 0; y < 156; y++)
            for (int x = 0; x < 370; x++) lit += a.GetPixel(x, y) != 0;
        Check(lit > 1000, "blind (no floor picture) still draws the walls");
    }

    char* dirBuf = nullptr;
    size_t dirLen = 0;
    _dupenv_s(&dirBuf, &dirLen, "WIDE_PREVIEW_DIR");
    if (const char* dir = dirBuf) {
        struct Spot { const char* name; int x, y, facing, dungeon; };
        const Spot spots[] = {{"corridor", 17, 20, 1, 1}, {"ice", 17, 20, 1, 2}, {"deadend", 17, 7, 1, 2},
                              {"gate", 17, 13, 1, 1},     {"junction", 17, 17, 1, 1}, {"room", 22, 10, 1, 2}};
        for (const Spot& s : spots) {
            GeneratedLevel lv = level;
            lv.number = s.dungeon;
            std::vector<GeneratedLevel> lvs{lv};
            DungeonView v(lvs, 0);
            Backbuffer orig;  // the original hand-drawn view
            PlayerState player;
            FrameRenderer::Render(orig, textures, v, s.x, s.y, s.facing, s.dungeon, player);
            WritePpm(std::string(dir) + "/" + s.name + "_orig.ppm", orig);
            Backbuffer wideBb = render(v, 370, s.x, s.y, s.facing, s.dungeon);
            WritePpm(std::string(dir) + "/" + s.name + "_wide.ppm", wideBb);
        }
        std::printf("previews written to %s\n", dir);
    }
    free(dirBuf);

    std::printf("%s\n", failures ? "FAILED" : "OK");
    return failures ? 1 : 0;
}
