#include "ui/level_up_menu.h"

#include "graphics/bitmap_font.h"
#include "player/player_leveling.h"

namespace stormhold {

namespace {

// Same small local palette/layout copy ui/pause_menu.cpp and its
// siblings each keep -- see any of their own comments for why.
constexpr uint16_t kTitleBg = PackRGB565(0, 0, 0);
constexpr uint16_t kTitleFg = PackRGB565(255, 255, 255);
constexpr uint16_t kBodyBg = PackRGB565(0xAE, 0x68, 0x2E);
constexpr uint16_t kItemFg = PackRGB565(255, 255, 0);
constexpr uint16_t kSelectedBg = PackRGB565(0x66, 0x66, 0x66);
constexpr uint16_t kBarBg = PackRGB565(255, 255, 255);
constexpr uint16_t kBarFg = PackRGB565(0, 0, 0);

constexpr int kLineHeight = BitmapFont::kGlyphHeight + 2;
constexpr int kContentTop = 16;
constexpr int kBarTop = Backbuffer::kHeight - 14;
constexpr int kMargin = 8;

// newLevelUpUI(step)'s own prompts, verbatim (its "\n" split into lines).
const std::array<std::array<const char*, 2>, 3> kPrompts = {{
    {"Select an attribute to", "increase 3 points:"},
    {"Select an attribute to", "increase 2 points:"},
    {"Select an attribute to", "increase 1 point:"},
}};

}  // namespace

void LevelUpMenu::Open(LevelUpMenuState& state, const PlayerState& p, const CharacterData& charData) {
    state = LevelUpMenuState{};
    state.active = true;
    state.names = PlayerLeveling::PendingLevelUpAttributeNames(p, charData);
}

void LevelUpMenu::MoveSelection(LevelUpMenuState& state, int delta) {
    int count = static_cast<int>(state.names.size());
    if (count == 0) return;
    state.selectedIndex = (state.selectedIndex + delta + count) % count;
}

bool LevelUpMenu::Confirm(LevelUpMenuState& state, PlayerState& p, const CharacterData& charData, ShopState& shop) {
    if (state.names.empty()) return false;

    const std::string& chosen = state.names[static_cast<size_t>(state.selectedIndex)];
    int attributeIndex = -1;
    for (size_t i = 0; i < charData.attributeNames.size(); i++) {
        if (charData.attributeNames[i] == chosen) {
            attributeIndex = static_cast<int>(i);
            break;
        }
    }
    state.choices[static_cast<size_t>(state.step)] = attributeIndex;

    if (state.step < 2) {
        state.step++;
        state.selectedIndex = 0;
        return false;
    }

    PlayerLeveling::ApplyLevelUpAttributeChoices(p, state.choices[0], state.choices[1], state.choices[2], shop);
    state.active = false;
    return true;
}

void LevelUpMenu::Render(Backbuffer& bb, const LevelUpMenuState& state) {
    bb.FillRect(0, 12, Backbuffer::kWidth, kBarTop - 12, kBodyBg);
    bb.FillRect(0, 0, Backbuffer::kWidth, 12, kTitleBg);
    const std::string title = "Level Up";
    BitmapFont::DrawString(bb, (Backbuffer::kWidth - BitmapFont::StringWidth(title)) / 2, 1, title, kTitleFg);

    int y = kContentTop;
    for (const char* line : kPrompts[static_cast<size_t>(state.step)]) {
        BitmapFont::DrawString(bb, kMargin, y, line, kItemFg);
        y += kLineHeight;
    }
    y += 2;

    for (size_t i = 0; i < state.names.size(); i++) {
        if (y + kLineHeight > kBarTop) break;
        if (static_cast<int>(i) == state.selectedIndex) {
            bb.FillRect(kMargin - 4, y - 1, Backbuffer::kWidth - 2 * (kMargin - 4), kLineHeight, kSelectedBg);
        }
        BitmapFont::DrawString(bb, kMargin, y, state.names[i], kItemFg);
        y += kLineHeight;
    }

    bb.FillRect(0, kBarTop, Backbuffer::kWidth, Backbuffer::kHeight - kBarTop, kBarBg);
    BitmapFont::DrawString(bb, 4, kBarTop + 4, "Enter: Ok", kBarFg);
}

}  // namespace stormhold
