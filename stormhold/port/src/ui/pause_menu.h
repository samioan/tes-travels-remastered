#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/shop_dialogue.h"
#include "assets/spell_database.h"
#include "dungeon/dungeon_runtime.h"
#include "graphics/backbuffer.h"
#include "player/player_state.h"
#include "ui/inventory_ui.h"
#include "world/shop_state.h"
#include "world/warden.h"

namespace stormhold {

// M63: renamed-source counterpart of ../../../src/ESGame.java's in-game
// "Options" pause menu (optionsUI, screenGroup 31: Stats/Inventory/Skills/
// Spells/Save Game/Load Game/Help/Quit Game) and everything it opens onto
// (newStatsUI/newSkillsListUI+newSkillInfoUI/newSpellsListUI+
// newSpellInfoUI, screenGroups 32/35/36/37/38), plus the real Save/Load
// trigger `docs/PORT_ROADMAP.md`'s own "what's next" note flagged as
// missing ever since M50 built `GameSave` with nothing yet calling it from
// live gameplay.
//
// **THE BIG FINDING this milestone turned up, bigger than any single
// screen's own content:** the real in-game pause menu is not just
// under-wired the way `openInventory()` was before M62 -- it is
// COMPLETELY UNREACHABLE in the original game, confirmed by reading every
// file in ../../../src/ for how `optionsUI` could ever become the active
// screen. `ESGame` declares `cmdMenu` (a `Command("Menu", SCREEN, 0)`,
// line ~84) clearly intended to open it, but that `Command` is never once
// passed to `addCommand` anywhere in ESGame.java OR GameCanvas.java (grep
// confirms `GameCanvas` never calls `addCommand` at all) -- so no MIDP
// soft-key ever exists to press it. `GameCanvas.keyPressed()`'s own
// numeric-hotkey dispatch (the same one M39/M41/M46/M62 already mapped to
// camp/interact/attack/spellcast/spellcycle/inventory) has no branch for
// it either. The ONLY confirmed code path that ever calls
// `showScreen(optionsUI)` is screenGroup 32's own `cmdOk` handler --
// i.e. the STATS screen's own "Ok" button, which requires already being
// on the Stats screen, which itself is only ever shown by picking
// "Stats" from... the Options menu. A closed loop with no entry point:
// **Stats, Skills, Spells, Save Game, Load Game, and Help are all
// completely unreachable during real gameplay in the original game**,
// and "Quit Game" -- see below -- doesn't even do what its own label
// says. This reads as a genuine shipped-game bug (almost certainly a
// missing `gameCanvas.addCommand(cmdMenu)` call, one line, never written),
// not a deliberately cut feature -- the screens themselves are fully
// built and functional, `GameSave` even already existed as of M50 with
// nothing calling it, and it would be a strange design to ship an entire
// working Save Game path a player could never reach.
//
// **The deliberate port-only exception this finding calls for, following
// the same category M62's own `openInventory()` softlock precedent
// established (`ui/inventory_ui.h`'s class comment) -- "faithful
// reproduction would require machinery this port doesn't have," not "a
// confirmed bug we're choosing to keep":** rather than reproduce total
// unreachability (which would make this whole milestone's work
// permanently dead code, including the only real Save trigger this port
// will ever have), a NEW hotkey ('P', the same "no MIDP soft-key system
// exists" class of pragmatic stand-in 'C'/'F'/'I' already are) opens this
// menu directly. This is a different flavor of exception than M62's own:
// M62 declined to reproduce a real bug (a softlock) whose faithful
// behavior would actively harm the port; here there is no real behavior
// to harm -- the original simply has no way to REACH this content at all,
// so "faithful" and "unusable" are the same thing. Making it reachable is
// closer to restoring a one-line-missing wire-up than overriding a
// confirmed design, but it is still a deliberate, player-visible
// divergence from the original's own observable behavior, so it gets the
// same explicit flag M62's own exception did.
//
// **A second, real, CONFIRMED bug preserved exactly, found while reading
// the same switch:** `optionsItems` (the menu's own label array) has
// exactly 8 entries, indices 0-7, ending in "Quit Game" at index 7 -- but
// screenGroup 31's own dispatch `switch (choice)` has a 9th case, `case
// 8:`, that opens a leftover debug form (`showDebugForm()`), and
// index 7 -- "Quit Game" itself -- instead runs `case 7`'s body, which
// opens `newCreditsUI(...)`, the SAME developer-credits screen the main
// menu's own "Credits" item shows (`ESGame.creditsText()`, a fuller
// dev-team/legal text than `ui/menu_flow.cpp`'s own `kCreditsText`, which
// comes from a different source, `UIScreen.creditsLines`). Read together:
// selecting "Quit Game" from the pause menu NEVER quits the game -- it
// shows the credits screen instead (`Cancel`/Confirm both return to
// Options from there, matching the real `nextScreen = optionsUI`, since
// `newCreditsUI`'s `backTarget` parameter is whatever screen was active
// when it's called), and the REAL "quit"/debug case (index 8) can never
// be reached at all with only 8 real menu items. Preserved exactly, not
// "fixed" to actually quit -- same "confirmed shipped bug, keep it"
// discipline as `player/game_save.h`'s own `Continue Game` neighbors and
// every other confirmed-real-bug entry in `docs/PORT_ROADMAP.md`.
//
// **WIRED, M69 (this session; was "NOT wired... the Java TRANSCRIPTION
// itself stops short"):** the `src/ESGame.java` transcription gap this
// class comment used to flag turned out to be exactly that -- a stalled
// hand-transcription, not a genuine decompiler/obfuscation gap.
// `decompiled/ESGame.java`'s own `private void a()` (the real
// `loadHelpTopicBodies()`) was always fully present and unambiguous past
// the point the Java reference tree had stopped copying it; finishing the
// copy (`helpTopicBodies[5..11]`, `Shop.dialogue[7][22..40]`) was the
// whole gap. `PauseScreen::Help` (the 12-topic list, screenGroup 203) and
// `PauseScreen::HelpTopic` (one topic's body, screenGroup 206) mirror
// Skills/SkillInfo's own list-then-detail shape, with one real, faithfully
// preserved quirk: `newHelpTopicUI(topicIndex).nextScreen = this.statsUI`
// -- leaving a help topic body (Ok OR Cancel, screenGroup 206's dispatch
// doesn't distinguish) goes to the STATS screen, not back to the topic
// list and not to Options either, the same "confirmed real bug, keep it"
// treatment "Quit Game" showing Credits instead of quitting already got
// above. Topic titles/bodies are real `ShopDialogue` (npcstrings.dat)
// text, group 7's own 41-entry pool, not literals -- `Render` now takes a
// `const ShopDialogue&` for them, the one public signature change this
// milestone makes.
//
// **Also NOT modeled, another same-category machinery gap:** the real
// `noSavedGameUI` (Load Game with no save file) and `saveErrorUI` (a
// save I/O failure) both have a `nextScreen` this port cannot reach from
// live gameplay -- `noSavedGameUI.nextScreen = mainMenuUI` (this port has
// no "return from a live game back to the main menu" transition at all;
// `ui/menu_flow.h`'s own state machine only ever runs BEFORE `gameStarted`
// flips true in main.cpp) and `saveErrorUI` sets no `nextScreen` at all
// (a likely real softlock in the original, never confirmed reachable
// mid-session either). Both port-only map back to the Options screen
// instead of the real target, on the same "the exact right machinery
// doesn't exist yet" grounds as the two exceptions above -- flagged here
// rather than silently defaulted.
enum class PauseScreen : uint8_t {
    Options,
    Stats,
    Skills,
    SkillInfo,
    Spells,
    SpellInfo,
    NoSavedGame,
    SaveError,
    Credits,
    Help,
    HelpTopic,
};

// What `PauseMenu::Confirm` just did, so `main.cpp` can run the same
// post-load fixups the "Continue Game" main-menu path already runs
// (refresh every level's tile flags, then `PlayerMovement::
// RefreshCorridorView`) -- `PauseMenu` itself has no access to `levels`/
// `levelLookup` (this module doesn't depend on `stormhold_dungeon`'s own
// per-level container shape beyond what `WorldRegistry` already gives
// it), matching this port's established "caller supplies/owns state"
// pattern (see e.g. `player/game_save.h`'s own header comment on tile-flag
// resync being the CALLER's job).
enum class PauseMenuAction { None, GameLoaded };

struct PauseMenuState {
    bool active = false;
    PauseScreen screen = PauseScreen::Options;
    int selectedIndex = 0;

    // Resolved (not row-index) targets for the two detail screens --
    // set once by Confirm() when entering SkillInfo/SpellInfo, read by
    // Render(). skillIndex: 0-13. spellIndex0Based: SpellCasting::
    // NthKnownSpellId's own return convention (0-based into
    // SpellDatabase::all, NOT a real 1-based spell id -- see that
    // method's own doc comment).
    int skillIndex = -1;
    int spellIndex0Based = -1;
    // M69: set once by Confirm() when entering HelpTopic from Help (a row
    // index into the fixed 12-topic table, NOT a `ShopDialogue` row --
    // see pause_menu.cpp's own `kHelpTitleRow`/`kHelpBodyRows`).
    int helpTopicIndex = -1;
};

class PauseMenu {
public:
    // ESGame's own optionsUI: fresh selection, first screen.
    static void Open(PauseMenuState& state);

    static void MoveSelection(PauseMenuState& state, int delta, const PlayerState& p, const SpellDatabase& spells);

    // The screenGroup 31/35/37 "cmdOk" dispatch, plus every detail
    // screen's own single "Ok" action -- see this class's own header
    // comment for the full real-vs-port-only mapping. `inventoryUi` is
    // opened directly (InventoryUi::Open) and this menu closes behind it
    // when "Inventory" is picked, matching `showScreen(inventoryUI)`
    // fully replacing the active screen in the original.
    // `world`/`shop`/`warden` are read (Save Game) or overwritten (Load
    // Game) via `GameSave`, exactly as `main.cpp`'s own "Continue Game"
    // load path already does -- neither is const here since a successful
    // Load Game replaces all three in place.
    static PauseMenuAction Confirm(PauseMenuState& state, PlayerState& p, const SpellDatabase& spells,
                                    InventoryUiState& inventoryUi, const std::string& savePath, size_t levelCount,
                                    WorldRegistry& world, ShopState& shop, WardenState& warden);

    // Detail/message screens (Stats/Skills/Spells/SkillInfo/SpellInfo/
    // NoSavedGame/SaveError/Credits) all go back to whatever screen the
    // real game's own `nextScreen`/explicit `cmdBack` branch targets (see
    // this class's own header comment for the two port-only exceptions).
    // From Options itself, matches the real screenGroup 31 `cmdBack`
    // branch: `showScreen(gameCanvas)`.
    static void Cancel(PauseMenuState& state);

    // M69: `dialogue` is only read for PauseScreen::Help/HelpTopic (the
    // topic titles/bodies, real `ShopDialogue` group-7 text) -- every
    // other screen ignores it, same as `spells` already goes unused
    // outside Spells/SpellInfo.
    static void Render(Backbuffer& bb, const PauseMenuState& state, const PlayerState& p, const CharacterData& charData,
                        const SpellDatabase& spells, const ShopDialogue& dialogue);
};

}  // namespace stormhold
