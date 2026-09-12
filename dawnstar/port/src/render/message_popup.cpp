#include "render/message_popup.h"

#include "graphics/bitmap_font.h"

namespace dawnstar {

namespace {

// GameCanvas.paintMessagePopup()'s own g.setColor(13080935)/color 0.
constexpr uint16_t kPopupBg = PackRGB565(0xC7, 0x99, 0x67);
constexpr uint16_t kTextColor = PackRGB565(0, 0, 0);
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

    if (BitmapFont::StringWidth(text) < maxWidthPx) {
        return {text};
    }

    text += " ";
    std::vector<std::string> lines;
    int lineStart = 0;
    maxWidthPx -= 8;

    size_t spaceAt;
    while ((spaceAt = text.find(' ', static_cast<size_t>(lineStart) + 1)) != std::string::npos) {
        if (BitmapFont::StringWidth(text.substr(0, spaceAt)) < maxWidthPx) {
            lineStart = static_cast<int>(spaceAt);
        } else {
            if (lineStart == 0) {
                int w = 0;
                // Defensive bounds check not in the original -- see
                // this method's own header doc comment.
                while (w < maxWidthPx && lineStart < static_cast<int>(text.size())) {
                    w += BitmapFont::kAdvance;
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
    std::vector<std::string> wrapped = WordWrap(text, 69);
    std::array<std::string, 2> result{"", ""};
    if (!wrapped.empty()) result[0] = wrapped[0];
    if (wrapped.size() >= 2) result[1] = wrapped[1];
    return result;
}

void MessagePopup::Paint(Backbuffer& bb, const MessagePopupState& state) {
    if (!state.visible) return;

    bb.FillRoundRect(96, 118, 75, 35, 5, 5, kPopupBg);
    BitmapFont::DrawString(bb, 100, 122, state.lines[0], kTextColor);
    BitmapFont::DrawString(bb, 100, 134, state.lines[1], kTextColor);
}

}  // namespace dawnstar
