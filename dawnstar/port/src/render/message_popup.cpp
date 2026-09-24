#include "render/message_popup.h"

#include "graphics/bitmap_font.h"

namespace dawnstar {

namespace {

// GameCanvas.paintMessagePopup()'s own g.setColor(13080935)/color 0.
constexpr uint16_t kPopupBg = PackRGB565(0xC7, 0x99, 0x67);
constexpr uint16_t kTextColor = PackRGB565(0, 0, 0);
// paintMessagePopup(): SMALL_FONT (LatinPlain12) -- the wrap measurements
// below must use the same face the text is drawn in.
constexpr BitmapFont::Face kPopupFace = BitmapFont::Face::SmallPlain;
constexpr int64_t kTimeoutMs = 3000;

}  // namespace

bool MessagePopup::Show(MessagePopupState& state, const std::array<std::string, 2>& lines, int priority,
                         int64_t nowMs) {
    if (priority <= state.priority && priority >= 0) return false;

    state.lines = lines;
    state.priority = priority < 0 ? 10 : priority;
    state.shownAtMs = nowMs;
    state.visible = true;
    return true;
}

void MessagePopup::Tick(MessagePopupState& state, int64_t nowMs) {
    if (nowMs - state.shownAtMs > kTimeoutMs) {
        state.visible = false;
        state.priority = 0;
    }
}

std::vector<std::string> MessagePopup::WordWrap(const std::string& textIn, int maxWidthPxIn) {
    std::string text = textIn;
    int maxWidthPx = maxWidthPxIn;

    size_t newlineAt = text.find('\n');
    if (newlineAt != std::string::npos) {
        if (newlineAt != text.size() - 1) {
            std::vector<std::string> firstLines;
            if (newlineAt == 0) {
                firstLines = {" "};
            } else {
                firstLines = WordWrap(text.substr(0, newlineAt), maxWidthPx);
            }
            std::vector<std::string> restLines = WordWrap(text.substr(newlineAt + 1), maxWidthPx);
            firstLines.insert(firstLines.end(), restLines.begin(), restLines.end());
            return firstLines;
        }
        text = text.substr(0, text.size() - 1);
    }

    if (BitmapFont::StringWidth(text, kPopupFace) < maxWidthPx) {
        return {text};
    }

    text += " ";
    std::vector<std::string> lines;
    int lineStart = 0;
    maxWidthPx -= 8;

    size_t spaceAt;
    while ((spaceAt = text.find(' ', static_cast<size_t>(lineStart) + 1)) != std::string::npos) {
        if (BitmapFont::StringWidth(text.substr(0, spaceAt), kPopupFace) < maxWidthPx) {
            lineStart = static_cast<int>(spaceAt);
        } else {
            if (lineStart == 0) {
                int w = 0;
                // Defensive bounds check not in the original -- see
                // this method's own header doc comment. M58: `CharWidth`
                // (not a flat kAdvance) matches the original's own
                // `w += font.charWidth(text.charAt(lineStart));` here --
                // now that BitmapFont is a real proportional font (see
                // its own class comment), there's no reason to keep
                // approximating this one spot with a flat advance when
                // the real per-character width is available.
                while (w < maxWidthPx && lineStart < static_cast<int>(text.size())) {
                    w += BitmapFont::CharWidth(text[static_cast<size_t>(lineStart)], kPopupFace);
                    lineStart++;
                }
                lines.push_back(text.substr(0, static_cast<size_t>(lineStart)));
                lineStart--;
            } else {
                lines.push_back(text.substr(0, static_cast<size_t>(lineStart)));
            }
            text = text.substr(static_cast<size_t>(lineStart) + 1);
            lineStart = 0;
        }
    }

    if (!text.empty() && text != " ") {
        lines.push_back(text);
    }

    return lines;
}

std::array<std::string, 2> MessagePopup::WrapToTwoLines(const std::string& text) {
    // M58: 80, not the original's own real (unrecoverable) SMALL_FONT-based
    // threshold: paintMessagePopup() draws unclipped at a fixed (100,
    // y) -- see this method's own header doc comment on why that exact
    // position is real, decompiled data this port keeps as-is -- so the
    // real constraint is "no wrapped line should ever run past the
    // 176px-wide screen's own right edge from x=100" (a 76px budget),
    // not the rounded background box's own narrower 75px width (which
    // the original never clips text to either -- confirmed directly:
    // paintMessagePopup() has no g.setClip call). 80 keeps every
    // wrapped line comfortably under that 76px budget for this port's
    // own real (GDI, proportional, wider-than-the-old-invented-font)
    // BitmapFont -- verified against the two longest real strings this
    // path ever wraps (Shop.NAMES' "Heavy Armor Peddler"/"Weapon
    // Peddler") in message_popup_smoke.cpp.
    //
    // M78: the threshold IS recoverable once the real font is: the original
    // calls `wordWrap(text, 69, SMALL_FONT)`, and SMALL_FONT is the
    // device's LatinPlain12 (graphics/bitmap_font.h). So 69 whenever that
    // face is really loaded; 80 stays only for the wider GDI stand-in.
    const int wrapWidth = BitmapFont::IsDeviceFace(kPopupFace) ? 69 : 80;
    std::vector<std::string> wrapped = WordWrap(text, wrapWidth);
    std::array<std::string, 2> result{"", ""};
    if (!wrapped.empty()) result[0] = wrapped[0];
    if (wrapped.size() >= 2) result[1] = wrapped[1];
    return result;
}

void MessagePopup::Paint(Backbuffer& bb, const MessagePopupState& state) {
    if (!state.visible) return;

    bb.FillRoundRect(96, 118, 75, 35, 5, 5, kPopupBg);
    BitmapFont::DrawString(bb, 100, 122, state.lines[0], kTextColor, kPopupFace);
    BitmapFont::DrawString(bb, 100, 134, state.lines[1], kTextColor, kPopupFace);
}

}  // namespace dawnstar
