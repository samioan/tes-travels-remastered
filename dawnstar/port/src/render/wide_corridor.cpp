#include "render/wide_corridor.h"

#include <algorithm>
#include <cmath>

namespace dawnstar {

namespace {

// Camera, fitted to the sloped side wall in wallsr.png (atlas columns 96..156,
// which is an exact perspective of a plane 0.5 tiles from the eye): straight
// lines through its top and bottom edges meet at the vanishing point, and their
// slopes give the ceiling / floor heights in units where the wall plane is 0.5
// away. All lengths are in tiles.
constexpr double kFocal = 120.0;         // a free scale (any value works if kDepthScale / kEyeBack are refitted)
constexpr double kHorizon = 36.13;       // the vanishing point's row
constexpr double kAtlasVanishX = 81.93;  // ... and column, in atlas coordinates
constexpr double kSideDist = 0.5;        // corridor half-width
constexpr double kCeilingDrop = 0.231;   // eye to ceiling
constexpr double kFloorDrop = 0.905;     // eye to floor

// The art squeezes depth: its tiles are shallower than they are wide (a wall four
// tiles ahead is drawn about as big as one 2.3 tiles away would be in true
// perspective), and the sprites -- monsters, chests -- are sized to match. So a
// distance of `f` tiles ahead of the player's tile centre projects as depth
// kDepthScale * (f + kEyeBack), fitted to where the art puts each tile's near
// face (depths ~0.5, 1.0, 1.3, 2.1 for the tiles 1..4 ahead) and side-wall strip.
constexpr double kDepthScale = 0.6;
constexpr double kEyeBack = 0.3;

// The recovered wall texture is the nearest stretch of the sloped wall, where the
// art has the most pixels (further along it is only a few columns per tile and
// smears): depths [kTexNear, kTexNear + kTexDepth), centred on one of its
// vertical mortar lines. Copies are laid end to end, alternately mirrored, so
// they always join without a seam.
constexpr double kTexNear = 0.76;
constexpr double kTexDepth = 0.46;
constexpr double kTexPixelsPerTile = 200.0;

constexpr int kMaxAhead = 4;  // as far as the original looks
constexpr int kMaxSide = 12;

constexpr uint16_t Shade(uint16_t c, int k256) {
    const int r = ((c >> 11) & 0x1F) * k256 >> 8;
    const int g = ((c >> 5) & 0x3F) * k256 >> 8;
    const int b = (c & 0x1F) * k256 >> 8;
    return static_cast<uint16_t>((r << 11) | (g << 5) | b);
}

}  // namespace

WideTexture WideTexture::BuildWall(const DecodedImage& atlas) {
    WideTexture t;
    if (atlas.width < 231 || atlas.height < 176) return t;
    t.width = static_cast<int>(kTexDepth * kTexPixelsPerTile);
    t.height = static_cast<int>((kCeilingDrop + kFloorDrop) * kTexPixelsPerTile);
    t.depthPerCopy = kTexDepth;
    t.mirrorCopies = true;
    t.pixels.assign(static_cast<size_t>(t.width) * t.height, 0);
    t.alpha.assign(t.pixels.size(), 255);

    for (int i = 0; i < t.width; i++) {
        const double depth = kTexNear + (i + 0.5) / kTexPixelsPerTile;
        int xa = static_cast<int>(std::lround(kAtlasVanishX + kFocal * kSideDist / depth));
        xa = std::max(0, std::min(atlas.width - 1, xa));
        // The opaque extent of this atlas column: rows outside it are sampled from its edge.
        int top = atlas.height, bottom = -1;
        for (int y = 0; y < atlas.height; y++)
            if (atlas.A(xa, y) != 0) {
                top = std::min(top, y);
                bottom = y;
            }
        for (int j = 0; j < t.height; j++) {
            const double drop = -kCeilingDrop + (j + 0.5) / kTexPixelsPerTile;
            int ya = static_cast<int>(std::lround(kHorizon + drop * kFocal / depth));
            if (bottom >= top) ya = std::max(top, std::min(bottom, ya));
            ya = std::max(0, std::min(atlas.height - 1, ya));
            t.pixels[static_cast<size_t>(j) * t.width + i] = PackRGB565(atlas.R(xa, ya), atlas.G(xa, ya), atlas.B(xa, ya));
        }
    }
    t.valid = true;
    return t;
}

WideTexture WideTexture::BuildGate(const DecodedImage& gate) {
    WideTexture t;
    if (gate.width < 36 || gate.height < 138) return t;
    // The gate's first frame is its nearest face: 36 columns of the full 138-row height, which is
    // exactly the height a wall face has at the nearest depth (the original draws it 8 rows lower,
    // like every gate frame). One copy is 36 pixels wide at that depth.
    t.width = 36;
    t.height = 138;
    t.depthPerCopy = 36.0 / kFocal;
    t.mirrorCopies = false;
    t.pixels.assign(static_cast<size_t>(t.width) * t.height, 0);
    t.alpha.assign(t.pixels.size(), 0);
    for (int y = 0; y < t.height; y++)
        for (int x = 0; x < t.width; x++) {
            const size_t i = static_cast<size_t>(y) * t.width + x;
            if (gate.A(x, y) == 0) continue;
            t.pixels[i] = PackRGB565(gate.R(x, y), gate.G(x, y), gate.B(x, y));
            t.alpha[i] = 255;
        }
    t.valid = true;
    return t;
}

WideTextures WideTextures::Build(const FrameTextures& textures) {
    WideTextures w;
    w.wall = WideTexture::BuildWall(textures.wall);
    w.wallIce = WideTexture::BuildWall(textures.wallIce);
    w.gate = WideTexture::BuildGate(textures.gate);
    return w;
}

WideCorridor::TileQuery WideCorridor::MakeTileQuery(const DungeonView& view, int x, int y, int facing) {
    // Same axes as DungeonView::SampleCorridorView: "ahead" is -y facing north, +x facing east, ...
    return [&view, x, y, facing](int right, int ahead) -> uint8_t {
        int tx = x, ty = y;
        switch (facing) {
            case 1: tx = x + right; ty = y - ahead; break;
            case 2: tx = x + ahead; ty = y + right; break;
            case 3: tx = x - right; ty = y + ahead; break;
            default: tx = x - ahead; ty = y - right; break;
        }
        return view.TileAt(tx, ty);
    };
}

void WideCorridor::Render(Backbuffer& bb, int centerX, const FrameTextures& textures, const WideTextures& wide,
                          const TileQuery& tileAt, int dungeonNumber, bool blind, bool trollThirst) {
    const int W = bb.RealWidth();
    const int viewLeft = centerX - Backbuffer::kWidth / 2;  // where the original 176 columns start

    // Ceiling / floor picture, as the original tiles it (its tile grid stays aligned with the
    // centre of the view). Blind skips it; Troll Thirst replaces it with a dark red block.
    bb.FillRect(0, 0, W, kViewHeight, 0);
    if (!blind) {
        if (trollThirst) {
            bb.FillRect(0, 0, W, textures.floor.height, PackRGB565(0xA0, 0, 0));
        } else {
            const DecodedImage& floorTex = dungeonNumber != 1 ? textures.floorIce : textures.floor;
            int x = viewLeft - ((viewLeft + 35) / 36) * 36;
            for (; x < W; x += 36) bb.Blit(x, 0, floorTex);
        }
    }
    // The floor picture is taller than the 3D view; below it is the HUD strip (the panel is drawn
    // over a black background).
    bb.FillRect(0, kViewHeight, W, Backbuffer::kHeight - kViewHeight, 0);
    const WideTexture& wallTex = dungeonNumber != 1 ? wide.wallIce : wide.wall;
    if (!wallTex.valid || !tileAt) return;

    const double eyeBack = kEyeBack;
    for (int X = 0; X < W; X++) {
        // The ray in map units: sideways per tile walked ahead.
        const double rx = kDepthScale * (X + 0.5 - centerX) / kFocal;
        const double invRx = rx != 0.0 ? 1.0 / std::fabs(rx) : 1e30;
        const int stepI = rx > 0 ? 1 : -1;
        double nextSide = 0.5 * invRx;     // walked distance at which the ray leaves its column of tiles
        double nextAhead = 0.5 + eyeBack;  // ... and its row
        int ci = 0, cj = 0;
        bool hit = false, sideFace = false, gate = false;
        double dist = 0;
        for (int guard = 0; guard < 64; guard++) {
            if (nextSide < nextAhead) {
                ci += stepI;
                dist = nextSide;
                nextSide += invRx;
                sideFace = true;
            } else {
                cj += 1;
                dist = nextAhead;
                nextAhead += 1.0;
                sideFace = false;
            }
            if (cj > kMaxAhead || ci > kMaxSide || ci < -kMaxSide) break;
            const uint8_t tile = tileAt(ci, cj);
            // As paintCorridorWalls: bit 1 is a wall, else bit 64 is a gate.
            if (tile & 1) {
                hit = true;
                gate = false;
                break;
            }
            if (tile & 64) {
                hit = true;
                gate = true;
                break;
            }
        }
        if (!hit) continue;
        const WideTexture& tex = gate ? wide.gate : wallTex;
        if (!tex.valid) continue;
        const double depth = kDepthScale * dist;  // projected depth
        if (depth < 0.02) continue;

        const double top = kHorizon - kCeilingDrop * kFocal / depth;
        const double bottom = kHorizon + kFloorDrop * kFocal / depth;
        // Position along the wall face in copies of the texture: a copy spans depthPerCopy of
        // projected depth along a side wall, and the same width across a wall facing the eye.
        double u = sideFace ? kDepthScale * (dist - eyeBack) / tex.depthPerCopy : rx * dist / tex.depthPerCopy;
        const double cell = std::floor(u);
        u -= cell;
        if (tex.mirrorCopies && (static_cast<long long>(cell) & 1)) u = 1.0 - u;
        const int tx = std::min(tex.width - 1, static_cast<int>(u * tex.width));

        // Fade with distance, and shade the faces that point at the eye a little darker.
        int k = static_cast<int>(256.0 * std::max(0.6, 1.0 - depth / 7.0));
        if (!sideFace) k = k * 232 >> 8;

        const int y0 = std::max(0, static_cast<int>(std::ceil(top - 0.5)));
        const int y1 = std::min(kViewHeight - 1, static_cast<int>(std::floor(bottom - 0.5)));
        const double inv = tex.height / (bottom - top);
        for (int y = y0; y <= y1; y++) {
            const int ty = std::max(0, std::min(tex.height - 1, static_cast<int>((y + 0.5 - top) * inv)));
            const size_t i = static_cast<size_t>(ty) * tex.width + tx;
            if (tex.alpha[i] == 0) continue;
            bb.SetPixel(X, y, Shade(tex.pixels[i], k));
        }
    }
}

}  // namespace dawnstar
