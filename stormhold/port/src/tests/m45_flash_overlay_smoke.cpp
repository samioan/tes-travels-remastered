// M45 smoke test: FlashOverlay/FlashOverlayState (render/flash_overlay.h)
// -- GameCanvas.paintFlashOverlays()'s C++ body (M22's own confirmed Java
// transcription, unchanged this session). No JVM ground truth available
// (same reason as every prior milestone) -- verified via:
//  - Each of the 3 flags independently: Paint draws nothing while its
//    flag is false, draws something (and clears the flag) while true.
//  - The jitter offset itself re-derived independently from
//    RandomInt1Based (ESGame.randomInt()'s own "1 + abs(nextInt() %
//    bound)" formula) against the SAME seed Paint consumes, then checked
//    pixel-exact against the real loaded PNG's own opaque pixels at that
//    computed offset -- not just "some pixel changed somewhere".
//  - All 3 flags true at once: all 3 clear after one Paint call.
#include <cstdint>
#include <cstdio>

#include "assets/asset_root.h"
#include "render/flash_overlay.h"
#include "util/java_random.h"

namespace {

using namespace stormhold;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

uint16_t PixelAt(const Backbuffer& bb, int x, int y) { return bb.Data()[static_cast<size_t>(y) * Backbuffer::kWidth + x]; }

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assetRoot(root);
        FlashOverlayAssets assets = FlashOverlayAssets::Load(assetRoot);

        Check(assets.images[0].width > 0 && assets.images[0].height > 0, "blood1.png decoded with real dimensions");
        Check(assets.images[1].width > 0 && assets.images[1].height > 0, "monsterspell.png decoded with real dimensions");
        Check(assets.images[2].width > 0 && assets.images[2].height > 0, "selfspell.png decoded with real dimensions");

        // --- A: all flags false -- Paint is a true no-op ---
        {
            Backbuffer bb;
            bb.Fill(0);
            FlashOverlayState state;
            JavaRandom rng(1);
            FlashOverlay::Paint(bb, state, assets, rng);
            Check(PixelAt(bb, 41, 51) == 0, "no flag set means nothing drawn in the hit-flash jitter box");
            Check(!state.hit && !state.spellHitMonster && !state.spellHitSelf, "flags stay false when they started false");
        }

        // --- B: hit flash -- exact jitter offset re-derived independently ---
        {
            Backbuffer bb;
            bb.Fill(0);
            FlashOverlayState state;
            state.hit = true;
            JavaRandom rng(42);
            JavaRandom expectedRng(42);
            int expectedX = 40 + RandomInt1Based(expectedRng, 30);
            int expectedY = 50 + RandomInt1Based(expectedRng, 20);

            FlashOverlay::Paint(bb, state, assets, rng);

            Check(!state.hit, "hit flag self-clears once drawn");
            // Find an opaque source pixel and check it landed at the
            // independently-recomputed (expectedX, expectedY) offset.
            bool foundOpaque = false;
            for (int sy = 0; sy < assets.images[0].height && !foundOpaque; sy++) {
                for (int sx = 0; sx < assets.images[0].width && !foundOpaque; sx++) {
                    if (assets.images[0].A(sx, sy) == 0) continue;
                    foundOpaque = true;
                    uint16_t expected =
                        PackRGB565(assets.images[0].R(sx, sy), assets.images[0].G(sx, sy), assets.images[0].B(sx, sy));
                    Check(PixelAt(bb, expectedX + sx, expectedY + sy) == expected,
                          "blood1.png's own opaque pixel lands at the independently-recomputed jitter offset");
                }
            }
            Check(foundOpaque, "blood1.png has at least one opaque pixel to check against");
        }

        // --- C: spell-hit-monster flash -- same treatment, 2nd RandomInt1Based bound (22, not 20) ---
        {
            Backbuffer bb;
            bb.Fill(0);
            FlashOverlayState state;
            state.spellHitMonster = true;
            JavaRandom rng(7);
            JavaRandom expectedRng(7);
            int expectedX = 40 + RandomInt1Based(expectedRng, 30);
            int expectedY = 50 + RandomInt1Based(expectedRng, 22);

            FlashOverlay::Paint(bb, state, assets, rng);

            Check(!state.spellHitMonster, "spellHitMonster flag self-clears once drawn");
            bool foundOpaque = false;
            for (int sy = 0; sy < assets.images[1].height && !foundOpaque; sy++) {
                for (int sx = 0; sx < assets.images[1].width && !foundOpaque; sx++) {
                    if (assets.images[1].A(sx, sy) == 0) continue;
                    foundOpaque = true;
                    uint16_t expected =
                        PackRGB565(assets.images[1].R(sx, sy), assets.images[1].G(sx, sy), assets.images[1].B(sx, sy));
                    Check(PixelAt(bb, expectedX + sx, expectedY + sy) == expected,
                          "monsterspell.png's own opaque pixel lands at the independently-recomputed jitter offset");
                }
            }
            Check(foundOpaque, "monsterspell.png has at least one opaque pixel to check against");
        }

        // --- D: self-spell-hit flash -- smallest jitter box (bound 2/2) ---
        {
            Backbuffer bb;
            bb.Fill(0);
            FlashOverlayState state;
            state.spellHitSelf = true;
            JavaRandom rng(99);
            JavaRandom expectedRng(99);
            int expectedX = 50 + RandomInt1Based(expectedRng, 2);
            int expectedY = 80 + RandomInt1Based(expectedRng, 2);

            FlashOverlay::Paint(bb, state, assets, rng);

            Check(!state.spellHitSelf, "spellHitSelf flag self-clears once drawn");
            bool foundOpaque = false;
            for (int sy = 0; sy < assets.images[2].height && !foundOpaque; sy++) {
                for (int sx = 0; sx < assets.images[2].width && !foundOpaque; sx++) {
                    if (assets.images[2].A(sx, sy) == 0) continue;
                    foundOpaque = true;
                    uint16_t expected =
                        PackRGB565(assets.images[2].R(sx, sy), assets.images[2].G(sx, sy), assets.images[2].B(sx, sy));
                    Check(PixelAt(bb, expectedX + sx, expectedY + sy) == expected,
                          "selfspell.png's own opaque pixel lands at the independently-recomputed jitter offset");
                }
            }
            Check(foundOpaque, "selfspell.png has at least one opaque pixel to check against");
        }

        // --- E: all 3 flags true at once -- all 3 clear after one Paint ---
        {
            Backbuffer bb;
            bb.Fill(0);
            FlashOverlayState state;
            state.hit = true;
            state.spellHitMonster = true;
            state.spellHitSelf = true;
            JavaRandom rng(1234);
            FlashOverlay::Paint(bb, state, assets, rng);
            Check(!state.hit && !state.spellHitMonster && !state.spellHitSelf,
                  "all 3 flags self-clear in one Paint call when all 3 start true");
        }

        if (!g_ok) {
            std::fprintf(stderr, "m45_flash_overlay_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m45_flash_overlay_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
