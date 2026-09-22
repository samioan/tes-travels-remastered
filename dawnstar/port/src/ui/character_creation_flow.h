#pragma once
#include <string>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/shop_dialogue.h"
#include "graphics/backbuffer.h"
#include "player/player_state.h"
#include "ui/name_entry.h"
#include "ui/screen.h"
#include "util/java_random.h"

namespace dawnstar {

// What main.cpp itself needs to DO in response to a Select/Cancel press
// -- same shape as ui/menu_flow.h's own MenuFlowAction/ui/options_menu.h's
// own OptionsMenuAction.
enum class CharacterCreationAction {
    None,
    // ClassSelect's own Cancel: `newGameUI.backTarget = mainMenuUI`.
    // main.cpp's job (not this class's) to switch back to M38's own
    // MenuFlow.
    CancelToMainMenu,
    // The real end of the whole flow (secondaryParam==102's own Ok):
    // `gameCanvas.player = character; character.resetState(false);
    // gameCanvas.startGameThread();`. main.cpp's own job to actually
    // construct the REAL PlayerState now (SelectedClassIndex()/
    // EnteredName() below) and start the game -- this class never
    // constructs the real, final character itself (see this class's
    // own doc comment on why).
    StartGame,
};

// M40: `ESGame`'s own real character-creation flow -- `newGameUI`
// (secondaryParam 3, a class-selection prompt list), `characterMainUI`
// (secondaryParam 4, "You selected: <class>" -> See Class Info/Create
// Character), the shared "GenericInfoUI" reused for a class-info
// preview (5), the "Character Created!" prompt (6), the name-length
// error (a real MIDP `Alert` in the original -- folded into the same
// Info-screen-reuse pattern here, since Alert has no real custom
// `paint()` to distinguish from a plain message + Ok either, see this
// class's own `Active::NameTooShort` doc comment), and the 3-screen
// "Welcome"/"Introduction" chain (7/101/102) -- plus `charNameTextForm`
// itself (`ui/name_entry.h`'s own `NameEntry`, a small purpose-built
// widget, not a generic MIDP `Form`/`TextField` port -- see its own
// header doc comment).
//
// A real, important design point checked by tracing `createNewGame()`
// (the "helperThreadState==4" background-thread body a real device ran
// while `createGameUI`'s `LoadingScreen` spun) before writing any of
// this: it does NOTHING but null out now-unneeded UI fields (a
// memory-constrained MIDP device's own GC pressure relief -- irrelevant
// here) and re-load `Shop`'s dialogue table (already loaded once at
// startup in this port, so redundant here too) before immediately
// showing the next real screen. There is no actual asynchronous work to
// wait for, unlike Save/Load's own real file I/O (save/game_save.h, M42)
// -- so this class skips modeling that loading
// screen/background-thread machinery entirely and transitions straight
// from a confirmed name to the "Welcome" screen, which is functionally
// identical to what a modern, non-memory-constrained runtime would do
// anyway.
//
// Real character construction (`PlayerCreation::CreateCharacter`, M11)
// is deliberately NOT done by this class until the very end
// (`StartGame`), matching the original's own real timing: the
// original's `character.resetState(false)` (which is where starting
// items / the hidden `traitorIndex` actually get rolled, via
// `grantStartingItems`/the constructor's own roll) happens only once,
// right before entering gameplay -- NOT at class-selection time, even
// though a live (but not yet fully "reset") `Player` object already
// exists in the original by then for `buildCreationSummary()`'s own
// preview to read from. This class reproduces that same shape with a
// throwaway `previewRng_` (see its own doc comment) rather than
// threading a real, only-half-constructed `PlayerState` through 3
// different states.
class CharacterCreationFlow {
public:
    CharacterCreationFlow(const CharacterData& charData, const ItemDatabase& items, ShopDialogue shopDialogue);

    void OnUp();
    void OnDown();
    CharacterCreationAction OnSelect();
    CharacterCreationAction OnCancel();

    // Text-entry input, routed here only while the flow's own name-entry
    // state is active (main.cpp's own job to gate that, the same way it
    // already gates Up/Down/Select/Cancel to whichever of MenuFlow/
    // OptionsMenu/this class is currently active).
    void OnChar(char c);
    void OnBackspace();

    void Render(Backbuffer& bb) const;

    // Valid only once StartGame has been returned from OnSelect --
    // main.cpp's own job to actually construct the real, final
    // PlayerState from these (see this class's own doc comment on why
    // this class doesn't do that itself).
    int SelectedClassIndex() const { return selectedClassIndex_; }
    const std::string& EnteredName() const { return nameEntry_.Text(); }

private:
    enum class Active {
        ClassSelect,
        CharacterMain,
        ClassInfo,     // secondaryParam 5
        CreatedPrompt, // secondaryParam 6
        NameEntry,     // charNameTextForm
        // The real `Alert` shown when the entered name is under 3
        // characters -- Ok returns to NameEntry to retry, the only real
        // behavior it has (see this class's own class comment on why
        // it's folded into the shared Info-screen-reuse pattern rather
        // than a distinct widget).
        NameTooShort,
        Welcome,  // secondaryParam 7
        Intro1,   // secondaryParam 101
        Intro2,   // secondaryParam 102 -- Ok -> StartGame
    };

    const CharacterData& charData_;
    const ItemDatabase& items_;
    ShopDialogue shopDialogue_;
    // A dedicated, throwaway RNG for the "See Class Info" preview
    // character ONLY (see this class's own doc comment above) -- never
    // the same generator as main.cpp's own real `globalRng`, so
    // repeatedly previewing (or re-previewing after backing out and
    // picking a different class) never perturbs the real character's
    // own eventual starting-item/traitor-index roll.
    JavaRandom previewRng_{1};

    Active active_ = Active::ClassSelect;
    int selectedClassIndex_ = 0;
    // The preview-only character built the moment a class is selected,
    // read by "See Class Info" -- never the real, final character (see
    // this class's own doc comment).
    PlayerState previewPlayer_;

    Screen classSelect_;
    Screen characterMain_;
    Screen info_;
    NameEntry nameEntry_;
};

}  // namespace dawnstar
