#pragma once
#include <array>

#include "assets/asset_root.h"
#include "assets/monster_image_set.h"
#include "assets/raw_image.h"

namespace stormhold {

// M28: every RawImage GameCanvas's object/monster sprite renderers
// (paintObjects()/paintMonsters(), M22) need -- confirmed real filenames
// via ESGame.java's own asset-loading call sites, same discipline M24's
// CorridorAssets already used for the plain-PNG side. `[0]`/`[1]`/`[2]`
// in each 3-element array are the near/mid/far zone image, matching
// GameCanvas's own renderObjectNear/Mid/Far indexing exactly.
struct VisibleObjectAssets {
    std::array<RawImage, 3> bagImages;      // baglarge/bagmid/bagsmall.cus
    std::array<RawImage, 3> crystalImages;  // crystalnear/mid/far.cus
    std::array<RawImage, 3> chestImages;    // chestnearclosed/midclosed/farclosed.cus
    MonsterImageSet monsterImages;

    static VisibleObjectAssets Load(const AssetRoot& assets) {
        VisibleObjectAssets result;
        result.bagImages = {RawImage::Load(assets, "baglarge.cus"), RawImage::Load(assets, "bagmid.cus"),
                             RawImage::Load(assets, "bagsmall.cus")};
        result.crystalImages = {RawImage::Load(assets, "crystalnear.cus"), RawImage::Load(assets, "crystalmid.cus"),
                                 RawImage::Load(assets, "crystalfar.cus")};
        result.chestImages = {RawImage::Load(assets, "chestnearclosed.cus"), RawImage::Load(assets, "chestmidclosed.cus"),
                               RawImage::Load(assets, "chestfarclosed.cus")};
        result.monsterImages = MonsterImageSet::Load(assets);
        return result;
    }
};

}  // namespace stormhold
