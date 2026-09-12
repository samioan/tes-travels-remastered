#pragma once
#include <array>

#include "assets/decoded_image.h"
#include "assets/img_archive.h"
#include "assets/monster_image_names.h"
#include "graphics/backbuffer.h"
#include "player/player_state.h"

namespace dawnstar {

// The monster/chest/dropped-item sprite sheets GameCanvas.
// paintVisibleObjects() draws with, decoded once up front -- the
// far/mid/close-distance chest images (chestnearclosed.png/
// chestmidclosed.png/chestfarclosed.png), the dropped-item "bag" images
// (baglarge.png/bagmid.png/bagsmall.png), and the 26-slot objectSprites
// sheet ESGame.runImageLoader() lazily loads a few entries of at a time
// on real MIDP hardware (to fit its limited heap) -- this port has no
// such constraint, so all 26 are decoded eagerly here instead, same
// simplification this project already made for floor/wall textures
// (M10) and every other image-loading milestone.
struct VisibleObjectTextures {
    // Indexed exactly like GameCanvas.objectSprites -- see
    // kMonsterImageIndexInfo (visible_object_renderer.cpp) for which
    // monster-type "bucket" (assets/monster_image_names.h) each index
    // range comes from. Index 23 is NOT a monster at all -- it's the
    // up/down stairs icon (GameCanvas.paintStairsIcon), sharing bucket
    // 4's index range with the roaming-monster's own mid/far icons
    // (24/25) purely because ESGame's lazy loader groups them together.
    std::array<DecodedImage, 26> objectSprites;
    std::array<DecodedImage, 3> chestSprites;
    std::array<DecodedImage, 3> itemBagSprites;

    static VisibleObjectTextures Load(const ImgArchive& archive, const MonsterImageNames& names);
};

// Renamed-source counterpart of GameCanvas.paintVisibleObjects() --
// paints PlayerState::visibleObjects' 13 slots (player/visible_objects.h,
// M25) as sprites: far (slots 8-12) and mid (4-6) distance monster/
// chest/dropped-item icons, plus the closest slot's (1) chest/dropped-
// item icon.
//
// DEFERRED to a later milestone: the closest slot's MONSTER case
// (GameCanvas.paintObjectAtPosition(), which needs the big
// OBJECT_DRAW_TABLE/OBJECT_ICON_TABLE/OBJECT_EXTRA_FLAGS static tables
// and drawSpriteFrame()'s multi-frame sprite-sheet slicing -- none of
// which this milestone's far/mid/loot icons need, since those are all
// single-frame plain image blits) and, by extension, the stairs icon
// (paintStairsIcon, only ever reached through that same code path for a
// posCode of 41/42) and full NPC portraits (paintNpcPortrait, which is
// actually keyed by a completely separate `npcInSight` mechanism this
// port hasn't traced/ported at all yet, not by visibleObjects). NPCs
// DO already render correctly at far/mid range here, though -- as a
// generic monster-shaped silhouette icon, exactly like the original
// (see the .cpp's own doc comment on the real, asymmetric icon-choice
// quirk this preserves).
class VisibleObjectRenderer {
public:
    static void Render(Backbuffer& bb, const VisibleObjectTextures& textures,
                        const std::array<VisibleSlot, 13>& slots);
};

}  // namespace dawnstar
