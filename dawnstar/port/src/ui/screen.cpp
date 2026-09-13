#include "ui/screen.h"

#include <algorithm>

#include "graphics/bitmap_font.h"
#include "render/message_popup.h"

namespace dawnstar {

namespace {

constexpr uint16_t FromRgb24(int rgb) {
    return PackRGB565(static_cast<uint8_t>((rgb >> 16) & 0xFF), static_cast<uint8_t>((rgb >> 8) & 0xFF),
                       static_cast<uint8_t>(rgb & 0xFF));
}

// Screen.java's own g.setColor(...) literals, one constant per distinct
// call site value -- see each use below for which renderX() method it
// came from.
constexpr uint16_t kBackgroundColor = FromRgb24(2510210);    // renderHighlightedText/renderPromptList/renderPlainList
constexpr uint16_t kTitleBarColor = FromRgb24(0);            // renderTitleBar's own fillRect
constexpr uint16_t kTitleTextColor = FromRgb24(16777215);    // renderTitleBar's own drawString
constexpr uint16_t kPromptTextColor = FromRgb24(16776960);   // renderPromptList's prompt lines + renderItemRows/renderPlainList's item text
constexpr uint16_t kFooterTextColor = FromRgb24(16777215);   // renderPromptList's footer lines (variant 2)
constexpr uint16_t kHighlightBoxColor = FromRgb24(6710886);  // renderItemRows' selected-row box
constexpr uint16_t kSoftKeyBarColor = FromRgb24(16777215);   // renderSoftKeyBar's own fillRect
constexpr uint16_t kSoftKeyTextColor = FromRgb24(0);         // renderSoftKeyBar's own drawString
constexpr uint16_t kScrollArrowColor = FromRgb24(0);         // drawScrollArrow

// Screen.java's own `textFont.getHeight()` -- an invented per-row pixel
// pitch (no real SMALL_FONT/DEFAULT_TEXT_FONT metric survives, same
// status as every other invented BitmapFont metric). Reuses M30's
// already-established `render/message_popup.cpp`'s own 12px line pitch
// (its `paintMessagePopup()` port draws two lines 12px apart) rather
// than inventing a second, different pitch for no reason.
constexpr int kLineHeight = 12;

std::string CommandLabel(CommandId c) {
    switch (c) {
        case CommandId::Ok:
            return "Ok";
        case CommandId::Select:
            return "Select";
        case CommandId::Cancel:
            return "Cancel";
        case CommandId::Back:
            return "Back";
        case CommandId::Exit:
            return "Exit";
    }
    return "";
}

// Screen.java's own `drawScrollArrow(g, x, y, direction)`: direction 1
// draws an upward-pointing triangle (narrow at the top, widening toward
// the bottom -- the "more above, scroll up" indicator), direction 2 a
// downward-pointing one (wide at the top, narrowing toward the bottom).
// Every real line this draws is horizontal (`g.drawLine(x-i,y+i,x+i,y+i)`
// -- same y for both endpoints), so this only ever needs a horizontal
// span, not a general line primitive.
void DrawScrollArrow(Backbuffer& bb, int x, int y, int direction) {
    constexpr int kSize = 5;
    for (int i = 0; i < kSize; i++) {
        int xa, xb;
        if (direction == 1) {
            xa = x - i;
            xb = x + i;
        } else {
            xa = x - (kSize - i);
            xb = x + (kSize - i);
        }
        for (int xx = xa; xx <= xb; xx++) bb.SetPixel(xx, y + i, kScrollArrowColor);
    }
}

const std::string kEmptyString;

}  // namespace

Screen::Screen(ScreenMode mode) : mode_(mode) {
    // Screen(ESGame, mode, secondaryParam)'s own `if (mode == 4) {
    // addCommand(okCommand); setCommandListener(game); }` -- no
    // CommandListener exists yet to set (see this class's own header
    // doc comment), so only the command itself is reproduced.
    if (mode_ == ScreenMode::PlainList) {
        AddCommand(CommandId::Ok);
    }
}

void Screen::SetupList(const std::string& title, std::vector<std::string> items, bool cancelable) {
    title_ = title;
    items_ = std::move(items);
    marginX_ = 15;
    marginRight_ = 15;
    scrollTop_ = 0;
    itemCount_ = static_cast<int>(items_.size());
    int visible = std::min(itemCount_, 10);
    scrollBottom_ = scrollTop_ + visible - 1;
    selectedIndex_ = 0;
    AddCommand(CommandId::Select);
    if (cancelable) AddCommand(CommandId::Cancel);
}

void Screen::SetupMessage(const std::string& title, const std::string& body) {
    promptLines_.clear();
    footerLines_.clear();
    items_.clear();
    title_ = title;
    marginX_ = 5;
    marginRight_ = 5;
    scrollTop_ = 0;
    items_ = WrapText(body);
    itemCount_ = static_cast<int>(items_.size());
    int visible = std::min(itemCount_, 11);
    scrollBottom_ = scrollTop_ + visible - 1;
}

void Screen::SetupPromptList(const std::string& title, const std::string& prompt, std::vector<std::string> items) {
    title_ = title;
    marginX_ = 10;
    marginRight_ = 10;
    promptLines_ = WrapText(prompt);
    items_ = std::move(items);
    scrollTop_ = 0;
    itemCount_ = static_cast<int>(items_.size());
    itemGroupStart_.assign(static_cast<size_t>(itemCount_), 0);
    for (int i = 0; i < itemCount_; i++) itemGroupStart_[static_cast<size_t>(i)] = i;

    // setupPromptList()'s own per-item word-wrap-splitting loop, ported
    // line-for-line including its real, easy-to-miss subtlety: `wrapped.
    // length` (renamed `extraLines` in both) is used ONCE at its
    // original value (sizing `merged` and copying `wrapped` itself into
    // it), then decremented exactly once (Java's own `--extraLines`,
    // buried inside the third `arraycopy` call's argument list as a
    // side effect) -- every use AFTER that point (the itemGroupStart_
    // shift loop, the itemCount_ increase, and the `item` index advance)
    // reads the DECREMENTED value (wrapped.length - 1: the net number of
    // NEW lines a 1-item-for-N-lines split actually adds). Both variants
    // are correct and intentional, not a bug -- reproduced exactly
    // rather than "cleaned up" into a single consistent variable.
    int visibleLines = 0;
    for (int item = 0; item < itemCount_; item++) {
        visibleLines++;
        if (BitmapFont::StringWidth(items_[static_cast<size_t>(item)]) > width() - marginX_ - marginRight_) {
            std::vector<std::string> wrapped = WrapText(items_[static_cast<size_t>(item)]);
            int extraLines = static_cast<int>(wrapped.size());
            std::vector<std::string> merged(static_cast<size_t>(itemCount_ + extraLines - 1));
            for (int k = 0; k < item; k++) merged[static_cast<size_t>(k)] = items_[static_cast<size_t>(k)];
            for (int k = 0; k < extraLines; k++) merged[static_cast<size_t>(item + k)] = wrapped[static_cast<size_t>(k)];
            --extraLines;
            for (int k = 0; k < itemCount_ - item - 1; k++) {
                merged[static_cast<size_t>(item + 1 + extraLines + k)] = items_[static_cast<size_t>(item + 1 + k)];
            }
            for (int g = static_cast<int>(itemGroupStart_.size()) - 1; g >= visibleLines; g--) {
                itemGroupStart_[static_cast<size_t>(g)] += extraLines;
            }
            itemCount_ += extraLines;
            item += extraLines;
            items_ = std::move(merged);
        }
    }

    int visible = std::min(itemCount_, 9);
    scrollBottom_ = scrollTop_ + visible - 1;
    AddCommand(CommandId::Select);
    AddCommand(CommandId::Cancel);
}

void Screen::SetupPromptList(const std::string& title, const std::string& prompt, const std::string& footer,
                             std::vector<std::string> items) {
    SetupPromptList(title, prompt, std::move(items));
    footerLines_ = WrapText(footer);
}

std::vector<std::string> Screen::WrapText(const std::string& text) const {
    return MessagePopup::WordWrap(text, width() - marginX_ - marginRight_);
}

void Screen::RenderTitleBar(Backbuffer& bb) const {
    bb.FillRect(0, 0, width(), 14, kTitleBarColor);
    // Anchor 17 (TOP|HCENTER) in the original: `x` names the text's
    // horizontal CENTER, not its left edge.
    int x = width() / 2 - BitmapFont::StringWidth(title_) / 2;
    BitmapFont::DrawString(bb, x, 0, title_, kTitleTextColor);
}

int Screen::RenderItemRows(Backbuffer& bb, int cursorY, int lineHeight) const {
    for (int i = scrollTop_; i <= scrollBottom_; i++) {
        bool isSelectedRow = itemGroupStart_.empty()
                                  ? (i == selectedIndex_)
                                  : (i == itemGroupStart_[static_cast<size_t>(selectedIndex_)]);
        if (isSelectedRow) {
            int boxWidth = width() - 2 * (marginX_ - 10);
            int boxHeightRows = 1;
            if (!itemGroupStart_.empty()) {
                if (selectedIndex_ + 1 != static_cast<int>(itemGroupStart_.size())) {
                    boxHeightRows = itemGroupStart_[static_cast<size_t>(selectedIndex_ + 1)] -
                                    itemGroupStart_[static_cast<size_t>(selectedIndex_)];
                } else {
                    boxHeightRows = itemCount_ - itemGroupStart_[static_cast<size_t>(selectedIndex_)];
                }
            }
            int boxHeight = boxHeightRows * lineHeight + 2;
            bb.FillRect(marginX_ - 10, cursorY - 1, boxWidth, boxHeight, kHighlightBoxColor);
        }
        // Defensive bounds check not in the original (a real Screen's
        // scrollBottom_ never outgrows items_ by construction) -- same
        // "C++ has no equivalent safety net" reasoning as every other
        // such guard already in this port.
        if (i >= 0 && i < static_cast<int>(items_.size())) {
            BitmapFont::DrawString(bb, marginX_, cursorY, items_[static_cast<size_t>(i)], kPromptTextColor);
        }
        cursorY += lineHeight;
        cursorY++;
    }
    return cursorY;
}

void Screen::RenderHighlightedText(Backbuffer& bb) const {
    bb.FillRect(0, 0, width(), 20 + height(), kBackgroundColor);
    RenderTitleBar(bb);
    RenderItemRows(bb, 20, kLineHeight);
    if (scrollTop_ > 0) DrawScrollArrow(bb, 155, 180, 1);
    if (scrollBottom_ + 1 < itemCount_) DrawScrollArrow(bb, 165, 180, 2);
}

void Screen::RenderPromptList(Backbuffer& bb, int variant) const {
    bb.FillRect(0, 0, width(), 20 + height(), kBackgroundColor);
    RenderTitleBar(bb);
    int cursorY = 20;
    for (const std::string& line : promptLines_) {
        BitmapFont::DrawString(bb, marginX_, cursorY, line, kPromptTextColor);
        cursorY += kLineHeight;
    }
    if (variant == 2) {
        for (const std::string& line : footerLines_) {
            BitmapFont::DrawString(bb, marginX_, cursorY, line, kFooterTextColor);
            cursorY += kLineHeight;
        }
    }
    cursorY += 5;
    RenderItemRows(bb, cursorY, kLineHeight);
    if (scrollTop_ > 0) DrawScrollArrow(bb, 155, 180, 1);
    if (scrollBottom_ + 1 < itemCount_) DrawScrollArrow(bb, 165, 180, 2);
}

void Screen::RenderPlainList(Backbuffer& bb) const {
    bb.FillRect(0, 0, width(), 20 + height(), kBackgroundColor);
    RenderTitleBar(bb);
    int cursorY = 20;
    for (int i = scrollTop_; i <= scrollBottom_; i++) {
        if (i >= 0 && i < static_cast<int>(items_.size())) {
            BitmapFont::DrawString(bb, marginX_, cursorY, items_[static_cast<size_t>(i)], kPromptTextColor);
        }
        cursorY += kLineHeight;
    }
    if (scrollTop_ > 0) DrawScrollArrow(bb, 155, 180, 1);
    if (scrollBottom_ + 1 < itemCount_) DrawScrollArrow(bb, 165, 180, 2);
}

void Screen::RenderSoftKeyBar(Backbuffer& bb) const {
    if (commands_.empty()) return;
    bb.FillRect(0, 190, width(), 20, kSoftKeyBarColor);
    CommandId left;
    if (LeftSoftKeyCommand(&left)) {
        BitmapFont::DrawString(bb, 10, 194, CommandLabel(left), kSoftKeyTextColor);
    }
    CommandId right;
    if (RightSoftKeyCommand(&right)) {
        // Anchor 24 (TOP|RIGHT): `x` names the text's right edge.
        std::string label = CommandLabel(right);
        int x = (width() - 10) - BitmapFont::StringWidth(label);
        BitmapFont::DrawString(bb, x, 194, label, kSoftKeyTextColor);
    }
}

void Screen::Paint(Backbuffer& bb) const {
    switch (mode_) {
        case ScreenMode::HighlightedList:
            RenderHighlightedText(bb);
            break;
        case ScreenMode::PlainList:
            RenderPlainList(bb);
            break;
        case ScreenMode::PromptList:
            RenderPromptList(bb, 1);
            break;
        case ScreenMode::PromptListWithFooter:
            RenderPromptList(bb, 2);
            break;
    }
    RenderSoftKeyBar(bb);
}

void Screen::MoveSelectionUp() {
    // Screen.java's own `handleKey()` game-action-1 (UP) branch: `if
    // (mode != 3 && mode != 5 && mode != 6)` is exactly `mode == 4` over
    // the closed 4-value domain `mode_` can actually hold in this port
    // (ScreenMode has no 5th value), so it's simplified to that directly
    // rather than reproducing the redundant triple negative.
    if (mode_ == ScreenMode::PlainList) {
        if (scrollTop_ > 0) {
            scrollTop_--;
            scrollBottom_--;
        }
        return;
    }
    if (!itemGroupStart_.empty()) {
        if (selectedIndex_ > 0) {
            selectedIndex_--;
            if (itemGroupStart_[static_cast<size_t>(selectedIndex_)] < scrollTop_) {
                scrollBottom_ -= (scrollTop_ - itemGroupStart_[static_cast<size_t>(selectedIndex_)]);
                scrollTop_ = itemGroupStart_[static_cast<size_t>(selectedIndex_)];
            }
        }
    } else if (selectedIndex_ > 0) {
        selectedIndex_--;
        if (scrollTop_ > selectedIndex_) {
            scrollTop_--;
            scrollBottom_--;
        }
    }
}

void Screen::MoveSelectionDown() {
    if (mode_ == ScreenMode::PlainList) {
        if (scrollBottom_ < itemCount_ - 1) {
            scrollTop_++;
            scrollBottom_++;
        }
        return;
    }
    if (!itemGroupStart_.empty()) {
        if (selectedIndex_ + 1 < static_cast<int>(itemGroupStart_.size())) {
            selectedIndex_++;
            int windowSize = scrollBottom_ - scrollTop_;
            if (selectedIndex_ + 1 == static_cast<int>(itemGroupStart_.size())) {
                scrollTop_ = itemCount_ - windowSize - 1;
                scrollBottom_ = itemCount_ - 1;
            } else if (itemGroupStart_[static_cast<size_t>(selectedIndex_ + 1)] > scrollBottom_) {
                scrollBottom_ = itemGroupStart_[static_cast<size_t>(selectedIndex_ + 1)];
                scrollTop_ = scrollBottom_ - windowSize;
            }
        }
    } else if (selectedIndex_ < static_cast<int>(items_.size()) - 1) {
        selectedIndex_++;
        if (scrollBottom_ < selectedIndex_) {
            scrollTop_++;
            scrollBottom_++;
        }
    }
}

void Screen::AddCommand(CommandId c) { commands_.push_back(c); }

void Screen::RemoveCommand(CommandId c) {
    auto it = std::find(commands_.begin(), commands_.end(), c);
    if (it != commands_.end()) commands_.erase(it);
}

bool Screen::RightSoftKeyCommand(CommandId* out) const {
    if (commands_.size() == 1) {
        *out = commands_[0];
        return true;
    }
    if (commands_.size() == 2) {
        for (int i = 0; i < 2; i++) {
            if (commands_[static_cast<size_t>(i)] == CommandId::Ok || commands_[static_cast<size_t>(i)] == CommandId::Select) {
                *out = commands_[static_cast<size_t>(i)];
                return true;
            }
        }
    }
    return false;
}

bool Screen::LeftSoftKeyCommand(CommandId* out) const {
    if (commands_.size() == 2) {
        for (int i = 0; i < 2; i++) {
            if (commands_[static_cast<size_t>(i)] == CommandId::Back || commands_[static_cast<size_t>(i)] == CommandId::Cancel) {
                *out = commands_[static_cast<size_t>(i)];
                return true;
            }
        }
    }
    return false;
}

int Screen::SelectedIndexOrMinusOne() const {
    switch (mode_) {
        case ScreenMode::HighlightedList:
        case ScreenMode::PromptList:
        case ScreenMode::PromptListWithFooter:
            return selectedIndex_;
        case ScreenMode::PlainList:
        default:
            return -1;
    }
}

void Screen::SetSelectedIndex(int index) {
    // Screen.java's own `switch (mode) { case 3: case 5: case 6: ...
    // case 4: }` -- case 4's body is empty and sits last, so falling
    // into it from 3/5/6 changes nothing; mode 4 alone does nothing at
    // all. Ported as an early return for mode 4 instead of reproducing
    // the (inert) fallthrough.
    if (mode_ != ScreenMode::HighlightedList && mode_ != ScreenMode::PromptList &&
        mode_ != ScreenMode::PromptListWithFooter) {
        return;
    }
    selectedIndex_ = index;
    if (selectedIndex_ >= static_cast<int>(items_.size())) {
        selectedIndex_ = static_cast<int>(items_.size()) - 1;
    }
    if (selectedIndex_ > scrollBottom_) {
        int delta = selectedIndex_ - scrollBottom_;
        scrollBottom_ += delta;
        scrollTop_ += delta;
    } else if (selectedIndex_ < scrollTop_) {
        int delta = scrollTop_ - selectedIndex_;
        scrollBottom_ -= delta;
        scrollTop_ -= delta;
    }
}

const std::string& Screen::SelectedItemText() const { return items_[static_cast<size_t>(selectedIndex_)]; }

const std::string& Screen::FirstLine() const {
    if (mode_ == ScreenMode::PromptList || mode_ == ScreenMode::PromptListWithFooter) {
        return promptLines_.empty() ? kEmptyString : promptLines_[0];
    }
    if (mode_ == ScreenMode::PlainList) {
        return items_.empty() ? kEmptyString : items_[0];
    }
    // Mode HighlightedList: the original returns null here too.
    return kEmptyString;
}

void Screen::SetTitle(const std::string& title) { title_ = title; }

void Screen::SetItems(const std::string& text) {
    // Screen.java's own `setItems()`: a real, preserved quirk -- neither
    // itemCount_ nor scrollBottom_ is refreshed here (only promptLines_/
    // items_ and scrollTop_), so a call that actually changes the
    // wrapped line count leaves them stale until something else (a
    // fresh setup* call, or SetSelectedIndex's own scroll-window
    // adjustment) resets them. Not "fixed" here.
    if (mode_ == ScreenMode::PromptList || mode_ == ScreenMode::PromptListWithFooter) {
        promptLines_ = WrapText(text);
    } else if (mode_ == ScreenMode::PlainList) {
        items_ = WrapText(text);
    }
    scrollTop_ = 0;
}

void Screen::SetTextColumn(int column, const std::string& text) {
    bool takeElseBranch = (column == 0) && (mode_ == ScreenMode::PromptList ||
                                             mode_ == ScreenMode::PromptListWithFooter || mode_ == ScreenMode::PlainList);
    if (takeElseBranch) {
        promptLines_ = WrapText(text);
        return;
    }
    if (column == 1 && mode_ == ScreenMode::PromptListWithFooter) {
        footerLines_ = WrapText(text);
    }
}

}  // namespace dawnstar
