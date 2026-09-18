// M14 smoke test: MonsterRuntime + CombatResolution against real
// MonsterDatabase/ItemDatabase/CharacterData/DungeonGeometry-derived data.
// RollOutcome itself (the tier formula both PlayerAttack and MonsterTick
// build on) was already independently verified against a from-scratch
// java.util.Random reimplementation in M13 -- this test focuses on the
// NEW wiring (Monster's own runtime fields/movement/AI, and the two
// combat entry points that tie Player and Monster together), not
// re-proving RollOutcome's own math.
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "combat/combat_resolution.h"
#include "monster/monster_runtime.h"
#include "player/player_creation.h"
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

void TestSpawnAndStatConventions(const stormhold::MonsterDatabase& monsters) {
    std::printf("-- Spawn/Stat/RawStat against real monstersin.dat --\n");

    stormhold::MonsterState m = stormhold::MonsterRuntime::Spawn(7, 1, 2, monsters);
    Expect(m.spawnId == 7, "Spawn should keep the caller-supplied spawnId");
    Expect(m.typeIndex == 1, "Spawn should keep the caller-supplied typeIndex");
    Expect(m.dungeonLevel == 2, "Spawn should keep the caller-supplied dungeonLevel");
    Expect(m.currentHp == monsters.RawStat(1, 14), "Spawn's starting HP should be the type's RAW (unmasked) column-14 read");

    // Stat() (masked) and RawStat() (raw signed) must always agree modulo
    // 256 -- they're reading the exact same stored byte two different
    // ways.
    for (int col = 0; col < 17; col++) {
        uint8_t masked = stormhold::MonsterRuntime::Stat(m, monsters, col);
        int8_t raw = stormhold::MonsterRuntime::RawStat(m, monsters, col);
        Expect(masked == static_cast<uint8_t>(raw), "Stat/RawStat should read the same underlying byte");
    }

    Expect(stormhold::MonsterRuntime::TypeName(m, monsters) == monsters.TypeName(1),
           "TypeName should forward to MonsterDatabase::TypeName(typeIndex)");
    Expect(!stormhold::MonsterRuntime::IsUndead(m), "type 1 should not be in the undead range (6-8)");

    stormhold::MonsterState undead = stormhold::MonsterRuntime::Spawn(8, 7, 2, monsters);
    Expect(stormhold::MonsterRuntime::IsUndead(undead), "type 7 should be in the undead range (6-8)");
}

void TestPickMonsterType() {
    std::printf("-- PickMonsterType's forced vs. rolled paths --\n");

    stormhold::JavaRandom rng(42);
    Expect(stormhold::MonsterRuntime::PickMonsterType(rng, 5, 12) == 12,
           "a non-negative forcedTypeIndex should bypass the roll entirely");

    // Same seed in, same tier -> same rolled type out (pure function of
    // the RNG stream + tier, no hidden state).
    stormhold::JavaRandom rngA(1234);
    stormhold::JavaRandom rngB(1234);
    int a = stormhold::MonsterRuntime::PickMonsterType(rngA, 10, -1);
    int b = stormhold::MonsterRuntime::PickMonsterType(rngB, 10, -1);
    Expect(a == b, "PickMonsterType should be deterministic for a given seed+tier");

    // Tier is clamped to [1,37] (zone [0,36]) before indexing.
    stormhold::JavaRandom rngLow(1);
    stormhold::JavaRandom rngHigh(1);
    int low = stormhold::MonsterRuntime::PickMonsterType(rngLow, -5, -1);
    int clampedLow = stormhold::MonsterRuntime::PickMonsterType(rngHigh, 1, -1);
    // Same RNG seed/consumption either way (one roll each), so a tier
    // clamped to the same zone must produce the same result.
    Expect(low == clampedLow, "a tier below 1 should clamp to the same zone as tier 1");
}

void TestTakeDamageClamping() {
    std::printf("-- TakeDamage's clamp-at-0 --\n");
    stormhold::MonsterState m;
    m.currentHp = 10;
    stormhold::MonsterRuntime::TakeDamage(m, 4);
    Expect(m.currentHp == 6, "a partial hit should subtract normally");
    stormhold::MonsterRuntime::TakeDamage(m, 100);
    Expect(m.currentHp == 0, "an overkill hit should clamp at 0, not go negative");
}

void TestToBytesFromBytesRoundTrip() {
    std::printf("-- ToBytes/FromBytes packed-record round trip --\n");
    stormhold::MonsterState m;
    m.spawnId = -12345;  // exercises the sign/byte-order path
    m.typeIndex = 5;
    m.currentHp = -3;  // a real signed byte (e.g. underflowed HP) round-trips as-is
    m.tileX = 17;
    m.tileY = 29;
    m.unconfirmedFlag = true;
    m.dungeonLevel = 9;
    m.chaseCadence = 3;
    m.aiPhase = 2;
    m.unconfirmedTimestamp = 1234567890123LL;
    for (int i = 0; i < 10; i++) m.scratch[static_cast<size_t>(i)] = static_cast<int8_t>(i - 5);

    std::array<uint8_t, 28> bytes = stormhold::MonsterRuntime::ToBytes(m);
    stormhold::MonsterState back = stormhold::MonsterRuntime::FromBytes(bytes);

    Expect(back.spawnId == m.spawnId, "spawnId should round-trip through ToBytes/FromBytes");
    Expect(back.typeIndex == m.typeIndex, "typeIndex should round-trip");
    Expect(back.currentHp == m.currentHp, "currentHp should round-trip, including a negative value");
    Expect(back.tileX == m.tileX && back.tileY == m.tileY, "tileX/tileY should round-trip");
    Expect(back.unconfirmedFlag == m.unconfirmedFlag, "unconfirmedFlag should round-trip");
    Expect(back.dungeonLevel == m.dungeonLevel, "dungeonLevel should round-trip");
    Expect(back.chaseCadence == m.chaseCadence, "chaseCadence should round-trip");
    Expect(back.aiPhase == m.aiPhase, "aiPhase should round-trip");
    Expect(back.unconfirmedTimestamp == m.unconfirmedTimestamp, "unconfirmedTimestamp should round-trip");
    Expect(back.scratch == m.scratch, "scratch[10] should round-trip");
}

void TestMoveAndStairway(const stormhold::DungeonGeometry& geometry, const stormhold::ItemDatabase& items,
                          const stormhold::MonsterDatabase& monsters) {
    std::printf("-- Move/IsStairwayTile against a real generated level --\n");
    std::vector<stormhold::GeneratedLevel> levels;
    levels.push_back(stormhold::GeneratedLevel{});  // index 0 unused (levels are 1-based)
    levels.push_back(stormhold::DungeonGenerator::PopulateLevel(2, geometry.rows[1], items, monsters));

    stormhold::GeneratedLevel& level = levels[1];
    Expect(!level.monsters.empty(), "level 2 should have at least one generated monster spawn to test against");

    const auto& spawn = level.monsters[0];
    stormhold::MonsterState m = stormhold::MonsterRuntime::Spawn(1, spawn.monsterType, 2, monsters);
    m.tileX = static_cast<int8_t>(spawn.x);
    m.tileY = static_cast<int8_t>(spawn.y);
    Expect((level.tiles[static_cast<size_t>(spawn.x)][static_cast<size_t>(spawn.y)] & 0x02) != 0,
           "the spawn's own tile should already carry the monster bit (set by PopulateLevel)");

    // Directly into a wall: find a direction whose neighbor tile has bit 1
    // set (there must be one -- rooms are enclosed by walls).
    int wallDir = 0;
    for (int dir = 1; dir <= 4 && wallDir == 0; dir++) {
        int dx = (dir == 2) ? 1 : (dir == 4) ? -1 : 0;
        int dy = (dir == 3) ? 1 : (dir == 1) ? -1 : 0;
        int nx = spawn.x + dx;
        int ny = spawn.y + dy;
        if (nx < 0 || ny < 0 || nx >= level.width || ny >= level.height) continue;
        if (level.tiles[static_cast<size_t>(nx)][static_cast<size_t>(ny)] & 0x01) wallDir = dir;
    }
    if (wallDir != 0) {
        bool moved = stormhold::MonsterRuntime::Move(m, wallDir, levels);
        Expect(!moved, "Move into a wall-flagged tile should fail");
        Expect(m.tileX == static_cast<int8_t>(spawn.x) && m.tileY == static_cast<int8_t>(spawn.y),
               "a failed Move should leave the monster's position unchanged");
    } else {
        std::printf("  (skipped: no adjacent wall tile found for this spawn)\n");
    }

    // IsStairwayTile: the level's own real stairsUpDir/stairsDownDir
    // should make exactly the matching one of the 4 fixed coordinate
    // pairs return true, and NEVER more than one -- same cascading-
    // priority quirk dawnstar's own port already documents for its
    // equivalent method.
    int stairwayHits = 0;
    if (stormhold::MonsterRuntime::IsStairwayTile(level, 17, 5)) stairwayHits++;
    if (stormhold::MonsterRuntime::IsStairwayTile(level, 17, 30)) stairwayHits++;
    if (stormhold::MonsterRuntime::IsStairwayTile(level, 5, 17)) stairwayHits++;
    if (stormhold::MonsterRuntime::IsStairwayTile(level, 30, 17)) stairwayHits++;
    Expect(stairwayHits <= 1, "at most one of the 4 fixed stairway coordinate pairs should ever match");
}

void TestIsAdjacentSideEffect() {
    std::printf("-- IsAdjacent's aiPhase-reset side effect --\n");
    stormhold::MonsterState m;
    m.tileX = 10;
    m.tileY = 10;
    m.aiPhase = 2;

    Expect(stormhold::MonsterRuntime::IsAdjacent(m, 11, 10), "manhattan distance 1 should be adjacent");
    Expect(m.aiPhase == 2, "a TRUE isAdjacent call should NOT touch aiPhase");

    m.aiPhase = 2;
    Expect(!stormhold::MonsterRuntime::IsAdjacent(m, 13, 10), "manhattan distance 3 should not be adjacent");
    Expect(m.aiPhase == 0, "a FALSE isAdjacent call should reset aiPhase to 0 as a side effect");
}

void TestChaseRangeGateAndCadence(std::vector<stormhold::GeneratedLevel>& levels) {
    std::printf("-- Chase's range gate + 1-in-5 cadence --\n");
    stormhold::GeneratedLevel& level = levels[1];

    stormhold::MonsterState far;
    far.dungeonLevel = 2;
    far.tileX = 1;
    far.tileY = 1;
    stormhold::JavaRandom rng(99);
    stormhold::MonsterRuntime::Chase(far, 20, 20, levels, rng);
    Expect(far.chaseCadence == 0, "Chase should be a complete no-op (including cadence) outside range 3");

    // Pick an open interior tile pair 2 apart so Chase has somewhere legal
    // to step for 5 calls running.
    int startX = -1, startY = -1;
    for (int x = 1; x < level.width - 1 && startX < 0; x++) {
        for (int y = 1; y < level.height - 1 && startX < 0; y++) {
            if ((level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] & 0x01) == 0 &&
                (level.tiles[static_cast<size_t>(x + 1)][static_cast<size_t>(y)] & 0x01) == 0) {
                startX = x;
                startY = y;
            }
        }
    }
    Expect(startX >= 0, "level 2 should have at least one pair of open adjacent interior tiles");
    if (startX < 0) return;

    stormhold::MonsterState m;
    m.dungeonLevel = 2;
    m.tileX = static_cast<int8_t>(startX);
    m.tileY = static_cast<int8_t>(startY);
    int targetX = startX + 2;
    int targetY = startY;

    int8_t cadenceAfterEachCall[5];
    int moveCount = 0;
    int prevX = m.tileX, prevY = m.tileY;
    for (int i = 0; i < 5; i++) {
        stormhold::JavaRandom callRng(1000 + i);
        stormhold::MonsterRuntime::Chase(m, targetX, targetY, levels, callRng);
        cadenceAfterEachCall[i] = m.chaseCadence;
        if (m.tileX != prevX || m.tileY != prevY) moveCount++;
        prevX = m.tileX;
        prevY = m.tileY;
    }
    // Call 0: cadence 0->1 (a real step attempted). Calls 1-3: cadence
    // 1->2->3->4 (no step). Call 4: cadence WAS 4, resets to 0 (no step).
    Expect(cadenceAfterEachCall[0] == 1, "the first in-range call should advance cadence to 1");
    Expect(cadenceAfterEachCall[3] == 4, "the fourth call should advance cadence to 4");
    Expect(cadenceAfterEachCall[4] == 0, "the fifth call should reset cadence back to 0");
    Expect(moveCount <= 1, "only the very first of 5 calls should ever actually move the monster");
}

void TestOnDeath(const stormhold::MonsterDatabase& monsters, const stormhold::ItemDatabase& items) {
    std::printf("-- OnDeath's guaranteed vs. rolled loot drop --\n");
    stormhold::MonsterState m = stormhold::MonsterRuntime::Spawn(3, 1, 2, monsters);
    m.tileX = 5;
    m.tileY = 6;

    stormhold::JavaRandom rng(7);
    auto guaranteed = stormhold::MonsterRuntime::OnDeath(m, monsters, items, 1, true, 42, rng);
    Expect(guaranteed.dropped, "guaranteedDrop=true should always drop");
    Expect(guaranteed.record[0] == 5 && guaranteed.record[1] == 6, "the drop record should carry the monster's tile position");
    Expect((guaranteed.record[6] & 4) != 0, "a guaranteed drop should set the flags byte's bit 4");
    uint16_t spawnIdBack = static_cast<uint16_t>((guaranteed.record[3] << 8) | guaranteed.record[4]);
    Expect(spawnIdBack == 42, "the drop record should carry the caller-supplied dropSpawnId");
}

void TestPlayerAttack(const stormhold::CharacterData& charData, const stormhold::ItemDatabase& items,
                       const stormhold::MonsterDatabase& monsters) {
    std::printf("-- PlayerAttack against a real created character + a real monster type --\n");
    stormhold::PlayerState player = stormhold::PlayerCreation::CreateCharacter(0, "Attacker", 7, charData, items);
    stormhold::MonsterState target = stormhold::MonsterRuntime::Spawn(9, 1, 2, monsters);
    int hpBefore = target.currentHp & 0xFF;

    // Run several seeds; regardless of the roll, invariants must hold:
    // lastCombatTargetId always gets set, and HP only ever goes down
    // (TakeDamage's own clamp-at-0 already covers the floor).
    for (int64_t seed = 0; seed < 20; seed++) {
        stormhold::MonsterState t = target;
        stormhold::JavaRandom rng(seed);
        stormhold::CombatResolution::PlayerAttack(player, t, charData, items, monsters, rng);
        Expect(player.lastCombatTargetId == t.spawnId, "PlayerAttack should always set lastCombatTargetId");
        int hpAfter = t.currentHp & 0xFF;
        Expect(hpAfter <= hpBefore, "PlayerAttack should never heal the target");
    }
}

void TestMonsterTick(const stormhold::CharacterData& charData, const stormhold::ItemDatabase& items,
                      const stormhold::MonsterDatabase& monsters) {
    std::printf("-- MonsterTick against a real created character + a real monster type --\n");
    stormhold::PlayerState basePlayer = stormhold::PlayerCreation::CreateCharacter(0, "Defender", 7, charData, items);
    int16_t hpBefore = basePlayer.coreStats[2];

    for (int64_t seed = 0; seed < 20; seed++) {
        stormhold::PlayerState player = basePlayer;
        stormhold::MonsterState m = stormhold::MonsterRuntime::Spawn(11, 1, 2, monsters);
        stormhold::JavaRandom rng(seed);
        stormhold::CombatResolution::MonsterTick(m, player, charData, items, monsters, 1000, rng);
        Expect(m.aiPhase == 1, "MonsterTick should always leave aiPhase at 1 (wound-up/waiting)");
        Expect(m.unconfirmedTimestamp == 1000, "MonsterTick should stamp the given `now` unconditionally");
        Expect(player.coreStats[2] <= hpBefore, "MonsterTick should never heal the player");
        Expect(player.coreStats[2] >= 0, "MonsterTick's HP write should stay clamped at >= 0");
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        stormhold::AssetRoot assets(root);
        stormhold::CharacterData charData = stormhold::CharacterData::Load(assets);
        stormhold::ItemDatabase items = stormhold::ItemDatabase::Load(assets);
        stormhold::MonsterDatabase monsters = stormhold::MonsterDatabase::Load(assets);
        stormhold::DungeonGeometry geometry = stormhold::DungeonGeometry::Load(assets);

        TestSpawnAndStatConventions(monsters);
        TestPickMonsterType();
        TestTakeDamageClamping();
        TestToBytesFromBytesRoundTrip();
        TestMoveAndStairway(geometry, items, monsters);
        TestIsAdjacentSideEffect();

        std::vector<stormhold::GeneratedLevel> levels;
        levels.push_back(stormhold::GeneratedLevel{});
        levels.push_back(stormhold::DungeonGenerator::PopulateLevel(2, geometry.rows[1], items, monsters));
        TestChaseRangeGateAndCadence(levels);

        TestOnDeath(monsters, items);
        TestPlayerAttack(charData, items, monsters);
        TestMonsterTick(charData, items, monsters);

        if (!g_ok) {
            std::fprintf(stderr, "m14_monster_combat_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m14_monster_combat_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
