#pragma once
#include <string>
#include <vector>

#include "graphics/backbuffer.h"
#include "player/player_inventory.h"

namespace stormhold {

// M62: the last branch of M41's dispatch web -- `ESGame.java`'s
// `newInventoryUI()` (screenGroup 5/33, the item-list screen) and
// `newInventoryItemUI()` (screenGroup 5/34, the per-slot Drop/Equip-or-
// Unequip/Learn/Use action menu), plus `GameCanvas.openInventory()`'s own
// hotkey dispatch (`unconfirmed_Z`, real key '7') -- all built on M61's
// already-ported `PlayerInventory` action logic
// (`ItemTooltip`/`CanEquipOrUnequip`/`IsEquippedSlot`(M62)/
// `CanLearnSpellFromScroll`/`LearnSpellFromScroll`/`CanUseItem`/`UseItem`).
// Same two-screen shape `ui/menu_flow.h`'s own list screens use, kept as
// its own separate file rather than folded into MenuFlow -- this is a
// live-gameplay modal (like `ui/npc_dialogue.h`), not a pre-game flow
// step, same reasoning that file's own class comment already gives for
// duplicating small rendering pieces rather than sharing them.
//
// **A real, confirmed bug, deliberately NOT reproduced (needs machinery
// this port doesn't have, same class of gap as `ui/npc_dialogue.h`'s own
// un-reproduced `npcChoicesUI` fallback):** the real `openInventory()`
// does NOT call `newInventoryUI()` at all -- it just does
// `this.game.showScreen(this.game.inventoryUI)`, showing whatever's
// CACHED in that field. `game.inventoryUI` is ONLY ever assigned by the
// in-game pause screen's own "Inventory" menu item (screenGroup 31 case
// 1, itself not built anywhere in this port -- see docs/PORT_ROADMAP.md's
// own "what's next" note) or by the 4 action-menu branches that refresh
// it after Drop/Equip/Learn/Use. So in the real game, pressing the
// inventory hotkey BEFORE ever opening the pause screen's Inventory tab
// at least once that session leaves `game.inventoryUI == null`, and
// `showScreen(null)` falls through every branch of its own instanceof
// checks -- pausing the game thread and disabling auto-repaint with no
// screen shown and no path back, a real softlock reachable from a
// completely ordinary fresh New Game. This is confirmed by direct
// exhaustive grep of every `inventoryUI =` assignment in `ESGame.java`
// (only those 5 sites exist) cross-referenced against `showScreen()`'s own
// body. Reproducing it faithfully would require first building the whole
// not-yet-existing pause/stats menu AND its own "populates a cache the
// hotkey path doesn't" quirk -- out of scope for this milestone, same
// "deliberately not reproduced" call M60 already made for its own
// UI-machinery gap. Instead, `Open` below always builds the list fresh --
// which is also what 4 of the real game's 5 `inventoryUI =` call sites
// already do (only the buggy hotkey entry point skips it), so this is the
// DOMINANT real behavior, not an invented one.
enum class InventoryScreen {
    List,
    ItemAction,
};

struct InventoryUiState {
    bool active = false;
    InventoryScreen screen = InventoryScreen::List;
    // ESGame's own UIScreen cursor position -- meaningful for whichever of
    // the two screens is current.
    int selectedIndex = 0;
    // ESGame.selectedInventorySlot -- the inventory slot the ItemAction
    // screen is about; -1 when not on that screen.
    int selectedSlot = -1;
    // newInventoryItemUI()'s own `labels`/`codes` Vectors, rebuilt by
    // `Confirm` every time a slot is selected off the List screen.
    std::vector<std::string> actionLabels;
    std::vector<int> actionCodes;
};

class InventoryUi {
public:
    // GameCanvas.openInventory()'s port-only stand-in (see this file's own
    // class comment for the real bug this deliberately does NOT reproduce)
    // -- always (re)opens straight to the fresh item list.
    static void Open(InventoryUiState& state);

    // UIScreen's own up/down cursor movement -- clamped, not wrapped,
    // matching `ui/menu_flow.h`'s own MoveSelection convention. A no-op
    // when inactive or the current list is empty.
    static void MoveSelection(InventoryUiState& state, int delta, const PlayerState& p);

    // The single "Ok" action, keyed on `state.screen` the same way
    // `commandAction()` keys on `activeScreen.screenGroup`: screenGroup
    // 33's "pick a slot -> open its action menu" (`newInventoryItemUI`),
    // or screenGroup 34's actual Drop/Equip-or-Unequip/Learn/Use dispatch.
    // `useItem()`'s real `target` argument is always `null` at its one
    // confirmed call site (M61's own finding) -- passed as `nullptr` here
    // to match exactly, not a port-only simplification.
    //
    // **Confirmed dead branch, not reproduced as a live fork:**
    // `Player.endOfGameTriggered` is declared, read here in the original,
    // but never confirmed SET true anywhere in `useItem()`'s own switch
    // (see `player_inventory.h`'s own UseItem comment) -- so the real
    // "close the whole screen back to gameplay" branch that field would
    // gate is unreachable dead code. This always takes the other (live,
    // reachable) branch: rebuild the list and stay on it.
    // `levels` is `PlayerInventory::UseItem`'s own parameter, needed only
    // by the camp-marker item (id 87) to close the `MarkCampAndReturnToTown`/
    // `WarpToCampMark` stale-corridor-view gap -- see that method's own
    // header comment. `level` (the player's CURRENT level, used by
    // Drop/DropInventoryItem) is unrelated and kept as its own parameter.
    static void Confirm(InventoryUiState& state, PlayerState& p, const ItemDatabase& items,
                         const SpellDatabase& spells, const MonsterDatabase& monsters, GeneratedLevel& level,
                         WorldRegistry& world, JavaRandom& rng, const GameAdvancement::LevelLookup& levels);

    // The single "Cancel"/"Back" action. From the ItemAction screen, goes
    // back to the List screen (`nextScreen = this.inventoryUI`, real
    // behavior). From the List screen, the real game's own `nextScreen`
    // is the not-yet-built pause menu (`this.optionsUI`) -- since that
    // screen doesn't exist in this port, the deliberate port-only mapping
    // here is closing the whole inventory view back to live gameplay,
    // same "no fallback screen exists, so don't show one" precedent
    // `ui/npc_dialogue.h`'s own class comment already used.
    static void Cancel(InventoryUiState& state);

    // Paints whichever of the two screens is current -- REPLACES the
    // normal corridor/HUD/minimap frame entirely while `state.active`,
    // matching `inventoryUI`/`inventoryActionUI` fully taking over
    // `Display.setCurrent()` in the original.
    static void Render(Backbuffer& bb, const InventoryUiState& state, const PlayerState& p, const ItemDatabase& items,
                        const SpellDatabase& spells, const CharacterData& charData);
};

}  // namespace stormhold
