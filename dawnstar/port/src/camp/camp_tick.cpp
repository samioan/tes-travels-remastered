#include "camp/camp_tick.h"

#include "player/player_camp.h"

namespace dawnstar {

void CampTick::TryEnterCamp(PlayerState& player, JavaRandom& globalRng, int64_t nowMs, int64_t& campStartTimeMs,
                            MessagePopupState& messagePopup, bool monsterAttacking) {
    if (monsterAttacking) {
        MessagePopup::Show(messagePopup, {"Cannot", "Camp!"}, 1, nowMs);
        return;
    }

    player.campState = 1;
    if (player.safeCampingBuff) {
        player.campState = 2;
    }
    if (!player.specialEncounterResolved && player.coreStats[0] > 3 && LingoRandomInt(globalRng, 10) == 1) {
        player.campState = 3;
    }
    if (player.currentLevel == 1) {
        player.campState = 2;
    }

    campStartTimeMs = nowMs;
}

bool CampTick::TickCampState(PlayerState& player, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                             const ItemDatabase& items, const MonsterDatabase& monsterDb, JavaRandom& globalRng,
                             int64_t nowMs, int64_t& campStartTimeMs, int16_t& nextMonsterSpawnId,
                             MessagePopupState& messagePopup, bool& suppressMoveThisTick) {
    if (player.campState == 1 || player.campState == 3) {
        if (nowMs - campStartTimeMs <= 2500) return false;

        if (player.campState != 3 && LingoRandomInt(globalRng, 10) != 1) {
            player.campState = 2;
            return false;
        }

        campStartTimeMs = 0;
        suppressMoveThisTick = true;
        PlayerCamp::Rest(player, levels, world, items, globalRng, false);
        int forcedTypeOrSentinel = player.campState == 3 ? 41 : -1;
        DungeonRuntime::TrySpawnMonsterNear(levels, world, player.currentLevel - 1, player.tileX, player.tileY,
                                             forcedTypeOrSentinel, globalRng, monsterDb, nextMonsterSpawnId);
        player.campState = 0;
        MessagePopup::Show(messagePopup, {"Rest", "disturbed!"}, 1, nowMs);
        return true;
    }

    if (player.campState == 2) {
        if (nowMs - campStartTimeMs <= 5000) return false;

        player.campState = 0;
        campStartTimeMs = 0;
        suppressMoveThisTick = true;
        PlayerCamp::Rest(player, levels, world, items, globalRng, true);
        MessagePopup::Show(messagePopup, {"Rest", "complete!"}, 1, nowMs);
        return true;
    }

    return true;
}

}  // namespace dawnstar
