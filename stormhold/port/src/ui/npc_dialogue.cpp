#include "ui/npc_dialogue.h"

#include <algorithm>
#include <vector>

#include "graphics/bitmap_font.h"

namespace stormhold {

namespace {

// Same palette `ui/menu_flow.cpp`'s own message screens use (UIScreen
// .java's real setColor() calls) -- kept identical rather than inventing
// a separate port-only palette for what is, structurally, the same kind
// of screen.
constexpr uint16_t kTitleBg = PackRGB565(0, 0, 0);
constexpr uint16_t kTitleFg = PackRGB565(255, 255, 255);
constexpr uint16_t kBodyBg = PackRGB565(0xAE, 0x68, 0x2E);
constexpr uint16_t kBodyFg = PackRGB565(255, 255, 0);
constexpr uint16_t kBarBg = PackRGB565(255, 255, 255);
constexpr uint16_t kBarFg = PackRGB565(0, 0, 0);

constexpr int kLineHeight = BitmapFont::kGlyphHeight + 2;
constexpr int kContentTop = 16;
constexpr int kBarTop = Backbuffer::kHeight - 14;
constexpr int kMargin = 8;

// Small, deliberate duplicate of `ui/menu_flow.cpp`'s own file-local
// `WordWrap` -- that one isn't exported (anonymous-namespace, tied to
// MenuFlow's own pre-game screen state machine), and this screen is a
// genuinely separate concern (a live-gameplay modal, not a pre-game
// flow step) -- same "small pieces duplicated rather than shared until
// it's clear they diverge" precedent docs/PORT_ROADMAP.md's own top-level
// "Decisions carried through every milestone" section already documents
// for GameClock/Backbuffer/Window/BinaryReader.
std::vector<std::string> WordWrap(const std::string& text, int maxWidthPx) {
    std::vector<std::string> lines;
    size_t paraStart = 0;

    while (true) {
        size_t nl = text.find('\n', paraStart);
        std::string paragraph =
            (nl == std::string::npos) ? text.substr(paraStart) : text.substr(paraStart, nl - paraStart);

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

}  // namespace

void NpcDialogue::Show(NpcDialogueState& state, const std::string& title, const std::string& body, int shopId) {
    state.active = true;
    state.title = title;
    state.body = body;
    state.shopId = shopId;
}

void NpcDialogue::Dismiss(NpcDialogueState& state) {
    state.active = false;
}

void NpcDialogue::Render(Backbuffer& bb, const NpcDialogueState& state) {
    bb.FillRect(0, 12, Backbuffer::kWidth, kBarTop - 12, kBodyBg);
    bb.FillRect(0, 0, Backbuffer::kWidth, 12, kTitleBg);
    int titleWidth = BitmapFont::StringWidth(state.title);
    // M74: y nudged from 3 to 1 -- see graphics/bitmap_font.cpp's own
    // kFontHeightPx comment for why the new GDI font needs the extra
    // headroom to stay inside this 12px bar.
    BitmapFont::DrawString(bb, (Backbuffer::kWidth - titleWidth) / 2, 1, state.title, kTitleFg);

    std::vector<std::string> lines = WordWrap(state.body, Backbuffer::kWidth - 2 * kMargin);
    int y = kContentTop;
    for (const std::string& line : lines) {
        if (y + kLineHeight > kBarTop) break;
        BitmapFont::DrawString(bb, kMargin, y, line, kBodyFg);
        y += kLineHeight;
    }

    bb.FillRect(0, kBarTop, Backbuffer::kWidth, Backbuffer::kHeight - kBarTop, kBarBg);
    BitmapFont::DrawString(bb, 4, kBarTop + 4, "Enter: Ok", kBarFg);
}

}  // namespace stormhold
