#include "render/hotbar_renderer.h"

#include <string>

#include "graphics/bitmap_font.h"

namespace dawnstar {

HotbarTextures HotbarTextures::Load(const ImgArchive& archive) {
    HotbarTextures t;
    t.icons = DecodedImage::FromPng(archive.Data("icons.png"));
    t.panel = DecodedImage::FromPng(archive.Data("panel.png"));
    return t;
}

int HotbarRenderer::ComputeHotbarContext(bool monsterTargeted, bool chestInSight, int npcInSight) {
    if (monsterTargeted) return 1;
    return (!chestInSight && npcInSight < 0) ? 0 : 2;
}

namespace {

// GameCanvas.HOTBAR_DIGIT_CHARS.
constexpr char kHotbarDigitChars[6] = {'1', '3', '5', '7', '9', '0'};

// GameCanvas.drawHotbarIcon(): draws `iconsSprite`'s `iconIdx`-th 30x24
// frame at (x, y), via the original's own `x - 30 * iconIdx` shift +
// g.setClip(x, y, 30, 24) trick -- Backbuffer::Blit's clip-range
// parameters reproduce the x-clip directly; no y-clip is needed since
// HotbarTextures::icons is exactly 24 tall (see its own doc comment).
void DrawHotbarIcon(Backbuffer& bb, const HotbarTextures& t, int iconIdx, int x, int y) {
    bb.Blit(x - 30 * iconIdx, y, t.icons, x, x + 30);
}

// paintHotbar()'s own two-pass digit draw: g.setColor(0) at (x+1, y+1),
// then g.setColor(16777215) at (x, y) -- a black "shadow" one pixel
// down-right of the white fill, given here as (x, y) = the white pass's
// own coordinates (matching every real call site, which always states
// the white position and derives the black one by +1/+1).
void DrawHotbarDigit(Backbuffer& bb, char c, int x, int y) {
    constexpr uint16_t kBlack = PackRGB565(0, 0, 0);
    constexpr uint16_t kWhite = PackRGB565(255, 255, 255);
    std::string s(1, c);
    BitmapFont::DrawString(bb, x + 1, y + 1, s, kBlack);
    BitmapFont::DrawString(bb, x, y, s, kWhite);
}

}  // namespace

void HotbarRenderer::Paint(Backbuffer& bb, const HotbarTextures& textures, int context) {
    bb.Blit(0, 156, textures.panel);

    if (context == 0) {
        DrawHotbarDigit(bb, kHotbarDigitChars[1], 25, 190);
        DrawHotbarDigit(bb, kHotbarDigitChars[2], 65, 190);
        DrawHotbarDigit(bb, kHotbarDigitChars[3], 105, 190);
        DrawHotbarDigit(bb, kHotbarDigitChars[5], 145, 190);
        DrawHotbarIcon(bb, textures, 1, 13, 164);
        DrawHotbarIcon(bb, textures, 2, 53, 164);
        DrawHotbarIcon(bb, textures, 3, 93, 164);
        DrawHotbarIcon(bb, textures, 5, 133, 164);
    } else if (context == 1) {
        DrawHotbarDigit(bb, kHotbarDigitChars[0], 25, 190);
        DrawHotbarDigit(bb, kHotbarDigitChars[1], 65, 190);
        DrawHotbarDigit(bb, kHotbarDigitChars[2], 105, 190);
        DrawHotbarDigit(bb, kHotbarDigitChars[3], 145, 190);
        DrawHotbarIcon(bb, textures, 0, 13, 164);
        DrawHotbarIcon(bb, textures, 1, 53, 164);
        DrawHotbarIcon(bb, textures, 2, 93, 164);
        DrawHotbarIcon(bb, textures, 3, 133, 164);
    } else if (context == 2) {
        DrawHotbarDigit(bb, kHotbarDigitChars[1], 25, 190);
        DrawHotbarDigit(bb, kHotbarDigitChars[2], 65, 190);
        DrawHotbarDigit(bb, kHotbarDigitChars[3], 105, 190);
        DrawHotbarDigit(bb, kHotbarDigitChars[4], 145, 190);
        DrawHotbarIcon(bb, textures, 1, 13, 164);
        DrawHotbarIcon(bb, textures, 2, 53, 164);
        DrawHotbarIcon(bb, textures, 3, 93, 164);
        DrawHotbarIcon(bb, textures, 4, 133, 164);
    }
}

}  // namespace dawnstar
