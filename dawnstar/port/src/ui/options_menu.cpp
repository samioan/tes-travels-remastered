#include "ui/options_menu.h"

#include <utility>

#include "player/player_combat_stats.h"

namespace dawnstar {

namespace {

// Player.java's own AILMENT_NAMES -- matches PlayerState::ailmentMask's
// own bit numbering (see player/player_combat_stats.h's HasAilment).
const char* const kAilmentNames[8] = {
    "Frost Limbs", "Snow Mirage",   "Blind",        "Troll Thirst",
    "Glacier Curse", "Grievous Harm", "Terrified", "Winter Worn",
};

// Player.java's own buildCharacterSheet(), transcribed line-for-line
// (including every literal "  " blank-ish spacer line and the exact
// placement of each '\n').
std::string BuildCharacterSheet(const PlayerState& p, const CharacterData& charData) {
    std::string out;
    out += p.name;
    out += '\n';
    out += charData.classNames[static_cast<size_t>(p.classIndex)];
    out += '\n';
    out += "Level " + std::to_string(p.coreStats[0]) + " (" + std::to_string(p.coreStats[1]) + "/10)\n";
    out += "Health: " + std::to_string(PlayerCombatStats::EffectiveStat(p, charData, 2)) + "/" +
           std::to_string(p.coreStats[3]) + "\n";
    out += "Magicka: " + std::to_string(PlayerCombatStats::EffectiveStat(p, charData, 4)) + "/" +
           std::to_string(p.coreStats[5]) + "\n";
    out += "Fatigue: " + std::to_string(PlayerCombatStats::EffectiveStat(p, charData, 6)) + "/" +
           std::to_string(p.coreStats[7]) + "\n";
    out += "  \n";
    out += "Status ailments: ";
    int count = 0;
    for (int i = 1; i <= 8; i++) {
        if (PlayerCombatStats::HasAilment(p, i)) {
            out += "\n";
            out += kAilmentNames[i - 1];
            count++;
        }
    }
    if (count == 0) out += "\nNone";
    out += "\n  \n";
    out += "Gift points found: " + std::to_string(p.giftPointsFound) + "\n  \nAttributes:\n";
    for (int i = 0; i < 8; i++) {
        int slot = 2 * i;
        out += charData.attributeNames[static_cast<size_t>(slot)];
        out += ": ";
        out += std::to_string(p.attributes[static_cast<size_t>(slot)]);
        out += "\n";
    }
    return out;
}

// Shop.java's own RUMOR_STRING_OFFSET/UNCONFIRMED_A/UNCONFIRMED_B --
// [traitorIndex][revealStep] offsets into ShopDialogue::groups[9] for
// the "Rumors" entry, and the two parallel "still unconfirmed" /
// "the traitor's own admission" offset tables newClueLogUI() picks
// between per flag, for the 4 named-suspect entries.
constexpr int8_t kRumorStringOffset[4][6] = {
    {1, 3, 5, 8, 10, 12}, {1, 2, 4, 7, 9, 12}, {2, 3, 6, 7, 10, 11}, {2, 4, 5, 8, 9, 11},
};
constexpr int8_t kUnconfirmedA[24] = {13, 19, 25, 31, 14, 20, 26, 32, 15, 21, 27, 33,
                                       17, 23, 29, 35, 16, 22, 28, 34, 18, 24, 30, 36};
constexpr int8_t kUnconfirmedB[24] = {37, 43, 49, 55, 38, 44, 50, 56, 39, 45, 51, 57,
                                       41, 47, 53, 59, 40, 46, 52, 58, 42, 48, 54, 60};

// ESGame.java's own newClueLogUI(idx), transcribed exactly -- idx==4 is
// the "Rumors" entry (accumulated from ANY suspect's own revealed rumor
// steps, `Player.eventFlags[90..95]`); idx 0-3 is one of the 4 named
// suspects, each tracking 6 "topics" (rows) x 3 "questions asked"
// (columns) worth of `eventFlags[18*idx .. 18*idx+17]` flags. The real,
// easy-to-miss subtlety: `bump` (Java's own `var6`) is reset to 0 only
// once per ROW, then STAYS whatever it was last set to (1, once a
// flag at column >= idx has been seen) for the rest of that row's own
// columns -- not reset per column. And when idx happens to be the
// PLAYER'S OWN real traitorIndex, a second, separate eventFlags range
// (`72 + row*3 + col`) picks the "the traitor's own admission" text
// (UNCONFIRMED_B) over the plain "still unconfirmed" text (UNCONFIRMED_A)
// for that same slot. Ported exactly, not simplified.
std::string BuildClueEntry(const PlayerState& player, const ShopDialogue& dialogue, int idx) {
    std::string body;
    if (idx == 4) {
        for (int i = 0; i < 6; i++) {
            if (player.eventFlags[static_cast<size_t>(90 + i)]) {
                body += dialogue.groups[9][static_cast<size_t>(5 + kRumorStringOffset[player.traitorIndex][i])];
                body += "\n";
            }
        }
    } else {
        int base = 18 * idx;
        bool isTraitor = idx == player.traitorIndex;
        for (int row = 0; row < 6; row++) {
            int bump = 0;
            for (int col = 0; col < 3; col++) {
                if (player.eventFlags[static_cast<size_t>(base + row * 3 + col)]) {
                    if (col >= idx) bump = 1;
                    int tableIdx = row * 4 + col + bump;
                    if (isTraitor && player.eventFlags[static_cast<size_t>(72 + row * 3 + col)]) {
                        body += dialogue.groups[9][static_cast<size_t>(5 + kUnconfirmedB[tableIdx])];
                    } else {
                        body += dialogue.groups[9][static_cast<size_t>(5 + kUnconfirmedA[tableIdx])];
                    }
                    body += "\n";
                }
            }
        }
    }
    if (body.empty()) body = "You have no information yet.";
    return body;
}

}  // namespace

OptionsMenu::OptionsMenu(HelpText helpText, ShopDialogue shopDialogue)
    : helpText_(std::move(helpText)),
      shopDialogue_(std::move(shopDialogue)),
      options_(ScreenMode::HighlightedList),
      clueLog_(ScreenMode::PromptList),
      helpTopics_(ScreenMode::HighlightedList),
      info_(ScreenMode::PlainList),
      quitConfirm_(ScreenMode::PromptList) {
    // ESGame.allocateAllUIs()'s own OptionsUI construction: `setupList(
    // "Options", ..., false)` (not cancelable -- Select only) plus its
    // own separately-added `backCommand`.
    options_.SetupList(
        "Options",
        {"Stats", "Inventory", "Clue Log", "Skills", "Spells", "Save Game", "Load Game", "Help", "Reveal Traitor",
         "Quit Game"},
        false);
    options_.AddCommand(CommandId::Back);
    // `this.ClueUI.setupPromptList("Clue Log", "", var12)` -- empty
    // prompt text, 4 named suspects plus "Rumors".
    clueLog_.SetupPromptList("Clue Log", "", {"Alhavara", "Beatrice", "Chung", "Delacroix", "Rumors"});
    // `this.helpUI.setupList("Help", helpTitles, true)` -- see this
    // class's own header doc comment on why this is a separate `Screen`
    // instance from M38's own `MenuFlow::helpTopics_`.
    helpTopics_.SetupList("Help", helpText_.titles, true);
    // `newConfirmQuitUI()`: see this class's own header doc comment (and
    // M38's `MenuFlow`, which established this exact same real bug).
    quitConfirm_.SetupPromptList("Quit?", "Are you sure?", {"Yes", "No"});
    quitConfirm_.RemoveCommand(CommandId::Cancel);
}

Screen& OptionsMenu::ActiveScreen() {
    switch (active_) {
        case Active::Options:
            return options_;
        case Active::ClueLog:
            return clueLog_;
        case Active::Help:
            return helpTopics_;
        case Active::Info:
            return info_;
        case Active::QuitConfirm:
            return quitConfirm_;
    }
    return options_;
}

const Screen& OptionsMenu::ActiveScreen() const { return const_cast<OptionsMenu*>(this)->ActiveScreen(); }

void OptionsMenu::OnUp() { ActiveScreen().MoveSelectionUp(); }

void OptionsMenu::OnDown() { ActiveScreen().MoveSelectionDown(); }

OptionsMenuAction OptionsMenu::OnSelect(const PlayerState& player, const CharacterData& charData) {
    switch (active_) {
        case Active::Options: {
            // secondaryParam==31's own Select branch.
            switch (options_.SelectedIndexOrMinusOne()) {
                case 0:  // "Stats": secondaryParam 32.
                    info_.SetupMessage("Stats", BuildCharacterSheet(player, charData));
                    infoBackTarget_ = Active::Options;
                    active_ = Active::Info;
                    return OptionsMenuAction::None;
                case 1:  // "Inventory" -- deferred no-op, see this
                         // class's own header doc comment.
                    return OptionsMenuAction::None;
                case 2:  // "Clue Log": `this.setCurrentDisplay(this.ClueUI);`
                    active_ = Active::ClueLog;
                    return OptionsMenuAction::None;
                case 3:  // "Skills" -- deferred no-op.
                    return OptionsMenuAction::None;
                case 4:  // "Spells" -- deferred no-op.
                    return OptionsMenuAction::None;
                case 5:  // "Save Game" -- deferred no-op.
                    return OptionsMenuAction::None;
                case 6:  // "Load Game" -- deferred no-op.
                    return OptionsMenuAction::None;
                case 7:  // "Help": `this.helpUI.backTarget = this.
                         // OptionsUI; this.setCurrentDisplay(this.helpUI);`
                    active_ = Active::Help;
                    return OptionsMenuAction::None;
                case 8:  // "Reveal Traitor" -- deferred no-op.
                    return OptionsMenuAction::None;
                case 9:  // "Quit Game": `this.confirmQuitUI = this.
                         // newConfirmQuitUI(uic); this.setCurrentDisplay(
                         // this.confirmQuitUI);` -- shows the (buggy)
                         // confirmation, doesn't exit directly.
                    active_ = Active::QuitConfirm;
                    return OptionsMenuAction::None;
                default:
                    return OptionsMenuAction::None;
            }
        }
        case Active::ClueLog: {
            // secondaryParam==60's own Select branch: `newClueLogUI(
            // selectedIndexOrMinusOne()); setCurrentDisplay(GenericInfoUI);`
            // -- no range check in the original either (see this file's
            // own BuildClueEntry doc comment); clueLog_ always has
            // exactly 5 real items, so this is never actually -1 or >=5
            // in practice.
            int idx = clueLog_.SelectedIndexOrMinusOne();
            if (idx >= 0 && idx < 5) {
                info_.SetupMessage(clueLog_.Items()[static_cast<size_t>(idx)],
                                    BuildClueEntry(player, shopDialogue_, idx));
                infoBackTarget_ = Active::ClueLog;
                active_ = Active::Info;
            }
            return OptionsMenuAction::None;
        }
        case Active::Help: {
            // secondaryParam==203's own Select branch, same as
            // MenuFlow's own HelpTopics case.
            int idx = helpTopics_.SelectedIndexOrMinusOne();
            if (idx >= 0 && idx < static_cast<int>(helpText_.titles.size())) {
                info_.SetupMessage(helpText_.titles[static_cast<size_t>(idx)],
                                    helpText_.bodies[static_cast<size_t>(idx)]);
                infoBackTarget_ = Active::Help;
                active_ = Active::Info;
            }
            return OptionsMenuAction::None;
        }
        case Active::Info:
            // secondaryParam==32 (Stats)/61 (Clue Log)/206 (Help) all
            // return to a hardcoded target on Ok -- see infoBackTarget_'s
            // own doc comment.
            active_ = infoBackTarget_;
            return OptionsMenuAction::None;
        case Active::QuitConfirm:
            // secondaryParam==202: unconditional `this.exit()` -- the
            // real, preserved bug (see this class's own header doc
            // comment and M38's MenuFlow, which established it first).
            return OptionsMenuAction::Exit;
    }
    return OptionsMenuAction::None;
}

OptionsMenuAction OptionsMenu::OnCancel() {
    switch (active_) {
        case Active::Options:
            // `else if (var1 == backCommand) { this.setCurrentDisplay(
            // this.gameCanvas); }` -- the one real way out of this whole
            // menu back to actually playing.
            return OptionsMenuAction::ReturnToGame;
        case Active::ClueLog:
            // The top-level `if (var1 == cancelCommand && uic.backTarget
            // != null)` check: `this.ClueUI.backTarget = this.OptionsUI;`
            active_ = Active::Options;
            return OptionsMenuAction::None;
        case Active::Help:
            // `this.helpUI.backTarget = this.OptionsUI` (set right
            // before display, from this entry point only).
            active_ = Active::Options;
            return OptionsMenuAction::None;
        case Active::Info:
            // Stats/a Clue Log entry/a Help topic's own body only ever
            // have an Ok command, no Cancel -- a real no-op.
            return OptionsMenuAction::None;
        case Active::QuitConfirm:
            // `newConfirmQuitUI()` explicitly removes its own Cancel
            // command -- same real "no way to back out" quirk M38's
            // MenuFlow already established.
            return OptionsMenuAction::None;
    }
    return OptionsMenuAction::None;
}

void OptionsMenu::Render(Backbuffer& bb) const { ActiveScreen().Paint(bb); }

}  // namespace dawnstar
