#pragma once
#include <string>

#include "assets/help_text.h"
#include "graphics/backbuffer.h"
#include "ui/screen.h"

namespace dawnstar {

// What main.cpp itself needs to DO in response to a Select press --
// the same "return what happened, let main.cpp perform the actual
// real-world effect" shape CombatTick/InteractTick/CampTick's own Tick
// methods already use, since starting a new game (constructing a real
// PlayerState) and exiting the app are both main.cpp's own job, not
// this class's.
enum class MenuFlowAction { None, StartNewGame, Exit };

// M38: a small, purpose-built navigation flow over M37's own generic
// `Screen`, covering exactly the real screens this milestone wires --
// NOT a port of `ESGame`'s own giant `commandAction1()`/
// `setCurrentDisplay()` machinery (every real screen/shop/dialogue
// transition in the whole game), which stays future work (see
// docs/PORT_ROADMAP.md's M38 entry).
//
// Reproduces ESGame's own REAL behavior for: the main menu (`../src/
// ESGame.java`'s own `mainMenuUI`, secondaryParam 2) -- New Game/
// Continue Game/Help/Credits/Exit; its own Help topic list (`helpUI`,
// secondaryParam 203) built from the real, already-grouped M8 `HelpText`
// titles/bodies; the shared "GenericInfoUI" info-message screen
// (secondaryParam 206 for a help topic's own body, 204 for Credits) --
// reused for both, exactly like the original reuses its own single real
// `GenericInfoUI` instance; and the "Are you sure?" quit confirmation
// (`newConfirmQuitUI`, secondaryParam 202).
//
// This is only possible to reproduce correctly because of a real,
// significant decompiler/rename bug this milestone found and fixed in
// `../src/ESGame.java` itself: `commandAction1()`'s entire dispatch
// (and `handleNPCChoices()`'s own one use) used to read `uic.mode` as
// its screen-identity discriminant -- but `Screen.mode` can only ever
// be 3/4/5/6 (Screen's own paint() dispatch value), never the dozens of
// distinct values actually compared against there. Every one of those
// values matches `uic.secondaryParam` instead (confirmed by
// cross-referencing every real `new Screen(...)` call site, and by
// `secondaryParam` being read NOWHERE else in the whole codebase before
// this fix) -- see ESGame.java's own doc comment on commandAction1 for
// the full writeup. Building this class against the WRONG (`.mode`)
// semantics would have produced completely broken dispatch (mainMenuUI/
// OptionsUI/helpUI all share `mode == 3`, so selecting anything on any
// of the three would have landed in the same, wrong branch).
//
// SIMPLIFIED, real gaps deliberately deferred rather than guessed at:
// "New Game" hands off to M40's own `ui/character_creation_flow.h`
// (`CharacterCreationFlow`), a separate class from this one (see
// `MenuFlowAction::StartNewGame`'s own doc comment) -- not modeled
// here. "Continue Game" is a genuine no-op (real save-file load/store
// was never wired to disk by any milestone so far -- `PlayerSave` only
// ever serializes to/from an in-memory byte buffer, see
// docs/PORT_ROADMAP.md's M12/M17 entries).
//
// A real, faithfully-preserved bug in the quit confirmation itself:
// `newConfirmQuitUI()` builds a 2-item "Yes"/"No" prompt list, but its
// own commandAction1 branch (secondaryParam 202) calls `exit()`
// UNCONDITIONALLY on Select -- never actually reading which item was
// selected. So in the original game, picking "No" on "Are you sure you
// want to quit?" ALSO quits. Reproduced exactly, not "fixed" -- see
// OnSelect()'s own doc comment.
class MenuFlow {
public:
    explicit MenuFlow(HelpText helpText);

    // Routed to whichever Screen is currently active -- Screen list
    // navigation is a discrete keypress event in the original (`Screen.
    // handleKey()`'s own game-action-1/6 branches), not a per-tick poll,
    // so main.cpp's own caller edge-detects the physical key the same
    // way M29's 'M' key / M34's interact key already do.
    void OnUp();
    void OnDown();

    // The real Select/Ok command (both map to this port's own single
    // "confirm" key -- see this class's own doc comment on why no
    // generic Left/RightSoftKeyCommand lookup is needed here, unlike a
    // future full ESGame::commandAction1 port).
    MenuFlowAction OnSelect();

    // The real Cancel/Back command.
    void OnCancel();

    void Render(Backbuffer& bb) const;

private:
    enum class Active { MainMenu, HelpTopics, Info, QuitConfirm };

    Screen& ActiveScreen();
    const Screen& ActiveScreen() const;

    Active active_ = Active::MainMenu;
    // Screen.backTarget's own real role, for the one Screen this flow
    // reuses for two different real purposes (Credits' own message, and
    // a selected Help topic's own body) -- ESGame.java's own
    // secondaryParam==206 branch always returns to `helpUI` (not
    // `backTarget`) for a help topic's body, while secondaryParam==204
    // always returns to `mainMenuUI` for Credits; both are hardcoded
    // targets in the original, not derived from a stored backTarget
    // field, so this mirrors that directly rather than adding a
    // Screen::backTarget this milestone doesn't otherwise need (see
    // ui/screen.h's own class comment on why that field isn't ported).
    Active infoBackTarget_ = Active::MainMenu;

    HelpText helpText_;
    Screen mainMenu_;
    Screen helpTopics_;
    Screen info_;
    Screen quitConfirm_;
};

}  // namespace dawnstar
