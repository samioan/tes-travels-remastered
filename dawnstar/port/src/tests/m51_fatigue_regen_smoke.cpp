// M51 smoke test: PlayerCombatStats::TickFatigueRegen (player/
// player_combat_stats.h) and the CombatTick::ProcessAttack/
// ProcessSpellCast `actionTaken` out-parameters main.cpp's own
// `actionTakenThisTick` gate now reads -- GameCanvas.processIdleTick()'s
// `tickFatigueRegen`/`!actionTakenThisTick` half, the other piece M50
// left out (see docs/PORT_ROADMAP.md's M50 and M51 entries).
//
// No JVM ground truth (same reason as every prior milestone).
// TickFatigueRegen itself is checked against a hand-derived transcription
// of ../../../src/Player.java's tickFatigueRegen(elapsedMs); the
// `actionTaken` out-parameters' own gating is already covered end-to-end
// by m32_combat_tick_smoke.cpp/m33_spellcast_tick_smoke.cpp (updated
// alongside this milestone), so this file only re-derives
// TickFatigueRegen's own formula.
#include <cstdio>
#include <string>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/item_database.h"
#include "player/player_combat_stats.h"
#include "player/player_creation.h"
#include "util/java_random.h"

namespace {

using dawnstar::PlayerCombatStats;
using dawnstar::PlayerCreation;
using dawnstar::PlayerState;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        dawnstar::CharacterData charData = dawnstar::CharacterData::Load(archive);
        dawnstar::JavaRandom globalRng(51);

        // --- A: hand-derived gain, well under max ---
        {
            PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            p.attributes[10] = 40;
            p.attributes[11] = 20;
            p.coreStats[6] = 0;
            p.coreStats[7] = 1000;
            // gain = elapsedMs * (40+20) / 2000 = elapsedMs * 60 / 2000.
            PlayerCombatStats::TickFatigueRegen(p, 2000);
            Check(p.coreStats[6] == 60, "2000ms * (40+20)/2000 should gain exactly 60 fatigue");
        }

        // --- B: integer truncation, matching Java's own (int) cast ---
        {
            PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            p.attributes[10] = 1;
            p.attributes[11] = 0;
            p.coreStats[6] = 0;
            p.coreStats[7] = 1000;
            // gain = 999 * 1 / 2000 = 0 (truncated, not rounded).
            PlayerCombatStats::TickFatigueRegen(p, 999);
            Check(p.coreStats[6] == 0, "a sub-threshold elapsed time should truncate to zero gain, not round up");
        }

        // --- C: capped at max, even with a huge elapsed time ---
        {
            PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            p.attributes[10] = 100;
            p.attributes[11] = 100;
            p.coreStats[6] = 50;
            p.coreStats[7] = 60;
            PlayerCombatStats::TickFatigueRegen(p, 60000);
            Check(p.coreStats[6] == 60, "regen should never push current Fatigue past max");
        }

        // --- D: zero elapsed time is a pure no-op ---
        {
            PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            p.coreStats[6] = 25;
            PlayerCombatStats::TickFatigueRegen(p, 0);
            Check(p.coreStats[6] == 25, "zero elapsed time should not change Fatigue");
        }

        // --- E: already at (or somehow above) max stays put ---
        {
            PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            p.attributes[10] = 50;
            p.attributes[11] = 50;
            p.coreStats[7] = 40;
            p.coreStats[6] = 40;
            PlayerCombatStats::TickFatigueRegen(p, 5000);
            Check(p.coreStats[6] == 40, "already at max should stay at max, never exceed it");
        }

        if (g_ok) {
            std::printf("fatigue_regen_smoke: all checks passed\n");
            return 0;
        } else {
            std::printf("fatigue_regen_smoke: FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("fatigue_regen_smoke: exception: %s\n", e.what());
        return 2;
    }
}
