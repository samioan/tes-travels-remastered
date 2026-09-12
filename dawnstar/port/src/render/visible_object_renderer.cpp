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

    // Slot 1 (closest): chest/dropped-item only -- the monster case
    // (paintObjectAtPosition) and NPCs (not handled at this range at all
    // in the original -- see this file's class comment) are deferred.
    const VisibleSlot& closest = slots[1];
    if (closest.kind == VisibleSlotKind::Chest) {
        DrawCenterLootIcon(bb, textures, true);
    } else if (closest.kind == VisibleSlotKind::DroppedItem) {
        DrawCenterLootIcon(bb, textures, false);
    }
}

}  // namespace dawnstar
