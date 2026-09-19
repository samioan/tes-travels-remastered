#pragma once
#include <array>

#include "assets/asset_root.h"
#include "assets/decoded_image.h"

namespace stormhold {

// GameCanvas.hotbarIcons -- paintHud()'s own 6 plain-PNG icons (see
// assets/decoded_image.h's own header comment), confirmed real
// filenames/order via ESGame.java's own asset-loading call site
// (`GameCanvas.hotbarIcons = new Image[6]`, then `createImage(...)` for
// each index in order):
//   0: icon_attack.png   3: icon_option.png
//   1: icon_cast.png     4: icon_action.png
//   2: icon_change.png   5: icon_camp.png
//
// Deliberately its own small bundle rather than folded into
// CorridorAssets -- same "named for exactly what it holds, don't invent
// scope" discipline CorridorAssets's own header comment already uses;
// this is paintHud()'s asset list, not paintWalls()'s.
struct HotbarAssets {
    std::array<DecodedImage, 6> icons;

    static HotbarAssets Load(const AssetRoot& assets) {
        HotbarAssets result;
        result.icons[0] = DecodedImage::Load(assets, "icon_attack.png");
        result.icons[1] = DecodedImage::Load(assets, "icon_cast.png");
        result.icons[2] = DecodedImage::Load(assets, "icon_change.png");
        result.icons[3] = DecodedImage::Load(assets, "icon_option.png");
        result.icons[4] = DecodedImage::Load(assets, "icon_action.png");
        result.icons[5] = DecodedImage::Load(assets, "icon_camp.png");
        return result;
    }
};

}  // namespace stormhold
