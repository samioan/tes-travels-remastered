#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/shop_dialogue.h"
#include "dungeon/dungeon_runtime.h"
#include "npc/shop_interaction.h"
#include "player/player_state.h"
#include "render/message_popup.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace dawnstar {

// Renamed-source counterpart of GameCanvas.processInteract() -- the
// player-initiated interact action (key '9' in the original, gated on
// hotbarContext == 2, same as attack's own key/context gate). Kept in
// its own module (the same reason combat/combat_tick.h itself is): it
// needs BOTH dawnstar_player (PlayerInventory::AddItem) and
// dawnstar_render (render/message_popup.h), plus dawnstar_dungeon
// (DungeonRuntime::RemoveChest/AddDroppedItem) and (M45) dawnstar_npc
// (ShopInteraction::Dialogue) -- none of which may depend on either of
// the others, so combining them anywhere else would either cycle or
// force main.cpp to duplicate untested logic inline. See
// docs/PORT_ROADMAP.md's M34/M45 entries.
class InteractTick {
public:
    // GameCanvas.processInteract(): opens the NPC-in-front's dialogue,
    // or loots the chest in front, whichever's actually present
    // (npcInSight takes priority, matching the original exactly).
    //
    // The npcInSight branch (M45) calls the real ShopInteraction::
    // Dialogue(..., action=1, extra=0) -- GameCanvas.openNpcDialogue()'s
    // own `Shop.dialogue(player, shopId, 1, 0)` call -- and returns its
    // result for main.cpp to show as a blocking message screen (this
    // module has no Screen of its own to paint into, same "logic here,
    // display there" split render/message_popup.h's own chest-loot
    // popups already use one level down). A nullopt return means either
    // nothing was in sight, or dialogue() itself returned null (Jakar's
    // own "I have nothing new to say" case, or the shopId 5-8/action==8
    // fallthrough -- see shop_interaction.h's own doc comment) -- either
    // way, nothing to show.
    //
    // NOT reproduced: the real game's own Ok dispatch on that popup
    // always proceeds into `NPCChoicesUI[shopId]`, a full buy/sell/
    // quest-turn-in/rumor-question list-menu this port has no
    // counterpart for yet (see shop_interaction.h's own class comment).
    // Dismissing this port's popup just returns to the game instead --
    // the same "the branch exists, but does less than the original
    // until its own UI milestone lands" reasoning M32's monsterType-42
    // end-of-game-UI skip already used.
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
    static std::optional<std::string> ProcessInteract(PlayerState& player, std::vector<GeneratedLevel>& levels,
                                                        WorldRegistry& world, const ItemDatabase& items,
                                                        const CharacterData& charData, const ShopDialogue& dialogue,
                                                        ShopState& shop, MessagePopupState& messagePopup,
                                                        JavaRandom& globalRng, int16_t& nextItemSpawnId,
                                                        int64_t nowMs);
};

}  // namespace dawnstar
