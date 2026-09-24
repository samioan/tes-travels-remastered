#pragma once
#include <cstdint>
#include <string>

#include "graphics/backbuffer.h"
#include "player/shop_interaction.h"

namespace stormhold {

// M64/M65/M66: renamed-source counterpart of ../../../src/ESGame.java's
// `npcChoicesUI[shopId]` interactive follow-up menu. M64 covered the 4
// quest shops (Arantamo/Celegil/Favela Dralor/Vander, `Shop::IsQuestShop`)
// -- `dispatchNpcChoice`'s own `case 0/1/2/3` group, `ESGame.java`'s
// screenGroups 9-12/20/22 (Train/Give/Befriend/Threaten/Kill). M65 added
// Beneca (shop 4, `case 4`, screenGroup 13/27: Give Item/Take Crystal).
// M66 (this milestone) adds Helga (shop 5, `case 5`, screenGroups
// 14/350/352/353/41/355: Rumors/Give Crystal/Enchant/Bless/Cure/Warp/
// Recovery) -- the last and biggest of the three, closing out this whole
// multi-part treatment `docs/PORT_ROADMAP.md`'s own "what's next" note
// (after M60) predicted would be needed.
//
// **The big finding M64 turned up, and this project's THIRD deliberate,
// clearly-labeled port-only exception in the same category M62's
// `openInventory` softlock and M63's whole-pause-menu-unreachable findings
// both established:** the real path from the M60 greeting screen
// (`npcHelloUI`) into this choices menu is **completely broken** for
// EVERY NPC. `talkToNpc()` sets `npcHelloUI.nextScreen =
// npcChoicesUI[npcId]` -- but `npcHelloUI`'s own screenGroup (8) is
// explicitly carved out of `commandAction()`'s big screenGroup-keyed
// dispatch chain (alongside `rumorsUI`'s screenGroup 360, see M66's own
// finding below), landing instead in the OUTERMOST catch-all: `else if
// (cmd == UIScreen.cmdOk) { showScreen(activeScreen.backTarget); }` --
// which reads `backTarget`, a COMPLETELY DIFFERENT `UIScreen` field from
// `nextScreen`, confirmed by reading `UIScreen.java`'s own field
// declarations directly. `backTarget` defaults to `null` and is
// explicitly set at only 3 call sites in the whole file (`classInfoUI`,
// and two `newGameOverUI`/`newLevelUpUI`-style screens) -- `npcHelloUI`
// is NEVER one of them. So pressing "Ok" on ANY NPC's greeting screen
// prints "ERROR: next is null!" and calls `showScreen(null)`, pausing the
// game thread with no path back. **In the real shipped game, talking to
// any NPC and pressing Ok past the very first greeting line softlocks the
// game outright.** Rather than reproduce it, `NpcChoicesMenu::Open` is
// wired directly off `nextScreen`'s own already-correct intended target
// (`main.cpp`'s own dismiss handling, now widened to shops 0-5 as of
// M66) -- honoring what the field was clearly FOR, not what the broken
// dispatch actually does with it.
//
// **M66's own big finding: the exact same root-cause bug independently
// breaks a SECOND screen, Helga's own "Rumors" popup (`rumorsUI`,
// screenGroup 360) -- and, separately, 4 of Helga's 5 remaining action
// results are their OWN kind of dead end, while 2 surprisingly work.**
// `showRumors()` (Helga's choice 0) sets `rumorsUI.nextScreen =
// npcChoicesUI[5]` -- but screenGroup 360 is excluded from the dispatch
// chain in EXACTLY the same way screenGroup 8 is (`else if
// (activeScreen.screenGroup != 8 && activeScreen.screenGroup != 360)`
// gates the entire rest of `commandAction()`), landing in the same
// `backTarget`-reading catch-all, which is ALSO never set for `rumorsUI`.
// So "Rumors" softlocks too, for the identical reason -- reproduced the
// same way (`NpcChoicesMenu` opens the Rumors line as a Result screen
// that returns to Choices, honoring `nextScreen`'s real intended target).
//
// Separately, EVERY per-action RESULT popup's own screenGroup (the third
// argument to `npcResponsePopup`) determines whether pressing its one
// real command (`cmdOk`, from `setupMessage`) does anything at all --
// confirmed by reading `commandAction()`'s own nested exclusion chains
// end to end for every one of the 11 distinct screenGroups these 3
// milestones' actions produce:
//   - 21 (Train), 23 (Give, shared by ALL shop groups), 24 (Befriend),
//     25 (Threaten), 28 (Beneca's Take Crystal), 351 (Enchant), 352
//     (Bless), 353 (Cure), 355 (Recovery): each is explicitly EXCLUDED
//     from every branch of the dispatch chain that could reach it (the
//     `!= 21 && != 23 && != 24 && != 25 && != 28` guard, or the sibling
//     `!= 351 && != 352 && != 353 && != 355` guard one level deeper),
//     with no other branch anywhere checking that exact screenGroup --
//     so `cmdOk` (the only command these screens ever actually send) has
//     NO matching handler at all. Not even the `backTarget` catch-all
//     applies here (that one is reserved for screenGroups 8/360
//     specifically) -- these 9 screens are simply inert: shown, but
//     unresponsive to their own only button.
//   - 26 (Kill) and 41 (Warp) are the two genuine exceptions: both
//     screenGroup NUMBERS happen to already be claimed by real, WORKING
//     handlers elsewhere in the dispatch chain for unrelated purposes --
//     26's own handler is a bare, UNCONDITIONAL `showScreen(gameCanvas)`
//     (fires for any command at all, unrelated to the Kill result's own
//     content), and 41's is `if (cmd == cmdOk) { player.justMarkedCamp =
//     false; showScreen(gameCanvas); }` (a real camp-mark-confirmation
//     handler, reused here purely because Warp's result happens to share
//     its screenGroup number). Both are near-certainly ACCIDENTAL id
//     reuse, not intentional shared behavior -- but the practical result
//     is real: **Kill's and Warp's own result screens genuinely work in
//     the original, closing straight back to gameplay, while the other 9
//     results (including Beneca's Give/Take Crystal and every one of
//     Helga's own except Warp) are confirmed dead ends.** `M64's OWN
//     Kill dispatch is corrected here` -- it previously (incorrectly)
//     treated Kill the same as the other port-only dead-end exceptions,
//     returning to Choices; this milestone fixes it to close the whole
//     menu instead, matching screenGroup 26's real, unconditional
//     behavior, discovered while researching Helga's own analogous
//     Warp/Bless/Cure/Recovery quartet. `NpcChoicesMenuState::resultCloses`
//     (new this milestone) tracks which of the two categories a given
//     Result screen belongs to; Confirm/Cancel branch on it identically.
//     Warp's own real side effect (`player.justMarkedCamp = false`) is
//     reproduced too, applied at Result-creation time rather than
//     Result-dismissal time -- an observably identical simplification
//     since nothing else reads that flag in between.
//
// **M65 also wired a related gap M60's own class comment left open, for
// Beneca specifically:** `talkToNpc()`'s own null-result fallback for
// npcId 4/5 -- `Shop.dialogue(player, npcId, 1, 0)` returns `null` on
// every visit after the first, and the real game re-shows
// `npcChoicesUI[npcId]` DIRECTLY in that case rather than opening an
// empty greeting. M66 now wires Helga's own identical-shaped fallback
// too (`main.cpp`'s interact-key dispatch), completing what M65 started.
//
// **A second, real, separate finding, NOT applicable to any shop this
// project has built a menu for, confirmed while reading the very same
// array:** `npcChoicesUI = new UIScreen[6]` (only 6 elements, valid
// indices 0-5) -- but Varus is shopId 6, and `Shop.dialogue(player, 6, 1,
// 0)` returns a real, non-null line once `wardenVisitCount >= 1`, so
// `talkToNpc(6)` would throw `ArrayIndexOutOfBoundsException` the first
// time a player talks to Varus after any real Warden visit. Varus has no
// real choices-menu CONTENT to reproduce even if this port built a 7th
// slot (his own dialogue ignores `action`/`extra` entirely), so this
// remains pure documentation, not a "what's next" item.
//
// **A third, minor, real finding, cheap to preserve exactly:** every
// per-action RESULT popup (`ESGame.npcResponsePopup`) calls
// `ui.setItemText(0, Shop.NAMES[shopId])` immediately followed by
// `ui.setMessageBody(result)` -- both write to the SAME single
// `StringItem`, so the shop-name write is invisible, clobbered
// immediately. The screen's real TITLE stays whatever
// `setupMessage("NPC name here", ...)` set it to at construction, so
// every popup built this way (Train/Befriend/Threaten/Kill/successful-
// Give/Bless/Cure/Warp/Recovery/Enchant/Beneca's-Take-Crystal) is
// permanently titled the literal placeholder "NPC name here". The one
// early-exit case that skips `npcResponsePopup` entirely (Give with an
// empty inventory, reusing the pre-built `npcResponseUI` object) has the
// SAME dead-write pattern against a DIFFERENT leftover placeholder,
// "Oracle". Helga's own "Rumors" popup (`rumorsUI`, ALSO pre-built, not
// from `npcResponsePopup`) has the identical dead-write pattern too,
// against ITS OWN pre-built placeholder, "Rumors" (`rumorsUI`'s own
// `setupMessage("Rumors", ...)` at startup) -- confirmed by reading
// `showRumors()` directly, the third distinct leftover placeholder title
// this system produces. All three reproduced exactly below.
//
// **A fourth, minor, real finding, also cheap to preserve:** the QUEST
// SHOPS' choices menu (`npcChoicesUI[i].setupPromptList("Name", "Aid:
// <TAG>", ...)` for `i` in 0-3) is built ONCE at startup with the literal
// placeholder title "Name", never patched afterward -- unlike
// `trainWhatMenu`/`giveWhatMenu`/`enchantWhatMenu`, which are rebuilt
// fresh on every open and correctly pass `Shop.NAMES[shopId]`. **Beneca's
// and Helga's OWN choices menus do NOT have this bug** -- both are
// confirmed, by reading their own separate constructions directly, to
// pass their real names literally (`"Beneca"`/`"Helga"`) instead of the
// generic placeholder the shops-0-3 LOOP uses. This port's own
// Choices-screen title is therefore `shopId <= 3 ? "Name" :
// Shop::kNames[shopId]`.
//
// **A fifth, real, minor finding confirmed while adding Beneca's own
// "Take Crystal" screen (M65, `takeWhatMenu`, screenGroup 27):** unlike
// `trainWhatMenu`/`giveWhatMenu`/`enchantWhatMenu` (all `nextScreen =
// gameCanvas`), `takeWhatMenu` explicitly sets `ui.nextScreen = null`,
// which routes its own Cancel press to screenGroup 27's OWN explicit
// `cmdBack` handler (shows `npcChoicesUI[shopId]`) instead of the generic
// top-level shortcut. **Cancel on "Take Crystal" genuinely, correctly
// returns to the Choices menu, while Cancel on every other sub-screen
// (including Helga's own Enchant) genuinely, correctly closes the whole
// thing straight to gameplay** -- a real behavioral asymmetry, reproduced
// exactly (`NpcChoicesMenu::Cancel`'s own `TakeWhat` case is still the
// only sub-screen that returns to Choices instead of closing).
//
// **Deliberately NOT modeled, an input-layer question this port has never
// attempted to answer for ANY screen, not just this one:** `UIScreen.
// setupList`/`setupPromptList` add `cmdSelect`/`cmdCancel` (distinct
// `Command` objects from `cmdOk`/`cmdBack`), while most of
// `commandAction()`'s own screenGroup branches check `cmdOk`/`cmdBack`
// specifically. Whether that's a real dispatch bug or resolves correctly
// through MIDP's own `List`/`ChoiceGroup` implicit-command semantics
// isn't something this project's transcribed Java source alone can
// settle -- doesn't change anything here either way, since this port has
// never modeled literal `Command`-object identity for any menu.
enum class NpcChoicesScreen : uint8_t { Choices, TrainWhat, GiveWhat, TakeWhat, EnchantWhat, Result };

struct NpcChoicesMenuState {
    bool active = false;
    // 0-5 as of M66 (see class comment) -- `main.cpp` never opens this
    // for shopId 6 (Varus's own confirmed array-bounds crash; no real
    // menu content exists for him either way).
    int shopId = -1;
    NpcChoicesScreen screen = NpcChoicesScreen::Choices;
    int selectedIndex = 0;
    // Set once by Confirm() when transitioning to Result; Render() just
    // displays these rather than re-deriving them.
    std::string resultTitle;
    std::string resultBody;
    // M66: true for the 2 confirmed-working real results (Kill, Warp) --
    // Confirm/Cancel then close the whole menu (matching screenGroup
    // 26/41's own real, working handlers) instead of the port-only
    // "return to Choices" mapping every other (confirmed-dead) result
    // uses. See class comment for the full real-vs-dead breakdown.
    bool resultCloses = false;
};

class NpcChoicesMenu {
public:
    // `main.cpp`'s own replacement for the real `npcHelloUI`/`rumorsUI`
    // dismiss -> `npcChoicesUI[shopId]` transition (see class comment for
    // why the real one is a confirmed softlock, not reproduced here).
    static void Open(NpcChoicesMenuState& state, int shopId);

    static void MoveSelection(NpcChoicesMenuState& state, int delta, const PlayerState& p);

    // Dispatches the active screen's own selection -- `dispatchNpcChoice`'s
    // `case 0/1/2/3` body (Choices, shops 0-3), `case 4` (Choices,
    // Beneca), or `case 5` (Choices, Helga); `trainWhatMenu`'s own
    // selection (`shopActionCode` -> `ShopInteraction::QuestShopDialogue`
    // action 5); `giveWhatMenu`'s own selection (`QuestShopDialogue`/
    // `BenecaDialogue`/`HelgaDialogue` action 4, `extra`=slot -- whichever
    // owns `state.shopId`); `takeWhatMenu`'s own selection
    // (`BenecaDialogue` action 7, `extra`=itemId, NOT a slot); Helga's own
    // `showRumors()`/`enchantWhatMenu` (actions 13/8); or Result's single
    // "Ok" (closes the whole menu for Kill/Warp, matching their own real,
    // working screenGroup handlers, or returns to Choices for every other
    // -- confirmed dead-end -- result; see class comment).
    // `spawnIdCounter` is `BenecaDialogue`'s own action-7 parameter (a
    // fresh item needs a fresh spawn id) -- unused for every other
    // action/screen, threaded through regardless since it's the same
    // function either way. `levels` is `HelgaDialogue`'s own Warp
    // (action 11) parameter, same "unused everywhere else, threaded
    // through anyway" reasoning -- see that method's own doc comment.
    static void Confirm(NpcChoicesMenuState& state, PlayerState& p, ShopState& shop, const ShopDialogue& text,
                         const CharacterData& charData, const ItemDatabase& items, GeneratedLevel& hub,
                         JavaRandom& rng, int16_t& spawnIdCounter, const GameAdvancement::LevelLookup& levels);

    // Choices: closes the whole menu back to live gameplay, matching the
    // real `npcChoicesUI[shopId].nextScreen = gameCanvas` exactly for
    // EVERY shop (this one direction is NOT part of the softlock --
    // screenGroup 9-14's own `cmd == cmdBack` branch reads `nextScreen`,
    // not `backTarget`). TrainWhat/GiveWhat/EnchantWhat: same real target
    // (`nextScreen = gameCanvas`), so this ALSO closes the whole menu --
    // faithful, not a simplification. TakeWhat: a real, confirmed
    // EXCEPTION -- returns to Choices instead of closing (see class
    // comment). Result: closes the whole menu when `resultCloses` is set
    // (Kill/Warp, matching their own real handlers), otherwise the
    // port-only "return to Choices" mapping every other confirmed-dead
    // result uses.
    static void Cancel(NpcChoicesMenuState& state);

    static void Render(Backbuffer& bb, const NpcChoicesMenuState& state, const PlayerState& p,
                        const CharacterData& charData, const ItemDatabase& items, const ShopState& shop);
};

}  // namespace stormhold
