// M67 smoke test: VisibleObjectRenderer::RenderUnknownB (GameCanvas.
// paintUnknown_b(), was decompiled/e.java's b(Graphics,int)) -- the
// NPC/shop-portrait icon overhead a quest-turn-in shop or Varus/the
// Warden sitting directly ahead of the player. Wired live at M67, once
// docs/PORT_ROADMAP.md's "what's next" confirmed `Player.
// questShopAtPendingTile()` (the value this switches on) IS
// PlayerMovement::ShopAheadOfPlayer's own return value -- see that
// note for the full evidence trail.
//
// No JVM ground truth possible here (same reasoning M28's own test
// already gives for the sibling RenderObjects/RenderMonsters) --
// verified via synthetic known-value RawImages for exact
// stat->typeIndex/columnOverride routing, and the new negative-tier
// guard this milestone adds (a real, reachable original
// ArrayIndexOutOfBoundsException -- approaching Varus, shop 6, before
// the wandering Warden's first visit -- that would be undefined
// behavior, not a safe throw, if left as a raw C++ array index).
#include <cstdio>
#include <stdexcept>
#include <string>

#include "render/visible_object_renderer.h"

namespace {

using namespace stormhold;

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

RawImage MakeSolidImage(int width, int height, uint16_t argb4444) {
    RawImage img;
    img.width = width;
    img.height = height;
    img.pixels.assign(static_cast<size_t>(width) * static_cast<size_t>(height), argb4444);
    return img;
}

RawImage MakeFramedImage(int frameWidth, int height, std::initializer_list<uint16_t> colors) {
    RawImage img;
    img.width = frameWidth * static_cast<int>(colors.size());
    img.height = height;
    img.pixels.assign(static_cast<size_t>(img.width) * static_cast<size_t>(height), 0);
    int frame = 0;
    for (uint16_t color : colors) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < frameWidth; x++) {
                img.pixels[static_cast<size_t>(y) * static_cast<size_t>(img.width) +
                           static_cast<size_t>(frame * frameWidth + x)] = color;
            }
        }
        frame++;
    }
    return img;
}

constexpr uint16_t kOpaqueRed = 0xFF00;
constexpr uint16_t kOpaqueBlue = 0xF00F;

// stat 0/3 (typeIndex 1/2) and stat 1 (typeIndex 6) land in DIFFERENT
// unconfirmedTable_ae rows (0 vs 1) with different primary/secondary
// image indices (0/1 vs 7/8), even though both rows happen to share the
// same baseX/baseY (31, 53) -- so distinguishing by WHICH image slot lit
// up, not by position, is what actually proves the routing (not a
// coincidence the position matches; the row lookup itself is already
// covered by M28's own tests).
VisibleObjectAssets MakeAssetsForRouting() {
    VisibleObjectAssets assets;
    assets.monsterImages.images[0] = MakeSolidImage(4, 4, kOpaqueRed);   // typeIndex 1-5's primary
    assets.monsterImages.images[1] =                                     // typeIndex 1-5's secondary, 4 frames
        MakeFramedImage(4, 4, {0x0000, kOpaqueBlue, 0x0000, 0x0000});    // only frame 1 opaque
    assets.monsterImages.images[7] = MakeSolidImage(4, 4, kOpaqueBlue);  // typeIndex 6-10's primary
    // typeIndex 6-10's secondary overlay (unconfirmedTable_ae row 1's
    // own secondaryImageIndex=8) -- present but not pixel-checked here;
    // RenderMonsterOrIconSprite always draws it (secondaryImageIndex>=0),
    // and MonsterImageAt() throws on an unset slot, so this just needs
    // to exist for TestStat1RoutesToADifferentTypeIndexThanStat0 to run.
    assets.monsterImages.images[8] = MakeSolidImage(4, 4, 0x0000);
    return assets;
}

void TestStat0RoutesToTypeIndex1WithColumnOverride() {
    std::printf("-- RenderUnknownB: stat=0 -> typeIndex 1, columnOverride 1 --\n");
    VisibleObjectAssets assets = MakeAssetsForRouting();
    Backbuffer bb;
    bb.Fill(PackRGB565(9, 9, 9));
    VisibleObjectRenderer::RenderUnknownB(bb, assets, /*stat=*/0, /*wardenVisitCount=*/0);

    Expect(bb.Data()[53 * Backbuffer::kWidth + 31] == PackRGB565(255, 0, 0),
           "primary sprite (monsterImages[0], frame 0) should draw at (31, 53) -- typeIndex 1's own row");
    // overlayBase = (31+40, 53-39) = (71, 14); columnOverride=1 forces
    // secondaryFrame=1 (frame 0 is transparent in this asset -- if the
    // override didn't apply, this pixel would stay the sentinel color).
    Expect(bb.Data()[14 * Backbuffer::kWidth + 71] == PackRGB565(0, 0, 255),
           "columnOverride=1 should force secondaryFrame=1 (blue), not frame 0 (transparent) -- confirms the "
           "literal `1` from paintUnknown_b()'s case 0 threads through as RenderMonsterOrIconSprite's "
           "columnOverride, not just typeIndex");
}

void TestStat1RoutesToADifferentTypeIndexThanStat0() {
    std::printf("-- RenderUnknownB: stat=1 -> typeIndex 6 (different image slot than stat=0) --\n");
    VisibleObjectAssets assets = MakeAssetsForRouting();
    Backbuffer bb;
    bb.Fill(PackRGB565(9, 9, 9));
    VisibleObjectRenderer::RenderUnknownB(bb, assets, /*stat=*/1, /*wardenVisitCount=*/0);

    Expect(bb.Data()[53 * Backbuffer::kWidth + 31] == PackRGB565(0, 0, 255),
           "stat=1 should draw typeIndex 6's own primary (monsterImages[7], blue), not typeIndex 1's "
           "(monsterImages[0], red) -- proves stat->typeIndex routing is per-case, not a shared fallthrough");
}

VisibleObjectAssets MakeAssetsForWardenCompass() {
    VisibleObjectAssets assets;
    assets.monsterImages.images[28] = MakeSolidImage(4, 4, kOpaqueRed);
    assets.monsterImages.images[29] = MakeFramedImage(4, 4, {kOpaqueBlue, 0x0000, 0x0000});
    return assets;
}

void TestStat6DrawsWardenCompassAtTierZero() {
    std::printf("-- RenderUnknownB: stat=6, wardenVisitCount=1 (tier 0) -- Warden compass icon --\n");
    VisibleObjectAssets assets = MakeAssetsForWardenCompass();
    Backbuffer bb;
    bb.Fill(PackRGB565(9, 9, 9));
    VisibleObjectRenderer::RenderUnknownB(bb, assets, /*stat=*/6, /*wardenVisitCount=*/1);

    Expect(bb.Data()[32 * Backbuffer::kWidth + 15] == PackRGB565(255, 0, 0),
           "primary compass sprite (monsterImages[28]) should draw at (15, 32)");
    Expect(bb.Data()[10 * Backbuffer::kWidth + 60] == PackRGB565(0, 0, 255),
           "badge sprite (monsterImages[29], tier 0's own kUnconfirmedTableO[0][1]=0 -> frame 0) should draw at "
           "(60, 10) -- (15+45, 32-22)");
}

void TestStat6WithZeroWardenVisitsThrows() {
    std::printf("-- RenderUnknownB: stat=6, wardenVisitCount=0 -- real ArrayIndexOutOfBoundsException, preserved "
                "as a throw --\n");
    VisibleObjectAssets assets = MakeAssetsForWardenCompass();
    Backbuffer bb;
    bool threw = false;
    try {
        // Reachable in the real game: Varus (shop 6) is a fixed,
        // always-present NPC, independent of whether the wandering
        // Warden has ever visited yet -- see this milestone's own
        // header comment in visible_object_renderer.cpp for the full
        // reasoning.
        VisibleObjectRenderer::RenderUnknownB(bb, assets, /*stat=*/6, /*wardenVisitCount=*/0);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Expect(threw, "wardenVisitCount=0 gives tier=-1 -- should throw rather than perform a negative C++ array "
                  "index (undefined behavior, unlike Java's own safe, catchable exception here)");
}

void TestUnmatchedStatIsANoOp() {
    std::printf("-- RenderUnknownB: stat outside 0-6 is a no-op (matches the original switch falling through) "
                "--\n");
    VisibleObjectAssets assets = MakeAssetsForRouting();
    Backbuffer bb;
    uint16_t sentinel = PackRGB565(9, 9, 9);
    bb.Fill(sentinel);
    VisibleObjectRenderer::RenderUnknownB(bb, assets, /*stat=*/-1, /*wardenVisitCount=*/0);

    Expect(bb.Data()[53 * Backbuffer::kWidth + 31] == sentinel, "stat=-1 (no matching case) should leave the "
                                                                  "backbuffer completely untouched");
}

}  // namespace

int main() {
    TestStat0RoutesToTypeIndex1WithColumnOverride();
    TestStat1RoutesToADifferentTypeIndexThanStat0();
    TestStat6DrawsWardenCompassAtTierZero();
    TestStat6WithZeroWardenVisitsThrows();
    TestUnmatchedStatIsANoOp();

    if (!g_ok) {
        std::fprintf(stderr, "m67_paint_unknown_b_smoke: FAILED\n");
        return 1;
    }

    std::printf("all checks passed\n");
    return 0;
}
