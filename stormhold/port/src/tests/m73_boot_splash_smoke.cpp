// M73 smoke test: BootSplash (ui/boot_splash.h) -- UIScreen.java's mode 2
// startup splash: runSplash()/holdRepainting()'s timeline and
// paintSplash()'s pixels.
//
// No JVM ground truth (same class of gap as every other port-only UI state
// machine in this project). The timeline is checked against a literal
// transcription of the original's two loops (`runSplash()`'s own `while
// (percent < 100 || elapsedMs < 4000)` and two `holdRepainting()` calls, a
// simulator that "sleeps" 500ms per iteration and records which
// showCredits/splashFadeStage state each repaint would see), never against
// boot_splash.cpp's own constants; the pixels are checked against
// independently placed Blit/FillRect/DrawString calls per paintSplash()'s
// own coordinates, using the real splash assets.
//
// Usage: boot_splash_smoke [assetRoot]   (default: ../../extracted)
#include <cstdio>
#include <string>
#include <vector>

#include "assets/asset_root.h"
#include "assets/decoded_image.h"
#include "graphics/backbuffer.h"
#include "graphics/bitmap_font.h"
#include "ui/boot_splash.h"

using namespace stormhold;

namespace {

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

uint16_t PixelAt(const Backbuffer& bb, int x, int y) { return bb.Data()[static_cast<size_t>(y) * Backbuffer::kWidth + x]; }

bool SameFrame(const Backbuffer& a, const Backbuffer& b) {
    for (int i = 0; i < Backbuffer::kWidth * Backbuffer::kHeight; i++) {
        if (a.Data()[i] != b.Data()[i]) return false;
    }
    return true;
}

// One repaint the original would have drawn: (time it happened, which
// state paintSplash() would read).
struct Repaint {
    long t;
    bool showCredits;
    bool splashFadeStage;
};

// A literal transcription of UIScreen.runSplash() + holdRepainting() with
// every Thread.sleep(500) replaced by advancing a virtual clock (each
// repaint itself is instant). With the assets already loaded (percent ==
// 100 on this port, see boot_splash.h's own class comment), the first loop
// only waits for elapsedMs >= 4000.
std::vector<Repaint> SimulateOriginal() {
    std::vector<Repaint> out;
    long now = 0;
    long elapsed = 0;
    while (elapsed < 4000) {  // percent is already 100
        out.push_back({now, false, false});
        now += 500;
        elapsed += 500;
    }
    // ESGame.showCredits = true; splashFadeStage = false; holdRepainting(2000).
    long waited = 0;
    do {
        out.push_back({now, true, false});
        now += 500;
        waited += 500;
    } while (waited <= 2000);
    // splashFadeStage = true; showCredits = false; holdRepainting(1000).
    waited = 0;
    do {
        out.push_back({now, false, true});
        now += 500;
        waited += 500;
    } while (waited <= 1000);
    // Then showScreen(nextScreen): `now` is the hand-off time.
    out.push_back({now, false, false});
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : "../../extracted";
    try {
        AssetRoot assets(root);
        BootSplash splash = BootSplash::Load(assets);

        DecodedImage top = DecodedImage::Load(assets, "splashtop.png");
        DecodedImage bottom = DecodedImage::Load(assets, "splashbot.png");
        DecodedImage headerLogo = DecodedImage::Load(assets, "mformaLogo.png");
        DecodedImage distributedLogo = DecodedImage::Load(assets, "vir2lLogo.png");
        Check(top.width > 0 && bottom.width > 0 && headerLogo.width > 0 && distributedLogo.width > 0,
              "all four splash images decode");

        // --- Timeline vs. the transcribed original ---
        std::vector<Repaint> trace = SimulateOriginal();
        Check(trace.back().t == 8000, "the original's whole sequence takes 8000ms (4000 + 2500 + 1500)");
        for (size_t i = 0; i + 1 < trace.size(); i++) {
            const Repaint& r = trace[i];
            BootSplash::Phase want = r.showCredits  ? BootSplash::Phase::Copyright
                                      : r.splashFadeStage ? BootSplash::Phase::SplashHold
                                                           : BootSplash::Phase::Splash;
            bool ok = true;
            for (long dt : {0L, 1L, 250L, 499L}) {
                if (BootSplash::PhaseAt(r.t + dt) != want) ok = false;
            }
            if (!ok) {
                std::printf("  FAIL: phase at t=%ld (showCredits=%d splashFadeStage=%d)\n", r.t, r.showCredits,
                            r.splashFadeStage);
                g_ok = false;
            }
        }
        Check(!BootSplash::IsDone(7999) && BootSplash::IsDone(8000), "the splash hands off at exactly 8000ms");
        Check(BootSplash::PhaseAt(-5) == BootSplash::Phase::Splash, "a negative time clamps to the first frame");

        // --- paintSplash pixels ---
        const uint16_t black = PackRGB565(0, 0, 0);
        const uint16_t white = PackRGB565(255, 255, 255);
        const uint16_t barFill = PackRGB565(0xA0, 0x00, 0x00);

        // Splash with bar at 100%: black background, top at y=20, bottom at
        // y=100, white 152x22 box at (12,165), 150px-wide red fill at (13,166).
        {
            Backbuffer got, want;
            splash.SetPercent(100);
            splash.Render(got, 0);
            want.Fill(black);
            want.Blit(88 - top.width / 2, 20, top);
            want.Blit(88 - bottom.width / 2, 100, bottom);
            want.FillRect(12, 165, 152, 22, white);
            want.FillRect(13, 166, 150, 20, barFill);
            Check(SameFrame(got, want), "t=0: black background + splashtop at y=20 + splashbot at y=100 + full bar");
            Check(PixelAt(got, 12, 165) == white && PixelAt(got, 13, 166) == barFill && PixelAt(got, 162, 185) == barFill &&
                      PixelAt(got, 163, 185) == white,
                  "the bar's edges: box at (12,165), fill inset 1px, 150 wide at 100%");

            splash.SetPercent(40);
            Backbuffer partial;
            splash.Render(partial, 3999);
            Check(PixelAt(partial, 13 + 59, 176) == barFill && PixelAt(partial, 13 + 60, 176) == white,
                  "40% draws 3*40/2 = 60px of bar");
            splash.SetPercent(100);
        }

        // Copyright card: mformaLogo at the header position (NOT vir2lLogo --
        // the logo roles are swapped from dawnstar's own splash, see
        // boot_splash.h's own class comment), vir2lLogo at "Distributed by:".
        {
            Backbuffer got, want;
            splash.Render(got, 4000);
            want.Fill(white);
            want.Blit(88 - headerLogo.width / 2, 10, headerLogo);
            int y = 10 + headerLogo.height + 3;
            const char* lines[6] = {"(c) 2003 Vir2L Studios, ",   "a ZeniMax Media company. ", "The Elder Scrolls and Vir2L ",
                                    "are registered trademarks ", "of ZeniMax Media Inc. ",    "All rights reserved."};
            for (const char* l : lines) {
                std::string s = l;
                BitmapFont::DrawString(want, 88 - BitmapFont::StringWidth(s) / 2, y, s, black);
                y += 14;
            }
            BitmapFont::DrawString(want, 88 - BitmapFont::StringWidth("Distributed by:") / 2, 143, "Distributed by:", black);
            want.Blit(88 - distributedLogo.width / 2, 158, distributedLogo);
            Check(SameFrame(got, want),
                  "t=4000: white card, mformaLogo header, six 14px-spaced lines, Distributed by, vir2lLogo");
            Backbuffer later;
            splash.Render(later, 6499);
            Check(SameFrame(got, later), "the card holds through t=6499");
        }

        // The hold frame is the splash again, without the bar; Done leaves
        // the buffer alone.
        {
            Backbuffer got, wantNoBar;
            splash.Render(got, 6500);
            wantNoBar.Fill(black);
            wantNoBar.Blit(88 - top.width / 2, 20, top);
            wantNoBar.Blit(88 - bottom.width / 2, 100, bottom);
            Check(SameFrame(got, wantNoBar), "t=6500: back to splashtop/splashbot, no bar this time");
            Backbuffer untouched;
            untouched.Fill(0x1234);
            splash.Render(untouched, 8000);
            Check(PixelAt(untouched, 0, 0) == 0x1234 && PixelAt(untouched, 175, 207) == 0x1234,
                  "a Done splash draws nothing");
        }
    } catch (const std::exception& e) {
        std::printf("boot_splash_smoke: exception: %s\n", e.what());
        return 2;
    }

    if (!g_ok) {
        std::printf("boot_splash_smoke: FAILED\n");
        return 1;
    }
    std::printf("boot_splash_smoke: all checks passed\n");
    return 0;
}
