#pragma once
#include <optional>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/shop_dialogue.h"
#include "graphics/backbuffer.h"
#include "player/game_save.h"
#include "player/player_state.h"

namespace stormhold {

// M40: a port-only state machine standing in for src/UIScreen.java's
// mode-driven paint/keyPressed dispatch plus src/ESGame.java's own
// commandAction() screenGroups 2 (main menu), 3-6 (class select/confirm/
// info/character-created), 7/101 (welcome/intro), and 305 (no-saved-
// game) -- see this file's own .cpp for the per-screen writeup of what's
// modeled vs. simplified. Everything past `Finished` (the live gameplay
// screen itself, GameCanvas/screenGroup 101's own commandAction target)
// is main.cpp's own already-wired tick loop, not this file's concern.
//
// M72: Help (`mainMenuItems[2]`, screenGroups 203/206) is now modeled too
// -- the transcription gap that used to block it (loadHelpTopicBodies()
// stopping short past topic index 4) was closed in `src/ESGame.java`
// directly (M69's own finding: a stalled hand-copy, not a real decompiler
// gap), and this state machine was simply missing the main menu's 5th
// real item (`mainMenuItems = {"New Game", "Continue Game", "Help",
// "Credits", "Exit"}` -- this file previously only modeled 4, silently
// dropping Help). `MenuScreen::Help`/`HelpTopic` and the topic-lookup
// itself are shared with `ui/pause_menu.h`'s own identical Help feature
// via `assets/help_topics.h`'s `HelpTopics` (both are real call sites of
// the SAME `newHelpMenuUI`/`newHelpTopicUI` pair in the original, just
// with different `backTarget`/`nextScreen` values -- see this file's own
// .cpp for the one place that differs: leaving a topic here goes to
// MainMenu, not Stats, since no player/character exists yet at this point
// in the flow, and the real `nextScreen = this.statsUI` target is
// confirmed null pre-game anyway -- `ui/pause_menu.h`'s own class comment
// has the full finding on why neither call site reproduces that as a
// literal softlock).
//
// M50: Continue Game's real load path -- Confirm()'s own MainMenu case 1
// now calls `GameSave::Exists(savePath)` (player/game_save.h) and only
// falls to `NoSavedGame` when that's false, matching `loadGameState()`'s
// own `name == null` early failure. The actual file read happens in
// main.cpp's own hand-off block (same place `state.draft` becomes the
// live `player` for a New Game), since only main.cpp holds the live
// `WorldRegistry`/level array this screen never touches -- this class
// itself only ever needs to know WHETHER a save exists, never its
// contents, so `state.loadRequested` (below) is the only new state this
// screen carries. `savePath` is empty in every existing call site
// (main.cpp is the only real caller that passes a non-empty one) --
// `GameSave::Exists("")` is always false, so every pre-M50 call site
// (this file's own M40 smoke test included) keeps taking the
// `NoSavedGame` branch exactly as before.
enum class MenuScreen {
    MainMenu,
    ClassSelect,
    ClassConfirm,
    ClassInfo,
    CharacterCreated,
    EnterName,
    NoSavedGame,
    Credits,
    // M72: the 12-topic list (screenGroup 203) and one topic's body
    // (screenGroup 206) -- see this file's own class comment.
    Help,
    HelpTopic,
    Welcome,
    Intro,
    // Hand-off point: main.cpp's own live tick/render loop takes over
    // once `screen` reaches this value (matches the real
    // commandAction() screenGroup==101 branch: gameCanvas.player=player;
    // gameCanvas.startGameThread(); showScreen(gameCanvas)).
    Finished,
};

struct MenuFlowState {
    MenuScreen screen = MenuScreen::MainMenu;
    int selectedIndex = 0;
    int chosenClassIndex = 0;
    // this.newCharacterDraft -- only meaningful from ClassSelect's own
    // Confirm() onward.
    std::optional<PlayerState> draft;
    // The real TextField's own maxSize is 10 (ESGame.java's `new
    // TextField(null, null, 10, 0)`) -- TypeChar enforces that same cap.
    std::string enteredName;
    // Alert's own equivalent ("Your character name must be at least 3
    // letters") -- rendered inline on the EnterName screen instead of a
    // separate popup Displayable, a harmless port-only presentation
    // simplification (the real Alert is timed/modal; this is just a
    // line of text that clears itself on the next keystroke).
    bool nameTooShort = false;
    // Set true by MainMenu's own "Exit" item -- main.cpp checks this
    // once per idle callback and closes the window, since this state
    // machine has no window handle of its own to close.
    bool exitRequested = false;
    // M50: set true by MainMenu's own "Continue Game" item once
    // `GameSave::Exists` has already confirmed a save file is there --
    // main.cpp's own hand-off block (see this file's class comment)
    // checks this the same way it checks `draft.has_value()`, and does
    // the real `GameSave::Load` call once `screen` reaches `Finished`.
    bool loadRequested = false;
    // M72: set by Confirm() when entering HelpTopic from Help -- a row
    // index into `HelpTopics` (assets/help_topics.h), not a `ShopDialogue`
    // row. Mirrors `PauseMenuState::helpTopicIndex`.
    int helpTopicIndex = -1;
};

class MenuFlow {
public:
    // Up/Down navigation -- only meaningful for the 3 list-shaped
    // screens (MainMenu/ClassSelect/ClassConfirm); a no-op everywhere
    // else, same as UIScreen.keyPressed()'s own action==1/6 branches
    // being gated on `this.mode`.
    static void MoveSelection(MenuFlowState& state, int delta, const CharacterData& charData);

    // The single soft-key "Ok"/"Select" action, keyed on `state.screen`
    // the same way commandAction() keys on `activeScreen.screenGroup`.
    // `savePath` (M50) is only consulted by the MainMenu "Continue Game"
    // item -- see this file's own class comment.
    static void Confirm(MenuFlowState& state, const CharacterData& charData, const ItemDatabase& items,
                         const std::string& savePath = std::string());

    // The single soft-key "Cancel"/"Back" action.
    static void Cancel(MenuFlowState& state);

    // EnterName only: appends one already-uppercased letter/digit.
    static void TypeChar(MenuFlowState& state, char c);
    static void Backspace(MenuFlowState& state);

    static void Render(Backbuffer& bb, const MenuFlowState& state, const CharacterData& charData,
                        const ShopDialogue& dialogue);
};

}  // namespace stormhold
