// M28 smoke test: VisibleObjectRenderer (GameCanvas.paintObjects()/
// paintMonsters()), the sprite drawing off M27's now-populated
// PlayerState::visibleObjects -- the "selection logic done, now draw it"
// pairing (M21->M25, now M27->M28).
//
// No JVM ground truth possible here (same reasoning M6/M21/M25/M27's own
// tests already give) -- verified via synthetic known-value RawImages
// for exact pixel-position checks per zone/slot (mirroring M23's own
// Blit() tests), the confirmed-dead renderObjectNear() crystal-glow
// branch, the confirmed-reachable unconfirmedTable_a out-of-bounds crash
// (typeIndex 32-40), and a real integration check against real .cus
// files loaded through VisibleObjectAssets::Load.
#include <cstdio>
#include <stdexcept>
#include <string>

#include "assets/asset_root.h"
#include "assets/monster_database.h"
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

// A `frameCount`-frame spritesheet, `frameWidth` px per frame, each frame
// filled with `colors[frame]` -- for BlitFrame()'s own frame-slicing.
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

// ARGB4444 (alpha nibble 0xF000 = opaque, see backbuffer.h) chosen so
// each nibble-widens to an exact 0/255 RGB888 channel.
constexpr uint16_t kOpaqueRed = 0xFF00;
constexpr uint16_t kOpaqueGreen = 0xF0F0;
constexpr uint16_t kOpaqueBlue = 0xF00F;
constexpr uint16_t kOpaqueWhite = 0xFFFF;

VisibleObjectAssets MakeAssetsForObjects() {
    VisibleObjectAssets assets;
    assets.chestImages = {MakeSolidImage(4, 4, kOpaqueRed), MakeSolidImage(4, 4, kOpaqueRed),
                           MakeSolidImage(4, 4, kOpaqueRed)};
    assets.bagImages = {MakeSolidImage(4, 4, kOpaqueGreen), MakeSolidImage(4, 4, kOpaqueGreen),
                         MakeSolidImage(4, 4, kOpaqueGreen)};
    assets.crystalImages = {MakeSolidImage(4, 4, kOpaqueBlue), MakeSolidImage(4, 4, kOpaqueBlue),
                             MakeSolidImage(4, 4, kOpaqueBlue)};
    return assets;
}

void TestRenderObjectsZonePositions() {
    std::printf("-- RenderObjects: chest/dropped-item zone positions --\n");
    VisibleObjectAssets assets = MakeAssetsForObjects();

    PlayerState p;
    p.visibleObjects[8].kind = VisibleSlotKind::Chest;   // far zone, x=10,y=59+28=87
    p.visibleObjects[5].kind = VisibleSlotKind::DroppedItem;  // mid zone, non-special, len7 -> x=73,y=80+20=100
    p.visibleObjects[5].droppedItemRecord[6] = 0;              // no crystal-glow bit
    p.visibleObjects[1].kind = VisibleSlotKind::DroppedItem;  // near zone -> x=60,y=94+14=108

    Backbuffer bb;
    uint16_t sentinel = PackRGB565(1, 1, 1);
    bb.Fill(sentinel);
    VisibleObjectRenderer::RenderObjects(bb, assets, p);

    Expect(bb.Data()[87 * Backbuffer::kWidth + 10] == PackRGB565(255, 0, 0),
           "far-zone chest at slot 8 should draw chestImages[2] (red) at (10, 87)");
    Expect(bb.Data()[100 * Backbuffer::kWidth + 73] == PackRGB565(0, 255, 0),
           "mid-zone dropped item at slot 5 (non-special) should draw bagImages[1] (green) at (73, 100)");
    Expect(bb.Data()[108 * Backbuffer::kWidth + 60] == PackRGB565(0, 255, 0),
           "near-zone dropped item at slot 1 should draw bagImages[0] (green) at (60, 108)");
}

void TestRenderObjectNearCrystalGlowIsDeadBranch() {
    std::printf("-- RenderObjects: near-zone crystal-glow bit is a confirmed DEAD branch --\n");
    VisibleObjectAssets assets = MakeAssetsForObjects();

    PlayerState p;
    p.visibleObjects[1].kind = VisibleSlotKind::DroppedItem;
    p.visibleObjects[1].droppedItemRecord[6] = 4;  // the crystal-glow bit, SET

    Backbuffer bb;
    uint16_t sentinel = PackRGB565(1, 1, 1);
    bb.Fill(sentinel);
    VisibleObjectRenderer::RenderObjects(bb, assets, p);

    Expect(bb.Data()[108 * Backbuffer::kWidth + 60] == PackRGB565(0, 255, 0),
           "even with the crystal-glow bit set, slot 1 should STILL draw the normal green bag sprite, not blue "
           "crystalImages[0] -- renderObjectNear()'s own crystalGlow parameter is always false in practice");
    Expect(bb.Data()[65 * Backbuffer::kWidth + 45] == sentinel,
           "crystalImages[0]'s own (45,65) position should be completely untouched -- confirms the branch never ran");
}

void TestRenderObjectMidSpecialCrystalPosition() {
    std::printf("-- RenderObjects: mid-zone crystal glow DOES work (only near is dead) --\n");
    VisibleObjectAssets assets = MakeAssetsForObjects();

    PlayerState p;
    p.visibleObjects[5].kind = VisibleSlotKind::DroppedItem;
    p.visibleObjects[5].droppedItemRecord[6] = 4;  // crystal-glow bit set

    Backbuffer bb;
    bb.Fill(PackRGB565(1, 1, 1));
    VisibleObjectRenderer::RenderObjects(bb, assets, p);

    // slot 5, special: x=73,y=55, then y+=13 -> (73,68).
    Expect(bb.Data()[68 * Backbuffer::kWidth + 73] == PackRGB565(0, 0, 255),
           "mid-zone slot 5 WITH the crystal-glow bit set should draw crystalImages[1] (blue) at (73, 68)");
}

VisibleObjectAssets MakeAssetsForMonsterType1() {
    VisibleObjectAssets assets;
    // unconfirmedTable_ae[0] (typeIndex 1-5): baseX=31,baseY=53,
    // primaryImageIndex=0 (count 1), secondaryImageIndex=1 (count 4),
    // overlayBase=(71,14). unconfirmedTable_a[0]={0,0} -> both frames 0.
    // unconfirmedTable_J[0]=all false -> no optional overlays.
    assets.monsterImages.images[0] = MakeSolidImage(4, 4, kOpaqueRed);
    assets.monsterImages.images[1] = MakeFramedImage(4, 4, {kOpaqueGreen, kOpaqueBlue, kOpaqueWhite, kOpaqueRed});
    return assets;
}

void TestRenderMonstersNearZoneTypeIndex1() {
    std::printf("-- RenderMonsters: near-zone (slot 1) typeIndex 1 --\n");
    VisibleObjectAssets assets = MakeAssetsForMonsterType1();

    PlayerState p;
    p.visibleObjects[1].kind = VisibleSlotKind::Monster;
    p.visibleObjects[1].monsterRecord[2] = 1;  // typeIndex

    Backbuffer bb;
    uint16_t sentinel = PackRGB565(9, 9, 9);
    bb.Fill(sentinel);
    bool anyDrawn = VisibleObjectRenderer::RenderMonsters(bb, assets, p);

    Expect(anyDrawn, "a real monster render should report anyMonsterDrawn=true");
    Expect(bb.Data()[53 * Backbuffer::kWidth + 31] == PackRGB565(255, 0, 0),
           "primary sprite (monsterImages[0], frame 0 of 1) should draw at (31, 53)");
    Expect(bb.Data()[14 * Backbuffer::kWidth + 71] == PackRGB565(0, 255, 0),
           "secondary overlay (monsterImages[1], frame 0 of 4) should draw its frame-0 color across (71..74, 14)");
    Expect(bb.Data()[14 * Backbuffer::kWidth + 74] == PackRGB565(0, 255, 0),
           "frame 0's clip range covers all 4 columns 71-74, so column 74 should ALSO be frame 0's color");
}

void TestRenderMonstersNearZoneFrameSlicing() {
    std::printf("-- RenderMonsters: near-zone secondary-overlay frame slicing (typeIndex 3) --\n");
    VisibleObjectAssets assets = MakeAssetsForMonsterType1();

    PlayerState p;
    p.visibleObjects[1].kind = VisibleSlotKind::Monster;
    // typeIndex 3: same unconfirmedTable_ae row (0) as typeIndex 1 above
    // (both in range 1-5), but unconfirmedTable_a[2] = {0, 3} -- a
    // NONZERO secondaryFrame, unlike typeIndex 1's {0, 0}. Exercises
    // BlitFrame()'s own shift-then-clip frame-slicing for real, not just
    // the frame-0 case.
    p.visibleObjects[1].monsterRecord[2] = 3;

    Backbuffer bb;
    bb.Fill(PackRGB565(9, 9, 9));
    VisibleObjectRenderer::RenderMonsters(bb, assets, p);

    // overlayBaseX=71, frameWidth=4, frame=3 -> shifted origin =
    // 71 - 3*4 = 59, clip=[71,75) -> source columns 12-15 -> the 4th
    // (index-3) color in MakeFramedImage's list: kOpaqueRed.
    Expect(bb.Data()[14 * Backbuffer::kWidth + 71] == PackRGB565(255, 0, 0),
           "secondary overlay frame 3 (not frame 0) should show its own color, not frame 0's green");
}

void TestRenderMonstersOutOfBoundsTypeIndexThrows() {
    std::printf("-- RenderMonsters: typeIndex 32-40 -- a real, reachable original crash, preserved as a throw --\n");
    VisibleObjectAssets assets = MakeAssetsForMonsterType1();

    PlayerState p;
    p.visibleObjects[1].kind = VisibleSlotKind::Monster;
    p.visibleObjects[1].monsterRecord[2] = 35;  // in [32,40], a real spawnable type, no unconfirmedTable_a row

    Backbuffer bb;
    bool threw = false;
    try {
        VisibleObjectRenderer::RenderMonsters(bb, assets, p);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Expect(threw, "typeIndex 35 at the near slot should throw, matching a real original "
                  "ArrayIndexOutOfBoundsException, not silently read out of bounds or clamp");

    // typeIndex 41 (the level-37 boss) is intercepted BEFORE reaching
    // that table at all -- should NOT throw.
    p.visibleObjects[1].monsterRecord[2] = 41;
    assets.monsterImages.images[28] = MakeSolidImage(4, 4, kOpaqueRed);
    assets.monsterImages.images[29] = MakeFramedImage(4, 4, {kOpaqueRed, kOpaqueGreen, kOpaqueBlue});
    bool threwFor41 = false;
    try {
        VisibleObjectRenderer::RenderMonsters(bb, assets, p);
    } catch (const std::runtime_error&) {
        threwFor41 = true;
    }
    Expect(!threwFor41, "typeIndex 41 is intercepted before the table lookup -- should NOT throw");
}

void TestRenderMonstersWardenZones() {
    std::printf("-- RenderMonsters: Warden placeholder in far/mid zones reuses rows 32/31 --\n");
    VisibleObjectAssets assets;
    assets.monsterImages.images[31] = MakeSolidImage(4, 4, kOpaqueGreen);  // mid-zone Warden row
    assets.monsterImages.images[32] = MakeSolidImage(4, 4, kOpaqueBlue);   // far-zone Warden row

    PlayerState p;
    p.visibleObjects[10].kind = VisibleSlotKind::Warden;  // far zone, slot 10 -> x=79,y=44
    p.visibleObjects[5].kind = VisibleSlotKind::Warden;   // mid zone, slot 5 -> x=62,y=38

    Backbuffer bb;
    bb.Fill(PackRGB565(9, 9, 9));
    bool anyMonsterDrawn = VisibleObjectRenderer::RenderMonsters(bb, assets, p);

    Expect(!anyMonsterDrawn, "the Warden placeholder should NOT set anyMonsterDrawn -- GameCanvas's own "
                             "unconfirmed_A is only ever set true by a real monster record");
    Expect(bb.Data()[44 * Backbuffer::kWidth + 79] == PackRGB565(0, 0, 255),
           "far-zone Warden (slot 10) should draw monsterImages[32] (blue) at (79, 44)");
    Expect(bb.Data()[38 * Backbuffer::kWidth + 62] == PackRGB565(0, 255, 0),
           "mid-zone Warden (slot 5) should draw monsterImages[31] (green) at (62, 38)");
}

void TestRealAssetIntegration(const std::string& root) {
    std::printf("-- integration: real .cus files via VisibleObjectAssets::Load --\n");
    AssetRoot assetRoot(root);
    VisibleObjectAssets assets = VisibleObjectAssets::Load(assetRoot);
    MonsterDatabase monsters = MonsterDatabase::Load(assetRoot);

    Expect(monsters.TypeCount() == 41, "sanity: 41 real monster types, matching this milestone's own table analysis");
    Expect(assets.monsterImages.images[4].has_value() == false, "typeIndex 4's slot should be a confirmed-imageless nullopt");
    Expect(assets.monsterImages.images[1].has_value(), "typeIndex 1's slot should be loaded");

    PlayerState p;
    p.visibleObjects[8].kind = VisibleSlotKind::Chest;
    std::array<int8_t, 8> chestRecord{};
    p.visibleObjects[8].chestRecord = chestRecord;

    Backbuffer bb;
    uint16_t sentinel = PackRGB565(3, 3, 3);
    bb.Fill(sentinel);
    VisibleObjectRenderer::RenderObjects(bb, assets, p);

    const RawImage& chestFar = assets.chestImages[2];
    bool mismatch = false;
    int opaqueChecked = 0;
    for (int y = 0; y < chestFar.height && (87 + y) < Backbuffer::kHeight; y++) {
        for (int x = 0; x < chestFar.width && (10 + x) < Backbuffer::kWidth; x++) {
            uint16_t pixel = chestFar.pixels[static_cast<size_t>(y) * static_cast<size_t>(chestFar.width) +
                                              static_cast<size_t>(x)];
            uint16_t actual = bb.Data()[(87 + y) * Backbuffer::kWidth + (10 + x)];
            if (IsOpaquePixel(pixel)) {
                opaqueChecked++;
                if (actual != Argb4444ToRgb565(pixel)) mismatch = true;
            }
        }
    }
    Expect(!mismatch, "every opaque pixel of the real chestfarclosed.cus render should match RawImage's own data");
    Expect(opaqueChecked > 0, "should have checked at least one real opaque chest pixel");
    std::printf("  checked %d real opaque chest pixels, all matched=%s\n", opaqueChecked, mismatch ? "false" : "true");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    TestRenderObjectsZonePositions();
    TestRenderObjectNearCrystalGlowIsDeadBranch();
    TestRenderObjectMidSpecialCrystalPosition();
    TestRenderMonstersNearZoneTypeIndex1();
    TestRenderMonstersNearZoneFrameSlicing();
    TestRenderMonstersOutOfBoundsTypeIndexThrows();
    TestRenderMonstersWardenZones();

    try {
        TestRealAssetIntegration(root);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m28_visible_object_renderer_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    if (!g_ok) {
        std::fprintf(stderr, "m28_visible_object_renderer_smoke: FAILED\n");
        return 1;
    }

    std::printf("all checks passed\n");
    return 0;
}
