#pragma once
#include "assets/character_data.h"
#include "player/player_state.h"

namespace stormhold {

// GameCanvas.paintStatusBars() (../../../src/GameCanvas.java lines
// 969-987, was decompiled/e.java's a(Graphics), M22)'s own width
// computations, separated from pixel drawing -- same "selection logic
// first, pixels later" split render/corridor_render_plan.h (M21) already
// uses. Three colored bars -- HP (coreStats[2]/[3]), Magicka ([4]/[5]),
// Fatigue ([6]/[7]) -- each PlayerCombatStats::EffectiveStat() scaled to
// the SAME 38px-wide track (`stat * 38 / max`).
//
// Deliberately entirely self-contained in PlayerState + CharacterData
// (PlayerCombatStats::EffectiveStat's own requirement) -- no ItemDatabase,
// no assets, no world state -- same "cheapest next slice" the M25
// entry's own "what's next" note already flagged this pair of methods as.
struct StatusBarPlan {
    int hpWidth = 0;
    int magickaWidth = 0;
    // A real, confirmed asymmetry vs. hpWidth/magickaWidth above,
    // preserved rather than "fixed": the original clamps ONLY this one
    // to 40 (the track's own outer width, 2px wider than the 38px fill
    // area) even though EffectiveStat's own internal clamp to the
    // matching max stat means none of the three should ever naturally
    // exceed 38 in practice. Reachable only if `coreStats[6]` (current
    // Fatigue) is ever above `coreStats[7]` (max Fatigue) while effect 23
    // is NOT active -- EffectiveStat only clamps while that effect is
    // active, so this guards a data state the original evidently
    // considered possible, not dead code being pedantically preserved.
    int fatigueWidth = 0;

    static StatusBarPlan Plan(const PlayerState& p, const CharacterData& charData);
};

}  // namespace stormhold
