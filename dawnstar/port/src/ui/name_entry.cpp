#include "ui/name_entry.h"

#include "graphics/bitmap_font.h"
#include "render/message_popup.h"

namespace dawnstar {

namespace {

constexpr uint16_t FromRgb24(int rgb) {
    return PackRGB565(static_cast<uint8_t>((rgb >> 16) & 0xFF), static_cast<uint8_t>((rgb >> 8) & 0xFF),
                       static_cast<uint8_t>(rgb & 0xFF));
}

// The exact same `Screen.java`-derived RGB24 literals M37's own
// `ui/screen.cpp` uses (not re-derived here), reused directly purely
// for visual consistency with the rest of this port's UI -- there is
// no real `charNameTextForm` palette to match in the first place (see
// this class's own header doc comment).
constexpr uint16_t kBackgroundColor = FromRgb24(2510210);
constexpr uint16_t kTitleBarColor = FromRgb24(0);
constexpr uint16_t kTitleTextColor = FromRgb24(16777215);
constexpr uint16_t kPromptTextColor = FromRgb24(16776960);
constexpr uint16_t kFieldBoxColor = FromRgb24(0);
constexpr uint16_t kSoftKeyBarColor = FromRgb24(16777215);
constexpr uint16_t kSoftKeyTextColor = FromRgb24(0);
constexpr int kLineHeight = 12;

bool IsSupportedChar(char c) {
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= '0' && c <= '9') return true;
    return c == ' ' || c == '\'' || c == '-';
}

}  // namespace

void NameEntry::OnChar(char c) {
    if (text_.size() >= kMaxLength) return;
    if (!IsSupportedChar(c)) return;
    text_ += c;
}

void NameEntry::OnBackspace() {
    if (!text_.empty()) text_.pop_back();
}

void NameEntry::Render(Backbuffer& bb) const {
    bb.FillRect(0, 0, Backbuffer::kWidth, Backbuffer::kHeight, kBackgroundColor);

    bb.FillRect(0, 0, Backbuffer::kWidth, 14, kTitleBarColor);
    int titleX = Backbuffer::kWidth / 2 - BitmapFont::StringWidth("Enter name") / 2;
    BitmapFont::DrawString(bb, titleX, 0, "Enter name", kTitleTextColor);

    int cursorY = 20;
    for (const std::string& line :
         MessagePopup::WordWrap("Enter a name for your character", Backbuffer::kWidth - 10 - 10)) {
        BitmapFont::DrawString(bb, 10, cursorY, line, kPromptTextColor);
        cursorY += kLineHeight;
    }
    cursorY += 5;

    // A simple boxed field showing the text typed so far, plus a
    // trailing cursor block -- invented, see this class's own header
    // doc comment on why there's no real layout to match here.
    int fieldWidth = Backbuffer::kWidth - 20;
    bb.FillRect(10, cursorY, fieldWidth, kLineHeight + 2, kFieldBoxColor);
    BitmapFont::DrawString(bb, 12, cursorY + 1, text_, kPromptTextColor);
    int cursorX = 12 + static_cast<int>(text_.size()) * BitmapFont::kAdvance;
    bb.FillRect(cursorX, cursorY + 1, BitmapFont::kGlyphWidth, BitmapFont::kGlyphHeight, kPromptTextColor);

    bb.FillRect(0, 190, Backbuffer::kWidth, 20, kSoftKeyBarColor);
    int okX = Backbuffer::kWidth - 10 - BitmapFont::StringWidth("OK");
    BitmapFont::DrawString(bb, okX, 194, "OK", kSoftKeyTextColor);
}

}  // namespace dawnstar
