#include "render/status_bar_plan.h"

#include "player/player_combat_stats.h"

namespace stormhold {

StatusBarPlan StatusBarPlan::Plan(const PlayerState& p, const CharacterData& charData) {
    StatusBarPlan result;
    result.hpWidth = PlayerCombatStats::EffectiveStat(p, charData, 2) * 38 / p.coreStats[3];
    result.magickaWidth = PlayerCombatStats::EffectiveStat(p, charData, 4) * 38 / p.coreStats[5];

    int fatigueWidth = PlayerCombatStats::EffectiveStat(p, charData, 6) * 38 / p.coreStats[7];
    if (fatigueWidth > 40) fatigueWidth = 40;
    result.fatigueWidth = fatigueWidth;

    return result;
}

}  // namespace stormhold
