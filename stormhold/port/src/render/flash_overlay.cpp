#include "render/flash_overlay.h"

namespace stormhold {

void FlashOverlay::Paint(Backbuffer& bb, FlashOverlayState& state, const FlashOverlayAssets& assets,
                          JavaRandom& rng) {
    if (state.hit) {
        int x = 40 + RandomInt1Based(rng, 30);
        int y = 50 + RandomInt1Based(rng, 20);
        bb.Blit(x, y, assets.images[0]);
        state.hit = false;
    }

    if (state.spellHitMonster) {
        int x = 40 + RandomInt1Based(rng, 30);
        int y = 50 + RandomInt1Based(rng, 22);
        bb.Blit(x, y, assets.images[1]);
        state.spellHitMonster = false;
    }

    if (state.spellHitSelf) {
        int x = 50 + RandomInt1Based(rng, 2);
        int y = 80 + RandomInt1Based(rng, 2);
        bb.Blit(x, y, assets.images[2]);
        state.spellHitSelf = false;
    }
}

}  // namespace stormhold
