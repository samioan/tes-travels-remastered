#include "ui/boot_splash.h"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

#include "graphics/bitmap_font.h"
#include "util/text.h"

namespace dawnstar {

namespace {

constexpr uint16_t FromRgb24(int rgb) {
    return PackRGB565(static_cast<uint8_t>((rgb >> 16) & 0xFF), static_cast<uint8_t>((rgb >> 8) & 0xFF),
                      static_cast<uint8_t>(rgb & 0xFF));
}

// renderSplash()'s own g.setColor(...) literals.
constexpr uint16_t kBlack = FromRgb24(0);
constexpr uint16_t kWhite = FromRgb24(16777215);
constexpr uint16_t kSplashBlue = FromRgb24(2510210);
constexpr uint16_t kBarFill = FromRgb24(10485760);

std::vector<uint8_t> ReadFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open " + path);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

}  // namespace

BootSplash BootSplash::Load(const std::string& root, const ImgArchive& images) {
    return BootSplash(DecodedImage::FromPng(ReadFile(root + "/splashtop.png")),
                      DecodedImage::FromPng(ReadFile(root + "/splashbot.png")),
                      DecodedImage::FromPng(images.Data("vir2lLogo.png")),
                      DecodedImage::FromPng(images.Data("mformaLogo.png")));
}

BootSplash::Phase BootSplash::PhaseAt(int64_t elapsedMs) {
    // The screen only repaints on 500ms boundaries -- see the class comment.
    const int64_t t = elapsedMs < 0 ? 0 : elapsedMs / kStepMs * kStepMs;
    if (t >= kEndMs) return Phase::Done;
    if (t >= kHoldStartMs) return Phase::SplashHold;
    if (t >= kCopyrightStartMs) return Phase::Copyright;
    if (t >= kBarStartMs) return Phase::SplashWithBar;
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
        DrawCentered(bb, vir2lLogo_, 10);
        int y = 10 + vir2lLogo_.height + 3;
        for (const char* line : kCopyStringParts) {
            const std::string s = line;
            // paintSplash() sets no font: the Graphics default, MIDP's
            // getFont(SYSTEM, PLAIN, MEDIUM) -- Alp13 on the device (see
            // bitmap_font.h). The lines are fixed strings the original never
            // wraps; Alp13's widest is 137px, well inside 176.
            BitmapFont::DrawString(bb, cx - BitmapFont::StringWidth(s, BitmapFont::Face::MediumPlain) / 2, y, s,
                                   kBlack, BitmapFont::Face::MediumPlain);
            y += 14;
        }
        const std::string distributed = "Distributed by:";
        BitmapFont::DrawString(bb, cx - BitmapFont::StringWidth(distributed, BitmapFont::Face::MediumPlain) / 2, 143,
                               distributed, kBlack, BitmapFont::Face::MediumPlain);
        DrawCentered(bb, carrierLogo_, 158);
        return;
    }

    bb.Fill(kSplashBlue);
    DrawCentered(bb, top_, 45);
    DrawCentered(bb, bottom_, 115);
    if (phase == Phase::SplashWithBar) {
        bb.FillRect(12, 165, 152, 22, kWhite);
        bb.FillRect(13, 166, 3 * percent_ / 2, 20, kBarFill);
    }
}

}  // namespace dawnstar
