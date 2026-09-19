#pragma once
#include "assets/asset_root.h"
#include "assets/decoded_image.h"

namespace stormhold {

// The two plain-PNG images GameCanvas.paintWalls() (../../../src/
// GameCanvas.java, M21/M22) actually draws pixels from -- confirmed real
// filenames via ESGame.java's own asset-loading call sites (see
// assets/decoded_image.h's M24 header comment): `GameCanvas.floorTexture
// = this.createImage("floor3.png")` / `GameCanvas.wallTexture = this.
// createImage("newwallsnok.png")`.
//
// Deliberately NOT a general "GameAssets" bundle also covering
// monsterImages/chestImages/bagImages/crystalImages (RawImage, M23's
// territory) or effectImages/hotbarIcons (DecodedImage, M24's) -- those
// feed paint methods (paintMonsters()/paintObjects()/paintHud()/etc)
// that need far more live state this port doesn't wire up yet (Player.
// visibleObjects, a populated WorldRegistry-backed monster/chest/hotbar
// cache). Loading their images now, with nothing to draw them AT, would
// just be dead weight -- named for exactly what it holds, same "don't
// invent scope" discipline render/corridor_render_plan.h's own class
// comment already uses.
struct CorridorAssets {
    DecodedImage floorTexture;
    DecodedImage wallTexture;

    static CorridorAssets Load(const AssetRoot& assets) {
        CorridorAssets result;
        result.floorTexture = DecodedImage::Load(assets, "floor3.png");
        result.wallTexture = DecodedImage::Load(assets, "newwallsnok.png");
        return result;
    }
};

}  // namespace stormhold
