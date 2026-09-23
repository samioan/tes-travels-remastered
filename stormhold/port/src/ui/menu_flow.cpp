#include "ui/menu_flow.h"

#include <algorithm>

#include "graphics/bitmap_font.h"
#include "player/player_creation.h"

namespace stormhold {

namespace {

// Real UIScreen.java colors, read straight off its own setColor() calls
// (paintList()/paintPromptList()'s body fill 11429934, paintTitleBar()'s
// black/white, paintRow()'s selected-row fill 6710886, item text
// 16776960, paintCommandBar()'s white bar/black text) -- kept identical
// rather than inventing a new port-only palette, even though this
// screen's own LAYOUT is a simplified reimplementation (see this file's
// class comment).
constexpr uint16_t kTitleBg = PackRGB565(0, 0, 0);
constexpr uint16_t kTitleFg = PackRGB565(255, 255, 255);
constexpr uint16_t kBodyBg = PackRGB565(0xAE, 0x68, 0x2E);
constexpr uint16_t kItemFg = PackRGB565(255, 255, 0);
constexpr uint16_t kSelectedBg = PackRGB565(0x66, 0x66, 0x66);
constexpr uint16_t kBarBg = PackRGB565(255, 255, 255);
constexpr uint16_t kBarFg = PackRGB565(0, 0, 0);
constexpr uint16_t kInputFg = PackRGB565(255, 255, 255);
constexpr uint16_t kErrorFg = PackRGB565(255, 96, 96);

constexpr int kLineHeight = BitmapFont::kGlyphHeight + 2;
constexpr int kContentTop = 16;
constexpr int kBarTop = Backbuffer::kHeight - 14;
constexpr int kMargin = 8;

// UIScreen.java's own creditsLines, joined with real line breaks --
// showExitCredits() itself concatenates them with NO separator at all
// (`body = body + creditsLines[i]`), which only reads sensibly there
// because that screen paints each entry as a separate centered line off
// the ORIGINAL creditsLines array, not off the concatenated string. This
// screen paints the concatenated string through the ordinary word-wrap
// path instead (no separate "Credits" UIScreen mode exists to reuse), so
// newlines are inserted here for readability -- a presentation-only
// adaptation, not a byte-exact transcription of any single real string.
const std::string kCreditsText =
    "(c) 2003 Vir2L Studios,\n"
    "a ZeniMax Media company.\n"
    "The Elder Scrolls and Vir2L\n"
    "are registered trademarks\n"
    "of ZeniMax Media Inc.\n"
    "All rights reserved.";

bool IsListScreen(MenuScreen s) {
    return s == MenuScreen::MainMenu || s == MenuScreen::ClassSelect || s == MenuScreen::ClassConfirm;
}

int ListItemCount(MenuScreen s, const CharacterData& charData) {
    switch (s) {
        case MenuScreen::MainMenu:
            return 4;
        case MenuScreen::ClassSelect:
            return charData.ClassCount();
        case MenuScreen::ClassConfirm:
            return 2;
        default:
            return 0;
    }
}

void DrawCentered(Backbuffer& bb, int y, const std::string& text, uint16_t color) {
    int w = BitmapFont::StringWidth(text);
    BitmapFont::DrawString(bb, (Backbuffer::kWidth - w) / 2, y, text, color);
}

void PaintTitleBar(Backbuffer& bb, const std::string& title) {
    bb.FillRect(0, 0, Backbuffer::kWidth, 12, kTitleBg);
    DrawCentered(bb, 3, title, kTitleFg);
}

void PaintBottomBar(Backbuffer& bb, const std::string& leftLabel, const std::string& rightLabel) {
    bb.FillRect(0, kBarTop, Backbuffer::kWidth, Backbuffer::kHeight - kBarTop, kBarBg);
    if (!leftLabel.empty()) BitmapFont::DrawString(bb, 4, kBarTop + 4, leftLabel, kBarFg);
    if (!rightLabel.empty()) {
        int w = BitmapFont::StringWidth(rightLabel);
        BitmapFont::DrawString(bb, Backbuffer::kWidth - 4 - w, kBarTop + 4, rightLabel, kBarFg);
    }
}

// UIScreen.wordWrapInto()'s counterpart -- word-wrap only (no char-wrap
// fallback for a single word wider than the line: every real string
// this screen displays is short common English words, so that fallback
// path UIScreen.charWrap() exists for is never actually exercised here).
std::vector<std::string> WordWrap(const std::string& text, int maxWidthPx) {
    std::vector<std::string> lines;
    size_t paraStart = 0;

    while (true) {
        size_t nl = text.find('\n', paraStart);
        std::string paragraph = (nl == std::string::npos) ? text.substr(paraStart) : text.substr(paraStart, nl - paraStart);

        std::vector<std::string> words;
        size_t p = 0;
        while (p < paragraph.size()) {
            while (p < paragraph.size() && paragraph[p] == ' ') p++;
            size_t wordStart = p;
            while (p < paragraph.size() && paragraph[p] != ' ') p++;
            if (p > wordStart) words.push_back(paragraph.substr(wordStart, p - wordStart));
        }

        std::string current;
        for (const std::string& word : words) {
            std::string candidate = current.empty() ? word : current + " " + word;
            if (BitmapFont::StringWidth(candidate) > maxWidthPx && !current.empty()) {
                lines.push_back(current);
                current = word;
            } else {
                current = candidate;
            }
        }
        lines.push_back(current);

        if (nl == std::string::npos) break;
        paraStart = nl + 1;
    }

    return lines;
}

void PaintPanel(Backbuffer& bb, const std::string& title) {
    bb.FillRect(0, 12, Backbuffer::kWidth, kBarTop - 12, kBodyBg);
    PaintTitleBar(bb, title);
}

void PaintList(Backbuffer& bb, const std::string& title, const std::vector<std::string>& header,
               const std::vector<std::string>& items, int selected) {
    PaintPanel(bb, title);
    int y = kContentTop;
    for (const std::string& line : header) {
        BitmapFont::DrawString(bb, kMargin, y, line, kItemFg);
        y += kLineHeight;
    }
    if (!header.empty()) y += 2;

    for (size_t i = 0; i < items.size(); i++) {
        if (static_cast<int>(i) == selected) {
            bb.FillRect(kMargin - 4, y - 1, Backbuffer::kWidth - 2 * (kMargin - 4), kLineHeight, kSelectedBg);
        }
        BitmapFont::DrawString(bb, kMargin, y, items[i], kItemFg);
        y += kLineHeight;
    }
}

// `scroll` is state.selectedIndex reinterpreted as a line-scroll offset
// for message-shaped screens (see menu_flow.h's own MoveSelection doc
// comment) -- clamped here at paint time rather than in MoveSelection
// itself, since clamping needs this screen's own real line count.
void PaintMessage(Backbuffer& bb, const std::string& title, const std::string& body, int scroll) {
    PaintPanel(bb, title);
    std::vector<std::string> lines = WordWrap(body, Backbuffer::kWidth - 2 * kMargin);
    int visibleRows = std::max(1, (kBarTop - kContentTop) / kLineHeight);
    int maxScroll = std::max(0, static_cast<int>(lines.size()) - visibleRows);
    int start = std::min(std::max(scroll, 0), maxScroll);

    int y = kContentTop;
    for (int i = start; i < static_cast<int>(lines.size()) && i < start + visibleRows; i++) {
        BitmapFont::DrawString(bb, kMargin, y, lines[static_cast<size_t>(i)], kItemFg);
        y += kLineHeight;
    }
}

}  // namespace

void MenuFlow::MoveSelection(MenuFlowState& state, int delta, const CharacterData& charData) {
    if (IsListScreen(state.screen)) {
        int count = ListItemCount(state.screen, charData);
        state.selectedIndex = std::max(0, std::min(state.selectedIndex + delta, count - 1));
    } else {
        // Message-shaped screens: up/down scrolls instead of selecting --
        // upper-bounded generously (see PaintMessage's own real clamp for
        // the exact per-screen bound).
        state.selectedIndex = std::max(0, std::min(state.selectedIndex + delta, 64));
    }
}

void MenuFlow::Confirm(MenuFlowState& state, const CharacterData& charData, const ItemDatabase& items,
                        const std::string& savePath) {
    switch (state.screen) {
        case MenuScreen::MainMenu:
            switch (state.selectedIndex) {
                case 0:  // New Game
                    state.screen = MenuScreen::ClassSelect;
                    state.selectedIndex = state.chosenClassIndex;
                    break;
                case 1:  // Continue Game -- see this file's own header comment.
                    if (GameSave::Exists(savePath)) {
                        state.loadRequested = true;
                        state.screen = MenuScreen::Finished;
                    } else {
                        state.screen = MenuScreen::NoSavedGame;
                        state.selectedIndex = 0;
                    }
                    break;
                case 2:  // Credits
                    state.screen = MenuScreen::Credits;
                    state.selectedIndex = 0;
                    break;
                case 3:  // Exit
                    state.exitRequested = true;
                    break;
                default:
                    break;
            }
            break;

        case MenuScreen::ClassSelect:
            state.chosenClassIndex = state.selectedIndex;
            state.draft = PlayerCreation::CreateCharacter(state.chosenClassIndex, "", 1, charData, items);
            state.screen = MenuScreen::ClassConfirm;
            state.selectedIndex = 0;
            break;

        case MenuScreen::ClassConfirm:
            if (state.selectedIndex == 0) {
                state.screen = MenuScreen::ClassInfo;
            } else {
                state.screen = MenuScreen::CharacterCreated;
            }
            state.selectedIndex = 0;
            break;

        case MenuScreen::ClassInfo:
            // The real classInfoUI's screenGroup==5 commandAction branch
            // only reacts to cmdBack, but the only command actually added
            // to that screen is cmdOk (setupMessage()'s own default) --
            // reading the whole dispatcher shows pressing the one softkey
            // there does nothing at all in the original (likely a genuine
            // original bug/dead end, not something worth reproducing).
            // This port's own Confirm/Cancel both just return to
            // ClassConfirm from here.
            state.screen = MenuScreen::ClassConfirm;
            state.selectedIndex = 0;
            break;

        case MenuScreen::CharacterCreated:
            state.screen = MenuScreen::EnterName;
            state.enteredName.clear();
            state.nameTooShort = false;
            break;

        case MenuScreen::EnterName:
            if (state.enteredName.size() < 3) {
                state.nameTooShort = true;
            } else {
                state.nameTooShort = false;
                if (state.draft.has_value()) state.draft->name = state.enteredName;
                state.screen = MenuScreen::Welcome;
                state.selectedIndex = 0;
            }
            break;

        case MenuScreen::NoSavedGame:
        case MenuScreen::Credits:
            state.screen = MenuScreen::MainMenu;
            state.selectedIndex = 0;
            break;

        case MenuScreen::Welcome:
            state.screen = MenuScreen::Intro;
            state.selectedIndex = 0;
            break;

        case MenuScreen::Intro:
            state.screen = MenuScreen::Finished;
            break;

        case MenuScreen::Finished:
            break;
    }
}

void MenuFlow::Cancel(MenuFlowState& state) {
    switch (state.screen) {
        case MenuScreen::ClassSelect:
            state.screen = MenuScreen::MainMenu;
            state.selectedIndex = 0;
            break;

        case MenuScreen::ClassConfirm:
            state.screen = MenuScreen::ClassSelect;
            state.selectedIndex = state.chosenClassIndex;
            break;

        case MenuScreen::ClassInfo:
        case MenuScreen::CharacterCreated:
            state.screen = MenuScreen::ClassConfirm;
            state.selectedIndex = 0;
            break;

        case MenuScreen::EnterName:
            state.screen = MenuScreen::CharacterCreated;
            state.nameTooShort = false;
            state.selectedIndex = 0;
            break;

        case MenuScreen::NoSavedGame:
        case MenuScreen::Credits:
            state.screen = MenuScreen::MainMenu;
            state.selectedIndex = 0;
            break;

        default:
            // MainMenu/Welcome/Intro/Finished: no cancel path in the
            // original either (no cmdBack/nextScreen wired for any of
            // them -- see src/ESGame.java's own allocateAllUIs()).
            break;
    }
}

void MenuFlow::TypeChar(MenuFlowState& state, char c) {
    if (state.screen != MenuScreen::EnterName) return;
    // ESGame.java's own `new TextField(null, null, 10, 0)` -- maxSize 10.
    if (state.enteredName.size() >= 10) return;
    state.enteredName.push_back(c);
    state.nameTooShort = false;
}

void MenuFlow::Backspace(MenuFlowState& state) {
    if (state.screen != MenuScreen::EnterName) return;
    if (!state.enteredName.empty()) state.enteredName.pop_back();
}

void MenuFlow::Render(Backbuffer& bb, const MenuFlowState& state, const CharacterData& charData,
                       const ShopDialogue& dialogue) {
    switch (state.screen) {
        case MenuScreen::MainMenu: {
            std::vector<std::string> items = {"New Game", "Continue Game", "Credits", "Exit"};
            PaintList(bb, "Main Menu", {}, items, state.selectedIndex);
            PaintBottomBar(bb, "", "Select");
            break;
        }
        case MenuScreen::ClassSelect: {
            PaintList(bb, "New Game", {"Select a Class:"}, charData.classNames, state.selectedIndex);
            PaintBottomBar(bb, "Cancel", "Select");
            break;
        }
        case MenuScreen::ClassConfirm: {
            std::string className =
                (state.chosenClassIndex >= 0 && state.chosenClassIndex < charData.ClassCount())
                    ? charData.classNames[static_cast<size_t>(state.chosenClassIndex)]
                    : "";
            std::vector<std::string> items = {"See Class Info", "Create Character"};
            PaintList(bb, "Character", {"You selected:", className}, items, state.selectedIndex);
            PaintBottomBar(bb, "Cancel", "Select");
            break;
        }
        case MenuScreen::ClassInfo: {
            std::string body = state.draft.has_value() ? PlayerCreation::CharacterSummaryShort(*state.draft, charData) : "";
            PaintMessage(bb, "Info", body, state.selectedIndex);
            PaintBottomBar(bb, "", "Back");
            break;
        }
        case MenuScreen::CharacterCreated:
            PaintMessage(bb, "New Character", "Character Created! Press 'Select' to enter a name.", state.selectedIndex);
            PaintBottomBar(bb, "Back", "Select");
            break;
        case MenuScreen::EnterName: {
            PaintPanel(bb, "Enter Name");
            BitmapFont::DrawString(bb, kMargin, kContentTop, "Enter a name for", kItemFg);
            BitmapFont::DrawString(bb, kMargin, kContentTop + kLineHeight, "your character:", kItemFg);
            std::string shown = state.enteredName + "_";
            BitmapFont::DrawString(bb, kMargin, kContentTop + 3 * kLineHeight, shown, kInputFg);
            if (state.nameTooShort) {
                BitmapFont::DrawString(bb, kMargin, kContentTop + 5 * kLineHeight, "Name must be at least", kErrorFg);
                BitmapFont::DrawString(bb, kMargin, kContentTop + 6 * kLineHeight, "3 letters.", kErrorFg);
            }
            PaintBottomBar(bb, "Back", "Ok");
            break;
        }
        case MenuScreen::NoSavedGame:
            PaintMessage(bb, "Unavailable", "No game is available for loading. Press OK to return to main menu.",
                         state.selectedIndex);
            PaintBottomBar(bb, "", "Ok");
            break;
        case MenuScreen::Credits:
            PaintMessage(bb, "Credits", kCreditsText, state.selectedIndex);
            PaintBottomBar(bb, "", "Ok");
            break;
        case MenuScreen::Welcome:
            PaintMessage(bb, "Welcome", "Welcome to The Elder Scrolls Travels!", state.selectedIndex);
            PaintBottomBar(bb, "", "Ok");
            break;
        case MenuScreen::Intro:
            PaintMessage(bb, "Introduction", dialogue.groups[7][3], state.selectedIndex);
            PaintBottomBar(bb, "", "Ok");
            break;
        case MenuScreen::Finished:
            // main.cpp's own live tick/render pipeline takes over.
            break;
    }
}

}  // namespace stormhold
