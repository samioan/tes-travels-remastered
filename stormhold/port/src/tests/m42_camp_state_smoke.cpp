// M42 smoke test: CampState/Camping (GameCanvas's own campState/
// campRollAt fields and run()'s already-transcribed camp state machine,
// finally wired to a real trigger via M41's startCampOrRest()) plus
// PlayerCombatStats::ApplyRestRecovery and DungeonRuntime::
// SpawnAmbushMonsterNearPlayer, the two Player/Dungeon methods it drives.
#include <cstdio>
#include <cstdlib>
#include <string>

#include "assets/asset_root.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "player/camp_state.h"
#include "player/player_combat_stats.h"
#include "world/dungeon_generator.h"

namespace {

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

using namespace stormhold;

void TestStart() {
    std::printf("-- Camping::Start: campState/safeCampingBuff/hub-town branches --\n");

    PlayerState p;
    p.currentLevel = 5;
    p.safeCampingBuff = false;
    CampState camp;
    Camping::Start(camp, p, 12345);
    Expect(camp.state == 1, "no buff, not in the hub -> rolls for interruption (state 1)");
    Expect(camp.rollAtMs == 12345, "rollAtMs stamped to the passed-in 'now'");

    PlayerState buffed;
    buffed.currentLevel = 5;
    buffed.safeCampingBuff = true;
    CampState campBuffed;
    Camping::Start(campBuffed, buffed, 0);
    Expect(campBuffed.state == 2, "safeCampingBuff active -> skips straight to safe wait (state 2)");

    PlayerState inHub;
    inHub.currentLevel = 1;
    inHub.safeCampingBuff = false;
    CampState campHub;
    Camping::Start(campHub, inHub, 0);
    Expect(campHub.state == 2, "standing in the hub town -> also skips straight to state 2");
}

void TestRollInterruptedBothOutcomes() {
    std::printf("-- Camping::RollInterrupted: both outcomes are reachable --\n");
    bool sawTrue = false;
    bool sawFalse = false;
    for (int64_t seed = 0; seed < 200 && !(sawTrue && sawFalse); seed++) {
        JavaRandom rng(seed);
        if (Camping::RollInterrupted(rng)) {
            sawTrue = true;
        } else {
            sawFalse = true;
        }
    }
    Expect(sawTrue, "at least one of the first 200 seeds rolls 'interrupted'");
    Expect(sawFalse, "at least one of the first 200 seeds rolls 'not interrupted'");
}

// Finds the first seed (searched independently, without consuming the
// caller's own rng) whose very first RollInterrupted() draw matches
// `wantInterrupted`, so TestTickStateMachine can exercise both of Tick()'s
// own state==1 branches deterministically.
int64_t FindSeedFor(bool wantInterrupted) {
    for (int64_t seed = 0; seed < 1000; seed++) {
        JavaRandom probe(seed);
        if (Camping::RollInterrupted(probe) == wantInterrupted) return seed;
    }
    std::abort();  // Would mean RollInterrupted is badly broken -- fail loud.
}

void TestTickStateMachine(const ItemDatabase& items, const MonsterDatabase& monsters, const DungeonGeometry& geometry) {
    std::printf("-- Camping::Tick: the full state==0/1/2 machine --\n");

    {
        CampState camp;  // state == 0 by default
        PlayerState p;
        GeneratedLevel level;
        level.number = 2;
        level.tiles.assign(35, std::vector<uint8_t>(35, 0));
        WorldRegistry world(37);
        JavaRandom rng(1);
        int16_t spawnIdCounter = 100;
        CampTickResult result = Camping::Tick(camp, p, level, world, items, monsters, rng, spawnIdCounter, 0);
        Expect(result == CampTickResult::NotCamping, "state==0 -> NotCamping, a completely ordinary tick");
        Expect(spawnIdCounter == 100, "NotCamping never touches spawnIdCounter");
    }

    {
        CampState camp;
        camp.state = 1;
        camp.rollAtMs = 1000;
        PlayerState p;
        GeneratedLevel level;
        level.number = 2;
        level.tiles.assign(35, std::vector<uint8_t>(35, 0));
        WorldRegistry world(37);
        JavaRandom rng(1);
        int16_t spawnIdCounter = 100;
        CampTickResult result = Camping::Tick(camp, p, level, world, items, monsters, rng, spawnIdCounter, 2000);
        Expect(result == CampTickResult::StillWaiting, "state==1, only 1000ms elapsed (<=2500) -> StillWaiting");
        Expect(camp.state == 1, "state untouched while still within the 2.5s window");
    }

    {
        // state==1, window elapsed, roll comes back "interrupted".
        CampState camp;
        camp.state = 1;
        camp.rollAtMs = 0;
        PlayerState p;
        p.currentLevel = 2;
        p.tileX = 17;
        p.tileY = 17;
        p.coreStats[2] = 5;
        p.coreStats[3] = 100;  // HP 5/100 -- plenty missing to recover
        p.coreStats[4] = 5;
        p.coreStats[5] = 100;
        p.coreStats[6] = 5;
        p.coreStats[7] = 100;
        GeneratedLevel level = DungeonGenerator::PopulateLevel(2, geometry.rows[1], items, monsters);
        WorldRegistry world(37);
        int16_t spawnIdCounter = 5000;  // well above any real generation-time spawnId
        int64_t seed = FindSeedFor(true);
        JavaRandom rng(seed);
        CampTickResult result = Camping::Tick(camp, p, level, world, items, monsters, rng, spawnIdCounter, 3000);
        Expect(result == CampTickResult::Disturbed, "window elapsed, roll hit -> Disturbed");
        Expect(camp.state == 0, "Disturbed resets state back to 0 (rest interrupted, not resumed)");
        Expect(camp.rollAtMs == 0, "Disturbed clears rollAtMs");
        Expect(spawnIdCounter == 5001, "an ambush monster attempt always burns exactly one spawnId (see "
                                        "SpawnAmbushMonsterNearPlayer's own declaration comment)");
        // 2/3 of the 95 missing on each stat = 63 (integer division),
        // landing at 5+63=68.
        Expect(p.coreStats[2] == 68, "partial (2/3) HP recovery on disturbance");
        Expect(p.coreStats[4] == 68, "partial (2/3) Magicka recovery on disturbance");
        Expect(p.coreStats[6] == 68, "partial (2/3) Fatigue recovery on disturbance");
    }

    {
        // state==1, window elapsed, roll comes back "not interrupted" ->
        // advances to state 2 with NO recovery applied yet.
        CampState camp;
        camp.state = 1;
        camp.rollAtMs = 0;
        PlayerState p;
        p.coreStats[2] = 5;
        p.coreStats[3] = 100;
        GeneratedLevel level;
        level.number = 2;
        level.tiles.assign(35, std::vector<uint8_t>(35, 0));
        WorldRegistry world(37);
        int64_t seed = FindSeedFor(false);
        JavaRandom rng(seed);
        int16_t spawnIdCounter = 100;
        CampTickResult result = Camping::Tick(camp, p, level, world, items, monsters, rng, spawnIdCounter, 3000);
        Expect(result == CampTickResult::StillWaiting, "window elapsed, roll missed -> StillWaiting (advances silently)");
        Expect(camp.state == 2, "advances to state 2 (safe wait) on a missed interruption roll");
        Expect(p.coreStats[2] == 5, "no recovery applied yet on the advance-to-state-2 tick");
        Expect(spawnIdCounter == 100, "no ambush monster attempt on a missed roll");
    }

    {
        // state==2, window elapsed -> full recovery, back to state 0.
        CampState camp;
        camp.state = 2;
        camp.rollAtMs = 0;
        PlayerState p;
        p.coreStats[2] = 10;
        p.coreStats[3] = 100;
        GeneratedLevel level;
        level.number = 2;
        level.tiles.assign(35, std::vector<uint8_t>(35, 0));
        WorldRegistry world(37);
        JavaRandom rng(1);
        int16_t spawnIdCounter = 100;
        CampTickResult result = Camping::Tick(camp, p, level, world, items, monsters, rng, spawnIdCounter, 6000);
        Expect(result == CampTickResult::Complete, "state==2, >5000ms elapsed -> Complete");
        Expect(camp.state == 0, "Complete resets state back to 0");
        Expect(p.coreStats[2] == 100, "FULL recovery (fullyRested==true) on Complete");
    }

    {
        // state==2, window NOT elapsed -> still waiting, untouched.
        CampState camp;
        camp.state = 2;
        camp.rollAtMs = 0;
        PlayerState p;
        GeneratedLevel level;
        level.number = 2;
        level.tiles.assign(35, std::vector<uint8_t>(35, 0));
        WorldRegistry world(37);
        JavaRandom rng(1);
        int16_t spawnIdCounter = 100;
        CampTickResult result = Camping::Tick(camp, p, level, world, items, monsters, rng, spawnIdCounter, 4000);
        Expect(result == CampTickResult::StillWaiting, "state==2, only 4000ms elapsed (<=5000) -> StillWaiting");
        Expect(camp.state == 2, "state untouched while still within the 5s window");
    }
}

void TestApplyRestRecoveryAilment8AndBuffs(const ItemDatabase& items) {
    std::printf("-- PlayerCombatStats::ApplyRestRecovery: ailment-8 compounding + buff clear --\n");
    PlayerState p;
    p.coreStats[2] = 0;
    p.coreStats[3] = 120;  // missing 120
    p.ailmentMask = static_cast<int8_t>(1 << 7);  // ailment 8 active (bit index 7)
    p.increaseHarmBuff = true;
    p.increaseArmorBuff = true;
    p.safeCampingBuff = true;
    JavaRandom rng(1);
    PlayerCombatStats::ApplyRestRecovery(p, true, items, rng);
    // fullyRested -> missing stays 120, THEN *3/4 for ailment 8 -> 90.
    Expect(p.coreStats[2] == 90, "ailment 8 reduces recovery to 3/4, even when fullyRested");
    Expect(!p.increaseHarmBuff && !p.increaseArmorBuff && !p.safeCampingBuff,
           "all 3 temporary combat buffs are cleared by resting");
    Expect((p.ailmentMask & (1 << 3)) == 0 && (p.ailmentMask & (1 << 4)) == 0,
           "ailments 4/5 are never cleared by ApplyRestRecovery (skipped in its own cure loop)");
}

void TestSpawnAmbushMonsterNearPlayerHubNoOp(const MonsterDatabase& monsters) {
    std::printf("-- DungeonRuntime::SpawnAmbushMonsterNearPlayer: no-op in the hub town --\n");
    GeneratedLevel hub;
    hub.number = 1;
    hub.tiles.assign(19, std::vector<uint8_t>(19, 0));
    WorldRegistry world(37);
    JavaRandom rng(1);
    int16_t spawnIdCounter = 100;
    DungeonRuntime::SpawnAmbushMonsterNearPlayer(hub, world, 9, 9, rng, monsters, spawnIdCounter);
    Expect(world.monsters[0].empty(), "the hub town (level 1) never gets an ambush monster");
    Expect(spawnIdCounter == 100, "the hub-town no-op doesn't even burn a spawnId (returns before Monster.spawn())");
}

void TestSpawnAmbushMonsterNearPlayerRealLevel(const ItemDatabase& items, const MonsterDatabase& monsters,
                                                const DungeonGeometry& geometry) {
    std::printf("-- DungeonRuntime::SpawnAmbushMonsterNearPlayer: a real generated level --\n");
    GeneratedLevel level = DungeonGenerator::PopulateLevel(2, geometry.rows[1], items, monsters);
    WorldRegistry world(37);
    JavaRandom rng(7);
    int16_t spawnIdCounter = 9000;

    // Try every open corridor tile as the player's position until one
    // actually succeeds in placing a monster (a real generated level is
    // dense with walls, so not every position has a walkable candidate
    // among the 5 fixed offsets) -- proves the success path for real
    // rather than just "it didn't crash".
    bool placed = false;
    for (int x = 1; x < level.width - 1 && !placed; x++) {
        for (int y = 1; y < level.height - 1 && !placed; y++) {
            if (level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] & 0x01) continue;  // wall
            int16_t before = spawnIdCounter;
            DungeonRuntime::SpawnAmbushMonsterNearPlayer(level, world, x, y, rng, monsters, spawnIdCounter);
            Expect(spawnIdCounter == before + 1, "every attempt burns exactly one spawnId, success or not");
            if (!world.monsters[1].empty()) placed = true;
        }
    }
    Expect(placed, "at least one player position among a real generated level's open tiles succeeds in placing a "
                    "monster adjacent to it");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assets(root);
        ItemDatabase items = ItemDatabase::Load(assets);
        MonsterDatabase monsters = MonsterDatabase::Load(assets);
        DungeonGeometry geometry = DungeonGeometry::Load(assets);

        TestStart();
        TestRollInterruptedBothOutcomes();
        TestTickStateMachine(items, monsters, geometry);
        TestApplyRestRecoveryAilment8AndBuffs(items);
        TestSpawnAmbushMonsterNearPlayerHubNoOp(monsters);
        TestSpawnAmbushMonsterNearPlayerRealLevel(items, monsters, geometry);

        if (!g_ok) {
            std::fprintf(stderr, "m42_camp_state_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m42_camp_state_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
