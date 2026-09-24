#include "player/death_sequence.h"

#include "player/player_combat_stats.h"
#include "player/player_creation.h"
#include "player/player_inventory.h"

namespace stormhold {

bool DeathSequence::TickDeathAndRegen(PlayerState& p, DeathState& death, const CharacterData& charData, int64_t now,
                                       int64_t deltaMs) {
    int hp = PlayerCombatStats::EffectiveStat(p, charData, 2);
    if (hp <= 0) {
        death.phase = 2;
        death.deathAtMs = now;
        return true;
    }

    PlayerCombatStats::TickFatigueRegen(p, deltaMs);
    return false;
}

DeathTickResult DeathSequence::Tick(PlayerState& p, DeathState& death, const ItemDatabase& items, int64_t now) {
    if (death.phase == 1) {
        return DeathTickResult::Alive;
    }

    if (death.phase == 2) {
        death.phase = 3;
        // unconfirmed_ad=false/messagePriority=0 message-clear NOT
        // modeled -- see this method's own header comment.
    }

    if (now - death.deathAtMs <= 5000) {
        return DeathTickResult::Waiting;
    }

    PlayerCreation::NormalizeToMaxStats(p.coreStats);

    for (int slot = p.inventoryCount - 1; slot >= 0; slot--) {
        if (!PlayerInventory::IsSlotEquipped(p, slot, items)) {
            PlayerInventory::RemoveInventorySlot(p, slot, items);
        }
    }

    PlayerCreation::RespawnAfterDeath(p);

    death.deathAtMs = 0;
    death.phase = 1;
    return DeathTickResult::Respawned;
}

std::array<std::string, 2> DeathSequence::RespawnMessageLines(const PlayerState& p, const DungeonNames& names) {
    if (p.enteredNewLevelZone) {
        return {"Warden's", "Camp"};
    }
    if (p.leftLevelZone) {
        // Confirmed permanently unreachable (player/player_movement.h's
        // own M10 finding) -- preserved, not deleted.
        return {"Outer", "Camp"};
    }
    return names.DisplayNames(p.currentLevel);
}

}  // namespace stormhold
