// M35 smoke test: CampTick::TryEnterCamp/TickCampState (camp/
// camp_tick.h) and PlayerCamp::Rest (player/player_camp.h) --
// GameCanvas.enterCampState()/run()'s own per-tick campState 1/2/3
// state machine, and Player.rest(). No JVM ground truth is available
// (same reason as every prior milestone) -- verified against the real
// 37-level generated world (M6/M24) and a real created character (M11),
// with the probabilistic interruption/ailment-cure/safe-camping-scroll
// rolls either forced via a probe RNG seed (for the ones this test
// needs a SPECIFIC outcome from, the same technique M14's rollOutcome
// tests established) or checked as outcome-independent invariants
// (M32/M33's own established philosophy) where forcing isn't needed.
#include <cstdio>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "camp/camp_tick.h"
#include "dungeon/dungeon_runtime.h"
#include "player/player_camp.h"
#include "player/player_creation.h"
#include "render/message_popup.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::CampTick;
using dawnstar::GeneratedLevel;
using dawnstar::MessagePopupState;
using dawnstar::PlayerCamp;
using dawnstar::PlayerCreation;
using dawnstar::PlayerState;
using dawnstar::WorldRegistry;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

// A seed whose FIRST LingoRandomInt(rng, 10) call returns the wanted
// value (1..10) -- same "probe a fresh JavaRandom until it produces the
// roll this test needs" technique as M14's rollOutcome tests.
int64_t FindSeedForFirstRoll10(int wanted) {
    for (int64_t seed = 1; seed < 100000; seed++) {
        dawnstar::JavaRandom probe(seed);
        if (dawnstar::LingoRandomInt(probe, 10) == wanted) return seed;
    }
    return 1;  // unreachable in practice
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        dawnstar::MonsterDatabase monsterDb = dawnstar::MonsterDatabase::Load(archive);
        dawnstar::CharacterData charData = dawnstar::CharacterData::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);

        std::vector<GeneratedLevel> levels;
        WorldRegistry world(geometry.rows.size());
        for (size_t i = 0; i < geometry.rows.size(); i++) {
            int levelNumber = static_cast<int>(i) + 1;
            levels.push_back(levelNumber == 1
                                  ? dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[0])
                                  : dawnstar::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[i], items,
                                                                               monsterDb));
            dawnstar::DungeonRuntime::RegisterGeneratedSpawns(levels.back(), world);
        }
        std::printf("generated + registered %zu levels\n", levels.size());

        // A real non-hub level, and a walkable interior tile on it (any
        // tile with all 4 cardinal neighbors in-bounds and not a wall/
        // monster/blocked, so TrySpawnMonsterNear has real room to work
        // with).
        GeneratedLevel& level = levels[1];
        int px = -1, py = -1;
        for (int x = 2; x < level.width - 2 && px < 0; x++) {
            for (int y = 2; y < level.height - 2; y++) {
                uint8_t t = level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)];
                if ((t & (1 | 2 | 32)) != 0) continue;
                px = x;
                py = y;
                break;
            }
        }
        Check(px >= 0, "should find a walkable interior tile on a real non-hub level");

        dawnstar::JavaRandom globalRng(111);
        int16_t nextMonsterSpawnId = 1;

        // --- A: TryEnterCamp -- monsterAttacking short-circuits into
        // "Cannot Camp!" and touches nothing else ---
        {
            PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            player.currentLevel = level.number;
            player.tileX = px;
            player.tileY = py;
            player.campState = 0;
            int64_t campStartTimeMs = -999;
            MessagePopupState popup;
            CampTick::TryEnterCamp(player, globalRng, 1000, campStartTimeMs, popup, true);
            Check(popup.visible && popup.lines[0] == "Cannot" && popup.lines[1] == "Camp!" && popup.priority == 1,
                  "monsterAttacking should show \"Cannot/Camp!\" at priority 1");
            Check(player.campState == 0, "monsterAttacking should leave campState untouched");
            Check(campStartTimeMs == -999, "monsterAttacking should leave campStartTimeMs untouched");
        }

        // --- B: TryEnterCamp -- an ordinary non-hub camp with no
        // buffs/high level (low character level, so the special-roll
        // condition's own coreStats[0] > 3 guard alone rules it out --
        // no RNG forcing needed) enters campState 1 ---
        {
            PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            player.currentLevel = level.number;
            player.tileX = px;
            player.tileY = py;
            player.coreStats[0] = 1;  // character level 1: coreStats[0] > 3 is false
            player.safeCampingBuff = false;
            int64_t campStartTimeMs = 0;
            MessagePopupState popup;
            CampTick::TryEnterCamp(player, globalRng, 2000, campStartTimeMs, popup, false);
            Check(player.campState == 1, "an ordinary non-hub, non-buffed, low-level camp should enter campState 1");
            Check(campStartTimeMs == 2000, "TryEnterCamp should stamp campStartTimeMs to nowMs");
            Check(!popup.visible, "entering camp normally shows no popup of its own");
        }

        // --- C: TryEnterCamp -- safeCampingBuff skips straight to
        // campState 2 ---
        {
            PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            player.currentLevel = level.number;
            player.coreStats[0] = 1;
            player.safeCampingBuff = true;
            int64_t campStartTimeMs = 0;
            MessagePopupState popup;
            CampTick::TryEnterCamp(player, globalRng, 3000, campStartTimeMs, popup, false);
            Check(player.campState == 2, "safeCampingBuff should enter campState 2");
        }

        // --- D: TryEnterCamp -- the hub town (level 1) always ends at
        // campState 2, even overriding a forced campState-3 roll ---
        {
            PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            player.currentLevel = 1;  // hub town
            player.coreStats[0] = 10;
            player.specialEncounterResolved = false;
            int64_t campStartTimeMs = 0;
            MessagePopupState popup;
            // A rng whose OWN first NextInt() call is TryEnterCamp's
            // special-encounter roll -- freshly seeded here rather than
            // reused from character creation (which would have already
            // consumed calls off it, probing the wrong position).
            dawnstar::JavaRandom forcedHitRng(FindSeedForFirstRoll10(1));
            CampTick::TryEnterCamp(player, forcedHitRng, 4000, campStartTimeMs, popup, false);
            Check(player.campState == 2,
                  "the hub town should always end at campState 2, even overriding a forced campState-3 roll");
        }

        // --- E: TryEnterCamp -- the rare scripted campState 3, forced
        // via a probe seed whose first roll lands the 1-in-10 hit ---
        {
            PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            player.currentLevel = level.number;  // NOT the hub
            player.coreStats[0] = 10;             // > 3
            player.specialEncounterResolved = false;
            player.safeCampingBuff = false;
            int64_t campStartTimeMs = 0;
            MessagePopupState popup;
            dawnstar::JavaRandom forcedHitRng(FindSeedForFirstRoll10(1));
            CampTick::TryEnterCamp(player, forcedHitRng, 5000, campStartTimeMs, popup, false);
            Check(player.campState == 3, "a forced 1-in-10 roll (level>3, not yet resolved, not the hub) should enter campState 3");
        }

        // --- F: TickCampState -- campState 1, before the 2500ms wait
        // has elapsed: a hard no-op ---
        {
            PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            player.currentLevel = level.number;
            player.tileX = px;
            player.tileY = py;
            player.campState = 1;
            int64_t campStartTimeMs = 1000;
            MessagePopupState popup;
            bool suppress = false;
            bool runTick = CampTick::TickCampState(player, levels, world, items, monsterDb, globalRng, 2000,
                                                    campStartTimeMs, nextMonsterSpawnId, popup, suppress);
            Check(!runTick, "campState 1 before 2500ms should keep runTick false");
            Check(player.campState == 1, "campState 1 before 2500ms should not change campState");
            Check(!popup.visible, "campState 1 before 2500ms should show no popup");
            Check(!suppress, "campState 1 before 2500ms should not touch suppressMoveThisTick");
        }

        // --- G: TickCampState -- campState 1, past 2500ms, NOT
        // interrupted (a seed whose first roll is anything but 1) ->
        // transitions to campState 2, still no rest/popup yet ---
        {
            int64_t seedNotHit = 1;  // seed 1's own first LingoRandomInt(10) is checked below to confirm != 1
            dawnstar::JavaRandom probe(seedNotHit);
            Check(dawnstar::LingoRandomInt(probe, 10) != 1, "setup: seed 1 should not hit the 1-in-10 roll");

            dawnstar::JavaRandom rngNotHit(seedNotHit);
            PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            player.currentLevel = level.number;
            player.tileX = px;
            player.tileY = py;
            player.campState = 1;
            int64_t campStartTimeMs = 0;
            MessagePopupState popup;
            bool suppress = false;
            bool runTick = CampTick::TickCampState(player, levels, world, items, monsterDb, rngNotHit, 2600,
                                                    campStartTimeMs, nextMonsterSpawnId, popup, suppress);
            Check(!runTick, "an uninterrupted campState-1 resolution should keep runTick false (still camping)");
            Check(player.campState == 2, "an uninterrupted campState-1 resolution should transition to campState 2");
            Check(!popup.visible, "transitioning to campState 2 shows no popup of its own");
            Check(!suppress, "transitioning to campState 2 should not suppress movement (camp isn't over yet)");
        }

        // --- H: TickCampState -- campState 1, past 2500ms, interrupted
        // (forced roll==1) -> resolves fully: rest(false), a monster
        // spawn attempt, "Rest disturbed!", suppressMoveThisTick ---
        {
            int64_t seedHit = FindSeedForFirstRoll10(1);
            dawnstar::JavaRandom rngHit(seedHit);
            PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            player.currentLevel = level.number;
            player.tileX = px;
            player.tileY = py;
            player.campState = 1;
            player.coreStats[2] = static_cast<int16_t>(player.coreStats[3] / 2);  // half HP, so rest has something to restore
            int16_t hpBefore = player.coreStats[2];
            int64_t campStartTimeMs = 0;
            MessagePopupState popup;
            bool suppress = false;
            bool runTick = CampTick::TickCampState(player, levels, world, items, monsterDb, rngHit, 2600,
                                                    campStartTimeMs, nextMonsterSpawnId, popup, suppress);
            Check(runTick, "an interrupted campState-1 resolution should set runTick true (the tick's other work still runs)");
            Check(player.campState == 0, "an interrupted camp should end at campState 0");
            Check(campStartTimeMs == 0, "an interrupted camp should reset campStartTimeMs to 0");
            Check(suppress, "an interrupted camp should set suppressMoveThisTick true");
            Check(popup.visible && popup.lines[0] == "Rest" && popup.lines[1] == "disturbed!" && popup.priority == 1,
                  "an interrupted camp should show \"Rest/disturbed!\" at priority 1");
            Check(player.coreStats[2] > hpBefore, "an interrupted camp should still apply a partial rest (HP increased)");
        }

        // --- I: TickCampState -- campState 3 ALWAYS resolves
        // interrupted, even with a roll that would NOT have hit for a
        // campState-1 camp ---
        {
            dawnstar::JavaRandom rngNotHit(1);
            PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            player.currentLevel = level.number;
            player.tileX = px;
            player.tileY = py;
            player.campState = 3;
            int64_t campStartTimeMs = 0;
            MessagePopupState popup;
            bool suppress = false;
            bool runTick = CampTick::TickCampState(player, levels, world, items, monsterDb, rngNotHit, 2600,
                                                    campStartTimeMs, nextMonsterSpawnId, popup, suppress);
            Check(runTick && player.campState == 0, "campState 3 should always resolve interrupted regardless of the roll");
            Check(popup.visible && popup.lines[1] == "disturbed!", "campState 3's resolution shows the same disturbed popup");
        }

        // --- J: TickCampState -- campState 2, before the 5000ms wait
        // has elapsed: a hard no-op ---
        {
            PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            player.currentLevel = level.number;
            player.tileX = px;
            player.tileY = py;
            player.campState = 2;
            int64_t campStartTimeMs = 1000;
            MessagePopupState popup;
            bool suppress = false;
            bool runTick = CampTick::TickCampState(player, levels, world, items, monsterDb, globalRng, 3000,
                                                    campStartTimeMs, nextMonsterSpawnId, popup, suppress);
            Check(!runTick && player.campState == 2 && !popup.visible && !suppress,
                  "campState 2 before 5000ms should be a hard no-op");
        }

        // --- K: TickCampState -- campState 2, past 5000ms: full rest,
        // "Rest complete!", suppressMoveThisTick ---
        {
            PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            player.currentLevel = level.number;
            player.tileX = px;
            player.tileY = py;
            player.campState = 2;
            player.coreStats[2] = static_cast<int16_t>(player.coreStats[3] / 2);
            int16_t hpBefore = player.coreStats[2];
            int64_t campStartTimeMs = 0;
            MessagePopupState popup;
            bool suppress = false;
            bool runTick = CampTick::TickCampState(player, levels, world, items, monsterDb, globalRng, 5100,
                                                    campStartTimeMs, nextMonsterSpawnId, popup, suppress);
            Check(runTick && player.campState == 0 && suppress, "a completed rest should end at campState 0 with movement suppressed");
            Check(popup.visible && popup.lines[0] == "Rest" && popup.lines[1] == "complete!" && popup.priority == 1,
                  "a completed rest should show \"Rest/complete!\" at priority 1");
            Check(player.coreStats[2] > hpBefore, "a completed rest should increase HP from its half-max starting point");
            Check(player.coreStats[2] == player.coreStats[3], "a FULL rest should restore HP exactly to max");
        }

        // --- L: TickCampState -- campState 0: always just runTick=true ---
        {
            PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            player.currentLevel = level.number;
            player.tileX = px;
            player.tileY = py;
            player.campState = 0;
            int64_t campStartTimeMs = 0;
            MessagePopupState popup;
            bool suppress = false;
            bool runTick = CampTick::TickCampState(player, levels, world, items, monsterDb, globalRng, 9999,
                                                    campStartTimeMs, nextMonsterSpawnId, popup, suppress);
            Check(runTick && !popup.visible && !suppress, "campState 0 should be a pure pass-through");
        }

        // --- M: PlayerCamp::Rest -- hand-derived: a partial (2/3) rest
        // with ailment 8 active applies BOTH scalings in sequence
        // (2/3 then 3/4 of what's left, i.e. exactly 1/2 of the missing
        // amount), not a single combined fraction ---
        {
            PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            player.coreStats[2] = 0;    // HP
            player.coreStats[3] = 120;  // max HP
            player.coreStats[4] = 0;    // Magicka
            player.coreStats[5] = 60;   // max Magicka
            player.coreStats[6] = 0;    // Fatigue
            player.coreStats[7] = 90;   // max Fatigue
            player.ailmentMask = static_cast<int8_t>(1 << 7);  // ailment 8 (bit 7)
            player.coreStats[8] = 5;
            player.coreStats[9] = 7;
            player.increaseHarmBuff = true;
            player.increaseArmorBuff = true;
            player.safeCampingBuff = true;

            PlayerCamp::Rest(player, levels, world, items, globalRng, false);

            // missing=120, 2/3=80, 3/4 of 80=60 -> exactly half of missing.
            Check(player.coreStats[2] == 60, "partial rest + ailment 8 should heal HP by exactly half the missing amount");
            Check(player.coreStats[4] == 30, "partial rest + ailment 8 should restore Magicka by exactly half the missing amount");
            Check(player.coreStats[6] == 45, "partial rest + ailment 8 should restore Fatigue by exactly half the missing amount");
            Check(player.coreStats[8] == 0 && player.coreStats[9] == 0, "rest should always zero both level-exp counters");
            Check(!player.increaseHarmBuff && !player.increaseArmorBuff && !player.safeCampingBuff,
                  "rest should always clear all 3 buff flags");
        }

        if (g_ok) {
            std::printf("all camp-tick checks passed\n");
            return 0;
        } else {
            std::printf("SOME CHECKS FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("EXCEPTION: %s\n", e.what());
        return 1;
    }
}
