#pragma once
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/help_text.h"
#include "assets/item_database.h"
#include "assets/shop_dialogue.h"
#include "assets/spell_database.h"
#include "dungeon/dungeon_runtime.h"
#include "graphics/backbuffer.h"
#include "player/player_state.h"
#include "ui/screen.h"
#include "world/dungeon_generator.h"

namespace dawnstar {

// What main.cpp itself needs to DO in response to a Select/Cancel press
// -- same shape as ui/menu_flow.h's own MenuFlowAction. M41's
// UseInventoryItem is the one action this class can't finish by itself:
// "Use" (an inventory-item's 87-99 "gift" action) needs CombatResolution::
// UseItem, which lives in dawnstar_combat -- a library this one
// (dawnstar_render) can't depend on without creating a cycle (dawnstar_combat
// already depends on dawnstar_render, for its own message-popup calls).
// So OnSelect stops short of calling it, returns this instead, and
// main.cpp (which already links both libraries) performs the real
// UseItem call itself before calling back into FinishUseItem() below --
// the same "return what happened, let main.cpp perform the actual
// real-world effect" shape MenuFlowAction/CharacterCreationAction's own
// doc comments already establish, just with an extra round-trip.
enum class OptionsMenuAction { None, ReturnToGame, Exit, UseInventoryItem };

// M39: the real IN-GAME options menu (`../src/ESGame.java`'s own
// `OptionsUI`, secondaryParam 31, opened by `GameCanvas.openOptionsMenu()`
// -- the numeric-keypad '7' key, `GameCanvas.keyPressed()`'s own `key ==
// 55` handler) -- a separate real screen graph from M38's own `MenuFlow`
// (the PRE-game main menu), reachable only once a character actually
// exists.
//
// A real, confirmed behavior this milestone had to reproduce correctly:
// `GameCanvas.run()`'s own per-tick loop takes a completely different
// branch whenever `activeScreen != null` (any real Screen, including
// `OptionsUI`, is currently showing) -- it skips `dispatchTickActions()`
// (movement/combat/camp/interact/casting, ALL of it) entirely, only
// repainting; `keyPressed()` similarly routes every key straight to
// `activeScreen.handleKey()` instead of any game key at all. So opening
// this menu doesn't just overlay the game view (like M30's message
// popup) -- it genuinely PAUSES the whole game, the same way M35's own
// camping-screen branch replaces `paintGameView()` outright, just one
// level higher (this replaces the ENTIRE tick, not just rendering).
// `main.cpp`'s own `inOptionsMenu` early-return mirrors that exactly.
//
// Reproduces, from secondaryParam==31's own real dispatch: "Stats"
// (secondaryParam 32's own `player.buildCharacterSheet()`, transcribed
// line-for-line -- see options_menu.cpp's own `BuildCharacterSheet`);
// "Clue Log" (`ClueUI`, secondaryParam 60/61, built from the real
// npcstrings.dat `ShopDialogue` (M8) plus `Player.eventFlags`/
// `traitorIndex` -- see `BuildClueEntry`'s own doc comment for the real,
// intricate "still unconfirmed vs. traitor's own admission" table lookup
// this transcribes exactly); "Help" (reopens the same real Help topic
// list M38's `MenuFlow` already models, from this menu's own entry point
// -- `this.helpUI.backTarget = this.OptionsUI` -- SIMPLIFIED: this is a
// SEPARATE `Screen` instance from `MenuFlow::helpTopics_`, not the same
// shared object the original's single `ESGame.helpUI` field really is;
// this is provably unobservable in THIS port's own control flow, since
// nothing here ever lets a player return to `MenuFlow`'s own screens
// once a game has actually started -- see main.cpp's own `inMenu`/
// `playerSlot` wiring); and "Quit Game" (`newConfirmQuitUI`, reusing the
// exact same real "either Yes or No exits" bug M38's own quit
// confirmation already faithfully reproduces).
//
// M41 additionally reproduces "Inventory" (`InventoryUI`/`InventoryItemUI`,
// secondaryParam 33/34 -- the item list, an item's own tooltip + dynamic
// Drop/Equip-or-Unequip/Learn/Use action list built in the exact same
// conditional order Player.java's own newInventoryItemUI()/dispatch use,
// so the selected index always maps back to the right action), "Skills"
// (`SkillsListUI`, secondaryParam 35/36 -- a rank>0 skill list, each
// entry's tooltip reusing the shared Info screen the same way Stats/Clue
// Log/Help already do), and "Spells" (`SpellsListUI`/`SpellInfoUI`,
// secondaryParam 37/38 -- a known-spell list, each entry's own tooltip +
// a real "Ready Spell" prompt that sets `selectedSpellId`). See
// options_menu.cpp's own `RebuildInventoryList`/`RebuildInventoryItem`
// doc comments for the real, easy-to-miss `suppressStrafeAdjust` quirk
// (using item 87, "Warp to Camp", from the Inventory screen exits
// straight back to the game view, not back to the item list) and this
// class's own `OptionsMenuAction::UseInventoryItem`/`FinishUseItem` doc
// comments for why "Use" alone needs an extra round-trip through
// main.cpp.
//
// Deliberately DEFERRED as real, silent no-ops (same "Continue Game"
// precedent M38 already established), since each needs a whole system
// this port hasn't built yet: "Save Game"/"Load Game" (real file I/O
// plus `LoadingScreen`'s own background-thread machinery -- `PlayerSave`
// only (de)serializes to/from an in-memory buffer so far, see
// docs/PORT_ROADMAP.md's M12/M17 entries); "Reveal Traitor" (secondaryParam
// 68/65/66's own multi-screen "who is the traitor?" mini-quiz, which on a
// correct guess calls `grantStarFrostItem()` and sets `newGamePlus`/
// `ambushTimer` -- none of which are ported: see player/player_state.h's
// own `starFrostBonusActive`/`traitorSuspicionCount` doc comments).
class OptionsMenu {
public:
    OptionsMenu(HelpText helpText, ShopDialogue shopDialogue);

    // Routed to whichever Screen is currently active -- same "caller
    // edge-detects the physical key" contract as MenuFlow's own OnUp/
    // OnDown.
    void OnUp();
    void OnDown();

    // The real Select command. Needs the CURRENT player/character/item/
    // spell data (unlike HelpText/ShopDialogue, static tables snapshotted
    // at construction) to build "Stats"/"Clue Log"/"Inventory"/"Skills"/
    // "Spells" text fresh every time they're actually opened, so all are
    // passed in here rather than stored. `player` is mutable (unlike
    // M39's original const reference) since M41's Drop/Equip/Unequip/
    // Learn inventory actions and "Ready Spell" all genuinely mutate it
    // -- `levels`/`world` are needed too, for Drop (DungeonRuntime::
    // AddDroppedItem) -- but NOT `MonsterDatabase`/`JavaRandom`: "Use"
    // never runs here at all, see OptionsMenuAction::UseInventoryItem's
    // own doc comment.
    OptionsMenuAction OnSelect(PlayerState& player, const CharacterData& charData, const ItemDatabase& items,
                               const SpellDatabase& spells, std::vector<GeneratedLevel>& levels, WorldRegistry& world);

    // The real Cancel/Back command (both map to this port's own single
    // "back" key -- same simplification MenuFlow::OnCancel already
    // makes, see its own doc comment).
    OptionsMenuAction OnCancel();

    // Only meaningful right after OnSelect returns UseInventoryItem: the
    // inventory slot "Use" was chosen for.
    int PendingUseItemSlot() const { return pendingUseItemSlot_; }

    // Completes an inventory "Use" action main.cpp already performed
    // (the real CombatResolution::UseItem call) -- Player.java's own
    // secondaryParam==34 tail: back to the game view if the item just
    // used set `player.suppressStrafeAdjust` (item 87, "Warp to Camp" --
    // clearing the flag right back to false the same way the original
    // does), otherwise back to a freshly-rebuilt Inventory list with the
    // same slot still selected.
    OptionsMenuAction FinishUseItem(PlayerState& player, const ItemDatabase& items);

    void Render(Backbuffer& bb) const;

private:
    enum class Active { Options, ClueLog, Help, Info, QuitConfirm, InventoryList, InventoryItem, SkillsList,
                         SpellsList, SpellInfo };

    Screen& ActiveScreen();
    const Screen& ActiveScreen() const;

    void RebuildInventoryList(const PlayerState& player, const ItemDatabase& items);
    void RebuildInventoryItem(const PlayerState& player, const CharacterData& charData, const ItemDatabase& items,
                              const SpellDatabase& spells, int slot);
    void RebuildSkillsList(const PlayerState& player, const CharacterData& charData);
    void RebuildSpellsList(const PlayerState& player, const SpellDatabase& spells);
    // The tail shared by every inventory-item action (Drop/Equip/Unequip/
    // Learn, and -- via FinishUseItem -- Use): Player.java's own
    // `if (character.suppressStrafeAdjust) {...} else { rebuild + reopen
    // InventoryUI }` at the end of secondaryParam==34's dispatch.
    OptionsMenuAction FinishInventoryItemAction(PlayerState& player, const ItemDatabase& items);

    Active active_ = Active::Options;
    // Screen.backTarget's own real role for the one Screen this flow
    // reuses for four different real purposes (Stats/a Clue Log entry/
    // a Help topic's own body/a Skill's own tooltip) -- same "hardcoded
    // per real dispatch branch, not a stored Screen::backTarget field"
    // reasoning as MenuFlow::infoBackTarget_ (see ui/screen.h's own class
    // comment on why that field isn't ported).
    Active infoBackTarget_ = Active::Options;

    HelpText helpText_;
    ShopDialogue shopDialogue_;
    Screen options_;
    Screen clueLog_;
    Screen helpTopics_;
    Screen info_;
    Screen quitConfirm_;

    // M41: unlike options_/clueLog_/helpTopics_ above (each a single
    // real Screen instance Player.java's own ESGame constructs ONCE and
    // reuses, so a stale selectedIndex_ genuinely persists across visits
    // -- see options_menu.cpp's own BuildClueEntry-adjacent test-bug note
    // in docs/PORT_ROADMAP.md's M39 entry), `InventoryUI`/`SkillsListUI`/
    // `SpellsListUI` are each rebuilt FRESH from scratch on every real
    // (re)entry in the original (`this.InventoryUI = this.newInventoryUI();`,
    // a brand new Screen every time) -- so these members are fully
    // REPLACED (assigned a fresh `Screen(ScreenMode::PromptList)`) by
    // Rebuild*, not just reconfigured in place, to reproduce that same
    // "selection always starts fresh unless explicitly restored via
    // SetSelectedIndex" real behavior.
    Screen inventoryList_;
    Screen inventoryItem_;
    Screen skillsList_;
    Screen spellsList_;
    Screen spellInfo_;
    // ESGame.currentItemIndex/currentSpellIndex -- which inventory slot/
    // known-spell index InventoryItemUI/SpellInfoUI's own single-item
    // action dispatch (secondaryParam 34/38) applies to, since neither
    // screen carries that itself.
    int currentItemIndex_ = -1;
    int currentSpellIndex_ = -1;
    // Set only when OnSelect returns UseInventoryItem; read back by
    // main.cpp via PendingUseItemSlot() before it calls FinishUseItem().
    int pendingUseItemSlot_ = -1;
};

}  // namespace dawnstar
