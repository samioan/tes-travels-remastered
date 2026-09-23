#include "player/movement_messages.h"

#include <cstdlib>

namespace stormhold {

MovementMessageResult MovementMessages::Resolve(const PlayerState& p, int8_t inventoryCountBefore,
                                                  const DungeonNames& names, const ItemDatabase& items) {
    MovementMessageResult result;

    if (p.pendingLockedItemFlag) {
        result.lockedItemEndOfGame = true;
    } else if (p.crossingLevelBoundary || p.enteredNewLevelZone || p.leftLevelZone) {
        if (p.enteredNewLevelZone) {
            result.crossingMessage = std::array<std::string, 2>{"Warden's", "Camp"};
        } else if (p.leftLevelZone) {
            // Confirmed permanently unreachable (player/player_movement.h's
            // own M10 finding) -- preserved, not deleted.
            result.crossingMessage = std::array<std::string, 2>{"Outer", "Camp"};
        } else {
            result.crossingMessage = names.DisplayNames(p.currentLevel);
        }
    }

    int itemsFound = p.inventoryCount - inventoryCountBefore;
    if (itemsFound == 1) {
        result.itemsFound = ItemsFoundKind::One;
        int8_t id = p.inventoryItemIds[static_cast<size_t>(p.inventoryCount - 1)];
        result.foundItemName = items.name[static_cast<size_t>(std::abs(id) - 1)];
    } else if (itemsFound > 1) {
        result.itemsFound = ItemsFoundKind::Several;
    }

    return result;
}

}  // namespace stormhold
