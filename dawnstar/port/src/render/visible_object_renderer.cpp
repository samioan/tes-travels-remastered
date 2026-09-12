#include "render/visible_object_renderer.h"

namespace dawnstar {

namespace {

// ESGame.monster_image_index_info: [objectSprites-start-index, count]
// per monster-type "bucket" (assets/monster_image_names.h's own row
// order) -- bucket 4 (index 23-25) is the odd one out: slot 23 is
// actually the stairs icon, not a monster (see VisibleObjectTextures's
// own doc comment), and slots 24/25 are the roaming-monster's mid/far
// icons respectively.
struct BucketInfo {
    int start;
    int count;
};
constexpr BucketInfo kMonsterImageIndexInfo[5] = {{0, 7}, {7, 7}, {14, 6}, {20, 3}, {23, 3}};

// Monster.java's monsterType (1-42) -> mid/far-slot objectSprites index.
// Ported straight from GameCanvas.midMonsterIconFor/farMonsterIconFor --
// note the parameter these take is genuinely the monster's TYPE id
// (Monster.toBytes() byte 2), not a "position code" despite the
// original's own local-variable naming.
int MidMonsterIcon(int monsterType) {
    if (monsterType >= 1 && monsterType <= 5) return 5;
    if (monsterType >= 6 && monsterType <= 10) return 12;
    if (monsterType >= 11 && monsterType <= 25) return 21;
    if (monsterType >= 26 && monsterType <= 40) return 18;
    return monsterType != 41 && monsterType != 42 ? -1 : 24;
}
int FarMonsterIcon(int monsterType) {
    if (monsterType >= 1 && monsterType <= 5) return 6;
    if (monsterType >= 6 && monsterType <= 10) return 13;
    if (monsterType >= 11 && monsterType <= 25) return 22;
    if (monsterType >= 26 && monsterType <= 40) return 19;
    return monsterType != 41 && monsterType != 42 ? -1 : 25;
}

// paintVisibleObjects' String-tag branch for an NPC slot: `npcShopIndex`
// (player/visible_objects.h) uniquely determines which of the 9 real
// tags (Shop.NAMES[0..4], or "A"/"B"/"C"/"D" for levels 3/12/21/30) was
// placed, which in turn determines the icon via a REAL, asymmetric
// condition ported exactly rather than "cleaned up": far icon 6 (mid 5)
// only for Shop.NAMES[0]/NAMES[1] and "C"/"D" (npcShopIndex 0,1,7,8);
// every other real tag -- NAMES[2..4] and "A"/"B" (npcShopIndex
// 2,3,4,5,6) -- gets far icon 13 (mid 12) instead. (Java's own dead "W"
// tag branch, never actually placed by placeVisibleObject, isn't
// reachable here at all -- see visible_objects.h's own doc comment.)
bool NpcUsesIconSetA(int npcShopIndex) {
    return npcShopIndex == 0 || npcShopIndex == 1 || npcShopIndex == 7 || npcShopIndex == 8;
}

void DrawMonsterMid(Backbuffer& bb, const VisibleObjectTextures& t, int iconIdx, int slot) {
    if (iconIdx < 0) return;  // unreachable for any real monster type, guarded defensively anyway
    int x = 0;
    int y = 38;
    switch (slot) {
        case 4: x = 10; break;
        case 5: x = 62; break;
        case 6: x = 112; break;
    }
    bb.Blit(x, y, t.objectSprites[static_cast<size_t>(iconIdx)]);
}

void DrawMonsterFar(Backbuffer& bb, const VisibleObjectTextures& t, int iconIdx, int slot) {
    if (iconIdx < 0) return;
    int x = 0;
    int y = 44;
    switch (slot) {
        case 8: x = 10; break;
        case 9: x = 44; break;
        case 10: x = 79; break;
        case 11: x = 112; break;
        case 12: x = 146; break;
    }
    bb.Blit(x, y, t.objectSprites[static_cast<size_t>(iconIdx)]);
}

void DrawMidLootIcon(Backbuffer& bb, const VisibleObjectTextures& t, bool isChest, int slot) {
    int x = 0;
    int y = 97;
    switch (slot) {
        case 4: x = 14; break;
        case 5: x = isChest ? 68 : 73; break;
        case 6: x = isChest ? 125 : 142; break;
    }
    if (isChest) {
        bb.Blit(x, y, t.chestSprites[1]);
    } else {
        bb.Blit(x, y + 8, t.itemBagSprites[1]);
    }
}

void DrawFarLootIcon(Backbuffer& bb, const VisibleObjectTextures& t, bool isChest, int slot) {
    int x = 0;
    int y = 87;
    switch (slot) {
        case 8: x = 10; break;
        case 9: x = 46; break;
        case 10: x = 84; break;
        case 11: x = 120; break;
        case 12: x = 156; break;
    }
    if (isChest) {
        bb.Blit(x, y, t.chestSprites[2]);
    } else {
        bb.Blit(x, y, t.itemBagSprites[2]);
    }
}

void DrawCenterLootIcon(Backbuffer& bb, const VisibleObjectTextures& t, bool isChest) {
    int x = 60;
    int y = 110;
    if (isChest) {
        bb.Blit(x, y, t.chestSprites[0]);
    } else {
        bb.Blit(x, y + 14, t.itemBagSprites[0]);
    }
}

// GameCanvas.positionBucketFor(): posCode (a monster TYPE id, 1-42) ->
// OBJECT_DRAW_TABLE row (0-3), or 4 for the special "draw the stairs
// icon instead" bucket (types 41/42 -- see PaintObjectAtPosition's own
// doc comment on why that's a real, surprising thing to preserve).
int PositionBucketFor(int posCode) {
    if (posCode >= 1 && posCode <= 5) return 0;
    if (posCode >= 6 && posCode <= 10) return 1;
    if (posCode >= 11 && posCode <= 25) return 3;
    if (posCode >= 26 && posCode <= 40) return 2;
    return posCode != 41 && posCode != 42 ? -1 : 4;
}

// GameCanvas.OBJECT_DRAW_TABLE: per-bucket base position + up to 4 extra
// decorations (each an (dx, dy, icon) triple relative to the base
// position), consumed by PaintObjectAtPosition. Columns actually read:
// [2]=baseX, [3]=baseY, [4]=iconA, [5]=frameCountA, [6]/[7]=second
// sprite's (dx,dy) offset from base, [8]=iconB (-1 = no second sprite),
// [9]=frameCountB, then [10..12]/[13..15]/[16..18]/[19..21] = each
// extra's own (dx,dy,icon).
//
// REAL FINDING, confirmed by checking every read site: columns [0]/[1]
// (which look exactly like a "posCode range" label, e.g. row 2 starts
// {11,25,...} and row 3 starts {26,40,...}) are NEVER actually read by
// any code -- only PositionBucketFor's OWN independent range checks
// above decide which row applies. And those labels are wrong for the
// row they sit on anyway: row index 2 (bucket 2, which
// PositionBucketFor assigns to posCode 26-40) opens with "11, 25", and
// row index 3 (bucket 3, posCode 11-25) opens with "26, 40" -- the
// middle two rows' own label values are swapped relative to how
// PositionBucketFor actually dispatches to them. Since nothing ever
// reads columns 0/1, this can't be a functional bug, just confusing
// dead data -- preserved verbatim (not reordered or "corrected") rather
// than silently tidied up.
//
// Rows are genuinely different lengths in the original (22, 22, 19, 10
// columns) -- padded to 22 here with trailing zeros for a uniform
// std::array, which is safe only because every extraN flag that would
// read a padded-zero column is confirmed always false for that bucket's
// posCodes (checked against every one of OBJECT_EXTRA_FLAGS's 41 real
// rows below): bucket 2's row has no real data for extra3 (would need
// columns 19-21), and bucket 3's row has no real data for ANY extra
// (would need columns 10-21) -- both buckets' real posCodes always have
// those exact extra flags false, so this never actually gets read.
constexpr int kObjectDrawTable[4][22] = {
    {1, 5, 43, 48, 0, 2, 23, 7, 1, 3, 0, 64, 2, 18, 27, 3, 24, 19, 4, 0, 0, 0},
    {6, 10, 43, 49, 7, 2, 18, 8, 8, 3, 3, 84, 11, 17, 24, 10, 0, 80, 9, 0, 0, 0},
    {11, 25, 40, 50, 14, 3, 0, 0, -1, -1, 9, 29, 16, 11, 0, 15, 41, 41, 17, 0, 0, 0},
    {26, 40, 37, 50, 20, 3, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

// GameCanvas.OBJECT_ICON_TABLE[posCode-1] -> {frameA, frameB-or-override}.
// Only 41 rows (posCode 1-41) -- posCode 42 has no entry at all, but
// that's harmless: PositionBucketFor(42) returns bucket 4, which
// PaintObjectAtPosition dispatches to PaintStairsIcon before this table
// (or OBJECT_EXTRA_FLAGS below) is ever indexed for either 41 or 42.
constexpr int kObjectIconTable[41][2] = {
    {0, 0}, {1, 0}, {0, 1}, {1, 2}, {0, 2}, {0, 1}, {1, 1}, {0, 2}, {1, 2}, {0, 2},
    {0, 0}, {1, 0}, {0, 0}, {1, 0}, {0, 0}, {2, 0}, {0, 0}, {2, 0}, {0, 0}, {2, 0},
    {1, 0}, {2, 0}, {1, 0}, {2, 0}, {2, 0}, {0, 0}, {1, 0}, {0, 0}, {1, 0}, {0, 0},
    {2, 0}, {0, 0}, {2, 0}, {0, 0}, {2, 0}, {1, 0}, {2, 0}, {1, 0}, {2, 0}, {1, 0},
    {0, 0},
};

// GameCanvas.OBJECT_EXTRA_FLAGS[posCode-1] -> whether each of 4 extra
// decorations is drawn. Same 41-row (not 42) shape as kObjectIconTable
// above, for the same reason. Extra index 3 (the 4th column of every
// row) is FALSE for all 41 rows -- a real, checked-not-assumed finding:
// the 4th "extra decoration" slot every OBJECT_DRAW_TABLE row big enough
// to hold one (buckets 0/1) allocates space for is never actually
// triggered by any real monster type in the whole game.
constexpr bool kObjectExtraFlags[41][4] = {
    {false, false, false, false}, {true, true, false, false},  {false, false, true, false},
    {true, false, false, false},  {true, true, true, false},   {false, false, false, false},
    {true, false, false, false},  {false, false, true, false}, {false, true, false, false},
    {true, true, false, false},   {false, false, false, false}, {false, false, false, false},
    {false, false, false, false}, {false, false, false, false}, {false, false, false, false},
    {false, false, false, false}, {false, false, false, false}, {false, false, false, false},
    {false, false, false, false}, {false, false, false, false}, {false, false, false, false},
    {false, false, false, false}, {false, false, false, false}, {false, false, false, false},
    {false, false, false, false}, {true, false, true, false},  {true, false, true, false},
    {true, false, true, false},   {true, false, true, false},  {true, false, true, false},
    {true, false, true, false},   {true, false, true, false},  {true, false, true, false},
    {true, true, false, false},   {true, true, false, false},  {true, true, false, false},
    {true, true, false, false},   {true, true, false, false},  {true, true, false, false},
    {true, true, false, false},   {false, false, false, false},
};

// GameCanvas.drawSpriteFrame(): draws frame `frame` (of `frameCount`
// equal-width horizontal slices) of `sheet` at (x, y) -- the existing
// Backbuffer::Blit's own column-clip range is exactly the mechanism
// GameCanvas.g.setClip(x, y, frameWidth, frameHeight) needs here (no Y
// clipping required: the sheet's own height always equals frameHeight,
// since these are horizontally- not vertically-tiled sheets, so nothing
// beyond frame `frame` can extend past the natural per-row backbuffer
// bounds check Blit already does).
void DrawSpriteFrame(Backbuffer& bb, const DecodedImage& sheet, int frame, int frameCount, int x, int y) {
    int frameWidth = sheet.width / frameCount;
    bb.Blit(x - frame * frameWidth, y, sheet, x, x + frameWidth);
}

// GameCanvas.paintStairsIcon(): the up/down stairs icon (objectSprites[23],
// 2 frames), at a fixed screen position. Only ever reached via
// PaintObjectAtPosition's bucket-4 dispatch below.
void PaintStairsIcon(Backbuffer& bb, const VisibleObjectTextures& t, bool up) {
    DrawSpriteFrame(bb, t.objectSprites[23], up ? 0 : 1, 2, 33, 48);
}

// ESGame.createImage()'s own leading-"/" stripping -- monsterfilenamesin.dat's
// stored names have one (e.g. "/ban_male_body.png") where every other
// image lookup in this port (floor3.png, etc.) doesn't, since those all
// come from ESGame's own createImage() call sites' string literals
// rather than round-tripping through a data file.
std::string StripLeadingSlash(const std::string& name) {
    return !name.empty() && name.front() == '/' ? name.substr(1) : name;
}

}  // namespace

VisibleObjectTextures VisibleObjectTextures::Load(const ImgArchive& archive, const MonsterImageNames& names) {
    VisibleObjectTextures t;
    for (int bucket = 0; bucket < 5; bucket++) {
        for (int i = 0; i < kMonsterImageIndexInfo[bucket].count; i++) {
            int slot = kMonsterImageIndexInfo[bucket].start + i;
            const std::string& name = names.names[static_cast<size_t>(bucket)][static_cast<size_t>(i)];
            t.objectSprites[static_cast<size_t>(slot)] = DecodedImage::FromPng(archive.Data(StripLeadingSlash(name)));
        }
    }

    t.chestSprites[0] = DecodedImage::FromPng(archive.Data("chestnearclosed.png"));
    t.chestSprites[1] = DecodedImage::FromPng(archive.Data("chestmidclosed.png"));
    t.chestSprites[2] = DecodedImage::FromPng(archive.Data("chestfarclosed.png"));

    t.itemBagSprites[0] = DecodedImage::FromPng(archive.Data("baglarge.png"));
    t.itemBagSprites[1] = DecodedImage::FromPng(archive.Data("bagmid.png"));
    t.itemBagSprites[2] = DecodedImage::FromPng(archive.Data("bagsmall.png"));

    return t;
}

// GameCanvas.paintObjectAtPosition(): the closest-slot monster renderer
// (posCode = the monster's TYPE id, 1-42 -- called with frameOverride=-1
// from paintVisibleObjects; the original also reuses this same method
// for all 9 NPC portrait dispatches with a real posCode/frameOverride
// pair of its own, via paintNpcPortrait -- not ported here, see this
// file's header doc comment).
//
// A REAL, surprising finding: bucket 4 (posCode 41/42 -- Monster.java's
// "Gehenoth"/"Gehenoth Thriceborn", confirmed via MonsterDatabase
// against the real extracted data, genuine named creatures with their
// own stats like any other monster type, NOT some kind of stairway
// sentinel) draws the STAIRS icon instead of any monster sprite at all.
// Traced this exactly as written rather than assumed a mislabeling:
// paintStairsIcon is ONLY ever called from this one branch, gated on
// nothing but the closest slot's monster TYPE being 41 or 42. So in the
// original game, standing right next to this specific rare monster
// renders a staircase icon in its place -- ported faithfully as this
// real (and striking) behavior, not "fixed" into drawing a creature
// sprite instead.
void VisibleObjectRenderer::PaintObjectAtPosition(Backbuffer& bb, const VisibleObjectTextures& textures, int posCode,
                                                    int frameOverride) {
    int bucket = PositionBucketFor(posCode);
    if (bucket == 4) {
        PaintStairsIcon(bb, textures, posCode == 41);
        return;
    }
    if (bucket < 0) return;

    const int* row = kObjectDrawTable[bucket];
    int baseX = row[2];
    int baseY = row[3];
    int iconA = row[4];
    int frameCountA = row[5];
    int secondX = baseX + row[6];
    int secondY = baseY + row[7];
    int iconB = row[8];
    int frameCountB = row[9];
    bool hasSecond = iconB >= 0;

    int frameA = kObjectIconTable[posCode - 1][0];
    int frameB = kObjectIconTable[posCode - 1][1];
    if (frameOverride >= 0) frameB = frameOverride;

    const bool* extraFlags = kObjectExtraFlags[posCode - 1];

    DrawSpriteFrame(bb, textures.objectSprites[static_cast<size_t>(iconA)], frameA, frameCountA, baseX, baseY);
    if (hasSecond) {
        DrawSpriteFrame(bb, textures.objectSprites[static_cast<size_t>(iconB)], frameB, frameCountB, secondX,
                         secondY);
    }

    if (extraFlags[0]) {
        int x = baseX + row[10], y = baseY + row[11], icon = row[12];
        DrawSpriteFrame(bb, textures.objectSprites[static_cast<size_t>(icon)], 0, 1, x, y);
    }
    if (extraFlags[1]) {
        int x = baseX + row[13], y = baseY + row[14], icon = row[15];
        DrawSpriteFrame(bb, textures.objectSprites[static_cast<size_t>(icon)], 0, 1, x, y);
    }
    if (extraFlags[2]) {
        int x = baseX + row[16], y = baseY + row[17], icon = row[18];
        DrawSpriteFrame(bb, textures.objectSprites[static_cast<size_t>(icon)], 0, 1, x, y);
    }
    if (extraFlags[3]) {
        int x = baseX + row[19], y = baseY + row[20], icon = row[21];
        DrawSpriteFrame(bb, textures.objectSprites[static_cast<size_t>(icon)], 0, 1, x, y);
    }
}

void VisibleObjectRenderer::Render(Backbuffer& bb, const VisibleObjectTextures& textures,
                                    const std::array<VisibleSlot, 13>& slots) {
    for (int slot = 8; slot <= 12; slot++) {
        const VisibleSlot& s = slots[static_cast<size_t>(slot)];
        if (s.kind == VisibleSlotKind::Monster) {
            if (s.monsterRecord[6] != 0) DrawMonsterFar(bb, textures, FarMonsterIcon(s.monsterRecord[2]), slot);
        } else if (s.kind == VisibleSlotKind::Chest) {
            DrawFarLootIcon(bb, textures, true, slot);
        } else if (s.kind == VisibleSlotKind::DroppedItem) {
            DrawFarLootIcon(bb, textures, false, slot);
        } else if (s.kind == VisibleSlotKind::Npc) {
            DrawMonsterFar(bb, textures, NpcUsesIconSetA(s.npcShopIndex) ? 6 : 13, slot);
        }
    }

    for (int slot = 4; slot <= 6; slot++) {
        const VisibleSlot& s = slots[static_cast<size_t>(slot)];
        if (s.kind == VisibleSlotKind::Monster) {
            if (s.monsterRecord[6] != 0) DrawMonsterMid(bb, textures, MidMonsterIcon(s.monsterRecord[2]), slot);
        } else if (s.kind == VisibleSlotKind::Chest) {
            DrawMidLootIcon(bb, textures, true, slot);
        } else if (s.kind == VisibleSlotKind::DroppedItem) {
            DrawMidLootIcon(bb, textures, false, slot);
        } else if (s.kind == VisibleSlotKind::Npc) {
            DrawMonsterMid(bb, textures, NpcUsesIconSetA(s.npcShopIndex) ? 5 : 12, slot);
        }
    }

    // Slot 1 (closest): monster (M27, via PaintObjectAtPosition, gated
    // on the same rec[6]!=0 "seen" flag as every other monster slot) or
    // chest/dropped-item (M26). NPCs are never handled at this range at
    // all in the original -- see this file's class comment.
    const VisibleSlot& closest = slots[1];
    if (closest.kind == VisibleSlotKind::Monster) {
        if (closest.monsterRecord[6] != 0) PaintObjectAtPosition(bb, textures, closest.monsterRecord[2], -1);
    } else if (closest.kind == VisibleSlotKind::Chest) {
        DrawCenterLootIcon(bb, textures, true);
    } else if (closest.kind == VisibleSlotKind::DroppedItem) {
        DrawCenterLootIcon(bb, textures, false);
    }
}

}  // namespace dawnstar
