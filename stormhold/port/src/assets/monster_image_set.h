#pragma once
#include <array>
#include <optional>

#include "assets/asset_root.h"
#include "assets/raw_image.h"

namespace stormhold {

// M28: renamed-source counterpart of ../../../src/ESGame.java's
// monsterfilenamesin.dat loader (loadMonsterImageFileNames()) plus
// runMonsterImageLoader()'s own use of it to populate GameCanvas.
// monsterImages[33] -- confirmed, per docs/ASSET_FORMATS.md, a
// no-count-prefix `<UTF x 5 x 7>` grid (ESGame.monsterImageFileNames),
// read via a raw getResourceAsStream (not Util.openResource), which
// makes no difference here since AssetRoot::OpenFile() is the single
// uniform path every resource in this port already loads through (see
// ShopDialogue's own identical note).
//
// `monsterImageChunks = {{0,7},{7,7},{14,7},{21,7},{28,5}}`: chunk c's
// row supplies typeIndex `start[c] .. start[c]+count[c]-1`, so the grid's
// row-major (chunk, i) position maps onto a flat typeIndex = start[c]+i
// -- NOT the same as the grid's own flat (5*7=35) index, since the last
// chunk only uses 5 of its row's 7 columns (2 columns unused, matching
// ASSET_FORMATS.md's own "not every slot used" note).
//
// `isImagelessMonsterType(typeIndex)` (types 4/11/18/23/30 -- exact
// reason unconfirmed) skips loading an image at all for those 5 slots,
// modeled here as `std::nullopt` rather than a null RawImage.
struct MonsterImageSet {
    // Indexed by typeIndex directly (0-32); index 0 is never populated
    // by any real caller (monster typeIndex is always >= 1) but kept to
    // match the original's own 0-based array size (33) exactly.
    std::array<std::optional<RawImage>, 33> images;

    static MonsterImageSet Load(const AssetRoot& assets);
};

}  // namespace stormhold
