#pragma once
#include <string>

#include "assets/character_data.h"
#include "assets/help_text.h"
#include "assets/shop_dialogue.h"
#include "graphics/backbuffer.h"
#include "player/player_state.h"
#include "ui/screen.h"

namespace dawnstar {

// What main.cpp itself needs to DO in response to a Select/Cancel press
// -- same shape as ui/menu_flow.h's own MenuFlowAction.
enum class OptionsMenuAction { None, ReturnToGame, Exit };

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
// Deliberately DEFERRED as real, silent no-ops (same "Continue Game"
// precedent M38 already established), since each needs a whole system
// this port hasn't built yet: "Inventory" (item-slot/equip UI); "Skills"/
// "Spells" (their own summary-list screens); "Save Game"/"Load Game"
// (real file I/O plus `LoadingScreen`'s own background-thread machinery
// -- `PlayerSave` only (de)serializes to/from an in-memory buffer so
// far, see docs/PORT_ROADMAP.md's M12/M17 entries); "Reveal Traitor"
// (secondaryParam 68/65/66's own multi-screen "who is the traitor?"
// mini-quiz, which on a correct guess calls `grantStarFrostItem()` and
// sets `newGamePlus`/`ambushTimer` -- none of which are ported: see
// player/player_state.h's own `starFrostBonusActive`/`traitorSuspicionCount`
// doc comments).
class OptionsMenu {
public:
    OptionsMenu(HelpText helpText, ShopDialogue shopDialogue);

    // Routed to whichever Screen is currently active -- same "caller
    // edge-detects the physical key" contract as MenuFlow's own OnUp/
    // OnDown.
    void OnUp();
    void OnDown();

    // The real Select command. Needs the CURRENT player/character data
    // (unlike HelpText/ShopDialogue, static tables snapshotted at
    // construction) to build "Stats"/"Clue Log" text fresh every time
    // they're actually opened, so both are passed in here rather than
    // stored.
    OptionsMenuAction OnSelect(const PlayerState& player, const CharacterData& charData);

    // The real Cancel/Back command (both map to this port's own single
    // "back" key -- same simplification MenuFlow::OnCancel already
    // makes, see its own doc comment).
    OptionsMenuAction OnCancel();

    void Render(Backbuffer& bb) const;

private:
    enum class Active { Options, ClueLog, Help, Info, QuitConfirm };

    Screen& ActiveScreen();
    const Screen& ActiveScreen() const;

    Active active_ = Active::Options;
    // Screen.backTarget's own real role for the one Screen this flow
    // reuses for three different real purposes (Stats/a Clue Log entry/
    // a Help topic's own body) -- same "hardcoded per real dispatch
    // branch, not a stored Screen::backTarget field" reasoning as
    // MenuFlow::infoBackTarget_ (see ui/screen.h's own class comment on
    // why that field isn't ported).
    Active infoBackTarget_ = Active::Options;

    HelpText helpText_;
    ShopDialogue shopDialogue_;
    Screen options_;
    Screen clueLog_;
    Screen helpTopics_;
    Screen info_;
    Screen quitConfirm_;
};

}  // namespace dawnstar
