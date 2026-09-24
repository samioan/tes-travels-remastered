#include "ui/boot_splash.h"

#include <algorithm>

#include "graphics/bitmap_font.h"

namespace stormhold {

namespace {

constexpr uint16_t kBlack = PackRGB565(0, 0, 0);
constexpr BitmapFont::Face kCreditsFace = BitmapFont::Face::MediumPlain;
constexpr uint16_t kWhite = PackRGB565(255, 255, 255);
// UIScreen.paintSplash()'s own literal, 10485760 = 0xA00000 = RGB(160,0,0)
// -- the SAME literal dawnstar's own identical splash bar uses (both games
// share this Vir2L toolkit default).
constexpr uint16_t kBarFill = PackRGB565(0xA0, 0x00, 0x00);

// UIScreen.java's own `creditsLines`, joined with real line breaks for
// this screen's own per-line paint loop (`ui/menu_flow.cpp`'s own
// `kCreditsText` is the same 6 lines, for the DIFFERENT main-menu Credits
// screen -- kept as its own small local copy here rather than shared,
// same "each UI file keeps a local copy" convention every sibling UI file
// in this port already follows for its own palette/text constants).
constexpr const char* kCreditLines[6] = {
    "(c) 2003 Vir2L Studios, ",   "a ZeniMax Media company. ",  "The Elder Scrolls and Vir2L ",
    "are registered trademarks ", "of ZeniMax Media Inc. ",     "All rights reserved.",
};

}  // namespace

BootSplash BootSplash::Load(const AssetRoot& assets) {
    return BootSplash(DecodedImage::Load(assets, "splashtop.png"), DecodedImage::Load(assets, "splashbot.png"),
                       DecodedImage::Load(assets, "mformaLogo.png"), DecodedImage::Load(assets, "vir2lLogo.png"));
}

BootSplash::Phase BootSplash::PhaseAt(int64_t elapsedMs) {
    // The screen only repaints on 500ms boundaries -- see the class comment.
    const int64_t t = elapsedMs < 0 ? 0 : elapsedMs / kStepMs * kStepMs;
    if (t >= kEndMs) return Phase::Done;
    if (t >= kHoldStartMs) return Phase::SplashHold;
    if (t >= kCopyrightStartMs) return Phase::Copyright;
    return Phase::Splash;
}

// drawImage(img, width()/2, y, 17): anchor 17 = HCENTER|TOP.
void BootSplash::DrawCentered(Backbuffer& bb, const DecodedImage& img, int y) const {
    bb.Blit(Backbuffer::kWidth / 2 - img.width / 2, y, img);
}

void BootSplash::Render(Backbuffer& bb, int64_t elapsedMs) const {
    const Phase phase = PhaseAt(elapsedMs);
    if (phase == Phase::Done) return;

    const int cx = Backbuffer::kWidth / 2;

    if (phase == Phase::Copyright) {
        bb.Fill(kWhite);
        // Header position -- mformaLogo.png here, NOT vir2lLogo.png (the
        // logo roles are swapped from dawnstar's own splash; see this
        // class's own header comment).
        DrawCentered(bb, headerLogo_, 10);
        int y = 10 + headerLogo_.height + 3;
        for (const char* line : kCreditLines) {
            const std::string s = line;
            // paintSplash() sets no font: the Graphics default, MIDP's
            // getFont(SYSTEM, PLAIN, MEDIUM) -- Alp13 on the device (see
            // bitmap_font.h). The lines are fixed strings the original never
            // wraps; Alp13's widest is 137px, well inside 176.
            BitmapFont::DrawString(bb, cx - BitmapFont::StringWidth(s, kCreditsFace) / 2, y, s, kBlack,
                                   kCreditsFace);
            y += 14;
        }
        const std::string distributed = "Distributed by:";
        BitmapFont::DrawString(bb, cx - BitmapFont::StringWidth(distributed, kCreditsFace) / 2, 143, distributed,
                               kBlack, kCreditsFace);
        DrawCentered(bb, distributedLogo_, 158);
        return;
    }

    bb.Fill(kBlack);
    DrawCentered(bb, top_, 20);
    DrawCentered(bb, bottom_, 100);
    if (phase == Phase::Splash) {
        bb.FillRect(12, 165, 152, 22, kWhite);
        bb.FillRect(13, 166, 3 * percent_ / 2, 20, kBarFill);
    }
}

}  // namespace stormhold
