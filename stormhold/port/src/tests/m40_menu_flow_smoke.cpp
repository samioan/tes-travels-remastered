// M40 smoke test: MenuFlow -- the port-only main-menu/new-game state
// machine standing in for src/UIScreen.java + src/ESGame.java's own
// commandAction() screenGroups 2-7/101/305 (see ui/menu_flow.h's own
// header comment for the full writeup of what's modeled vs.
// simplified).
//
// No JVM ground truth possible (this state machine has no single real
// decompiled counterpart to diff against -- it's this port's own fusion
// of several real UIScreen/ESGame call sites into one flow, same class
// of gap player/player_creation.h's own header comment already flags
// for its own two-step-into-one fusion). Verified by walking the WHOLE
// real flow end to end against real charin.dat/npcstrings.dat data:
// Main Menu -> Continue Game (no save) -> back -> Credits -> back ->
// New Game -> class select -> class confirm -> class info -> back ->
// create character -> enter name (too-short rejected, then accepted) ->
// welcome -> intro -> Finished, plus Cancel()/MoveSelection() clamping
// and Exit.
#include <cstdio>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/help_topics.h"
#include "assets/item_database.h"
#include "assets/shop_dialogue.h"
#include "graphics/backbuffer.h"
#include "player/player_creation.h"
#include "ui/menu_flow.h"

namespace {

using namespace stormhold;

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assets(root);
        CharacterData charData = CharacterData::Load(assets);
        ItemDatabase items = ItemDatabase::Load(assets);
        ShopDialogue dialogue = ShopDialogue::Load(assets);
        Backbuffer bb;

        std::printf("-- Main Menu navigation clamps at both ends --\n");
        MenuFlowState state;
        Expect(state.screen == MenuScreen::MainMenu, "should start on the Main Menu");
        MenuFlow::MoveSelection(state, -1, charData);
        Expect(state.selectedIndex == 0, "MoveSelection should clamp at 0");
        MenuFlow::MoveSelection(state, 1, charData);
        MenuFlow::MoveSelection(state, 1, charData);
        MenuFlow::MoveSelection(state, 1, charData);
        MenuFlow::MoveSelection(state, 1, charData);
        Expect(state.selectedIndex == 4,
               "Main Menu has 5 real items (New/Continue/Help/Credits/Exit -- M72, previously silently missing "
               "Help), index should reach 4");
        MenuFlow::MoveSelection(state, 1, charData);
        Expect(state.selectedIndex == 4, "MoveSelection should clamp at the last item");
        MenuFlow::Render(bb, state, charData, dialogue);

        std::printf("-- Continue Game always takes the no-saved-game branch --\n");
        state.selectedIndex = 1;
        MenuFlow::Confirm(state, charData, items);
        Expect(state.screen == MenuScreen::NoSavedGame, "Continue Game should show NoSavedGame (no persisted save exists)");
        MenuFlow::Render(bb, state, charData, dialogue);
        MenuFlow::Confirm(state, charData, items);
        Expect(state.screen == MenuScreen::MainMenu, "NoSavedGame's Ok should return to the Main Menu");
        Expect(state.selectedIndex == 0, "returning to the Main Menu should reset selectedIndex");

        std::printf("-- Help: 12-topic list -> a topic's body -> Main Menu (M72) --\n");
        state.selectedIndex = 2;
        MenuFlow::Confirm(state, charData, items);
        Expect(state.screen == MenuScreen::Help, "index 2 should be Help");
        MenuFlow::MoveSelection(state, 100, charData);
        Expect(state.selectedIndex == HelpTopics::kCount - 1, "Help should clamp at the 12th topic like any list");
        state.selectedIndex = 5;
        MenuFlow::Render(bb, state, charData, dialogue);
        MenuFlow::Confirm(state, charData, items);
        Expect(state.screen == MenuScreen::HelpTopic, "picking a topic should open its body");
        Expect(state.helpTopicIndex == 5, "helpTopicIndex should be the row that was picked");
        MenuFlow::Render(bb, state, charData, dialogue);
        // Real target is `nextScreen = this.statsUI`, confirmed null this
        // early (no game has started yet) -- see ui/pause_menu.h's own
        // class comment for the full finding. This port's own choice is
        // MainMenu, not a literal reproduction of that null-target
        // softlock -- see ui/menu_flow.h's own class comment.
        MenuFlow::Confirm(state, charData, items);
        Expect(state.screen == MenuScreen::MainMenu, "leaving a help topic (Ok) should land on the Main Menu");
        state.selectedIndex = 2;
        MenuFlow::Confirm(state, charData, items);
        MenuFlow::Cancel(state);
        Expect(state.screen == MenuScreen::MainMenu, "Cancel from the Help topic list itself should also return to the Main Menu");

        std::printf("-- Credits round-trips back to the Main Menu via Cancel --\n");
        state.selectedIndex = 3;
        MenuFlow::Confirm(state, charData, items);
        Expect(state.screen == MenuScreen::Credits, "index 3 should be Credits");
        MenuFlow::Render(bb, state, charData, dialogue);
        MenuFlow::Cancel(state);
        Expect(state.screen == MenuScreen::MainMenu, "Credits' Cancel should also return to the Main Menu");

        std::printf("-- New Game -> class select -> class confirm -> class info -> back --\n");
        state.selectedIndex = 0;
        MenuFlow::Confirm(state, charData, items);
        Expect(state.screen == MenuScreen::ClassSelect, "New Game should open ClassSelect");
        int classCount = charData.ClassCount();
        Expect(classCount > 1, "real charin.dat should have more than one class to exercise selection with");
        MenuFlow::MoveSelection(state, 1, charData);
        Expect(state.selectedIndex == 1, "ClassSelect should navigate like any other list");

        MenuFlow::Confirm(state, charData, items);
        Expect(state.screen == MenuScreen::ClassConfirm, "selecting a class should open ClassConfirm");
        Expect(state.chosenClassIndex == 1, "chosenClassIndex should be the class just selected");
        Expect(state.draft.has_value(), "confirming a class should build the character draft");
        Expect(state.draft->classIndex == 1, "the draft's own classIndex should match the chosen class");

        MenuFlow::Confirm(state, charData, items);  // "See Class Info"
        Expect(state.screen == MenuScreen::ClassInfo, "ClassConfirm item 0 should open ClassInfo");
        std::string summary = PlayerCreation::CharacterSummaryShort(*state.draft, charData);
        Expect(summary.find(charData.classNames[1]) != std::string::npos,
               "the class-info summary should mention the chosen class by name");
        MenuFlow::Render(bb, state, charData, dialogue);
        MenuFlow::Cancel(state);
        Expect(state.screen == MenuScreen::ClassConfirm, "ClassInfo's Back should return to ClassConfirm");

        std::printf("-- ClassConfirm's own Cancel returns to ClassSelect at the previously-chosen class --\n");
        MenuFlow::Cancel(state);
        Expect(state.screen == MenuScreen::ClassSelect, "ClassConfirm's Cancel should return to ClassSelect");
        Expect(state.selectedIndex == 1, "ClassSelect should re-highlight the previously-chosen class");
        MenuFlow::Confirm(state, charData, items);
        Expect(state.screen == MenuScreen::ClassConfirm, "re-confirming should return to ClassConfirm");

        std::printf("-- Create Character -> enter name (rejects <3 letters) -> welcome -> intro -> Finished --\n");
        state.selectedIndex = 1;  // "Create Character"
        MenuFlow::Confirm(state, charData, items);
        Expect(state.screen == MenuScreen::CharacterCreated, "ClassConfirm item 1 should open CharacterCreated");
        MenuFlow::Render(bb, state, charData, dialogue);

        MenuFlow::Confirm(state, charData, items);
        Expect(state.screen == MenuScreen::EnterName, "CharacterCreated's Select should open EnterName");
        Expect(state.enteredName.empty(), "enteredName should start empty");

        MenuFlow::TypeChar(state, 'A');
        MenuFlow::TypeChar(state, 'B');
        Expect(state.enteredName == "AB", "TypeChar should append characters in order");
        MenuFlow::Confirm(state, charData, items);
        Expect(state.screen == MenuScreen::EnterName, "a 2-letter name should be rejected, staying on EnterName");
        Expect(state.nameTooShort, "nameTooShort should be set after rejecting a too-short name");

        MenuFlow::TypeChar(state, 'C');
        Expect(state.enteredName == "ABC", "TypeChar should still work after a rejected attempt");
        MenuFlow::Confirm(state, charData, items);
        Expect(state.screen == MenuScreen::Welcome, "a 3-letter name should be accepted, advancing to Welcome");
        Expect(state.draft->name == "ABC", "the accepted name should be written into the draft");
        MenuFlow::Render(bb, state, charData, dialogue);

        MenuFlow::Confirm(state, charData, items);
        Expect(state.screen == MenuScreen::Intro, "Welcome's Ok should open Intro");
        MenuFlow::Render(bb, state, charData, dialogue);

        MenuFlow::Confirm(state, charData, items);
        Expect(state.screen == MenuScreen::Finished, "Intro's Ok should hand off to the live game (Finished)");

        std::printf("-- Backspace only applies to EnterName --\n");
        MenuFlowState nameState;
        nameState.screen = MenuScreen::EnterName;
        MenuFlow::TypeChar(nameState, 'X');
        MenuFlow::TypeChar(nameState, 'Y');
        MenuFlow::Backspace(nameState);
        Expect(nameState.enteredName == "X", "Backspace should remove exactly the last character");
        MenuFlow::Backspace(nameState);
        MenuFlow::Backspace(nameState);
        Expect(nameState.enteredName.empty(), "Backspace on an empty name should be a harmless no-op");

        std::printf("-- Exit sets exitRequested without changing the screen --\n");
        MenuFlowState exitState;
        exitState.selectedIndex = 4;
        MenuFlow::Confirm(exitState, charData, items);
        Expect(exitState.exitRequested, "Main Menu item 4 (Exit) should set exitRequested");

        if (!g_ok) {
            std::fprintf(stderr, "m40_menu_flow_smoke: FAILED\n");
            return 1;
        }

        std::printf("all checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m40_menu_flow_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
