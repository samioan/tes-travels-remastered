// M26 smoke test: VisibleObjectRenderer (render/visible_object_renderer.h)
// -- the far/mid/close-slot monster/chest/dropped-item/NPC sprite
// renderer, consuming M25's PlayerState::visibleObjects data model.
// Verified against the real extracted PNG textures (M7/M10's own
// "actually look at the pixels" standard), with slot contents
// constructed directly (this milestone is about rendering GIVEN slot
// contents, not about producing them -- M25's own smoke test already
// covers that half against real gameplay data).
//
// Each scenario below renders on its OWN freshly-cleared Backbuffer,
// deliberately never combining multiple populated slots in one Render()
// call: this real asset set's sprites turn out to be far larger than
// their "icon" name suggests (e.g. a mid-distance monster sprite can be
// 36x96 pixels, drawn with no clipping at all, matching
// GameCanvas.drawMonsterMid's own plain drawImage()) -- exactly the
// back-to-front "farther things get overpainted by nearer ones"
// layering paintVisibleObjects' own draw order (far slots, then mid,
// then closest) is presumably designed to produce. That's real,
// intentional overlap this renderer should NOT be prevented from
// producing, so isolating scenarios (rather than asserting several
// unrelated sprites' pixels all survive in one shared frame) is the
// correct way to test each one on its own merits.
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>

#include "assets/dat_archive.h"
#include "assets/decoded_image.h"
#include "assets/img_archive.h"
#include "assets/monster_image_names.h"
#include "graphics/backbuffer.h"
#include "player/player_state.h"
#include "render/visible_object_renderer.h"

namespace {

using dawnstar::Backbuffer;
using dawnstar::DecodedImage;
using dawnstar::PackRGB565;
using dawnstar::VisibleObjectRenderer;
using dawnstar::VisibleObjectTextures;
using dawnstar::VisibleSlot;
using dawnstar::VisibleSlotKind;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

// Finds the first non-transparent pixel in `img` (scanning row-major),
// used as a distinctive sample point to confirm a sprite really landed
// at the expected screen offset -- same "look at real decoded pixels"
// standard M10 established, not just "something non-black got drawn".
bool FindFirstOpaquePixel(const DecodedImage& img, int* outX, int* outY) {
    for (int y = 0; y < img.height; y++) {
        for (int x = 0; x < img.width; x++) {
            if (img.A(x, y) != 0) {
                *outX = x;
                *outY = y;
                return true;
            }
        }
    }
    return false;
}

void CheckBlitAt(const Backbuffer& bb, int drawX, int drawY, const DecodedImage& img, const char* label) {
    int sx = 0, sy = 0;
    bool found = FindFirstOpaquePixel(img, &sx, &sy);
    Check(found, "sprite should have at least one opaque pixel to sample");
    if (!found) return;

    uint16_t expected = PackRGB565(img.R(sx, sy), img.G(sx, sy), img.B(sx, sy));
    int px = drawX + sx;
    int py = drawY + sy;
    uint16_t actual = bb.Data()[static_cast<size_t>(py) * Backbuffer::kWidth + px];
    if (actual != expected) {
        std::printf("  FAIL: %s: pixel at (%d,%d) expected 0x%04x, got 0x%04x\n", label, px, py, expected, actual);
        g_ok = false;
    }
}

bool RegionAllBackground(const Backbuffer& bb, int x, int y, int w, int h, uint16_t background) {
    for (int yy = y; yy < y + h; yy++) {
        for (int xx = x; xx < x + w; xx++) {
            if (bb.Data()[static_cast<size_t>(yy) * Backbuffer::kWidth + xx] != background) return false;
        }
    }
    return true;
}

// Renders `slots` (all other slots left Empty) on a fresh Backbuffer and
// returns it, so each scenario below gets an isolated frame.
Backbuffer RenderIsolated(const VisibleObjectTextures& textures, std::array<VisibleSlot, 13> slots) {
    Backbuffer bb;
    bb.Fill(0);
    VisibleObjectRenderer::Render(bb, textures, slots);
    return bb;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ImgArchive imageArchive(root + "/imgfiles.lmp");
        dawnstar::MonsterImageNames names = dawnstar::MonsterImageNames::Load(archive);
        VisibleObjectTextures textures = VisibleObjectTextures::Load(imageArchive, names);
        std::printf("loaded 26 object sprites + 3 chest + 3 bag textures\n");

        // --- far monster (slot 8), type 3 -> bucket 0 -> far icon 6,
        // drawn at (10,44) ---
        {
            std::array<VisibleSlot, 13> slots{};
            slots[8].kind = VisibleSlotKind::Monster;
            slots[8].monsterRecord[2] = 3;
            slots[8].monsterRecord[6] = 1;  // "seen" flag set
            Backbuffer bb = RenderIsolated(textures, slots);
            CheckBlitAt(bb, 10, 44, textures.objectSprites[6], "far monster (slot 8, type 3, seen)");
        }

        // --- an otherwise-identical monster whose "seen" flag is NOT
        // set -- GameCanvas's own `rec[6] != 0` gate should suppress
        // drawing it entirely ---
        {
            std::array<VisibleSlot, 13> slots{};
            slots[8].kind = VisibleSlotKind::Monster;
            slots[8].monsterRecord[2] = 3;
            slots[8].monsterRecord[6] = 0;
            Backbuffer bb = RenderIsolated(textures, slots);
            Check(RegionAllBackground(bb, 10, 44, textures.objectSprites[6].width, textures.objectSprites[6].height,
                                       0),
                  "an unseen far monster should NOT be drawn -- the rec[6]!=0 gate should suppress it");
        }

        // --- far dropped item (slot 10) -> itemBagSprites[2] at
        // (84,87) ---
        {
            std::array<VisibleSlot, 13> slots{};
            slots[10].kind = VisibleSlotKind::DroppedItem;
            Backbuffer bb = RenderIsolated(textures, slots);
            CheckBlitAt(bb, 84, 87, textures.itemBagSprites[2], "far dropped item (slot 10)");
        }

        // --- far chest (slot 11) -> chestSprites[2] at (120,87) ---
        {
            std::array<VisibleSlot, 13> slots{};
            slots[11].kind = VisibleSlotKind::Chest;
            Backbuffer bb = RenderIsolated(textures, slots);
            CheckBlitAt(bb, 120, 87, textures.chestSprites[2], "far chest (slot 11)");
        }

        // --- mid chest (slot 4) -> chestSprites[1] at (14,97) ---
        {
            std::array<VisibleSlot, 13> slots{};
            slots[4].kind = VisibleSlotKind::Chest;
            Backbuffer bb = RenderIsolated(textures, slots);
            CheckBlitAt(bb, 14, 97, textures.chestSprites[1], "mid chest (slot 4)");
        }

        // --- mid NPC using icon-set A (Shop.NAMES[0], npcShopIndex 0)
        // -> mid icon 5, at (62,38) ---
        {
            std::array<VisibleSlot, 13> slots{};
            slots[5].kind = VisibleSlotKind::Npc;
            slots[5].npcShopIndex = 0;
            Backbuffer bb = RenderIsolated(textures, slots);
            CheckBlitAt(bb, 62, 38, textures.objectSprites[5], "mid NPC icon-set A (slot 5, npcShopIndex 0)");
        }

        // --- mid NPC using icon-set B (Shop.NAMES[2], npcShopIndex 2)
        // -> mid icon 12, at (112,38) -- a DIFFERENT icon than the
        // previous case, exercising the real asymmetric icon-choice
        // condition (see visible_object_renderer.cpp's NpcUsesIconSetA
        // doc comment) ---
        {
            std::array<VisibleSlot, 13> slots{};
            slots[6].kind = VisibleSlotKind::Npc;
            slots[6].npcShopIndex = 2;
            Backbuffer bb = RenderIsolated(textures, slots);
            CheckBlitAt(bb, 112, 38, textures.objectSprites[12], "mid NPC icon-set B (slot 6, npcShopIndex 2)");
        }

        // --- closest slot (1): a chest -> chestSprites[0] at
        // (60,110) ---
        {
            std::array<VisibleSlot, 13> slots{};
            slots[1].kind = VisibleSlotKind::Chest;
            Backbuffer bb = RenderIsolated(textures, slots);
            CheckBlitAt(bb, 60, 110, textures.chestSprites[0], "closest chest (slot 1)");
        }

        // --- closest slot (1): a dropped item -> itemBagSprites[0] at
        // (60,124) ---
        {
            std::array<VisibleSlot, 13> slots{};
            slots[1].kind = VisibleSlotKind::DroppedItem;
            Backbuffer bb = RenderIsolated(textures, slots);
            CheckBlitAt(bb, 60, 124, textures.itemBagSprites[0], "closest dropped item (slot 1)");
        }

        // The closest slot's monster case (paintObjectAtPosition) is
        // ported in M27 -- see m27_object_at_position_smoke.cpp.

        if (g_ok) {
            std::printf("all visible-object rendering checks passed\n");
            return 0;
        } else {
            std::printf("SOME CHECKS FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("EXCEPTION: %s\n", e.what());
        return 2;
    }
}
