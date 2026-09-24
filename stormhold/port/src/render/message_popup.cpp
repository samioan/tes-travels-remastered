#include "render/message_popup.h"

#include "graphics/bitmap_font.h"

namespace stormhold {

namespace {

// GameCanvas.paintMessagePopup()'s own g.setColor(13080935)/color 0 --
// the same numeric popup-background color GameCanvas.paintHud() itself
// uses for its hotbar panel (../../../src/GameCanvas.java line 1027),
// confirmed shared with dawnstar's own identical `kPopupBg`.
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

void MessagePopup::Paint(Backbuffer& bb, const MessagePopupState& state) {
    if (!state.visible) return;

    bb.FillRoundRect(96, 118, 75, 35, 5, 5, kPopupBg);
    // paintMessagePopup(): smallFont (LatinPlain12).
    BitmapFont::DrawString(bb, 100, 122, state.lines[0], kTextColor, BitmapFont::Face::SmallPlain);
    BitmapFont::DrawString(bb, 100, 134, state.lines[1], kTextColor, BitmapFont::Face::SmallPlain);
}

}  // namespace stormhold
