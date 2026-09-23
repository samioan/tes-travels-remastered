#pragma once
#include <array>

#include "assets/asset_root.h"
#include "assets/decoded_image.h"

namespace stormhold {

// GameCanvas.effectImages -- paintFlashOverlays()'s own 3 plain-PNG
// one-shot flash sprites (see assets/decoded_image.h's own header
// comment), confirmed real filenames/order via ESGame.java's own
// asset-loading call site (`GameCanvas.effectImages = new Image[3]`,
// then `createImage(...)` for each index in order):
//   0: blood1.png       -- monster-hit flash (GameCanvas.unconfirmed_S)
//   1: monsterspell.png -- spell-hit-monster flash (unconfirmed_ao)
//   2: selfspell.png    -- self-spell-hit flash (unconfirmed_am)
//
// Deliberately its own small bundle rather than folded into
// HotbarAssets -- same "named for exactly what it holds, don't invent
// scope" discipline HotbarAssets's own header comment already uses;
// this is paintFlashOverlays()'s asset list, not paintHud()'s.
struct FlashOverlayAssets {
    std::array<DecodedImage, 3> images;

    static FlashOverlayAssets Load(const AssetRoot& assets) {
        FlashOverlayAssets result;
        result.images[0] = DecodedImage::Load(assets, "blood1.png");
        result.images[1] = DecodedImage::Load(assets, "monsterspell.png");
        result.images[2] = DecodedImage::Load(assets, "selfspell.png");
        return result;
    }
};

}  // namespace stormhold
