// M48 smoke test: BootSplash (ui/boot_splash.h) -- LoadingScreen.java's
// mode 2 startup splash: runSplashSequence()/waitAtLeast()'s timeline and
// renderSplash()'s pixels.
//
// No JVM ground truth (same reason as every prior milestone). The timeline
// is checked against a literal transcription of the original's two loops
// (a simulator that "sleeps" 500ms per iteration and records which
// showCarrierLogo/showSplash state each repaint drew), never against
// boot_splash.cpp's own constants; the pixels are checked against
// independently placed Blit/FillRect/DrawString calls per renderSplash()'s
// own coordinates, using the real splash assets.
//
// Usage: boot_splash_smoke [assetRoot]   (default: ../extracted)
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "assets/decoded_image.h"
#include "assets/img_archive.h"
#include "graphics/backbuffer.h"
#include "graphics/bitmap_font.h"
#include "ui/boot_splash.h"

using dawnstar::Backbuffer;
namespace BitmapFont = dawnstar::BitmapFont;
using dawnstar::BootSplash;
using dawnstar::DecodedImage;

namespace {

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

uint16_t PixelAt(const Backbuffer& bb, int x, int y) {
    return bb.Data()[static_cast<size_t>(y) * Backbuffer::kWidth + x];
}

bool SameFrame(const Backbuffer& a, const Backbuffer& b) {
    for (int i = 0; i < Backbuffer::kWidth * Backbuffer::kHeight; i++) {
        if (a.Data()[i] != b.Data()[i]) return false;
    }
    return true;
}

// One repaint the original would have drawn: (time it happened, card flag,
// splash flag).
struct Repaint {
    long t;
    bool carrier;
    bool showSplash;
};

// A literal transcription of LoadingScreen.runSplashSequence() +
// waitAtLeast() with every Thread.sleep(500) replaced by advancing a
// virtual clock (each repaint itself is instant). runAppload's own first
// second (showSplash=true, then false) is the prologue: with the assets
// already loaded (percent == 100), the hold loop only waits for
// elapsedMs >= 4000.
std::vector<Repaint> SimulateOriginal() {
    std::vector<Repaint> out;
    long now = 0;
    bool carrier = false;
    bool showSplash = true;

    // runAppload: showSplash = true for 1s, then false.
    // The splash thread's own loop starts at t=0 with elapsedMs = 0.
    long elapsed = 0;
    while (elapsed < 4000) {  // percent is already 100
        if (now >= 1000) showSplash = false;
        out.push_back({now, carrier, showSplash});
        now += 500;
        elapsed += 500;
    }
    carrier = true;
    showSplash = false;
    long waited = 0;
    do {
        out.push_back({now, carrier, showSplash});
        now += 500;
        waited += 500;
    } while (waited <= 2000);
    showSplash = true;
    carrier = false;
    waited = 0;
    do {
        out.push_back({now, carrier, showSplash});
        now += 500;
        waited += 500;
    } while (waited <= 1000);
    // Then setCurrentDisplay(returnDisplay): `now` is the hand-off time.
    out.push_back({now, false, false});
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : "../extracted";
    try {
        dawnstar::ImgArchive images(root + "/imgfiles.lmp");
        BootSplash splash = BootSplash::Load(root, images);

        // Independently decoded copies to compute the expected pixels.
        auto readAll = [](const std::string& path) {
            std::ifstream in(path, std::ios::binary);
            return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        };
        DecodedImage top = DecodedImage::FromPng(readAll(root + "/splashtop.png"));
        DecodedImage bot = DecodedImage::FromPng(readAll(root + "/splashbot.png"));
        DecodedImage vir = DecodedImage::FromPng(images.Data("vir2lLogo.png"));
        DecodedImage mforma = DecodedImage::FromPng(images.Data("mformaLogo.png"));
        Check(top.width > 0 && bot.width > 0 && vir.width > 0 && mforma.width > 0, "all four splash images decode");

        // --- Timeline vs. the transcribed original ---
        std::vector<Repaint> trace = SimulateOriginal();
        Check(trace.back().t == 8000, "the original's whole sequence takes 8000ms (4000 + 2500 + 1500)");
        for (size_t i = 0; i + 1 < trace.size(); i++) {
            const Repaint& r = trace[i];
            BootSplash::Phase want = r.carrier ? BootSplash::Phase::Copyright
                                     : r.showSplash ? (r.t >= 6500 ? BootSplash::Phase::SplashHold
                                                                   : BootSplash::Phase::Splash)
                                                    : BootSplash::Phase::SplashWithBar;
            // Every instant between this repaint and the next shows this frame.
            bool ok = true;
            for (long dt : {0L, 1L, 250L, 499L}) {
                if (BootSplash::PhaseAt(r.t + dt) != want) ok = false;
            }
            if (!ok) {
                std::printf("  FAIL: phase at t=%ld (carrier=%d showSplash=%d)\n", r.t, r.carrier, r.showSplash);
                g_ok = false;
            }
        }
        Check(!BootSplash::IsDone(7999) && BootSplash::IsDone(8000), "the splash hands off at exactly 8000ms");
        Check(BootSplash::PhaseAt(-5) == BootSplash::Phase::Splash, "a negative time clamps to the first frame");

        // --- renderSplash pixels ---
        const uint16_t blue = dawnstar::PackRGB565((2510210 >> 16) & 255, (2510210 >> 8) & 255, 2510210 & 255);
        const uint16_t white = 0xFFFF;
        const uint16_t barRed = dawnstar::PackRGB565((10485760 >> 16) & 255, (10485760 >> 8) & 255, 10485760 & 255);

        // Splash without bar: blue background, top at (88-w/2, 45), bottom at (88-w/2, 115).
        {
            Backbuffer got, want;
            splash.Render(got, 0);
            want.Fill(blue);
            want.Blit(88 - top.width / 2, 45, top);
            want.Blit(88 - bot.width / 2, 115, bot);
            Check(SameFrame(got, want), "t=0: blue background + splashtop at y=45 + splashbot at y=115, no bar");
            Check(PixelAt(got, 20, 175) == blue, "t=0: no progress bar area");
        }

        // With the bar at 100%: white 152x22 at (12,165), red 3*100/2=150 wide at (13,166) 20 tall.
        {
            Backbuffer got, want;
            splash.SetPercent(100);
            splash.Render(got, 1000);
            want.Fill(blue);
            want.Blit(88 - top.width / 2, 45, top);
            want.Blit(88 - bot.width / 2, 115, bot);
            want.FillRect(12, 165, 152, 22, white);
            want.FillRect(13, 166, 150, 20, barRed);
            Check(SameFrame(got, want), "t=1000: splash images + white 152x22 box + 150px red bar at 100%");
            Check(PixelAt(got, 12, 165) == white && PixelAt(got, 13, 166) == barRed && PixelAt(got, 162, 185) == barRed &&
                      PixelAt(got, 163, 185) == white,
                  "the bar's edges: box at (12,165), fill inset 1px, 150 wide");

            splash.SetPercent(40);
            Backbuffer half;
            splash.Render(half, 3999);
            Check(PixelAt(half, 13 + 59, 176) == barRed && PixelAt(half, 13 + 60, 176) == white,
                  "40% draws 3*40/2 = 60px of bar");
            splash.SetPercent(100);
        }

        // Copyright card.
        {
            Backbuffer got, want;
            splash.Render(got, 4000);
            want.Fill(white);
            want.Blit(88 - vir.width / 2, 10, vir);
            int y = 10 + vir.height + 3;
            const char* lines[6] = {"(c) 2003 Vir2L Studios, ",   "a ZeniMax Media company. ", "The Elder Scrolls and Vir2L ",
                                    "are registered trademarks ", "of ZeniMax Media Inc. ",    "All rights reserved."};
            for (const char* l : lines) {
                std::string s = l;
                BitmapFont::DrawString(want, 88 - BitmapFont::StringWidth(s) / 2, y, s, 0);
                y += 14;
            }
            BitmapFont::DrawString(want, 88 - BitmapFont::StringWidth("Distributed by:") / 2, 143, "Distributed by:", 0);
            want.Blit(88 - mforma.width / 2, 158, mforma);
            Check(SameFrame(got, want), "t=4000: white card, Vir2L logo, six 14px-spaced lines, Distributed by, carrier logo");
            Check(std::string(BootSplash::kCopyString[5]) == "All rights reserved." &&
                      std::string(BootSplash::kCopyString[0]) == "(c) 2003 Vir2L Studios, ",
                  "ESGame.copyString's six lines");
            Backbuffer later;
            splash.Render(later, 6499);
            Check(SameFrame(got, later), "the card holds through t=6499");
        }

        // The hold frame is the bar-less splash again; Done leaves the buffer alone.
        {
            Backbuffer got, first;
            splash.Render(got, 6500);
            splash.Render(first, 0);
            Check(SameFrame(got, first), "t=6500: back to the bar-less splash");
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
