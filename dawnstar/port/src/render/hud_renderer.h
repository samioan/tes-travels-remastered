#pragma once
#include "assets/character_data.h"
#include "graphics/backbuffer.h"
#include "player/player_combat_stats.h"
#include "player/player_state.h"

namespace dawnstar {

// Renamed-source counterpart of GameCanvas.paintStatusBars() -- the
// Health(red)/Magicka(green)/Fatigue(blue) 3-bar HUD meter, each over a
// shared yellow background bar. Out of scope: paintHotbar()/
// paintMessagePopup()/paintVisibleObjects()/the minimap (GameCanvas.
// paintGameView()'s other calls) -- see docs/PORT_ROADMAP.md's M21
// entry. The only module here that needs both a Backbuffer (render/)
// and PlayerCombatStats::EffectiveStat (player/), so dawnstar_render
// gained a dependency on dawnstar_player for this one method -- no
// cycle results, since nothing in player/ or world/ depends back on
// render/.
class HudRenderer {
public:
    // Ported exactly, including one real quirk: the HP and Magicka
    // bars' fill width has no upper clamp against the 40px background
    // (so a stat that's temporarily above its own max -- e.g. M16's
    // documented unclamped-Magicka-refund spellcasting bug -- draws a
    // fill past the bar's background), while the Fatigue bar alone
    // explicitly clamps its fill to 40px. Not a rendering bug to "fix"
    // -- the original really does treat the third bar differently from
    // the first two.
    static void PaintStatusBars(Backbuffer& bb, const PlayerState& p, const CharacterData& charData) {
        constexpr uint16_t kBarBg = PackRGB565(255, 255, 0);
        constexpr uint16_t kHpFill = PackRGB565(255, 0, 0);
        constexpr uint16_t kMagickaFill = PackRGB565(0, 255, 0);
        constexpr uint16_t kFatigueFill = PackRGB565(0, 0, 255);

        bb.FillRect(5, 130, 40, 7, kBarBg);
        bb.FillRect(5, 138, 40, 7, kBarBg);
        bb.FillRect(5, 146, 40, 7, kBarBg);

        int hpFill = PlayerCombatStats::EffectiveStat(p, charData, 2) * 38 / p.coreStats[3];
        bb.FillRect(6, 131, hpFill, 5, kHpFill);

        int magickaFill = PlayerCombatStats::EffectiveStat(p, charData, 4) * 38 / p.coreStats[5];
        bb.FillRect(6, 139, magickaFill, 5, kMagickaFill);

        int fatigueFill = PlayerCombatStats::EffectiveStat(p, charData, 6) * 38 / p.coreStats[7];
        if (fatigueFill > 40) fatigueFill = 40;
        bb.FillRect(6, 147, fatigueFill, 5, kFatigueFill);
    }
};

}  // namespace dawnstar
