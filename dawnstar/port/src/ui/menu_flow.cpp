#include "ui/menu_flow.h"

#include <utility>

namespace dawnstar {

namespace {

// ESGame.getCreditsString() -- a plain hardcoded literal in the
// original (no data file backs it, unlike helptext.dat), so it's
// reproduced directly here rather than needing a new asset loader.
const std::string kCreditsText =
    "Game Design: Anthony Gill and Greg Gorden\n"
    "Art: Mark Jones\n"
    "Programming: Marc Ilgen, Roland Kemp\n"
    "Technical Director: Andrew Friedman\n"
    "(C) 2003 Vir2L Studos, a ZeniMax Media company. The Elder Scrolls and Vir2L are "
    "registered trademarks of ZeniMax Media Inc. All rights reserved.\n";

}  // namespace

MenuFlow::MenuFlow(HelpText helpText)
    : helpText_(std::move(helpText)),
      mainMenu_(ScreenMode::HighlightedList),
      helpTopics_(ScreenMode::HighlightedList),
      info_(ScreenMode::PlainList),
      quitConfirm_(ScreenMode::PromptList) {
    // ESGame.allocateAllUIs()'s own mainMenuUI construction: `new
    // String[]{"New Game", "Continue Game", "Help", "Credits", "Exit"}`,
    // `setupList("Main Menu", ..., false)` -- not cancelable, matching
    // the real screen having no Back/Cancel command at all.
    mainMenu_.SetupList("Main Menu", {"New Game", "Continue Game", "Help", "Credits", "Exit"}, false);
    // `this.helpUI.setupList("Help", helpTitles, true)` -- cancelable,
    // using M8's own already-grouped 12 real titles.
    helpTopics_.SetupList("Help", helpText_.titles, true);
    // `newConfirmQuitUI()`: a 2-item "Yes"/"No" prompt list with its own
    // Cancel command explicitly removed right after construction -- see
    // this class's own header doc comment on the real, preserved "either
    // answer quits" bug this leads to.
    quitConfirm_.SetupPromptList("Quit?", "Are you sure?", {"Yes", "No"});
    quitConfirm_.RemoveCommand(CommandId::Cancel);
}

Screen& MenuFlow::ActiveScreen() {
    switch (active_) {
        case Active::MainMenu:
            return mainMenu_;
        case Active::HelpTopics:
            return helpTopics_;
        case Active::Info:
            return info_;
        case Active::QuitConfirm:
            return quitConfirm_;
    }
    return mainMenu_;
}

const Screen& MenuFlow::ActiveScreen() const { return const_cast<MenuFlow*>(this)->ActiveScreen(); }

void MenuFlow::OnUp() { ActiveScreen().MoveSelectionUp(); }

void MenuFlow::OnDown() { ActiveScreen().MoveSelectionDown(); }

MenuFlowAction MenuFlow::OnSelect() {
    switch (active_) {
        case Active::MainMenu: {
            // ESGame's own secondaryParam==2 branch (see this class's
            // own header doc comment on why `.mode` would have been the
            // WRONG field to key this on).
            switch (mainMenu_.SelectedIndexOrMinusOne()) {
                case 0:  // "New Game" -- see this class's own header
                         // doc comment on why this still starts M20's
                         // own fixed stand-in character directly, rather
                         // than the original's own real (unported)
                         // class-selection/name-entry flow.
                    return MenuFlowAction::StartNewGame;
                case 1:  // "Continue Game" -- M52, see this class's own
                         // header doc comment: main.cpp does the real
                         // GameSave load/resume work.
                    return MenuFlowAction::ContinueGame;
                case 2:  // "Help": `this.helpUI.backTarget =
                         // this.mainMenuUI; this.setCurrentDisplay(this.
                         // helpUI);` -- backTarget itself isn't modeled
                         // (always MainMenu in practice, since this is
                         // the only real place Help is ever entered
                         // from in this milestone's own scope), so
                         // OnCancel's own HelpTopics case just returns
                         // to MainMenu directly.
                    active_ = Active::HelpTopics;
                    return MenuFlowAction::None;
                case 3:  // "Credits": `GenericInfoUI.setSecondaryParam(204);
                         // setupMessage("Credits", creditsString);`
                    info_.SetupMessage("Credits", kCreditsText);
                    infoBackTarget_ = Active::MainMenu;
                    active_ = Active::Info;
                    return MenuFlowAction::None;
                case 4:  // "Exit": `this.confirmQuitUI =
                         // this.newConfirmQuitUI(uic); this.
                         // setCurrentDisplay(this.confirmQuitUI);` --
                         // NOT an immediate exit, shows the (buggy)
                         // confirmation first.
                    active_ = Active::QuitConfirm;
                    return MenuFlowAction::None;
                default:
                    return MenuFlowAction::None;
            }
        }
        case Active::HelpTopics: {
            // secondaryParam==203's own Select branch: `GenericInfoUI.
            // setSecondaryParam(206); setupMessage(helpTitles[i],
            // helpStrings[i]);`
            int idx = helpTopics_.SelectedIndexOrMinusOne();
            if (idx >= 0 && idx < static_cast<int>(helpText_.titles.size())) {
                info_.SetupMessage(helpText_.titles[static_cast<size_t>(idx)],
                                    helpText_.bodies[static_cast<size_t>(idx)]);
                infoBackTarget_ = Active::HelpTopics;
                active_ = Active::Info;
            }
            return MenuFlowAction::None;
        }
        case Active::Info:
            // secondaryParam==206 always returns to helpUI; ==204
            // always returns to mainMenuUI -- both hardcoded targets in
            // the original (see infoBackTarget_'s own doc comment).
            active_ = infoBackTarget_;
            return MenuFlowAction::None;
        case Active::QuitConfirm:
            // secondaryParam==202: `this.exit();` -- unconditional,
            // regardless of which of "Yes"/"No" is actually selected.
            // The real, preserved bug this class's own header doc
            // comment describes: reproduced exactly, not "fixed" into
            // checking SelectedIndex().
            return MenuFlowAction::Exit;
    }
    return MenuFlowAction::None;
}

void MenuFlow::OnCancel() {
    switch (active_) {
        case Active::HelpTopics:
            // The top-level `if (var1 == cancelCommand && uic.backTarget
            // != null) { setCurrentDisplay(uic.backTarget); return; }`
            // check in commandAction1 -- helpUI.backTarget is always
            // mainMenuUI in this milestone's own scope (see this
            // method's own case-2 doc comment above).
            active_ = Active::MainMenu;
            return;
        case Active::MainMenu:
            // No Cancel command exists on the real mainMenuUI at all
            // (setupList(..., false)) -- a real no-op.
            return;
        case Active::Info:
            // The real Credits/help-topic-body screens only ever have
            // an Ok command, no Cancel -- a real no-op.
            return;
        case Active::QuitConfirm:
            // `newConfirmQuitUI()` explicitly REMOVES its own Cancel
            // command -- a real, preserved quirk: there is no way to
            // back out of the quit confirmation at all, only to answer
            // it (and either answer quits, see OnSelect's own doc
            // comment).
            return;
    }
}

void MenuFlow::ShowNoSavedGame() {
    // ESGame.run()'s own helperThreadState==6 else-branch:
    // `setCurrentDisplay(noSavedGameUI)` -- reuses `info_` exactly like
    // Credits/a Help topic's body do (see `infoBackTarget_`'s own doc
    // comment), with `backTarget` left at its default `MainMenu` (the
    // original's own `noSavedGameUI.backTarget = mainMenuUI`, set right
    // before this call at ESGame.java's own line ~762).
    info_.SetupMessage("Unavailable", "No game is available for loading. Press OK to return to main menu.");
    infoBackTarget_ = Active::MainMenu;
    active_ = Active::Info;
}

void MenuFlow::Render(Backbuffer& bb) const { ActiveScreen().Paint(bb); }

}  // namespace dawnstar
