#include "render/visible_object_renderer.h"

#include <stdexcept>
#include <string>

namespace stormhold {

namespace {

// GameCanvas.unconfirmedTable_ae -- 4 rows x 22 cols. Cols [0]/[1] are
// redundant range metadata (monsterNearZoneRow() below has its own
// separate hardcoded thresholds and never reads this table's cols
// [0]/[1] at all); only [2..21] are actually consumed, per the column
// layout documented on RenderMonsterOrIconSprite below.
constexpr int8_t kUnconfirmedTableAe[4][22] = {
    {1, 5, 31, 53, 0, 1, 40, -39, 1, 4, 13, -2, 3, 6, 71, 4, 30, 64, 2, 0, 0, 0},
    {6, 10, 31, 53, 7, 1, 27, -35, 8, 4, 1, 69, 11, 27, 66, 9, 33, 10, 10, 0, 0, 0},
    {11, 25, 31, 20, 14, 1, 0, 0, -1, -1, 2, 25, 15, 81, 8, 16, 9, 0, 17, 60, 57, 18},
    {26, 40, 31, 32, 21, 1, 0, 0, -1, -1, 43, 44, 22, 50, 25, 23, -36, 9, 24, -25, 44, 25},
};

// GameCanvas.unconfirmedTable_a -- **only 31 rows, NOT 41** (an earlier
// pass's comment there was wrong -- see that field's own header comment
// in ../../../src/GameCanvas.java for the full real-bug writeup this
// mismatch turned out to be). Rows 0-30 cover typeIndex 1-31.
constexpr int8_t kUnconfirmedTableA[31][2] = {
    {0, 0}, {0, 0}, {0, 3}, {0, 3}, {0, 3}, {0, 2}, {0, 2}, {0, 3}, {0, 3}, {0, 3}, {0, 0},
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
};

// GameCanvas.unconfirmedTable_J -- 41 rows x 4 cols, indexed [typeIndex-1].
constexpr bool kUnconfirmedTableJ[41][4] = {
    {false, false, false, false}, {true, false, false, false}, {false, false, false, false},
    {false, false, true, false}, {true, false, true, false}, {false, false, false, false},
    {false, false, false, false}, {false, true, false, false}, {false, false, true, false},
    {false, true, true, false}, {true, false, false, false}, {false, true, false, false},
    {true, false, true, false}, {false, true, true, false}, {true, true, true, false},
    {true, false, false, false}, {false, true, false, false}, {true, false, true, false},
    {false, true, true, false}, {true, true, true, false}, {true, true, false, false},
    {true, true, false, false}, {true, true, false, false}, {true, true, true, false},
    {true, true, true, false}, {false, false, false, false}, {false, false, false, false},
    {false, false, false, false}, {false, false, false, false}, {false, false, false, false},
    {false, false, false, false}, {false, false, false, false}, {false, false, false, false},
    {false, false, false, false}, {false, false, false, false}, {false, true, false, true},
    {false, true, false, true}, {true, false, false, true}, {true, false, true, false},
    {false, true, true, false}, {false, false, false, false},
};

// GameCanvas.unconfirmedTable_o -- 3 rows x 3 cols; renderWardenCompassIcon
// only ever reads cols [0]/[1] (col [2] unused by any confirmed call site).
constexpr int kUnconfirmedTableO[3][3] = {{0, 0, 0}, {0, 1, 0}, {0, 2, 1}};

// GameCanvas.hasCrystalGlow() -- record.length==7 (dropped item) && bit
// 2 (value 4) of record[6]. A DIFFERENT bit of the same byte M17 already
// confirmed bit 1 (value 2) of for gift-points eligibility -- a genuinely
// separate flag, not a collision.
bool HasCrystalGlow(const VisibleSlot& slot) {
    return slot.kind == VisibleSlotKind::DroppedItem && (slot.droppedItemRecord[6] & 4) != 0;
}

// GameCanvas.drawRawImageFull(): a plain, unclipped, unmirrored blit.
void BlitFull(Backbuffer& bb, const RawImage& img, int x, int y) { bb.Blit(x, y, img); }

// GameCanvas.drawRawImageFrame(): frameWidth = img.width/frameCount,
// shifted-then-clipped to [x, x+frameWidth) -- Blit()'s own frame-slicing
// idiom (see graphics/backbuffer.h's header comment), `transform==8192`
// (DirectGraphics' horizontal-mirror flag) mapped onto `mirrorX`.
void BlitFrame(Backbuffer& bb, const RawImage& img, int frame, int frameCount, int x, int y, bool mirrorX = false) {
    int frameWidth = img.width / frameCount;
    bb.Blit(x - frame * frameWidth, y, img, x, x + frameWidth, mirrorX);
}

// Player.placeVisibleObject()'s monster case never leaves a
// confirmed-imageless slot (4/11/18/23/30) referenced along a live path
// in practice (every table row that names one of those 5 indices is
// paired with an unconfirmedTable_J flag this port has not proven always
// false for every real monster type) -- so this throws rather than
// dereferencing an empty std::optional, the same "surface loudly instead
// of silently producing nonsense" discipline player/player_movement.h's
// own ComputeMoveTarget already uses. Mirrors what the ORIGINAL would do
// too: `monsterImages[index]` would be `null` there, and
// `dg.drawPixels(img.pixels, ...)` would throw a real
// NullPointerException on real hardware.
const RawImage& MonsterImageAt(const VisibleObjectAssets& assets, int index) {
    const auto& opt = assets.monsterImages.images[static_cast<size_t>(index)];
    if (!opt.has_value()) {
        throw std::runtime_error("VisibleObjectRenderer: monsterImages[" + std::to_string(index) +
                                  "] is one of the 5 confirmed-imageless slots (4/11/18/23/30) -- matches a real "
                                  "original NullPointerException if this code path is ever actually reached");
    }
    return *opt;
}

// --- paintObjects() family -------------------------------------------

// GameCanvas.renderObjectNear() -- **the `crystalGlow` parameter is
// always passed false by every real caller** (RenderObjectAt below,
// mirroring the original exactly), and unlike Mid/Far this method never
// calls HasCrystalGlow() itself either -- so a dropped item with the
// crystal-glow bit set NEVER actually renders as crystalImages[0] at the
// near slot in the original. A real, confirmed dead branch, preserved
// rather than "fixed" to call HasCrystalGlow() -- see GameCanvas.java's
// own identical note on renderObjectNear().
void RenderObjectNear(Backbuffer& bb, const VisibleObjectAssets& assets, const VisibleSlot& slot, bool crystalGlow) {
    if (crystalGlow) {
        BlitFull(bb, assets.crystalImages[0], 45, 65);
        return;
    }

    int x = 60;
    int y = 94;
    if (slot.kind == VisibleSlotKind::Chest) {
        BlitFull(bb, assets.chestImages[0], x, y);
    } else if (slot.kind == VisibleSlotKind::DroppedItem) {
        y += 14;
        BlitFull(bb, assets.bagImages[0], x, y);
    }
}

// GameCanvas.renderObjectMid().
void RenderObjectMid(Backbuffer& bb, const VisibleObjectAssets& assets, const VisibleSlot& slot, int slotIndex) {
    int x = 0;
    int y = 0;
    bool special = HasCrystalGlow(slot);
    switch (slotIndex) {
        case 4:
            x = 14;
            y = 80;
            if (special) {
                x = 14;
                y = 55;
            }
            break;
        case 5:
            x = 68;
            y = 80;
            if (special) {
                x = 73;
                y = 55;
            } else if (slot.kind == VisibleSlotKind::DroppedItem) {
                x = 73;
                y = 80;
            }
            break;
        case 6:
            x = 122;
            y = 80;
            if (special) {
                x = 125;
                y = 55;
            } else if (slot.kind == VisibleSlotKind::DroppedItem) {
                x = 132;
                y = 80;
            }
            break;
        default:
            break;
    }

    if (special) {
        y += 13;
        BlitFull(bb, assets.crystalImages[1], x, y);
    } else if (slot.kind == VisibleSlotKind::Chest) {
        y += 17;
        BlitFull(bb, assets.chestImages[1], x, y);
    } else if (slot.kind == VisibleSlotKind::DroppedItem) {
        y += 20;
        BlitFull(bb, assets.bagImages[1], x, y);
    }
}

// GameCanvas.renderObjectFar().
void RenderObjectFar(Backbuffer& bb, const VisibleObjectAssets& assets, const VisibleSlot& slot, int slotIndex) {
    int x = 0;
    int y = 0;
    bool special = HasCrystalGlow(slot);
    switch (slotIndex) {
        case 8:
            x = 10;
            y = special ? 52 : 59;
            break;
        case 9:
            x = 44;
            y = special ? 52 : 59;
            break;
        case 10:
            x = 79;
            y = special ? 52 : 59;
            break;
        case 11:
            x = 112;
            y = special ? 52 : 59;
            break;
        case 12:
            x = 146;
            y = special ? 52 : 59;
            break;
        default:
            break;
    }

    if (special) {
        y += 20;
        BlitFull(bb, assets.crystalImages[2], x, y);
    } else if (slot.kind == VisibleSlotKind::Chest) {
        y += 28;
        BlitFull(bb, assets.chestImages[2], x, y);
    } else if (slot.kind == VisibleSlotKind::DroppedItem) {
        y += 28;
        BlitFull(bb, assets.bagImages[2], x, y);
    }
}

// GameCanvas.renderObjectAt().
void RenderObjectAt(Backbuffer& bb, const VisibleObjectAssets& assets, const VisibleSlot& slot, int slotIndex) {
    if (slotIndex == 1) {
        RenderObjectNear(bb, assets, slot, /*crystalGlow=*/false);
    } else if (slotIndex >= 4 && slotIndex <= 6) {
        RenderObjectMid(bb, assets, slot, slotIndex);
    } else if (slotIndex >= 8 && slotIndex <= 12) {
        RenderObjectFar(bb, assets, slot, slotIndex);
    }
}

// --- paintMonsters() family -------------------------------------------

int MonsterMidZoneRow(int typeIndex) {
    if (typeIndex >= 1 && typeIndex <= 5) return 5;
    if (typeIndex >= 6 && typeIndex <= 10) return 12;
    if (typeIndex >= 11 && typeIndex <= 25) return 19;
    if (typeIndex >= 26 && typeIndex <= 40) return 26;
    return typeIndex == 41 ? 31 : -1;
}

int MonsterFarZoneRow(int typeIndex) {
    if (typeIndex >= 1 && typeIndex <= 5) return 6;
    if (typeIndex >= 6 && typeIndex <= 10) return 13;
    if (typeIndex >= 11 && typeIndex <= 25) return 20;
    if (typeIndex >= 26 && typeIndex <= 40) return 27;
    return typeIndex == 41 ? 32 : -1;
}

int MonsterNearZoneRow(int typeIndex) {
    if (typeIndex >= 1 && typeIndex <= 5) return 0;
    if (typeIndex >= 6 && typeIndex <= 10) return 1;
    if (typeIndex >= 11 && typeIndex <= 25) return 2;
    return (typeIndex >= 26 && typeIndex <= 40) ? 3 : -1;
}

// GameCanvas.renderWardenCompassIcon() -- monsterImages[28]/[29] reused
// for the Warden's own compass-relative icon (see that method's own
// header comment on the sprite-sheet overlap).
void RenderWardenCompassIcon(Backbuffer& bb, const VisibleObjectAssets& assets, int tier) {
    int x = 15;
    int y = 32;
    const RawImage& primary = MonsterImageAt(assets, 28);
    BlitFrame(bb, primary, kUnconfirmedTableO[tier][0], 1, x, y);
    int frameWidth = primary.width;  // frameCount==1 here, so width/1 == width, same as Java's own img.width() read
    BlitFrame(bb, primary, kUnconfirmedTableO[tier][0], 1, x + frameWidth, y, /*mirrorX=*/true);
    const RawImage& badge = MonsterImageAt(assets, 29);
    BlitFrame(bb, badge, kUnconfirmedTableO[tier][1], 3, x + 45, y - 22);
}

// GameCanvas.renderMonsterOrIconSprite() -- the near-zone (slot 1)
// monster/NPC-icon renderer. `unconfirmedTable_ae[row]`'s column layout:
// [2]=baseX [3]=baseY [4]=primaryImageIndex [5]=primaryFrameCount
// [6]/[7]=overlay-anchor offset from base [8]=secondaryImageIndex
// [9]=secondaryFrameCount [10..12]/[13..15]/[16..18]/[19..21]=four
// (dx,dy,imageIndex) optional-overlay triples, each gated by
// unconfirmedTable_J[typeIndex-1][0..3].
void RenderMonsterOrIconSprite(Backbuffer& bb, const VisibleObjectAssets& assets, int typeIndex, int columnOverride) {
    if (typeIndex == 41) {
        RenderWardenCompassIcon(bb, assets, 2);
        return;
    }

    int row = MonsterNearZoneRow(typeIndex);
    if (row < 0) return;

    const int8_t* ae = kUnconfirmedTableAe[row];
    int baseX = ae[2];
    int baseY = ae[3];
    int primaryImageIndex = ae[4];
    int primaryFrameCount = ae[5];
    int overlayBaseX = baseX + ae[6];
    int overlayBaseY = baseY + ae[7];
    int secondaryImageIndex = ae[8];
    int secondaryFrameCount = ae[9];
    bool hasSecondary = secondaryImageIndex >= 0;

    // **A real, reachable original-game crash, preserved as a thrown
    // exception rather than silently clamped or read out of bounds** --
    // see GameCanvas.java's own unconfirmedTable_a header comment for
    // the full writeup. typeIndex 41 already returned above; typeIndex
    // 32-40 are ordinary, confirmed-spawnable monster types with no such
    // interception, and index further reaches this exact table.
    if (typeIndex - 1 >= static_cast<int>(sizeof(kUnconfirmedTableA) / sizeof(kUnconfirmedTableA[0]))) {
        throw std::runtime_error("VisibleObjectRenderer: monster typeIndex " + std::to_string(typeIndex) +
                                  " has no unconfirmedTable_a row (only 31 exist, covering typeIndex 1-31) -- a "
                                  "real, reachable original-game ArrayIndexOutOfBoundsException");
    }

    int primaryFrame = kUnconfirmedTableA[typeIndex - 1][0];
    int secondaryFrame = kUnconfirmedTableA[typeIndex - 1][1];
    if (columnOverride >= 0) secondaryFrame = columnOverride;

    const bool* jFlags = kUnconfirmedTableJ[typeIndex - 1];

    BlitFrame(bb, MonsterImageAt(assets, primaryImageIndex), primaryFrame, primaryFrameCount, baseX, baseY);
    if (hasSecondary) {
        BlitFrame(bb, MonsterImageAt(assets, secondaryImageIndex), secondaryFrame, secondaryFrameCount, overlayBaseX,
                  overlayBaseY);
    }

    if (jFlags[0]) {
        BlitFrame(bb, MonsterImageAt(assets, ae[12]), 0, 1, baseX + ae[10], baseY + ae[11]);
    }
    if (jFlags[1]) {
        BlitFrame(bb, MonsterImageAt(assets, ae[15]), 0, 1, baseX + ae[13], baseY + ae[14]);
    }
    if (jFlags[2]) {
        BlitFrame(bb, MonsterImageAt(assets, ae[18]), 0, 1, baseX + ae[16], baseY + ae[17]);
    }
    if (jFlags[3]) {
        BlitFrame(bb, MonsterImageAt(assets, ae[21]), 0, 1, baseX + ae[19], baseY + ae[20]);
    }
}

// GameCanvas.drawMonsterZoneFrame() -- the mid zone's fixed per-slot
// screen position table.
void DrawMonsterZoneFrame(Backbuffer& bb, const VisibleObjectAssets& assets, int spriteRow, int slotIndex, int frame,
                           int frameCount) {
    int x = 0;
    int y = 0;
    switch (slotIndex) {
        case 4:
            x = 10;
            y = 38;
            break;
        case 5:
            x = 62;
            y = 38;
            break;
        case 6:
            x = 112;
            y = 38;
            break;
        default:
            break;
    }

    BlitFrame(bb, MonsterImageAt(assets, spriteRow), frame, frameCount, x, y);
}

// GameCanvas.renderMonsterMidZoneSprite() -- always frame 0 of 1 (a
// single static frame, no animation, unlike the near-zone renderer).
void RenderMonsterMidZoneSprite(Backbuffer& bb, const VisibleObjectAssets& assets, int spriteRow, int slotIndex) {
    DrawMonsterZoneFrame(bb, assets, spriteRow, slotIndex, 0, 1);
}

// GameCanvas.renderMonsterFarZoneSprite() -- draws straight through, no
// per-frame width slicing (the far-zone sprites are single-frame).
void RenderMonsterFarZoneSprite(Backbuffer& bb, const VisibleObjectAssets& assets, int spriteRow, int slotIndex) {
    int x = 0;
    int y = 0;
    switch (slotIndex) {
        case 8:
            x = 10;
            y = 44;
            break;
        case 9:
            x = 44;
            y = 44;
            break;
        case 10:
            x = 79;
            y = 44;
            break;
        case 11:
            x = 112;
            y = 44;
            break;
        case 12:
            x = 146;
            y = 44;
            break;
        default:
            break;
    }

    BlitFull(bb, MonsterImageAt(assets, spriteRow), x, y);
}

// GameCanvas.renderMonsterSpriteForSlot() -- dispatches to one of three
// zone-specific renderers by slot. The original also resets `g`'s clip
// region here after each zone renderer narrows it -- nothing to reset in
// this port, since Blit()'s clip range is a per-call argument, not
// stateful (see backbuffer.h's own header comment).
void RenderMonsterSpriteForSlot(Backbuffer& bb, const VisibleObjectAssets& assets, int typeIndex, int slotIndex) {
    if (slotIndex == 1) {
        RenderMonsterOrIconSprite(bb, assets, typeIndex, -1);  // renderMonsterNearSprite()
    } else if (slotIndex >= 4 && slotIndex <= 6) {
        RenderMonsterMidZoneSprite(bb, assets, MonsterMidZoneRow(typeIndex), slotIndex);
    } else if (slotIndex >= 8 && slotIndex <= 12) {
        RenderMonsterFarZoneSprite(bb, assets, MonsterFarZoneRow(typeIndex), slotIndex);
    }
}

}  // namespace

void VisibleObjectRenderer::RenderObjects(Backbuffer& bb, const VisibleObjectAssets& assets, const PlayerState& p) {
    for (int slot = 8; slot <= 12; slot++) {
        const VisibleSlot& s = p.visibleObjects[static_cast<size_t>(slot)];
        if (s.kind == VisibleSlotKind::Chest || s.kind == VisibleSlotKind::DroppedItem) {
            RenderObjectAt(bb, assets, s, slot);
        }
    }

    for (int slot = 4; slot <= 6; slot++) {
        const VisibleSlot& s = p.visibleObjects[static_cast<size_t>(slot)];
        if (s.kind == VisibleSlotKind::Chest || s.kind == VisibleSlotKind::DroppedItem) {
            RenderObjectAt(bb, assets, s, slot);
        }
    }

    const VisibleSlot& near = p.visibleObjects[1];
    if (near.kind == VisibleSlotKind::Chest || near.kind == VisibleSlotKind::DroppedItem) {
        RenderObjectAt(bb, assets, near, 1);
    }
}

bool VisibleObjectRenderer::RenderMonsters(Backbuffer& bb, const VisibleObjectAssets& assets, const PlayerState& p) {
    bool anyMonsterDrawn = false;

    for (int slot = 8; slot <= 12; slot++) {
        const VisibleSlot& s = p.visibleObjects[static_cast<size_t>(slot)];
        if (s.kind == VisibleSlotKind::Monster) {
            anyMonsterDrawn = true;
            RenderMonsterSpriteForSlot(bb, assets, s.monsterRecord[2], slot);
        } else if (s.kind == VisibleSlotKind::Warden) {
            RenderMonsterFarZoneSprite(bb, assets, 32, slot);
        }
    }

    for (int slot = 4; slot <= 6; slot++) {
        const VisibleSlot& s = p.visibleObjects[static_cast<size_t>(slot)];
        if (s.kind == VisibleSlotKind::Monster) {
            anyMonsterDrawn = true;
            RenderMonsterSpriteForSlot(bb, assets, s.monsterRecord[2], slot);
        } else if (s.kind == VisibleSlotKind::Warden) {
            RenderMonsterMidZoneSprite(bb, assets, 31, slot);
        }
    }

    const VisibleSlot& near = p.visibleObjects[1];
    if (near.kind == VisibleSlotKind::Monster) {
        anyMonsterDrawn = true;
        RenderMonsterSpriteForSlot(bb, assets, near.monsterRecord[2], 1);
    }
    // Warden at slot 1: GameCanvas.paintMonsters()'s own `else if`
    // branch there is empty -- preserved exactly, no render at all.

    return anyMonsterDrawn;
}

}  // namespace stormhold
