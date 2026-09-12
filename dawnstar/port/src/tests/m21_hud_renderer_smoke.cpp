// M21 smoke test: HudRenderer::PaintStatusBars (render/hud_renderer.h)
// and PlayerCombatStats::EffectiveStat (player/player_combat_stats.h)
// against a real created character (M11's PlayerCreation), the same
// methodology as M14/M16 -- no JVM ground truth is available (see
// player/player_movement.h's header comment for why that's still true
// here). Independently re-derives GameCanvas.paintStatusBars()'s exact
// pixel math (bar backgrounds, fill widths, colors) from
// ../../../src/GameCanvas.java/Player.java rather than reusing
// hud_renderer.h's own formula, then reads the rendered Backbuffer back
// pixel-by-pixel to confirm they match -- including forcing the
// "Regeneration"-style buff (effect 23) active to exercise
// effectiveStat()'s only real branch, and forcing a stat above its own
// max to confirm the HP/Magicka bars' fill overflows past the 40px
// background with no clamp while the Fatigue bar's clamps at 40 (a real
// difference in the original, not a bug to "fix" -- see
// hud_renderer.h's doc comment).
#include <cstdio>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/item_database.h"
#include "graphics/backbuffer.h"
#include "player/player_combat_stats.h"
#include "player/player_creation.h"
#include "render/hud_renderer.h"
#include "util/java_random.h"

namespace {

using dawnstar::Backbuffer;
using dawnstar::CharacterData;
using dawnstar::HudRenderer;
using dawnstar::ItemDatabase;
using dawnstar::PackRGB565;
using dawnstar::PlayerCombatStats;
using dawnstar::PlayerState;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

// Counts how many consecutive pixels starting at (x,y) match `color`,
// scanning rightward -- used to measure a fill bar's actual rendered
// width back out of the Backbuffer.
int MeasureFillWidth(const Backbuffer& bb, int x, int y, uint16_t color, int maxScan) {
    int width = 0;
    for (int i = 0; i < maxScan; i++) {
        // Backbuffer has no public per-pixel getter beyond Data() --
        // read the row directly, same layout FrameRenderer/tests
        // elsewhere already rely on (row-major, kWidth stride).
        uint16_t px = bb.Data()[y * Backbuffer::kWidth + (x + i)];
        if (px != color) break;
        width++;
    }
    return width;
}

// Independent transcription of Player.java's effectiveStat(index),
// written straight from the source rather than copied from
// player_combat_stats.cpp, to actually cross-check that file's version.
int ExpectedEffectiveStat(const PlayerState& p, const CharacterData& charData, int index, bool effect23Active) {
    int value = p.coreStats[static_cast<size_t>(index)];
    if (effect23Active) {
        int skill10 = PlayerCombatStats::SkillValue(p, charData, 10, false);
        if (index == 2) {
            value += skill10;
            if (value > p.coreStats[3]) value = p.coreStats[3];
        } else if (index == 4) {
            value += skill10;
            if (value > p.coreStats[5]) value = p.coreStats[5];
        } else if (index == 6) {
            value += skill10;
            if (value > p.coreStats[7]) value = p.coreStats[7];
        }
    }
    return value;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        CharacterData charData = CharacterData::Load(archive);
        ItemDatabase items = ItemDatabase::Load(archive);
        dawnstar::JavaRandom globalRng(777);

        const uint16_t kBarBg = PackRGB565(255, 255, 0);
        const uint16_t kHpFill = PackRGB565(255, 0, 0);
        const uint16_t kMagickaFill = PackRGB565(0, 255, 0);
        const uint16_t kFatigueFill = PackRGB565(0, 0, 255);

        // --- baseline: a fresh character, no buffs active ---
        PlayerState p = dawnstar::PlayerCreation::CreateCharacter(0, "Test", charData, items, globalRng);
        Backbuffer bb;
        HudRenderer::PaintStatusBars(bb, p, charData);

        int expectedHp = ExpectedEffectiveStat(p, charData, 2, false) * 38 / p.coreStats[3];
        int expectedMagicka = ExpectedEffectiveStat(p, charData, 4, false) * 38 / p.coreStats[5];
        int expectedFatigue = ExpectedEffectiveStat(p, charData, 6, false) * 38 / p.coreStats[7];

        std::printf("baseline: hpFill=%d magickaFill=%d fatigueFill=%d (of 38 max)\n", expectedHp, expectedMagicka,
                    expectedFatigue);

        Check(MeasureFillWidth(bb, 6, 133, kHpFill, 60) == expectedHp, "HP bar fill width should match hand-derivation");
        Check(MeasureFillWidth(bb, 6, 141, kMagickaFill, 60) == expectedMagicka,
              "Magicka bar fill width should match hand-derivation");
        Check(MeasureFillWidth(bb, 6, 149, kFatigueFill, 60) == expectedFatigue,
              "Fatigue bar fill width should match hand-derivation");

        // Background bars should show through past each fill's end (all
        // 3 bars are 40px backgrounds, so at x=6 that's 40 px of
        // possible background/fill, matching the smoke test's own
        // MeasureFillWidth bound of 40 below).
        Check(bb.Data()[133 * Backbuffer::kWidth + (6 + expectedHp)] == kBarBg,
              "background should show through right past the HP fill's end");

        // --- effect 23 ("Regeneration") active: effectiveStat's only
        // real branch ---
        p.effectDurations[22] = -1;  // effect 23, "active until cured"
        Backbuffer bb2;
        HudRenderer::PaintStatusBars(bb2, p, charData);

        int expectedHpBuffed = ExpectedEffectiveStat(p, charData, 2, true) * 38 / p.coreStats[3];
        int expectedMagickaBuffed = ExpectedEffectiveStat(p, charData, 4, true) * 38 / p.coreStats[5];
        int expectedFatigueBuffed = ExpectedEffectiveStat(p, charData, 6, true) * 38 / p.coreStats[7];
        int skill10 = PlayerCombatStats::SkillValue(p, charData, 10, false);

        std::printf("effect-23-active: skillValue(10)=%d hpFill=%d magickaFill=%d fatigueFill=%d\n", skill10,
                    expectedHpBuffed, expectedMagickaBuffed, expectedFatigueBuffed);

        Check(MeasureFillWidth(bb2, 6, 133, kHpFill, 60) == expectedHpBuffed,
              "buffed HP bar fill width should match hand-derivation");
        Check(MeasureFillWidth(bb2, 6, 141, kMagickaFill, 60) == expectedMagickaBuffed,
              "buffed Magicka bar fill width should match hand-derivation");
        Check(MeasureFillWidth(bb2, 6, 149, kFatigueFill, 60) == expectedFatigueBuffed,
              "buffed Fatigue bar fill width should match hand-derivation");
        if (skill10 > 0) {
            Check(expectedHpBuffed >= expectedHp, "the buff should never reduce the HP fill");
        }

        // --- HP/Magicka overflow past 40px with no clamp; Fatigue
        // clamps at 40 -- forcing coreStats above their own max ---
        p.effectDurations[22] = 0;
        p.coreStats[2] = static_cast<int16_t>(p.coreStats[3] * 3);  // HP way above maxHP
        p.coreStats[4] = static_cast<int16_t>(p.coreStats[5] * 3);  // Magicka way above maxMagicka
        p.coreStats[6] = static_cast<int16_t>(p.coreStats[7] * 3);  // Fatigue way above maxFatigue
        Backbuffer bb3;
        HudRenderer::PaintStatusBars(bb3, p, charData);

        int overflowHp = MeasureFillWidth(bb3, 6, 133, kHpFill, 170);
        int overflowMagicka = MeasureFillWidth(bb3, 6, 141, kMagickaFill, 170);
        int overflowFatigue = MeasureFillWidth(bb3, 6, 149, kFatigueFill, 170);
        std::printf("overflow: hpFill=%d magickaFill=%d fatigueFill=%d (background is only 40px wide)\n", overflowHp,
                    overflowMagicka, overflowFatigue);

        Check(overflowHp > 40, "an over-max HP stat should overflow the HP bar's 40px background, uncapped");
        Check(overflowMagicka > 40, "an over-max Magicka stat should overflow the Magicka bar's 40px background, uncapped");
        Check(overflowFatigue == 40, "an over-max Fatigue stat should clamp the Fatigue bar's fill at exactly 40px");

        if (g_ok) {
            std::printf("all HUD status-bar checks passed\n");
            return 0;
        } else {
            std::printf("SOME CHECKS FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("EXCEPTION: %s\n", e.what());
        return 2;
    }
}
