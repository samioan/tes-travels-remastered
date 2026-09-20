#pragma once
#include <cstdint>
#include <string>
#include <utility>

#include "assets/decoded_image.h"
#include "assets/img_archive.h"
#include "graphics/backbuffer.h"

namespace dawnstar {

// M48: LoadingScreen.java's mode 2 -- the startup splash sequence
// (`renderSplash()` + `runSplashSequence()`/`waitAtLeast()`), the piece
// ui/loading_screen.h deliberately left out.
//
// The original runs it on its own Thread, repainting every ~500ms. That
// timeline is a pure function of elapsed time, so this port models it as
// one: `PhaseAt(elapsedMs)` and `Render(bb, elapsedMs)`, no thread, no
// sleeping. Every duration in the original is a multiple of 500ms and the
// screen only repaints at those boundaries, so time is quantised to 500ms
// steps first -- what's on screen at any instant is exactly what the
// original's last repaint drew.
//
// Timeline (t = ms since the splash was shown):
//   [0, 1000)     splash images, no bar  (runAppload's `sleep(1000)` with
//                                         showSplash=true)
//   [1000, 4000)  splash images + progress bar (`while (percent < 100 ||
//                                         elapsedMs < 4000)`)
//   [4000, 6500)  Vir2L/ZeniMax copyright card + "Distributed by" carrier
//                 logo (`waitAtLeast(2000)`: its do-while runs 5 times)
//   [6500, 8000)  splash images, no bar again (`waitAtLeast(1000)`: 3 runs)
//   8000+         done -> hand off to `returnDisplay` (the main menu here)
//
// Assets load instantly on a PC, so the caller passes the bar's percent
// (100 once loading is done, which is before the window even opens) rather
// than driving it from a loader thread; the `percent < 100` half of the
// original's hold condition therefore never extends the 4s minimum here.
class BootSplash {
public:
    enum class Phase { Splash, SplashWithBar, Copyright, SplashHold, Done };

    static constexpr int64_t kStepMs = 500;
    static constexpr int64_t kBarStartMs = 1000;
    static constexpr int64_t kCopyrightStartMs = 4000;
    static constexpr int64_t kHoldStartMs = 6500;
    static constexpr int64_t kEndMs = 8000;

    // The four images renderSplash() draws: `/splashtop.png` and
    // `/splashbot.png` (loose files next to the .lmp archives) and
    // `vir2lLogo.png`/`mformaLogo.png` (entries of imgfiles.lmp).
    BootSplash(DecodedImage top, DecodedImage bottom, DecodedImage vir2lLogo, DecodedImage carrierLogo)
        : top_(std::move(top)),
          bottom_(std::move(bottom)),
          vir2lLogo_(std::move(vir2lLogo)),
          carrierLogo_(std::move(carrierLogo)) {}

    // Reads `<root>/splashtop.png`, `<root>/splashbot.png` and the two logos
    // out of `images`. Throws std::runtime_error if any is missing/undecodable.
    static BootSplash Load(const std::string& root, const ImgArchive& images);

    static Phase PhaseAt(int64_t elapsedMs);
    static bool IsDone(int64_t elapsedMs) { return PhaseAt(elapsedMs) == Phase::Done; }

    // splashUI.percent. Deliberately unclamped, same as LoadingScreen's.
    int Percent() const { return percent_; }
    void SetPercent(int percent) { percent_ = percent; }

    // renderSplash(): a Done splash draws nothing (the caller has moved on),
    // leaving the backbuffer untouched.
    void Render(Backbuffer& bb, int64_t elapsedMs) const;

    // ESGame.copyString: the six copyright lines.
    static const char* const kCopyString[6];

private:
    void DrawCentered(Backbuffer& bb, const DecodedImage& img, int y) const;

    DecodedImage top_;
    DecodedImage bottom_;
    DecodedImage vir2lLogo_;
    DecodedImage carrierLogo_;
    int percent_ = 100;
};

}  // namespace dawnstar
