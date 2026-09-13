#pragma once
#include <cstdint>
#include <vector>

#include "assets/item_database.h"
#include "dungeon/dungeon_runtime.h"
#include "player/player_state.h"
#include "render/message_popup.h"
#include "world/dungeon_generator.h"

namespace dawnstar {

// Renamed-source counterpart of GameCanvas.processInteract() -- the
// player-initiated interact action (key '9' in the original, gated on
// hotbarContext == 2, same as attack's own key/context gate). Kept in
// its own module (the same reason combat/combat_tick.h itself is): it
// needs BOTH dawnstar_player (PlayerInventory::AddItem) and
// dawnstar_render (render/message_popup.h), plus dawnstar_dungeon
// (DungeonRuntime::RemoveChest/AddDroppedItem) -- none of which may
// depend on either of the others, so combining them anywhere else would
// either cycle or force main.cpp to duplicate untested logic inline.
// See docs/PORT_ROADMAP.md's M34 entry.
class InteractTick {
public:
    // GameCanvas.processInteract(): opens the NPC-in-front's dialogue,
    // or loots the chest in front, whichever's actually present
    // (npcInSight takes priority, matching the original exactly).
    //
    // The npcInSight branch is a deliberate no-op stand-in --
    // openNpcDialogue() needs Shop.dialogue() (only npcstrings.dat's
    // raw text is loaded so far, M8 -- not the actual line-selection
    // logic) plus a whole GenericInfoUI screen, neither of which this
    // port has yet. Same "the branch exists, but does nothing until its
    // own UI milestone lands" reasoning as M32's monsterType-42
    // end-of-game-UI skip.
    //
    // The chest-loot half (Player.pickUpDroppedItem()) is fully ported,
    // including two preserved real quirks: (1) the `result == -1`
    // ("Chest locked!") branch in the original is dead code --
    // pickUpDroppedItem() never actually returns -1 given its own
    // current body, so this port has nothing to reproduce there
    // either; (2) when a chest record's itemId low byte is 86 (world/
    // dungeon_generator.h's own "extended itemId" encoding, where the
    // record's own last byte holds the real id's high byte), the
    // original passes the literal byte 86 straight into
    // addInventoryItem() -- it never resolves the record's own high
    // byte back into the real extended id here, unlike
    // DungeonGenerator's own write side that encoded it. A real,
    // silently-inert bug, ported exactly rather than "fixed".
    static void ProcessInteract(PlayerState& player, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                                 const ItemDatabase& items, MessagePopupState& messagePopup, int64_t nowMs);
};

}  // namespace dawnstar
