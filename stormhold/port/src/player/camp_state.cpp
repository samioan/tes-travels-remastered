#include "player/camp_state.h"

#include "player/player_combat_stats.h"

namespace stormhold {

void Camping::Start(CampState& camp, const PlayerState& player, int64_t now) {
    camp.state = 1;
    if (player.safeCampingBuff) {
        camp.state = 2;
    }

    if (player.currentLevel == 1) {
        camp.state = 2;
    }

    camp.rollAtMs = now;
}

bool Camping::RollInterrupted(JavaRandom& rng) { return RandomInt1Based(rng, 10) == 1; }

CampTickResult Camping::Tick(CampState& camp, PlayerState& player, GeneratedLevel& level, WorldRegistry& world,
                              const ItemDatabase& items, const MonsterDatabase& monsterDb, JavaRandom& rng,
                              int16_t& spawnIdCounter, int64_t now) {
    if (camp.state == 1) {
        if (now - camp.rollAtMs <= 2500) return CampTickResult::StillWaiting;

        if (RollInterrupted(rng)) {
            camp.state = 0;
            DungeonRuntime::SpawnAmbushMonsterNearPlayer(level, world, player.tileX, player.tileY, rng, monsterDb,
                                                          spawnIdCounter);
            camp.rollAtMs = 0;
            PlayerCombatStats::ApplyRestRecovery(player, false, items, rng);
            return CampTickResult::Disturbed;
        }

        camp.state = 2;
        return CampTickResult::StillWaiting;
    }

    if (camp.state == 2) {
        if (now - camp.rollAtMs <= 5000) return CampTickResult::StillWaiting;

        camp.state = 0;
        camp.rollAtMs = 0;
        PlayerCombatStats::ApplyRestRecovery(player, true, items, rng);
        return CampTickResult::Complete;
    }

    return CampTickResult::NotCamping;
}

}  // namespace stormhold
