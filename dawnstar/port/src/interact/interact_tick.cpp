#include "interact/interact_tick.h"

#include <array>
#include <cstdlib>

#include "player/player_inventory.h"
#include "player/player_movement.h"

namespace dawnstar {

void InteractTick::ProcessInteract(PlayerState& player, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                                    const ItemDatabase& items, MessagePopupState& messagePopup, int64_t nowMs) {
    if (player.npcInSight >= 0) {
        // openNpcDialogue(): not ported yet -- see this method's own doc
        // comment in interact_tick.h.
        return;
    }

    if (!player.chestInSight) return;

    const std::array<uint8_t, 8>* chestPtr = PlayerMovement::ChestInFront(player, levels, world);
    if (chestPtr == nullptr) return;  // chestInSight should already guarantee this; defensive only.
    const std::array<uint8_t, 8>& chest = *chestPtr;

    GeneratedLevel& level = levels[static_cast<size_t>(player.currentLevel - 1)];
    // Player.pickUpDroppedItem()'s own `rec[2] = 2;` write is dead code
    // (see dungeon/dungeon_runtime.h's own WorldRegistry doc comment --
    // nothing ever reads a chest record's [2] byte back), so this port
    // skips constructing a mutated copy just to discard it via
    // RemoveChest below.
    int itemId = chest[4];
    int packed = (chest[5] << 8) + chest[6];
    int charge = chest[7];

    if (PlayerInventory::AddItem(player, itemId, packed, charge)) {
        DungeonRuntime::RemoveChest(level, world, chest);
        int itemIdx = itemId - 1;
        if (items.category[static_cast<size_t>(itemIdx)] == 11) {
            player.giftPointsFound = static_cast<int16_t>(player.giftPointsFound + items.subtype[static_cast<size_t>(itemIdx)]);
        }

        player.chestInSight = false;
        player.minimapDirty = true;

        int slot = player.inventoryCount - 1;
        int foundItemId = std::abs(static_cast<int>(player.inventoryItemIds[slot]));
        MessagePopup::Show(messagePopup, MessagePopup::WrapToTwoLines(items.name[static_cast<size_t>(foundItemId - 1)]),
                            -1, nowMs);
    } else {
        std::array<uint8_t, 7> floorRec = {chest[0], chest[1], chest[4], chest[5], chest[6], chest[7], 1};
        DungeonRuntime::AddDroppedItem(level, world, floorRec);
        DungeonRuntime::RemoveChest(level, world, chest);
        MessagePopup::Show(messagePopup, {"Inventory", "full!"}, -1, nowMs);
    }
}

}  // namespace dawnstar
