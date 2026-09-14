#include "ui/options_menu.h"

#include <cstdlib>
#include <utility>

#include "player/player_combat_stats.h"
#include "player/player_inventory.h"
#include "player/player_movement.h"
#include "player/player_spellcasting.h"

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
// Shop.java's own NAMES[5..8] -- the 4 named-suspect NPC identities the
// whole "who is the traitor?" subplot runs over (the Clue Log's own item
// list and newRevealWhomUI() below are the same 4 names in the original
// too; NAMES[0..4], the hub-town peddler identities, have no
// reveal-traitor role). ESGame.java builds both arrays inline as the
// same literals; kept here as one shared table rather than two duplicated
// literal lists, the same reuse-over-duplication call M16/M17 already
// made for the inventory/starting-spell helpers.
const char* const kSuspectNames[4] = {"Alhavara", "Beatrice", "Chung", "Delacroix"};

// Util.java's own replace(source, tag, value) (M43, its first real ported
// call site): replaces only the FIRST occurrence of `tag` -- Util.java's
// own doc comment notes that callers substituting several distinct
// placeholders must call it once per placeholder in order, relying on
// that. Ported as a small file-local helper rather than a new util module
// because nothing else in this port has needed it yet; the two other
// original call sites that substitute a suspect name (newGameOverUI's
// dialogue[9][73] text, secondaryParam==201, and the Clue Log's own
// dialogue[9][5+RUMOR_STRING_OFFSET...] lines) read their <TAG>-bearing
// text straight from npcstrings.dat with the placeholder already
// substituted in the data itself.
std::string ReplaceFirstTag(const std::string& source, const std::string& tag, const std::string& value) {
    size_t at = source.find(tag);
    if (at == std::string::npos) return source;
    return source.substr(0, at) + value + source.substr(at + tag.size());
}


}  // namespace

OptionsMenu::OptionsMenu(HelpText helpText, ShopDialogue shopDialogue)
    : helpText_(std::move(helpText)),
      shopDialogue_(std::move(shopDialogue)),
      options_(ScreenMode::HighlightedList),
      clueLog_(ScreenMode::PromptList),
      helpTopics_(ScreenMode::HighlightedList),
      info_(ScreenMode::PlainList),
      quitConfirm_(ScreenMode::PromptList),
      // M42: ESGame's own `noSavedGameUI = new Screen(this, 4, 305)` --
      // mode 4 (PlainList), built once, message set in the body below.
      noSavedGame_(ScreenMode::PlainList),
      // M41: InventoryUI/SkillsListUI/SpellsListUI/InventoryItemUI/
      // SpellInfoUI are all mode 5 (PromptList) in the original -- these
      // placeholder constructions are never actually shown; each is
      // fully replaced by its own Rebuild* on first real (re)entry (see
      // this class's own header doc comment on why that's a full
      // replacement, not just a reconfiguration).
      inventoryList_(ScreenMode::PromptList),
      inventoryItem_(ScreenMode::PromptList),
      skillsList_(ScreenMode::PromptList),
      spellsList_(ScreenMode::PromptList),
      spellInfo_(ScreenMode::PromptList),
      // M43: placeholder constructions, never actually shown in this
      // state -- each is fully replaced by fresh construction inside
      // OnSelect's RevealIntro/RevealConfirm branches below (ESGame's own
      // newRevealUI()/newRevealWhomUI() build a brand-new Screen every
      // time), same reasoning as inventoryList_ above.
      revealConfirm_(ScreenMode::PromptList),
      revealWhom_(ScreenMode::PromptList) {
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
    clueLog_.SetupPromptList("Clue Log", "",
                             {kSuspectNames[0], kSuspectNames[1], kSuspectNames[2], kSuspectNames[3], "Rumors"});
    // `this.helpUI.setupList("Help", helpTitles, true)` -- see this
    // class's own header doc comment on why this is a separate `Screen`
    // instance from M38's own `MenuFlow::helpTopics_`.
    helpTopics_.SetupList("Help", helpText_.titles, true);
    // `newConfirmQuitUI()`: see this class's own header doc comment (and
    // M38's `MenuFlow`, which established this exact same real bug).
    quitConfirm_.SetupPromptList("Quit?", "Are you sure?", {"Yes", "No"});
    quitConfirm_.RemoveCommand(CommandId::Cancel);
    // ESGame.allocateAllUIs()'s own `noSavedGameUI.setupMessage(...)` --
    // see ShowNoSavedGame's own doc comment on why its Ok actually returns
    // to the Options menu rather than to the main menu its own text claims.
    noSavedGame_.SetupMessage("Unavailable", "No game is available for loading. Press OK to return to main menu.");
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
        case Active::InventoryList:
            return inventoryList_;
        case Active::InventoryItem:
            return inventoryItem_;
        case Active::SkillsList:
            return skillsList_;
        case Active::SpellsList:
            return spellsList_;
        case Active::SpellInfo:
            return spellInfo_;
        case Active::SaveError:
            // The SAME shared info_ Screen object -- ESGame has exactly one
            // GenericInfoUI, reused for the Save Error message too (see
            // ShowSaveError's own doc comment); only its secondaryParam, and
            // therefore its Ok dispatch, differs.
            return info_;
        case Active::NoSavedGame:
            return noSavedGame_;
        case Active::RevealIntro:
        case Active::RevealResult:
            // The same shared info_ Screen again -- ESGame reuses its one
            // GenericInfoUI for secondaryParam 68 (the quiz intro) and 67
            // (the guess result) too, so both render info_; only their
            // secondaryParams, and therefore their Ok dispatches, differ
            // (exactly Active::SaveError's own reasoning above).
            return info_;
        case Active::RevealConfirm:
            return revealConfirm_;
        case Active::RevealWhom:
            return revealWhom_;
    }
    return options_;
}

const Screen& OptionsMenu::ActiveScreen() const { return const_cast<OptionsMenu*>(this)->ActiveScreen(); }

void OptionsMenu::OnUp() { ActiveScreen().MoveSelectionUp(); }

void OptionsMenu::OnDown() { ActiveScreen().MoveSelectionDown(); }

// ESGame.java's own newInventoryUI(): item names ("E: <name>" for an
// equipped slot -- inventoryItemIds' own sign encodes that, matching
// PlayerInventory::IsEquipped's convention) and the "Your gold: <TAG>"
// prompt. Always a FULL replacement of inventoryList_ (see this class's
// own header doc comment on why), so a stale selectedIndex_ never
// leaks in from a previous, unrelated visit the way it deliberately does
// for options_/clueLog_/helpTopics_.
void OptionsMenu::RebuildInventoryList(const PlayerState& player, const ItemDatabase& items) {
    std::vector<std::string> names;
    for (int i = 0; i < player.inventoryCount; i++) {
        int id = static_cast<int>(player.inventoryItemIds[static_cast<size_t>(i)]);
        std::string name = items.name[static_cast<size_t>(std::abs(id) - 1)];
        names.push_back(id < 0 ? "E: " + name : name);
    }
    inventoryList_ = Screen(ScreenMode::PromptList);
    inventoryList_.SetupPromptList("Inventory", "Your gold: " + std::to_string(player.gold), names);
}

// ESGame.java's own newInventoryItemUI(slot): the item's own tooltip
// (PlayerInventory::ItemTooltip) plus a Drop/[Equip-or-Unequip]/[Learn]/
// [Use] action list, built in the EXACT SAME conditional order (Drop
// always first, then equip/unequip iff CanEquipOrUnequip, then Learn iff
// CanLearnSpell, then Use iff CanUseItem) that OnSelect's own
// Active::InventoryItem branch below decrements a selected index against
// -- the two must stay in lockstep the same way the original's own
// construction and dispatch code do (see that branch's own doc comment).
void OptionsMenu::RebuildInventoryItem(const PlayerState& player, const CharacterData& charData,
                                       const ItemDatabase& items, const SpellDatabase& spells, int slot) {
    std::string tooltip = PlayerInventory::ItemTooltip(player, charData, items, spells, slot);
    std::vector<std::string> actions;
    actions.push_back("Drop");
    if (PlayerInventory::CanEquipOrUnequip(player, items, slot)) {
        actions.push_back(PlayerInventory::IsEquipped(player, items, slot) ? "Unequip" : "Equip");
    }
    if (PlayerSpellcasting::CanLearnSpell(player, items, spells, slot)) actions.push_back("Learn");
    if (PlayerInventory::CanUseItem(player, items, slot)) actions.push_back("Use");

    inventoryItem_ = Screen(ScreenMode::PromptList);
    inventoryItem_.SetupPromptList("Item", tooltip, actions);
}

// ESGame.java's own newSkillsListUI(): PlayerCombatStats::KnownSkillsSummary
// ("Skill: rank" for every rank>0 skill).
void OptionsMenu::RebuildSkillsList(const PlayerState& player, const CharacterData& charData) {
    skillsList_ = Screen(ScreenMode::PromptList);
    skillsList_.SetupPromptList("Skills", "Your Skills:", PlayerCombatStats::KnownSkillsSummary(player, charData));
}

// ESGame.java's own newSpellsListUI(): PlayerSpellcasting::KnownSpellsSummary
// (every known spell's name, "R: "-prefixed for the currently-selected
// one).
void OptionsMenu::RebuildSpellsList(const PlayerState& player, const SpellDatabase& spells) {
    spellsList_ = Screen(ScreenMode::PromptList);
    spellsList_.SetupPromptList("Spells", "Your Spells:", PlayerSpellcasting::KnownSpellsSummary(player, spells));
}

// ESGame.java's own secondaryParam==34 dispatch tail, shared by every
// real way an inventory-item action can finish (Drop/Equip/Unequip/Learn
// resolve synchronously inside OnSelect below; Use finishes here too, but
// only via FinishUseItem, after main.cpp has performed the real
// CombatResolution::UseItem call -- see this class's own header doc
// comment on why "Use" alone needs that extra round-trip). A real,
// easy-to-miss quirk preserved exactly: using item 87 ("Warp to Camp")
// sets `suppressStrafeAdjust`, which routes back to the GAME VIEW
// directly here, not back to the Inventory list at all.
OptionsMenuAction OptionsMenu::FinishInventoryItemAction(PlayerState& player, const ItemDatabase& items) {
    if (player.suppressStrafeAdjust) {
        player.suppressStrafeAdjust = false;
        currentItemIndex_ = -1;
        active_ = Active::Options;
        return OptionsMenuAction::ReturnToGame;
    }
    RebuildInventoryList(player, items);
    inventoryList_.SetSelectedIndex(currentItemIndex_);
    active_ = Active::InventoryList;
    currentItemIndex_ = -1;
    return OptionsMenuAction::None;
}

OptionsMenuAction OptionsMenu::FinishUseItem(PlayerState& player, const ItemDatabase& items) {
    return FinishInventoryItemAction(player, items);
}

OptionsMenuAction OptionsMenu::OnSelect(PlayerState& player, const CharacterData& charData, const ItemDatabase& items,
                                        const SpellDatabase& spells, std::vector<GeneratedLevel>& levels,
                                        WorldRegistry& world, int16_t& nextItemSpawnId) {
    switch (active_) {
        case Active::Options: {
            // secondaryParam==31's own Select branch.
            switch (options_.SelectedIndexOrMinusOne()) {
                case 0:  // "Stats": secondaryParam 32.
                    info_.SetupMessage("Stats", BuildCharacterSheet(player, charData));
                    infoBackTarget_ = Active::Options;
                    active_ = Active::Info;
                    return OptionsMenuAction::None;
                case 1:  // "Inventory": `this.InventoryUI = this.
                         // newInventoryUI(); this.setCurrentDisplay(
                         // this.InventoryUI);`
                    RebuildInventoryList(player, items);
                    active_ = Active::InventoryList;
                    return OptionsMenuAction::None;
                case 2:  // "Clue Log": `this.setCurrentDisplay(this.ClueUI);`
                    active_ = Active::ClueLog;
                    return OptionsMenuAction::None;
                case 3:  // "Skills": `this.SkillsListUI = this.
                         // newSkillsListUI(); this.setCurrentDisplay(
                         // this.SkillsListUI);`
                    RebuildSkillsList(player, charData);
                    active_ = Active::SkillsList;
                    return OptionsMenuAction::None;
                case 4:  // "Spells": `this.SpellsListUI = this.
                         // newSpellsListUI(); this.setCurrentDisplay(
                         // this.SpellsListUI);`
                    RebuildSpellsList(player, spells);
                    active_ = Active::SpellsList;
                    return OptionsMenuAction::None;
                case 5:  // "Save Game": secondaryParam==31's own case 5 --
                         // `saveGameUI = new LoadingScreen(this, 10, 303);
                         // saveGameUI.unusedHook2(); helperThreadState = 5;
                         // setCurrentDisplay(saveGameUI); var54.start();`
                         // All ESGame-level display/thread work, and the
                         // real save itself runs on that new thread -- see
                         // OptionsMenuAction's own doc comment on why this
                         // class only reports it. OptionsUI's own state is
                         // untouched (the original never reassigns it here
                         // either), so a failed save's Ok-exit or a
                         // successful save's return to the game view both
                         // leave this menu exactly as it was.
                    return OptionsMenuAction::SaveGame;
                case 6:  // "Load Game": case 6 -- `System.gc();
                         // gameCanvas.stopGameThread(); loadGameUI = new
                         // LoadingScreen(this, 9, 302);
                         // loadGameUI.unusedHook2(); helperThreadState = 6;
                         // noSavedGameUI.backTarget = this.OptionsUI;
                         // setCurrentDisplay(loadGameUI); var66.start();`
                         // Same split as case 5; the noSavedGameUI
                         // backTarget assignment is what ShowNoSavedGame's
                         // own Ok dispatch below relies on.
                    return OptionsMenuAction::LoadGame;
                case 7:  // "Help": `this.helpUI.backTarget = this.
                         // OptionsUI; this.setCurrentDisplay(this.helpUI);`
                    active_ = Active::Help;
                    return OptionsMenuAction::None;
                case 8:
                    // "Reveal Traitor": secondaryParam==31's own case 8 --
                    // `this.GenericInfoUI.setSecondaryParam(68); this.
                    // GenericInfoUI.setupMessage("Reveal Traitor",
                    // Shop.dialogue[9][66]); this.setCurrentDisplay(this.
                    // GenericInfoUI);`. The intro text is dialogue row 9's
                    // own index 66; the shared info_ Screen renders it, and
                    // Active::RevealIntro (not Active::Info) carries its
                    // distinct Ok dispatch below.
                    info_.SetupMessage("Reveal Traitor", shopDialogue_.groups[9][66]);
                    active_ = Active::RevealIntro;
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
            // secondaryParam==32 (Stats)/61 (Clue Log)/206 (Help)/36
            // (Skill Info) all return to a hardcoded target on Ok -- see
            // infoBackTarget_'s own doc comment.
            active_ = infoBackTarget_;
            return OptionsMenuAction::None;
        case Active::QuitConfirm:
            // secondaryParam==202: unconditional `this.exit()` -- the
            // real, preserved bug (see this class's own header doc
            // comment and M38's MenuFlow, which established it first).
            return OptionsMenuAction::Exit;
        case Active::InventoryList: {
            // secondaryParam==33's own Select branch: `int var28 =
            // selectedIndexOrMinusOne(); if (var28 >= 0) { InventoryItemUI
            // = newInventoryItemUI(var28); currentItemIndex = var28;
            // setCurrentDisplay(InventoryItemUI); }` (the original also
            // wraps this in a try/catch that builds an error Form on any
            // exception -- pure decompiled defensive boilerplate around
            // array access, not real gameplay behavior, so not ported;
            // the `idx >= 0` guard alone already prevents any equivalent
            // issue here).
            int idx = inventoryList_.SelectedIndexOrMinusOne();
            if (idx >= 0) {
                currentItemIndex_ = idx;
                RebuildInventoryItem(player, charData, items, spells, idx);
                active_ = Active::InventoryItem;
            }
            return OptionsMenuAction::None;
        }
        case Active::InventoryItem: {
            // secondaryParam==34's own Select branch: decrements the
            // selected index against the SAME conditional order
            // RebuildInventoryItem's own action list was just built in
            // (Drop always index 0; then Equip-or-Unequip iff
            // CanEquipOrUnequip; then Learn iff CanLearnSpell; then Use
            // iff CanUseItem) -- ported exactly, including the original
            // re-evaluating each gate FRESH here rather than trusting the
            // already-built action list's own implied semantics (safe,
            // since nothing mutates `player` between the two).
            int idx = inventoryItem_.SelectedIndexOrMinusOne();
            if (idx == 0) {
                PlayerInventory::DropInventoryItem(player, items, levels, world, currentItemIndex_);
            } else {
                int remaining = idx;
                if (PlayerInventory::CanEquipOrUnequip(player, items, currentItemIndex_)) {
                    if (--remaining == 0) {
                        if (!PlayerInventory::IsEquipped(player, items, currentItemIndex_)) {
                            PlayerInventory::Equip(player, items, currentItemIndex_, true);
                        } else {
                            PlayerInventory::UnequipSlot(player, items, currentItemIndex_);
                        }
                    }
                }
                if (remaining > 0 && PlayerSpellcasting::CanLearnSpell(player, items, spells, currentItemIndex_)) {
                    if (--remaining == 0) PlayerSpellcasting::LearnSpellFromScroll(player, items, currentItemIndex_);
                }
                if (remaining > 0 && PlayerInventory::CanUseItem(player, items, currentItemIndex_)) {
                    if (--remaining == 0) {
                        // "Use" alone needs a live Monster target and
                        // CombatResolution::UseItem -- outside this
                        // library's reach (see this class's own header
                        // doc comment on OptionsMenuAction::UseInventoryItem).
                        // main.cpp performs the real call, then calls
                        // FinishUseItem() to run this same method's own
                        // tail below.
                        pendingUseItemSlot_ = currentItemIndex_;
                        return OptionsMenuAction::UseInventoryItem;
                    }
                }
            }
            return FinishInventoryItemAction(player, items);
        }
        case Active::SkillsList: {
            // secondaryParam==35's own Select branch: `int var32 =
            // selectedIndexOrMinusOne(); int var56 = character.
            // nthKnownSkillIndex(var32); ... setupMessage("Skill Info",
            // character.skillTooltip(var56));` -- no idx>=0 guard in the
            // original (unreachable in practice: every class starts with
            // at least one rank>0 skill, so this list is never actually
            // empty -- see player/player_movement.h's own "no real tile
            // ever borders a no-neighbor edge" precedent for the same
            // "guard defensively in C++ anyway" reasoning).
            int idx = skillsList_.SelectedIndexOrMinusOne();
            if (idx >= 0) {
                int skillIndex = PlayerCombatStats::NthKnownSkillIndex(player, idx);
                if (skillIndex >= 0) {
                    info_.SetupMessage("Skill Info", PlayerCombatStats::SkillTooltip(player, charData, skillIndex));
                    infoBackTarget_ = Active::SkillsList;
                    active_ = Active::Info;
                }
            }
            return OptionsMenuAction::None;
        }
        case Active::SpellsList: {
            // secondaryParam==37's own Select branch: `int var33 =
            // selectedIndexOrMinusOne(); if (var33 >= 0) { SpellInfoUI =
            // newSpellInfoUI(var33); currentSpellIndex = var33;
            // setCurrentDisplay(SpellInfoUI); } }`
            int idx = spellsList_.SelectedIndexOrMinusOne();
            if (idx >= 0) {
                currentSpellIndex_ = idx;
                int spellIndex0 = PlayerSpellcasting::NthKnownSpellId(player, spells, idx);
                spellInfo_ = Screen(ScreenMode::PromptList);
                spellInfo_.SetupPromptList("Spell Info", PlayerSpellcasting::SpellTooltip(charData, spells, spellIndex0),
                                            {"Ready Spell"});
                active_ = Active::SpellInfo;
            }
            return OptionsMenuAction::None;
        }
        case Active::SpellInfo: {
            // secondaryParam==38's own Select branch: only one real item
            // ("Ready Spell"), so the original doesn't check which index
            // was selected at all -- any Select here sets selectedSpellId.
            int spellIndex0 = PlayerSpellcasting::NthKnownSpellId(player, spells, currentSpellIndex_);
            player.selectedSpellId = static_cast<int8_t>(spellIndex0 + 1);
            RebuildSpellsList(player, spells);
            spellsList_.SetSelectedIndex(currentSpellIndex_);
            active_ = Active::SpellsList;
            currentSpellIndex_ = -1;
            return OptionsMenuAction::None;
        }
        case Active::SaveError:
            // secondaryParam==499's own Ok dispatch: `this.exit();` --
            // unconditional, with no command check and no backTarget read
            // at all (see ShowSaveError's own doc comment).
            return OptionsMenuAction::Exit;
        case Active::NoSavedGame:
            // secondaryParam==305's own Ok dispatch: backTarget is
            // OptionsUI here (case 6 set it just before showing the
            // LoadingScreen), so `gameCanvas.startGameThread()` +
            // `setCurrentDisplay(OptionsUI)`. This port needs no thread
            // restart -- see ShowNoSavedGame's own doc comment.
            active_ = Active::Options;
            return OptionsMenuAction::None;
        case Active::RevealIntro: {
            // secondaryParam==68: NO command check in the original
            // (`else if (uic.secondaryParam == 68) { this.RevealUI =
            // this.newRevealUI(); this.setCurrentDisplay(this.RevealUI);
            // }`) -- but GenericInfoUI (mode 4) has only its own Ok
            // command attached, so Ok is the only command that can ever
            // arrive here anyway; ported as Select. newRevealUI(): mode 5,
            // `setupPromptList("Reveal Traitor", Shop.dialogue[9][67],
            // {"Yes", "No"})` then `removeCommand(cancelCommand)` -- there
            // is NO way to Cancel out of this screen, "Yes" or "No" are
            // the only exits (the same real "no way to back out" quirk as
            // the quit confirmation), and backTarget is OptionsUI. A
            // FRESH Screen every entry (`this.RevealUI = this.
            // newRevealUI()`), so a full replacement here too, the same
            // reasoning as RebuildInventoryList.
            revealConfirm_ = Screen(ScreenMode::PromptList);
            revealConfirm_.SetupPromptList("Reveal Traitor", shopDialogue_.groups[9][67], {"Yes", "No"});
            revealConfirm_.RemoveCommand(CommandId::Cancel);
            active_ = Active::RevealConfirm;
            return OptionsMenuAction::None;
        }
        case Active::RevealConfirm: {
            // secondaryParam==65: `if (var1 == selectCommand) { if
            // (selectedIndexOrMinusOne() == 0) { this.RevealUI = this.
            // newRevealWhomUI(); this.setCurrentDisplay(this.RevealUI); }
            // else { this.setCurrentDisplay(uic.backTarget /* OptionsUI
            // */); } }`. Index 0 is "Yes" -- anything else (i.e. "No") is
            // the decline path straight back to Options.
            int idx = revealConfirm_.SelectedIndexOrMinusOne();
            if (idx == 0) {
                // newRevealWhomUI(): mode 5, "Who is the Traitor?" over the
                // 4 suspect names (Cancel present this time), backTarget
                // OptionsUI. Fresh Screen per entry, same as revealConfirm_
                // above.
                revealWhom_ = Screen(ScreenMode::PromptList);
                revealWhom_.SetupPromptList("Reveal Traitor", "Who is the Traitor?",
                                           {kSuspectNames[0], kSuspectNames[1], kSuspectNames[2], kSuspectNames[3]});
                active_ = Active::RevealWhom;
            } else {
                active_ = Active::Options;
            }
            return OptionsMenuAction::None;
        }
        case Active::RevealWhom: {
            // secondaryParam==66: `if (var1 == selectCommand) { ... }` --
            // the guess itself. The result message is ALWAYS
            // dialogue[9][68] + "\n" + dialogue[9][69] + "\n" plus a
            // third, guess-dependent line; on a correct guess the
            // original also sets newGamePlus and awards the StarFrost
            // item (PlayerInventory::GrantStarFrostItem, taking its spawn
            // id from `nextItemSpawnId` -- main.cpp's own
            // nextDropSpawnId); on a wrong guess the third line is
            // dialogue[9][72] with <TAG> replaced by the REAL traitor's
            // own name (Shop.NAMES[5 + traitorIndex], kSuspectNames here)
            // -- the quiz tells you who it actually was. A real quirk
            // preserved as found: EITHER way the player is then
            // teleported back to the hub (`character.
            // resetToHubPosition(false)` runs outside the if/else), so
            // even a wrong guess yanks you home.
            int guess = revealWhom_.SelectedIndexOrMinusOne();
            std::string message = shopDialogue_.groups[9][68] + "\n" + shopDialogue_.groups[9][69] + "\n";
            if (guess == player.traitorIndex) {
                player.newGamePlus = true;
                PlayerInventory::GrantStarFrostItem(player, items, nextItemSpawnId);
                message += shopDialogue_.groups[9][70];
            } else {
                message += ReplaceFirstTag(shopDialogue_.groups[9][72], "<TAG>",
                                          kSuspectNames[static_cast<size_t>(player.traitorIndex)]);
            }
            info_.SetupMessage("Reveal Traitor", message);
            PlayerMovement::ResetToHubPosition(player, false, levels, world);
            active_ = Active::RevealResult;
            return OptionsMenuAction::None;
        }
        case Active::RevealResult:
            // secondaryParam==67: NO command check in the original either
            // (`else if (uic.secondaryParam == 67) { this.character.
            // ambushTimer = 1; this.character.specialEncounterResolved =
            // true; this.setCurrentDisplay(this.gameCanvas); }`) -- Ok is
            // the only command attached to the mode-4 GenericInfoUI. This
            // is the ONLY assignment of ambushTimer anywhere in the whole
            // game (the one CLASS_MAP.md had mis-resolved as dead code
            // before M38's dispatch fix -- see M43's roadmap entry): it
            // arms GameCanvas.tickPerSecond's still-unported "overstayed
            // in one place" ambush spawner. specialEncounterResolved
            // already round-trips through the save format (M12's packed
            // traitor byte), but ambushTimer/newGamePlus/
            // starFrostBonusActive are all transient -- see player_state.h.
            // Then `setCurrentDisplay(this.gameCanvas)`: back to the game,
            // with the persistent OptionsUI list ready for the next time
            // the Options menu is opened.
            player.ambushTimer = 1;
            player.specialEncounterResolved = true;
            active_ = Active::Options;
            return OptionsMenuAction::ReturnToGame;
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
        case Active::InventoryList:
            // `this.InventoryUI.backTarget = this.OptionsUI` (set in
            // newInventoryUI()).
            active_ = Active::Options;
            return OptionsMenuAction::None;
        case Active::InventoryItem:
            // `var2.backTarget = this.InventoryUI` (set in
            // newInventoryItemUI()).
            active_ = Active::InventoryList;
            return OptionsMenuAction::None;
        case Active::SkillsList:
            // `var1.backTarget = this.OptionsUI` (set in
            // newSkillsListUI()).
            active_ = Active::Options;
            return OptionsMenuAction::None;
        case Active::SpellsList:
            // `var1.backTarget = this.OptionsUI` (set in
            // newSpellsListUI()).
            active_ = Active::Options;
            return OptionsMenuAction::None;
        case Active::SpellInfo:
            // `var2.backTarget = this.SpellsListUI` (set in
            // newSpellInfoUI()).
            active_ = Active::SpellsList;
            return OptionsMenuAction::None;
        case Active::SaveError:
        case Active::NoSavedGame:
            // Both are mode-4 (PlainList) Screens, whose own constructor
            // adds an Ok command ONLY -- there is no Cancel/back command on
            // either to press, so this is a real no-op exactly like
            // Active::Info's above.
            return OptionsMenuAction::None;
        case Active::RevealIntro:
        case Active::RevealResult:
            // Mode-4 GenericInfoUI again -- Ok only, no Cancel/back
            // command to press, exactly like Active::Info/SaveError/
            // NoSavedGame above.
            return OptionsMenuAction::None;
        case Active::RevealConfirm:
            // `newRevealUI()` explicitly removed its own Cancel command --
            // the same real "no way to back out" quirk as Active::QuitConfirm
            // above (and M38's MenuFlow quit confirmation before it): the
            // only exits are selecting "Yes" or "No".
            return OptionsMenuAction::None;
        case Active::RevealWhom:
            // The top-level `if (var1 == cancelCommand && uic.backTarget !=
            // null)` check: `newRevealWhomUI()`'s own backTarget is
            // OptionsUI (`var1.v = this.OptionsUI`).
            active_ = Active::Options;
            return OptionsMenuAction::None;
    }
    return OptionsMenuAction::None;
}

void OptionsMenu::ShowSaveError() {
    // ESGame.run()'s own helperThreadState==5 else-branch:
    // `GenericInfoUI.setSecondaryParam(499)` + `setupMessage("Save Error",
    // ...)` + `setCurrentDisplay(GenericInfoUI)`. `infoBackTarget_` is
    // deliberately left alone -- the original never sets
    // GenericInfoUI.backTarget for this screen, and 499's own dispatch
    // exits unconditionally without ever reading it (see this class's own
    // header doc comment).
    info_.SetupMessage("Save Error",
                       "There was an error in saving your character record. Your previous character record is still "
                       "saved. Try turning your phone off then on again to clear the memory.");
    active_ = Active::SaveError;
}

void OptionsMenu::ShowNoSavedGame() {
    // ESGame.run()'s own helperThreadState==6 else-branch:
    // `setCurrentDisplay(this.noSavedGameUI)` -- the screen itself was
    // already built once in the constructor above, and its backTarget
    // (OptionsUI) was set by case 6 before the LoadingScreen was shown, so
    // there is nothing left to configure here.
    active_ = Active::NoSavedGame;
}

void OptionsMenu::Render(Backbuffer& bb) const { ActiveScreen().Paint(bb); }

}  // namespace dawnstar
