#include "ui/loading_screen.h"

#include <string>

#include "graphics/bitmap_font.h"

namespace dawnstar {

namespace {

constexpr uint16_t FromRgb24(int rgb) {
    return PackRGB565(static_cast<uint8_t>((rgb >> 16) & 0xFF), static_cast<uint8_t>((rgb >> 8) & 0xFF),
                      static_cast<uint8_t>(rgb & 0xFF));
}

// LoadingScreen.java's own g.setColor(...) literals in renderProgress() -- the
// background blue is the very same 2510210 Screen.java's own render methods use
// (see ui/screen.cpp's own kBackgroundColor).
constexpr uint16_t kBackgroundColor = FromRgb24(2510210);
constexpr uint16_t kTextColor = FromRgb24(16777215);      // both drawString calls
constexpr uint16_t kBarBoxColor = FromRgb24(16777215);    // the outline box's own fillRect
constexpr uint16_t kBarFillColor = FromRgb24(255);        // the bar itself: pure blue

// renderProgress()'s own two fillRect geometries, all call-site literals: a
// white 90x20 box at y=60, and the bar inset 1px inside it ((width-88)/2, 61,
// percent*88/100, 18).
constexpr int kBarBoxWidth = 90;
constexpr int kBarBoxHeight = 20;
constexpr int kBarBoxY = 60;
constexpr int kBarMaxWidth = 88;
constexpr int kBarHeight = 18;

// The per-mode action line, exactly renderProgress()'s own four literals.
std::string TitleForMode(LoadingScreenMode mode) {
    switch (mode) {
        case LoadingScreenMode::CreatingNewGame:
            return "Creating New Game";
        case LoadingScreenMode::LoadingGame:
            return "Loading Game";
        case LoadingScreenMode::SavingGame:
            return "Saving Game";
        case LoadingScreenMode::LoadingDungeon:
            return "Loading Dungeon";
    }
    return "";
}

}  // namespace

void LoadingScreen::Render(Backbuffer& bb) const {
    // g.fillRect(0, 0, width(), 20 + height()) -- the original's own title-bar
    // offset idiom, which for this screen (no title bar, no soft-key bar, no
    // list) simply means the whole 176x208 backbuffer.
    bb.Fill(kBackgroundColor);

    const int cx = Width() / 2;
    // Anchor 17 in the original: `x` names the text's horizontal CENTER, not its
    // left edge -- the same convention ui/screen.cpp's own RenderTitleBar
    // already established for that anchor.
    const std::string title = TitleForMode(mode_);
    BitmapFont::DrawString(bb, cx - BitmapFont::StringWidth(title) / 2, 30, title, kTextColor);
    BitmapFont::DrawString(bb, cx - BitmapFont::StringWidth("Please Wait") / 2, 45, "Please Wait", kTextColor);

    bb.FillRect((Width() - kBarBoxWidth) / 2, kBarBoxY, kBarBoxWidth, kBarBoxHeight, kBarBoxColor);
    // percent * 88 / 100 with no clamp -- see SetPercent's own doc comment.
    const int barWidth = percent_ * kBarMaxWidth / 100;
    bb.FillRect((Width() - kBarMaxWidth) / 2, kBarBoxY + 1, barWidth, kBarHeight, kBarFillColor);
}

}  // namespace dawnstar
