#include "ui/pause_menu.h"

#include <algorithm>

#include "assets/help_topics.h"
#include "combat/spell_casting.h"
#include "graphics/bitmap_font.h"
#include "player/game_save.h"
#include "player/player_combat_stats.h"
#include "player/player_leveling.h"

namespace stormhold {

namespace {

// Same small local palette/layout copy `ui/npc_dialogue.cpp`/
// `ui/menu_flow.cpp`/`ui/inventory_ui.cpp` each already keep -- see any
// one of those files' own class comments for why this port duplicates
// rather than shares them.
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

const std::vector<std::string> kOptionsItems = {"Stats",     "Inventory", "Skills", "Spells",
                                                 "Save Game", "Load Game", "Help",   "Quit Game"};

// ESGame.creditsText() verbatim, including its own real "Studos" typo --
// preserved exactly, not "corrected" (see this file's own header comment
// for why this is a DIFFERENT string than ui/menu_flow.cpp's own
// kCreditsText).
const std::string kPauseCreditsText =
    "Game Design: Anthony Gill and Greg Gorden\n"
    "Art: Mark Jones\n"
    "Programming: Marc Ilgen\n"
    "Technical Director: Andrew Friedman\n"
    "(C) 2003 Vir2L Studos, a ZeniMax Media company. The Elder Scrolls and Vir2L are "
    "registered trademarks of ZeniMax Media Inc. All rights reserved.\n";

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

void PaintPanel(Backbuffer& bb, const std::string& title) {
    bb.FillRect(0, 12, Backbuffer::kWidth, kBarTop - 12, kBodyBg);
    bb.FillRect(0, 0, Backbuffer::kWidth, 12, kTitleBg);
    int titleWidth = BitmapFont::StringWidth(title);
    BitmapFont::DrawString(bb, (Backbuffer::kWidth - titleWidth) / 2, 3, title, kTitleFg);
}

void PaintBottomBar(Backbuffer& bb, const std::string& leftLabel, const std::string& rightLabel) {
    bb.FillRect(0, kBarTop, Backbuffer::kWidth, Backbuffer::kHeight - kBarTop, kBarBg);
    if (!leftLabel.empty()) BitmapFont::DrawString(bb, 4, kBarTop + 4, leftLabel, kBarFg);
    if (!rightLabel.empty()) {
        int w = BitmapFont::StringWidth(rightLabel);
        BitmapFont::DrawString(bb, Backbuffer::kWidth - 4 - w, kBarTop + 4, rightLabel, kBarFg);
    }
}

void PaintList(Backbuffer& bb, const std::string& title, const std::vector<std::string>& header,
               const std::vector<std::string>& items, int selected) {
    PaintPanel(bb, title);
    int y = kContentTop;
    for (const std::string& line : header) {
        if (y + kLineHeight > kBarTop) break;
        BitmapFont::DrawString(bb, kMargin, y, line, kItemFg);
        y += kLineHeight;
    }
    if (!header.empty()) y += 2;

    for (size_t i = 0; i < items.size(); i++) {
        if (y + kLineHeight > kBarTop) break;
        if (static_cast<int>(i) == selected) {
            bb.FillRect(kMargin - 4, y - 1, Backbuffer::kWidth - 2 * (kMargin - 4), kLineHeight, kSelectedBg);
        }
        BitmapFont::DrawString(bb, kMargin, y, items[i], kItemFg);
        y += kLineHeight;
    }
}

void PaintMessage(Backbuffer& bb, const std::string& title, const std::string& body) {
    PaintList(bb, title, WordWrap(body, Backbuffer::kWidth - 2 * kMargin), {}, -1);
}

// Which screens are plain "read it, press Ok/Esc to go back" message
// screens (no navigable list of their own) -- MoveSelection no-ops on
// all of them, matching there being nothing to move a cursor over.
bool IsMessageScreen(PauseScreen s) {
    return s == PauseScreen::Stats || s == PauseScreen::SkillInfo || s == PauseScreen::NoSavedGame ||
           s == PauseScreen::SaveError || s == PauseScreen::Credits || s == PauseScreen::HelpTopic;
}

// Confirm()'s and Cancel()'s shared "go back" target for every screen
// that isn't Options itself (which Confirm dispatches per-item and
// Cancel closes back to gameplay) -- see ui/pause_menu.h's own class
// comment for the two port-only NoSavedGame/SaveError/Credits mappings.
void GoBack(PauseMenuState& state) {
    switch (state.screen) {
        case PauseScreen::SkillInfo:
            state.screen = PauseScreen::Skills;
            break;
        case PauseScreen::SpellInfo:
            state.screen = PauseScreen::Spells;
            break;
        // M69: NOT `PauseScreen::Help` -- `newHelpTopicUI(topicIndex).
        // nextScreen = this.statsUI` in the original, a real, confirmed
        // quirk (leaving a help topic body goes to Stats, not back to the
        // topic list), faithfully preserved rather than "corrected" to the
        // more intuitive Help -- see this file's own class comment.
        case PauseScreen::HelpTopic:
            state.screen = PauseScreen::Stats;
            break;
        default:
            state.screen = PauseScreen::Options;
            break;
    }
    state.selectedIndex = 0;
}

}  // namespace

void PauseMenu::Open(PauseMenuState& state) {
    state.active = true;
    state.screen = PauseScreen::Options;
    state.selectedIndex = 0;
    state.skillIndex = -1;
    state.spellIndex0Based = -1;
}

void PauseMenu::MoveSelection(PauseMenuState& state, int delta, const PlayerState& p, const SpellDatabase& spells) {
    if (!state.active) return;

    int count = 0;
    switch (state.screen) {
        case PauseScreen::Options:
            count = static_cast<int>(kOptionsItems.size());
            break;
        case PauseScreen::Skills:
            // A fresh CharacterData isn't threaded through here just to
            // count learned skills -- p.skills[i][0] > 0 is the whole
            // predicate PlayerLeveling::SkillSummaryList itself uses.
            for (int i = 0; i < 14; i++) {
                if (p.skills[static_cast<size_t>(i)][0] > 0) count++;
            }
            break;
        case PauseScreen::Spells:
            for (int i = 0; i < spells.Count(); i++) {
                if ((p.knownSpellsMask & (1u << i)) != 0) count++;
            }
            break;
        case PauseScreen::SpellInfo:
            count = 1;  // "Ready Spell", the screen's own sole action.
            break;
        case PauseScreen::Help:
            count = 12;  // Fixed topic count, not data-dependent.
            break;
        default:
            count = 0;  // Message-only screens: nothing to move over.
            break;
    }

    if (count <= 0) {
        state.selectedIndex = 0;
        return;
    }
    state.selectedIndex = std::max(0, std::min(state.selectedIndex + delta, count - 1));
}

PauseMenuAction PauseMenu::Confirm(PauseMenuState& state, PlayerState& p, const SpellDatabase& spells,
                                    InventoryUiState& inventoryUi, const std::string& savePath, size_t levelCount,
                                    WorldRegistry& world, ShopState& shop, WardenState& warden) {
    if (!state.active) return PauseMenuAction::None;

    if (IsMessageScreen(state.screen)) {
        GoBack(state);
        return PauseMenuAction::None;
    }

    if (state.screen == PauseScreen::Skills) {
        int skillIndex = PlayerLeveling::NthLearnedSkillIndex(p, state.selectedIndex);
        if (skillIndex < 0) return PauseMenuAction::None;
        state.skillIndex = skillIndex;
        state.screen = PauseScreen::SkillInfo;
        state.selectedIndex = 0;
        return PauseMenuAction::None;
    }

    if (state.screen == PauseScreen::Spells) {
        int spellIndex0Based = SpellCasting::NthKnownSpellId(p, spells, state.selectedIndex);
        if (spellIndex0Based < 0) return PauseMenuAction::None;
        state.spellIndex0Based = spellIndex0Based;
        state.screen = PauseScreen::SpellInfo;
        state.selectedIndex = 0;
        return PauseMenuAction::None;
    }

    if (state.screen == PauseScreen::Help) {
        state.helpTopicIndex = state.selectedIndex;
        state.screen = PauseScreen::HelpTopic;
        state.selectedIndex = 0;
        return PauseMenuAction::None;
    }

    if (state.screen == PauseScreen::SpellInfo) {
        // ESGame's own screenGroup 38: player.selectedSpellId = spellId+1,
        // spellId being nthKnownSpellId's own 0-based return -- see
        // SpellCasting::NthKnownSpellId's own doc comment.
        p.selectedSpellId = static_cast<int8_t>(state.spellIndex0Based + 1);
        state.screen = PauseScreen::Spells;
        state.selectedIndex = 0;
        return PauseMenuAction::None;
    }

    // Options: dispatch by row, matching screenGroup 31's own switch.
    switch (state.selectedIndex) {
        case 0:  // Stats
            state.screen = PauseScreen::Stats;
            state.selectedIndex = 0;
            return PauseMenuAction::None;
        case 1:  // Inventory -- hands off to the already-live M62 screen.
            state.active = false;
            InventoryUi::Open(inventoryUi);
            return PauseMenuAction::None;
        case 2:  // Skills
            state.screen = PauseScreen::Skills;
            state.selectedIndex = 0;
            return PauseMenuAction::None;
        case 3:  // Spells
            state.screen = PauseScreen::Spells;
            state.selectedIndex = 0;
            return PauseMenuAction::None;
        case 4:  // Save Game
            if (GameSave::Save(savePath, p, world, shop, warden)) {
                state.active = false;
            } else {
                state.screen = PauseScreen::SaveError;
            }
            return PauseMenuAction::None;
        case 5:  // Load Game
            if (GameSave::Exists(savePath) &&
                GameSave::Load(savePath, levelCount, p, world, shop, warden)) {
                state.active = false;
                return PauseMenuAction::GameLoaded;
            }
            state.screen = PauseScreen::NoSavedGame;
            return PauseMenuAction::None;
        case 6:  // Help -- wired M69, see this file's own header comment.
            state.screen = PauseScreen::Help;
            state.selectedIndex = 0;
            return PauseMenuAction::None;
        case 7:  // "Quit Game" -- a confirmed real bug, faithfully
                 // reproduced: this shows the credits screen, not a real
                 // quit (see this file's own header comment).
            state.screen = PauseScreen::Credits;
            state.selectedIndex = 0;
            return PauseMenuAction::None;
        default:
            return PauseMenuAction::None;
    }
}

void PauseMenu::Cancel(PauseMenuState& state) {
    if (!state.active) return;
    if (state.screen == PauseScreen::Options) {
        state.active = false;
        return;
    }
    GoBack(state);
}

void PauseMenu::Render(Backbuffer& bb, const PauseMenuState& state, const PlayerState& p, const CharacterData& charData,
                        const SpellDatabase& spells, const ShopDialogue& dialogue) {
    switch (state.screen) {
        case PauseScreen::Options:
            PaintList(bb, "Options", {}, kOptionsItems, state.selectedIndex);
            PaintBottomBar(bb, "Enter: Ok", "Esc: Back");
            return;
        case PauseScreen::Stats:
            PaintMessage(bb, "Stats", PlayerCombatStats::CharacterSheetText(p, charData));
            PaintBottomBar(bb, "Enter: Ok", "");
            return;
        case PauseScreen::Skills:
            PaintList(bb, "Skills", {"Your Skills:"}, PlayerLeveling::SkillSummaryList(p, charData),
                      state.selectedIndex);
            PaintBottomBar(bb, "Enter: Ok", "Esc: Back");
            return;
        case PauseScreen::SkillInfo:
            PaintMessage(bb, "Skill Info",
                         state.skillIndex >= 0 ? PlayerLeveling::SkillTooltip(p, charData, state.skillIndex) : "");
            PaintBottomBar(bb, "Enter: Ok", "");
            return;
        case PauseScreen::Spells:
            PaintList(bb, "Spells", {"Your Spells:"}, SpellCasting::KnownSpellsSummary(p, spells),
                      state.selectedIndex);
            PaintBottomBar(bb, "Enter: Ok", "Esc: Back");
            return;
        case PauseScreen::SpellInfo: {
            std::string tooltip = state.spellIndex0Based >= 0
                                       ? SpellCasting::SpellTooltip(spells, charData, state.spellIndex0Based)
                                       : "";
            std::vector<std::string> header = WordWrap(tooltip, Backbuffer::kWidth - 2 * kMargin);
            PaintList(bb, "Spell Info", header, {"Ready Spell"}, state.selectedIndex);
            PaintBottomBar(bb, "Enter: Ok", "Esc: Back");
            return;
        }
        case PauseScreen::NoSavedGame:
            PaintMessage(bb, "Unavailable", "No game is available for loading. Press OK to return to the menu.");
            PaintBottomBar(bb, "Enter: Ok", "");
            return;
        case PauseScreen::SaveError:
            PaintMessage(bb, "Save Error",
                         "There was an error in saving your character record. Your previous character record is "
                         "still saved. Try turning your phone off then on again to clear the memory.");
            PaintBottomBar(bb, "Enter: Ok", "");
            return;
        case PauseScreen::Credits:
            PaintMessage(bb, "Credits", kPauseCreditsText);
            PaintBottomBar(bb, "Enter: Ok", "Esc: Back");
            return;
        case PauseScreen::Help:
            PaintList(bb, "Help", {}, HelpTopics::Titles(dialogue), state.selectedIndex);
            PaintBottomBar(bb, "Enter: Ok", "Esc: Back");
            return;
        case PauseScreen::HelpTopic:
            // M69: matches screenGroup 206's own dispatch -- any command
            // (not just Ok) leaves to Stats (GoBack's own HelpTopic case),
            // so the bottom bar only ever advertises one action, same as
            // Stats/SkillInfo/SaveError above.
            PaintMessage(bb, state.helpTopicIndex >= 0 ? HelpTopics::Title(dialogue, state.helpTopicIndex) : "Help",
                         state.helpTopicIndex >= 0 ? HelpTopics::Body(dialogue, state.helpTopicIndex) : "");
            PaintBottomBar(bb, "Enter: Ok", "");
            return;
    }
}

}  // namespace stormhold
