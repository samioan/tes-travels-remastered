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
// chest/dropped-item icons (M26), plus the closest slot's (1) chest/
// dropped-item icon (M26) AND its monster case (M27, via
// paintObjectAtPosition -- see PaintObjectAtPosition's own doc comment).
//
// STILL DEFERRED: full NPC portraits (paintNpcPortrait, actually keyed
// by a completely separate `npcInSight` mechanism this port hasn't
// traced/ported at all yet, not by visibleObjects). NPCs DO already
// render correctly at far/mid range (M26) as a generic monster-shaped
// silhouette icon, exactly like the original.
class VisibleObjectRenderer {
public:
    static void Render(Backbuffer& bb, const VisibleObjectTextures& textures,
                        const std::array<VisibleSlot, 13>& slots);

    // GameCanvas.paintObjectAtPosition(): the closest-slot monster
    // renderer, also (separately) reused by the original for all 9 NPC
    // portrait dispatches (paintNpcPortrait -- not ported here, see the
    // class comment above) -- exposed publicly for that reason, even
    // though VisibleObjectRenderer::Render is currently its only real
    // caller. `frameOverride` >= 0 overrides OBJECT_ICON_TABLE's default
    // second-sprite frame (the NPC-portrait dispatcher's own use, unused
    // here -- always -1, matching paintVisibleObjects' own call).
    static void PaintObjectAtPosition(Backbuffer& bb, const VisibleObjectTextures& textures, int posCode,
                                       int frameOverride);
};

}  // namespace dawnstar
