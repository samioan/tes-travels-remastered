#include "world/warden.h"

namespace stormhold {

bool WardenState::ShouldVisit(int elapsedCounter) const {
    if (visitCount == 0 && !present && elapsedCounter >= 13) return true;
    if (visitCount == 1 && !present && elapsedCounter >= 26) return true;
    if (visitCount == 2 && !present && elapsedCounter >= 39) return true;
    return false;
}

void WardenState::Arrive(GeneratedLevel& hub) {
    present = true;
    visitCount++;
    hub.tiles[kShopX][kShopY] = static_cast<uint8_t>(hub.tiles[kShopX][kShopY] | 32);
}

void WardenState::Leave(GeneratedLevel& hub, const GeneratedLevel& level2) {
    present = false;
    uint8_t current = level2.tiles[kShopX][kShopY];
    hub.tiles[kShopX][kShopY] = static_cast<uint8_t>(current & ~uint8_t{32});
}

}  // namespace stormhold
