#pragma once
#include <cstdint>
#include <string>

#include "assets/asset_root.h"
#include "assets/decoded_image.h"
#include "graphics/backbuffer.h"

namespace stormhold {

// M73: `UIScreen`'s mode 2 -- the startup splash sequence
// (`UIScreen.paintSplash()` + `runSplash()`/`holdRepainting()`), following
// the sibling dawnstar project's own identical M48 `BootSplash` pattern
// (same underlying Vir2L toolkit -- the pixel positions below (12,165,
// 152,22 / 13,166) and the bar-fill color (10485760) are literally
// identical constants to dawnstar's own splash), adapted to Stormhold's
// own real, confirmed-different details:
//
// - The logo roles are SWAPPED relative to dawnstar: `UIScreen.java`'s
//   copyright card draws `mformaLogoImage` at the top (the "header"
//   position) and `vir2lLogoImage` at the bottom under "Distributed by:"
//   -- dawnstar's own splash puts vir2l at the header and mforma
//   ("carrier") at the bottom. Confirmed by reading `paintSplash()`
//   directly, not assumed to carry over.
// - No distinct "no bar yet" sub-phase: dawnstar's own runAppload() has a
//   real `sleep(1000)` before repainting begins at all, giving it a real
//   [0,1000) window with nothing shown. Stormhold's own `paintSplash()`
//   has no equivalent gate -- the bar's white container is drawn
//   unconditionally whenever `showCredits`/`splashFadeStage` are both
//   false, which is true for this whole first phase -- so the bar is
//   modeled as present from t=0 here (a static-100%-percent port, same as
//   dawnstar, just without a separate pre-bar sub-phase to skip).
//
// Timeline (t = ms since the splash was shown), all 4 numbers confirmed
// directly from `UIScreen.runSplash()`/`holdRepainting()`:
//   [0, 4000)     splashtop.png/splashbot.png + progress bar
//                 (`while (percent < 100 || elapsedMs < 4000)`, repainted
//                 every 500ms; percent is always 100 on this port -- see
//                 `Splash` below)
//   [4000, 6500)  copyright card: mformaLogo.png + credits text +
//                 "Distributed by:" + vir2lLogo.png (`holdRepainting
//                 (2000)`'s own do-while overshoot: 5 x 500ms = 2500ms)
//   [6500, 8000)  splashtop.png/splashbot.png again, no bar
//                 (`holdRepainting(1000)`: 3 x 500ms = 1500ms)
//   8000+         done -> hand off to the main menu
//
// Assets load instantly on a PC (this port's own `main.cpp` already loads
// everything synchronously before the window opens), so the caller drives
// the bar's percent as a plain animation across [0, kCopyrightStartMs)
// instead of real progress -- same reasoning/precedent dawnstar's own
// main.cpp already established for its own bar window.
class BootSplash {
public:
    enum class Phase { Splash, Copyright, SplashHold, Done };

    static constexpr int64_t kStepMs = 500;
    static constexpr int64_t kCopyrightStartMs = 4000;
    static constexpr int64_t kHoldStartMs = 6500;
    static constexpr int64_t kEndMs = 8000;

    BootSplash(DecodedImage top, DecodedImage bottom, DecodedImage headerLogo, DecodedImage distributedLogo)
        : top_(std::move(top)),
          bottom_(std::move(bottom)),
          headerLogo_(std::move(headerLogo)),
          distributedLogo_(std::move(distributedLogo)) {}

    // Reads `mformaLogo.png`/`vir2lLogo.png`/`splashtop.png`/`splashbot.png`
    // out of `assets` (all 4 are loose files at the asset root, same as
    // every other `.png` this port reads via `DecodedImage::Load` -- no
    // archive indirection, see decoded_image.h's own class comment).
    // Throws std::runtime_error if any is missing/undecodable.
    static BootSplash Load(const AssetRoot& assets);

    static Phase PhaseAt(int64_t elapsedMs);
    static bool IsDone(int64_t elapsedMs) { return PhaseAt(elapsedMs) == Phase::Done; }

    int Percent() const { return percent_; }
    void SetPercent(int percent) { percent_ = percent; }

    // A Done splash draws nothing (the caller has moved on).
    void Render(Backbuffer& bb, int64_t elapsedMs) const;

private:
    void DrawCentered(Backbuffer& bb, const DecodedImage& img, int y) const;

    DecodedImage top_;
    DecodedImage bottom_;
    DecodedImage headerLogo_;
    DecodedImage distributedLogo_;
    int percent_ = 100;
};

}  // namespace stormhold
