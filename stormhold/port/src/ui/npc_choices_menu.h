#pragma once
#include <cstdint>
#include <string>

#include "graphics/backbuffer.h"
#include "player/shop_interaction.h"

namespace stormhold {

// M64: renamed-source counterpart of ../../../src/ESGame.java's
// `npcChoicesUI[shopId]` interactive follow-up menu for the 4 quest shops
// (Arantamo/Celegil/Favela Dralor/Vander, `Shop::IsQuestShop`) -- the
// Train/Give/Befriend/Threaten/Kill menu `ui/npc_dialogue.h`'s own class
// comment (M60) flagged as a substantially bigger, separate lift, and
// `docs/PORT_ROADMAP.md`'s own "what's next" note said would likely need
// its own dawnstar-M46-sized multi-part treatment. This milestone is part
// 1 of that: shops 0-3 only (`dispatchNpcChoice`'s own `case 0/1/2/3`
// group, `ESGame.java`'s screenGroups 9-12/20/22). Beneca (shop 4, `case
// 4`, screenGroup 13/27) and Helga (shop 5, `case 5`, screenGroup
// 14/350) are their own bespoke menus, deliberately deferred to a follow-
// up milestone, the same "one coherent slice at a time" split M56-M59's
// own quartet already used for the dialogue TEXT side of this same gap.
//
// **The big finding this milestone turned up, and this project's THIRD
// deliberate, clearly-labeled port-only exception in the same category
// M62's `openInventory` softlock and M63's whole-pause-menu-unreachable
// findings both established:** the real path from the M60 greeting screen
// (`npcHelloUI`) into this choices menu is **completely broken** for
// EVERY NPC, not just shops 0-3. `talkToNpc()` sets `npcHelloUI.nextScreen
// = npcChoicesUI[npcId]` -- but `npcHelloUI`'s own screenGroup (8) is
// explicitly carved out of `commandAction()`'s big screenGroup-keyed
// dispatch chain (alongside `rumorsUI`'s screenGroup 360), landing instead
// in the OUTERMOST catch-all: `else if (cmd == UIScreen.cmdOk) {
// showScreen(activeScreen.backTarget); }` -- which reads `backTarget`, a
// COMPLETELY DIFFERENT `UIScreen` field from `nextScreen`, confirmed by
// reading `UIScreen.java`'s own field declarations directly (both fields
// exist, both are real, they are NOT aliases of each other). `backTarget`
// defaults to `null` (`UIScreen`'s own constructor) and is explicitly set
// at only 3 call sites in the whole file -- `classInfoUI`, and two
// `newGameOverUI`/`newLevelUpUI`-style screens (screenGroups 200/201,
// `ESGame.java` lines ~2224/2234) -- `npcHelloUI` is NEVER one of them.
// So pressing "Ok" on ANY NPC's greeting screen prints "ERROR: next is
// null!" to debug output and then calls `showScreen(null)` -- which,
// per `ui/inventory_ui.h`'s own class comment on `showScreen(Object)`'s
// full body (the exact same method, re-read again here to confirm this
// finding rather than assumed from memory), pauses the game thread and
// shows nothing, with no path back. **In the real shipped game, talking
// to any NPC and pressing Ok past the very first greeting line softlocks
// the game outright** -- the entire `npcChoicesUI` system (Train/Give/
// Befriend/Threaten/Kill, Beneca's item exchange, Helga's whole bespoke
// menu) is confirmed UNREACHABLE, exactly the same "faithful reproduction
// would make real, fully-built, clearly-intended content permanently dead
// in this port too" situation M62/M63 already made peace with. Rather
// than reproduce it, `NpcChoicesMenu::Open` is wired directly off
// `nextScreen`'s own already-correct intended target (`main.cpp`'s own
// dismiss handling for shops 0-3) -- honoring what the field was clearly
// FOR, not what the broken dispatch actually does with it.
//
// **A second, real, separate finding, NOT applicable to this milestone's
// own shops 0-3 scope but confirmed while reading the very same array:**
// `npcChoicesUI = new UIScreen[6]` (`ESGame.java`, only 6 elements, valid
// indices 0-5) -- but Varus is shopId 6. `Shop.dialogue(player, 6, 1, 0)`
// (Varus's own greeting) returns a real, non-null line once
// `wardenVisitCount >= 1` (`Shop.java`'s own shopId-6 switch case, the
// same state machine `ShopInteraction::VarusDialogue`, M59, already
// ports), so `talkToNpc(6)`'s own `npcChoicesUI[npcId]` read would throw
// `ArrayIndexOutOfBoundsException` the very first time a player talks to
// Varus after the Warden has visited at least once -- a genuine crash bug
// in the original, independent of and in addition to the softlock above.
// Not modeled either way: Varus has no real choices-menu CONTENT to
// reproduce even if this port built a 7th array slot (his own dialogue
// ignores `action`/`extra` entirely, per `VarusDialogue`'s own doc
// comment), so there is nothing for a future milestone to actually build
// here -- documented for completeness, not as a "what's next" item.
//
// **A third, minor, real finding, cheap to preserve exactly:** every
// per-action RESULT popup (`ESGame.npcResponsePopup`) calls
// `ui.setItemText(0, Shop.NAMES[shopId])` immediately followed by
// `ui.setMessageBody(result)` -- both write to the SAME single `StringItem`
// (`UIScreen.setItemText`'s own doc comment confirms `setItemText(0, ...)`
// on a mode-4 message screen and `setMessageBody` both target
// `form.get(0)`), so the shop-name write is invisible, clobbered
// immediately by the dialogue result. The screen's real TITLE (a
// SEPARATE Form-level property, `Form(title)`'s own constructor arg) is
// never touched after `setupMessage("NPC name here", ...)` sets it at
// construction, so every Train/Befriend/Threaten/Kill/successful-Give
// result window is permanently titled the literal placeholder "NPC name
// here" -- reproduced exactly below, not "fixed" to show the real name.
// The one early-exit case that skips `npcResponsePopup` entirely (Give
// with an empty inventory, reusing the pre-built `npcResponseUI` object
// directly) has the SAME dead-write pattern against a DIFFERENT
// leftover placeholder title, "Oracle" (`npcResponseUI`'s own
// `setupMessage("Oracle", ...)` at startup, apparently reused from
// elsewhere and never renamed) -- also reproduced exactly.
//
// **A fourth, minor, real finding, also cheap to preserve:** the choices
// menu itself (`npcChoicesUI[i].setupPromptList("Name", "Aid: <TAG>", ...)`)
// is built ONCE at startup with the literal placeholder title "Name" --
// unlike `trainWhatMenu`/`giveWhatMenu`, which are rebuilt fresh on every
// open and correctly pass `Shop.NAMES[shopId]` as their own title, nothing
// ever patches `npcChoicesUI[i]`'s own title afterward. Reproduced exactly
// (this menu's title is the literal string "Name" for all 4 shops, not
// each shop's real name) -- "Train What?"/"Give What?" DO show the real
// shop name, since those two are freshly built per-open with the correct
// value from the start.
//
// **Deliberately NOT modeled, an input-layer question this port has never
// attempted to answer for ANY screen, not just this one:** `UIScreen.
// setupList`/`setupPromptList` add `cmdSelect`/`cmdCancel` (distinct
// `Command` objects from `cmdOk`/`cmdBack`, confirmed via `UIScreen.java`'s
// own static initializer), while most of `commandAction()`'s own
// screenGroup branches -- including this menu's own Train/Give sub-screen
// dispatch (screenGroups 20/22) -- check `cmd == UIScreen.cmdOk`/`cmdBack`
// specifically. Whether that's a real dispatch bug or resolves correctly
// through MIDP's own `List`/`ChoiceGroup` implicit-command semantics
// (platform behavior this project's transcribed Java source alone can't
// settle) is a genuinely open question -- but it doesn't change anything
// here either way, since this port has NEVER modeled literal `Command`-
// object identity for ANY menu (every prior milestone's own UI, back to
// `ui/menu_flow.cpp`'s M40 debut, already abstracts every screen's real
// command down to this port's own uniform Enter="confirm"/Escape="cancel"
// convention). Noted here only because this milestone's own research
// happened to turn it up, not as a new precedent or a new gap.
enum class NpcChoicesScreen : uint8_t { Choices, TrainWhat, GiveWhat, Result };

struct NpcChoicesMenuState {
    bool active = false;
    // 0-3 only this milestone (see class comment) -- `main.cpp` never
    // opens this for shopId 4/5/6.
    int shopId = -1;
    NpcChoicesScreen screen = NpcChoicesScreen::Choices;
    int selectedIndex = 0;
    // Set once by Confirm() when transitioning to Result; Render() just
    // displays these rather than re-deriving them.
    std::string resultTitle;
    std::string resultBody;
};

class NpcChoicesMenu {
public:
    // `main.cpp`'s own replacement for the real `npcHelloUI` dismiss ->
    // `npcChoicesUI[shopId]` transition (see class comment for why the
    // real one is a confirmed softlock, not reproduced here).
    static void Open(NpcChoicesMenuState& state, int shopId);

    static void MoveSelection(NpcChoicesMenuState& state, int delta, const PlayerState& p);

    // Dispatches the active screen's own selection -- `dispatchNpcChoice`'s
    // `case 0/1/2/3` body (Choices), `trainWhatMenu`'s own selection
    // (`shopActionCode` -> `ShopInteraction::QuestShopDialogue` action 5),
    // `giveWhatMenu`'s own selection (action 4, `extra`=slot), or Result's
    // single "Ok" (returns to Choices -- a deliberate port-only mapping,
    // see class comment: the real result popups have no reachable
    // `nextScreen`/`backTarget` of their own at all).
    static void Confirm(NpcChoicesMenuState& state, PlayerState& p, ShopState& shop, const ShopDialogue& text,
                         const CharacterData& charData, const ItemDatabase& items, GeneratedLevel& hub,
                         JavaRandom& rng);

    // Choices: closes the whole menu back to live gameplay, matching the
    // real `npcChoicesUI[shopId].nextScreen = gameCanvas` exactly (this
    // one direction is NOT part of the softlock -- screenGroup 9-12's own
    // `cmd == cmdBack` branch reads `nextScreen`, not `backTarget`).
    // TrainWhat/GiveWhat: same real target (`nextScreen = gameCanvas`),
    // so this ALSO closes the whole menu, not just backing up one screen
    // -- faithful, not a simplification. Result: same port-only mapping
    // as Confirm above (back to Choices).
    static void Cancel(NpcChoicesMenuState& state);

    static void Render(Backbuffer& bb, const NpcChoicesMenuState& state, const PlayerState& p,
                        const CharacterData& charData, const ItemDatabase& items, const ShopState& shop);
};

}  // namespace stormhold
