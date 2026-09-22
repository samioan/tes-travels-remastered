#pragma once
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/help_text.h"
#include "assets/item_database.h"
#include "assets/shop_dialogue.h"
#include "assets/spell_database.h"
#include "dungeon/dungeon_runtime.h"
#include "graphics/backbuffer.h"
#include "player/player_state.h"
#include "ui/screen.h"
#include "world/dungeon_generator.h"

namespace dawnstar {

// What main.cpp itself needs to DO in response to a Select/Cancel press
// -- same shape as ui/menu_flow.h's own MenuFlowAction. M41's
// UseInventoryItem is the one action this class can't finish by itself:
// "Use" (an inventory-item's 87-99 "gift" action) needs CombatResolution::
// UseItem, which lives in dawnstar_combat -- a library this one
// (dawnstar_render) can't depend on without creating a cycle (dawnstar_combat
// already depends on dawnstar_render, for its own message-popup calls).
// So OnSelect stops short of calling it, returns this instead, and
// main.cpp (which already links both libraries) performs the real
// UseItem call itself before calling back into FinishUseItem() below --
// the same "return what happened, let main.cpp perform the actual
// real-world effect" shape MenuFlowAction/CharacterCreationAction's own
// doc comments already establish, just with an extra round-trip.
//
// M42's SaveGame/LoadGame are handed back for a DIFFERENT reason, and not
// a dependency-cycle one: in the original, secondaryParam==31's own case
// 5/6 do all their work in `ESGame.commandAction()` itself, not in any
// Screen -- they construct a fresh `LoadingScreen`, set
// `noSavedGameUI.backTarget`, swap the current display to it, set
// `helperThreadState = 5`/`6`, and start a background `Thread` that then
// runs `saveGameState()`/`loadGameState()` (case 6 also does
// `System.gc()` + `gameCanvas.stopGameThread()` first). OptionsMenu is
// this port's counterpart of the Screen, not of ESGame, so performing the
// save/load here would put ESGame-level display/thread orchestration (and
// real file I/O) inside a UI class. Returning the action keeps the split
// exactly where the original has it.
enum class OptionsMenuAction { None, ReturnToGame, Exit, UseInventoryItem, SaveGame, LoadGame };

// M39: the real IN-GAME options menu (`../src/ESGame.java`'s own
// `OptionsUI`, secondaryParam 31, opened by `GameCanvas.openOptionsMenu()`
// -- the numeric-keypad '7' key, `GameCanvas.keyPressed()`'s own `key ==
// 55` handler) -- a separate real screen graph from M38's own `MenuFlow`
// (the PRE-game main menu), reachable only once a character actually
// exists.
//
// A real, confirmed behavior this milestone had to reproduce correctly:
// `GameCanvas.run()`'s own per-tick loop takes a completely different
// branch whenever `activeScreen != null` (any real Screen, including
// `OptionsUI`, is currently showing) -- it skips `dispatchTickActions()`
// (movement/combat/camp/interact/casting, ALL of it) entirely, only
// repainting; `keyPressed()` similarly routes every key straight to
// `activeScreen.handleKey()` instead of any game key at all. So opening
// this menu doesn't just overlay the game view (like M30's message
// popup) -- it genuinely PAUSES the whole game, the same way M35's own
// camping-screen branch replaces `paintGameView()` outright, just one
// level higher (this replaces the ENTIRE tick, not just rendering).
// `main.cpp`'s own `inOptionsMenu` early-return mirrors that exactly.
//
// Reproduces, from secondaryParam==31's own real dispatch: "Stats"
// (secondaryParam 32's own `player.buildCharacterSheet()`, transcribed
// line-for-line -- see options_menu.cpp's own `BuildCharacterSheet`);
// "Clue Log" (`ClueUI`, secondaryParam 60/61, built from the real
// npcstrings.dat `ShopDialogue` (M8) plus `Player.eventFlags`/
// `traitorIndex` -- see `BuildClueEntry`'s own doc comment for the real,
// intricate "still unconfirmed vs. traitor's own admission" table lookup
// this transcribes exactly); "Help" (reopens the same real Help topic
// list M38's `MenuFlow` already models, from this menu's own entry point
// -- `this.helpUI.backTarget = this.OptionsUI` -- SIMPLIFIED: this is a
// SEPARATE `Screen` instance from `MenuFlow::helpTopics_`, not the same
// shared object the original's single `ESGame.helpUI` field really is;
// this is provably unobservable in THIS port's own control flow, since
// nothing here ever lets a player return to `MenuFlow`'s own screens
// once a game has actually started -- see main.cpp's own `inMenu`/
// `playerSlot` wiring); and "Quit Game" (`newConfirmQuitUI`, reusing the
// exact same real "either Yes or No exits" bug M38's own quit
// confirmation already faithfully reproduces).
//
// M41 additionally reproduces "Inventory" (`InventoryUI`/`InventoryItemUI`,
// secondaryParam 33/34 -- the item list, an item's own tooltip + dynamic
// Drop/Equip-or-Unequip/Learn/Use action list built in the exact same
// conditional order Player.java's own newInventoryItemUI()/dispatch use,
// so the selected index always maps back to the right action), "Skills"
// (`SkillsListUI`, secondaryParam 35/36 -- a rank>0 skill list, each
// entry's tooltip reusing the shared Info screen the same way Stats/Clue
// Log/Help already do), and "Spells" (`SpellsListUI`/`SpellInfoUI`,
// secondaryParam 37/38 -- a known-spell list, each entry's own tooltip +
// a real "Ready Spell" prompt that sets `selectedSpellId`). See
// options_menu.cpp's own `RebuildInventoryList`/`RebuildInventoryItem`
// doc comments for the real, easy-to-miss `suppressStrafeAdjust` quirk
// (using item 87, "Warp to Camp", from the Inventory screen exits
// straight back to the game view, not back to the item list) and this
// class's own `OptionsMenuAction::UseInventoryItem`/`FinishUseItem` doc
// comments for why "Use" alone needs an extra round-trip through
// main.cpp.
//
// M42 additionally reproduces "Save Game"/"Load Game" (secondaryParam==31's
// own case 5/6) -- no longer the silent no-ops M39 left them as. See
// OptionsMenuAction's own doc comment above for why OnSelect only RETURNS
// them, save/game_save.h for the real persistence behind them, and
// ui/loading_screen.h for the "Saving Game"/"Loading Game" progress bar
// main.cpp shows while that runs.
//
// M43 additionally reproduces "Reveal Traitor" (secondaryParam==31's own
// case 8, then 68/65/66/67's own chain) -- no longer the silent no-op M39
// left it as, and with it the Options menu's deferred-no-op list is now
// EMPTY. See the Active::RevealIntro/RevealConfirm/RevealWhom/RevealResult
// dispatch comments in options_menu.cpp for the exact screen chain, and
// m43_reveal_traitor_smoke.cpp for the deep behavior checks.
class OptionsMenu {
public:
    OptionsMenu(HelpText helpText, ShopDialogue shopDialogue);

    // Routed to whichever Screen is currently active -- same "caller
    // edge-detects the physical key" contract as MenuFlow's own OnUp/
    // OnDown.
    void OnUp();
    void OnDown();

    // The real Select command. Needs the CURRENT player/character/item/
    // spell data (unlike HelpText/ShopDialogue, static tables snapshotted
    // at construction) to build "Stats"/"Clue Log"/"Inventory"/"Skills"/
    // "Spells" text fresh every time they're actually opened, so all are
    // passed in here rather than stored. `player` is mutable (unlike
    // M39's original const reference) since M41's Drop/Equip/Unequip/
    // Learn inventory actions and "Ready Spell" all genuinely mutate it
    // -- `levels`/`world` are needed too, for Drop (DungeonRuntime::
    // AddDroppedItem) -- but NOT `MonsterDatabase`/`JavaRandom`: "Use"
    // never runs here at all, see OptionsMenuAction::UseInventoryItem's
    // own doc comment.
    //
    // M43 added `nextItemSpawnId` (main.cpp's own nextDropSpawnId,
    // Item.nextSpawnId()'s stand-in): the Reveal Traitor quiz's
    // correct-guess award hands the granted StarFrost its spawn id from
    // that same live counter every other item-granting call site already
    // takes it from -- passed by reference so the hand-out genuinely
    // advances the shared counter, the same grown-signature reasoning as
    // M41's own `items`/`spells`/`levels`/`world` additions.
    OptionsMenuAction OnSelect(PlayerState& player, const CharacterData& charData, const ItemDatabase& items,
                               const SpellDatabase& spells, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                               int16_t& nextItemSpawnId);

    // The real Cancel/Back command (both map to this port's own single
    // "back" key -- same simplification MenuFlow::OnCancel already
    // makes, see its own doc comment).
    OptionsMenuAction OnCancel();

    // Only meaningful right after OnSelect returns UseInventoryItem: the
    // inventory slot "Use" was chosen for.
    int PendingUseItemSlot() const { return pendingUseItemSlot_; }

    // Completes an inventory "Use" action main.cpp already performed
    // (the real CombatResolution::UseItem call) -- Player.java's own
    // secondaryParam==34 tail: back to the game view if the item just
    // used set `player.suppressStrafeAdjust` (item 87, "Warp to Camp" --
    // clearing the flag right back to false the same way the original
    // does), otherwise back to a freshly-rebuilt Inventory list with the
    // same slot still selected.
    OptionsMenuAction FinishUseItem(PlayerState& player, const ItemDatabase& items);

    // M42: the two failure screens ESGame's own `run()` shows when a
    // save/load actually fails -- both called by main.cpp AFTER it has run
    // the real GameSave call, since only then is the outcome known.
    //
    // ShowSaveError() is `run()`'s helperThreadState==5 else-branch:
    // `GenericInfoUI.setSecondaryParam(499); setupMessage("Save Error",
    // "There was an error in saving your character record. Your previous
    // character record is still saved. Try turning your phone off then on
    // again to clear the memory."); setCurrentDisplay(GenericInfoUI);` --
    // i.e. the SAME shared info Screen Stats/Clue Log/Help already use
    // (ESGame has exactly one GenericInfoUI), reused here as Active::
    // SaveError rather than as Active::Info because its own secondaryParam
    // dispatches differently: `else if (uic.secondaryParam == 499) {
    // this.exit(); }` -- pressing Ok on a Save Error EXITS THE GAME. A
    // real, easy-to-miss behavior (and consistent with the message's own
    // "turn your phone off then on again" advice), reproduced as
    // OptionsMenuAction::Exit rather than as a return to whatever screen
    // last opened info_ (which is what Active::Info would do, and what
    // 499's own unconditional exit() deliberately does NOT).
    void ShowSaveError();

    // ShowNoSavedGame() is `run()`'s helperThreadState==6 else-branch:
    // `setCurrentDisplay(this.noSavedGameUI)` -- a SEPARATE Screen from
    // GenericInfoUI (`new Screen(this, 4, 305)` + `setupMessage(
    // "Unavailable", "No game is available for loading. Press OK to return
    // to main menu.")`, both built once in allocateAllUIs(), hence a
    // persistent member here rather than a rebuilt one). Its own Ok
    // dispatch is secondaryParam==305: `if (uic.backTarget ==
    // this.OptionsUI) { this.gameCanvas.startGameThread(); }
    // setCurrentDisplay(uic.backTarget);` -- and case 6 set that backTarget
    // to OptionsUI right before showing the LoadingScreen, so Ok returns
    // here, to the Options menu. Two real quirks worth noting: the message
    // says "return to main menu" while the code actually returns to the
    // Options menu (the main-menu "Continue Game" path -- M52, `ui/
    // menu_flow.h`'s own `MenuFlow::ShowNoSavedGame` -- is the one that
    // sets backTarget to mainMenuUI, at ESGame.java's own line 555;
    // that class keeps its own separate copy of this same screen rather
    // than sharing this one, since the two classes don't otherwise
    // share state), and the game thread is only restarted for the
    // OptionsUI case (this port needs no equivalent: main.cpp's own
    // inOptionsMenu early-return already pauses/resumes the whole tick).
    void ShowNoSavedGame();

    void Render(Backbuffer& bb) const;

private:
    enum class Active {
        Options,
        ClueLog,
        Help,
        Info,
        QuitConfirm,
        InventoryList,
        InventoryItem,
        SkillsList,
        SpellsList,
        SpellInfo,
        SaveError,
        NoSavedGame,
        // M43: the "Reveal Traitor" chain. RevealIntro/RevealResult both
        // RENDER the same shared info_ Screen (ESGame reuses its one
        // GenericInfoUI for secondaryParam 68 and 67), kept as distinct
        // Active states because their own dispatches differ -- exactly
        // the same reasoning as Active::SaveError's own doc comment
        // above. RevealConfirm/RevealWhom are the two PROMPT-list
        // Screens ESGame's newRevealUI()/newRevealWhomUI() build FRESH
        // on every entry.
        RevealIntro,
        RevealConfirm,
        RevealWhom,
        RevealResult
    };

    Screen& ActiveScreen();
    const Screen& ActiveScreen() const;

    void RebuildInventoryList(const PlayerState& player, const ItemDatabase& items);
    void RebuildInventoryItem(const PlayerState& player, const CharacterData& charData, const ItemDatabase& items,
                              const SpellDatabase& spells, int slot);
    void RebuildSkillsList(const PlayerState& player, const CharacterData& charData);
    void RebuildSpellsList(const PlayerState& player, const SpellDatabase& spells);
    // The tail shared by every inventory-item action (Drop/Equip/Unequip/
    // Learn, and -- via FinishUseItem -- Use): Player.java's own
    // `if (character.suppressStrafeAdjust) {...} else { rebuild + reopen
    // InventoryUI }` at the end of secondaryParam==34's dispatch.
    OptionsMenuAction FinishInventoryItemAction(PlayerState& player, const ItemDatabase& items);

    Active active_ = Active::Options;
    // Screen.backTarget's own real role for the one Screen this flow
    // reuses for four different real purposes (Stats/a Clue Log entry/
    // a Help topic's own body/a Skill's own tooltip) -- same "hardcoded
    // per real dispatch branch, not a stored Screen::backTarget field"
    // reasoning as MenuFlow::infoBackTarget_ (see ui/screen.h's own class
    // comment on why that field isn't ported).
    Active infoBackTarget_ = Active::Options;

    HelpText helpText_;
    ShopDialogue shopDialogue_;
    Screen options_;
    Screen clueLog_;
    Screen helpTopics_;
    Screen info_;
    Screen quitConfirm_;
    // M42: ESGame's own `noSavedGameUI` -- built ONCE in allocateAllUIs()
    // with a fixed message (see ShowNoSavedGame's own doc comment), so a
    // persistent member like options_/clueLog_/helpTopics_ above rather
    // than a Rebuild*-style fresh one.
    Screen noSavedGame_;

    // M41: unlike options_/clueLog_/helpTopics_ above (each a single
    // real Screen instance Player.java's own ESGame constructs ONCE and
    // reuses, so a stale selectedIndex_ genuinely persists across visits
    // -- see options_menu.cpp's own BuildClueEntry-adjacent test-bug note
    // in docs/PORT_ROADMAP.md's M39 entry), `InventoryUI`/`SkillsListUI`/
    // `SpellsListUI` are each rebuilt FRESH from scratch on every real
    // (re)entry in the original (`this.InventoryUI = this.newInventoryUI();`,
    // a brand new Screen every time) -- so these members are fully
    // REPLACED (assigned a fresh `Screen(ScreenMode::PromptList)`) by
    // Rebuild*, not just reconfigured in place, to reproduce that same
    // "selection always starts fresh unless explicitly restored via
    // SetSelectedIndex" real behavior.
    Screen inventoryList_;
    Screen inventoryItem_;
    Screen skillsList_;
    Screen spellsList_;
    Screen spellInfo_;
    // M43: ESGame's own single RevealUI field, reassigned to a brand-new
    // Screen by newRevealUI()/newRevealWhomUI() on every (re)entry -- two
    // members here (one per construction) rather than one, so each keeps
    // its own fresh-replacement semantics explicit, same "selection
    // always starts fresh" reasoning as inventoryList_ above.
    Screen revealConfirm_;
    Screen revealWhom_;
    // ESGame.currentItemIndex/currentSpellIndex -- which inventory slot/
    // known-spell index InventoryItemUI/SpellInfoUI's own single-item
    // action dispatch (secondaryParam 34/38) applies to, since neither
    // screen carries that itself.
    int currentItemIndex_ = -1;
    int currentSpellIndex_ = -1;
    // Set only when OnSelect returns UseInventoryItem; read back by
    // main.cpp via PendingUseItemSlot() before it calls FinishUseItem().
    int pendingUseItemSlot_ = -1;
};

}  // namespace dawnstar
