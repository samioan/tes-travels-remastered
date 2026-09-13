#include "player/player_camp.h"

#include <cstdlib>

#include "player/player_combat_stats.h"
#include "player/player_inventory.h"
#include "player/player_movement.h"

namespace dawnstar {

void PlayerCamp::Rest(PlayerState& p, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                       const ItemDatabase& items, JavaRandom& globalRng, bool fullRest) {
    PlayerMovement::CleanupRoamingMonsterIfPresent(p, levels, world);

    int hpGain = p.coreStats[3] - p.coreStats[2];
    int magickaGain = p.coreStats[5] - p.coreStats[4];
    int fatigueGain = p.coreStats[7] - p.coreStats[6];
    if (!fullRest) {
        hpGain = 2 * hpGain / 3;
        magickaGain = 2 * magickaGain / 3;
        fatigueGain = 2 * fatigueGain / 3;
    }

    p.coreStats[9] = 0;
    p.coreStats[8] = 0;
    if (PlayerCombatStats::HasAilment(p, 8)) {
        hpGain = 3 * hpGain / 4;
        magickaGain = 3 * magickaGain / 4;
        fatigueGain = 3 * fatigueGain / 4;
    }

    p.coreStats[2] = static_cast<int16_t>(p.coreStats[2] + hpGain);
    p.coreStats[4] = static_cast<int16_t>(p.coreStats[4] + magickaGain);
    p.coreStats[6] = static_cast<int16_t>(p.coreStats[6] + fatigueGain);
    p.increaseHarmBuff = false;
    p.increaseArmorBuff = false;
    p.safeCampingBuff = false;

    if (LingoRandomInt(globalRng, 100) <= 10) {
        for (int slot = 0; slot < p.inventoryCount; slot++) {
            int itemId = std::abs(static_cast<int>(p.inventoryItemIds[slot]));
            if (itemId == 96) {
                PlayerInventory::RemoveSlot(p, items, slot);
                break;
            }
        }
    }

    for (int ailment = 0; ailment < 8; ailment++) {
        int num = ailment + 1;
        if (num != 4 && num != 5) {
            if (LingoRandomInt(globalRng, 100) <= 25) {
                p.ailmentMask = static_cast<int8_t>(p.ailmentMask & ~(1 << ailment));
            }
        }
    }
}

}  // namespace dawnstar
