#pragma once
#include <optional>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/shop_dialogue.h"
#include "graphics/backbuffer.h"
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
// Deliberately NOT modeled: Help (loadHelpTopicBodies()'s own Java
// transcription is itself incomplete past topic index 4 -- a real,
// pre-existing gap, not one this milestone introduces) and Continue
// Game's REAL load path (this port has no persisted save file to load
// yet -- PlayerSave, M20, only round-trips a PlayerState in memory, and
// there's no WorldRegistry/master-list save format at all) -- selecting
// "Continue Game" always takes the real game's own "no saved game"
// branch (NoSavedGame below), which is simply the truth for every run of
// this port today.
enum class MenuScreen {
    MainMenu,
    ClassSelect,
    ClassConfirm,
    ClassInfo,
    CharacterCreated,
    EnterName,
    NoSavedGame,
    Credits,
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
    static void Confirm(MenuFlowState& state, const CharacterData& charData, const ItemDatabase& items);

    // The single soft-key "Cancel"/"Back" action.
    static void Cancel(MenuFlowState& state);

    // EnterName only: appends one already-uppercased letter/digit.
    static void TypeChar(MenuFlowState& state, char c);
    static void Backspace(MenuFlowState& state);

    static void Render(Backbuffer& bb, const MenuFlowState& state, const CharacterData& charData,
                        const ShopDialogue& dialogue);
};

}  // namespace stormhold
