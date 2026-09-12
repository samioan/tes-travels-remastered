// M15 smoke test: MonsterRuntime + CombatResolution against real
// MonsterDatabase/ItemDatabase/CharacterData and a real generated world.
// No JVM ground truth available (same reason as M6/M9/M11/M13/M14).
#include <cstdio>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "combat/combat_resolution.h"
#include "monster/monster_runtime.h"
#include "player/player_creation.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::GeneratedLevel;
using dawnstar::MonsterRuntime;
using dawnstar::MonsterState;
using dawnstar::PlayerState;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

// Independent transcription of Monster.java's private isStairwayTile(),
// written straight from the source rather than copied from
// monster_runtime.cpp, to actually cross-check that cascading (and, for
// a level with two differently-directioned stairways, priority-order-
// sensitive: N checked before S before W before E) nested-if logic.
bool ExpectedIsStairwayTile(const GeneratedLevel& level, int x, int y) {
    if (level.stairsUpDir != 1 && level.stairsDownDir != 1) {
        if (level.stairsUpDir != 3 && level.stairsDownDir != 3) {
            if (level.stairsUpDir != 4 && level.stairsDownDir != 4) {
                if ((level.stairsUpDir == 2 || level.stairsDownDir == 2) && x == 30 && y == 17) return true;
            } else if (x == 5 && y == 17) {
                return true;
            }
        } else if (x == 17 && y == 30) {
            return true;
        }
    } else if (x == 17 && y == 5) {
        return true;
    }
    return false;
}

bool FindInteriorWalkable(const GeneratedLevel& level, int* outX, int* outY) {
    for (int x = 1; x < level.width - 1; x++) {
        for (int y = 1; y < level.height - 1; y++) {
            if ((level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] & (1 | 2 | 8 | 32)) == 0) {
                *outX = x;
                *outY = y;
                return true;
            }
        }
    }
    return false;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::CharacterData charData = dawnstar::CharacterData::Load(archive);
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        dawnstar::MonsterDatabase monsters = dawnstar::MonsterDatabase::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);

        std::vector<GeneratedLevel> levels;
        for (size_t i = 0; i < geometry.rows.size(); i++) {
            int levelNumber = static_cast<int>(i) + 1;
            levels.push_back(levelNumber == 1
                                  ? dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[i])
                                  : dawnstar::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[i], items, monsters));
        }
        std::printf("generated %zu levels\n", levels.size());

        // --- PickMonsterType: forced passthrough + hand-traced roll ---
        {
            dawnstar::JavaRandom unusedRng(1);
            Check(MonsterRuntime::PickMonsterType(unusedRng, 5, 7) == 7,
                  "PickMonsterType should pass forcedType through unchanged");

            dawnstar::JavaRandom probe(999);
            int tierRoll = dawnstar::LingoRandomInt(probe, 10);
            int bucket = tierRoll <= 4 ? 0 : tierRoll <= 7 ? 1 : tierRoll <= 9 ? 2 : 3;
            int expected = dawnstar::DungeonGenerator::MonsterTypeForTierBucket(4, bucket);  // tier 5 -> tierIndex 4

            dawnstar::JavaRandom rng(999);
            int actual = MonsterRuntime::PickMonsterType(rng, 5, -1);
            Check(actual == expected, "PickMonsterType's rolled type should match the hand-traced tier/bucket formula");
        }

        // --- Spawn / Stat / RawStat / IsUndead / TakeDamage ---
        {
            MonsterState m = MonsterRuntime::Spawn(42, 3, 7, monsters);
            Check(m.spawnId == 42 && m.monsterType == 3 && m.dungeonLevel == 7, "Spawn should set spawnId/type/level");
            Check(static_cast<uint8_t>(m.hp) == monsters.Stat(3, 14), "Spawn's hp should copy the type's max-HP column");
            Check(m.flag == false && m.aiPhase == 0, "Spawn should start with flag=false, aiPhase=0");

            Check(MonsterRuntime::IsUndead(MonsterRuntime::Spawn(1, 6, 1, monsters)), "type 6 should be undead");
            Check(MonsterRuntime::IsUndead(MonsterRuntime::Spawn(1, 8, 1, monsters)), "type 8 should be undead");
            Check(!MonsterRuntime::IsUndead(MonsterRuntime::Spawn(1, 5, 1, monsters)), "type 5 should not be undead");
            Check(!MonsterRuntime::IsUndead(MonsterRuntime::Spawn(1, 9, 1, monsters)), "type 9 should not be undead");

            m.hp = 10;
            MonsterRuntime::TakeDamage(m, 4);
            Check(m.hp == 6, "TakeDamage should subtract normally");
            MonsterRuntime::TakeDamage(m, 100);
            Check(m.hp == 0, "TakeDamage should clamp at 0, not go negative");
        }

        // --- ToBytes/FromBytes (packed 28-byte) round-trip ---
        {
            MonsterState m;
            m.spawnId = -12345;  // exercises the sign bit through the u16 pack/unpack
            m.monsterType = 5;
            m.hp = -1;  // 0xFF -- exercises raw-byte round-trip at the sign boundary
            m.x = 30;
            m.y = 17;
            m.flag = true;
            m.dungeonLevel = 12;
            m.moveCooldown = 3;
            m.aiPhase = 2;
            m.timestamp = 1234567890123LL;
            for (int i = 0; i < 10; i++) m.scratch[static_cast<size_t>(i)] = static_cast<int8_t>(i * 17 - 5);

            std::array<uint8_t, 28> bytes = MonsterRuntime::ToBytes(m);
            MonsterState m2 = MonsterRuntime::FromBytes(bytes);

            Check(m2.spawnId == m.spawnId, "ToBytes/FromBytes: spawnId");
            Check(m2.monsterType == m.monsterType, "ToBytes/FromBytes: monsterType");
            Check(m2.hp == m.hp, "ToBytes/FromBytes: hp (sign-boundary byte)");
            Check(m2.x == m.x && m2.y == m.y, "ToBytes/FromBytes: x/y");
            Check(m2.flag == m.flag, "ToBytes/FromBytes: flag");
            Check(m2.dungeonLevel == m.dungeonLevel, "ToBytes/FromBytes: dungeonLevel");
            Check(m2.moveCooldown == m.moveCooldown && m2.aiPhase == m.aiPhase, "ToBytes/FromBytes: moveCooldown/aiPhase");
            Check(m2.timestamp == m.timestamp, "ToBytes/FromBytes: timestamp");
            Check(m2.scratch == m.scratch, "ToBytes/FromBytes: scratch");
        }

        // --- ReadFrom/WriteTo (DataStream format) round-trip ---
        {
            MonsterState m;
            m.spawnId = 777;
            m.monsterType = 9;
            m.hp = 50;
            m.x = 5;
            m.y = 5;
            m.flag = false;
            m.dungeonLevel = 3;
            m.moveCooldown = 1;
            m.aiPhase = 1;
            m.timestamp = -99;  // exercises the sign bit through the u32/u32 pack/unpack
            for (int i = 0; i < 10; i++) m.scratch[static_cast<size_t>(i)] = static_cast<int8_t>(i - 5);

            std::vector<uint8_t> buf;
            dawnstar::BinaryWriter out(buf);
            MonsterRuntime::WriteTo(out, m);

            std::string raw(reinterpret_cast<const char*>(buf.data()), buf.size());
            std::istringstream stream(raw);
            dawnstar::BinaryReader in(stream);
            MonsterState m2 = MonsterRuntime::ReadFrom(in);

            Check(m2.spawnId == m.spawnId && m2.monsterType == m.monsterType && m2.hp == m.hp,
                  "ReadFrom/WriteTo: spawnId/monsterType/hp");
            Check(m2.x == m.x && m2.y == m.y && m2.flag == m.flag, "ReadFrom/WriteTo: x/y/flag");
            Check(m2.dungeonLevel == m.dungeonLevel && m2.moveCooldown == m.moveCooldown && m2.aiPhase == m.aiPhase,
                  "ReadFrom/WriteTo: dungeonLevel/moveCooldown/aiPhase");
            Check(m2.timestamp == m.timestamp, "ReadFrom/WriteTo: timestamp (negative)");
            Check(m2.scratch == m.scratch, "ReadFrom/WriteTo: scratch");
        }

        // --- Move / IsStairwayTile, against the real generated hub level ---
        {
            int x = 0, y = 0;
            Check(FindInteriorWalkable(levels[0], &x, &y), "need an interior walkable hub tile");

            MonsterState m = MonsterRuntime::Spawn(1, 1, 1, monsters);
            m.x = static_cast<int8_t>(x);
            m.y = static_cast<int8_t>(y);
            levels[0].tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] =
                static_cast<uint8_t>(levels[0].tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] | 2);

            bool northWalkable = (levels[0].tiles[static_cast<size_t>(x)][static_cast<size_t>(y - 1)] & (1 | 2 | 8 | 32)) == 0;
            bool moved = MonsterRuntime::Move(m, 1, levels);
            if (northWalkable) {
                Check(moved, "Move onto a walkable tile should succeed");
                Check(m.x == x && m.y == y - 1, "Move direction 1 should move north (y-1)");
                Check((levels[0].tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] & 2) == 0,
                      "Move should clear the monster bit at the old tile");
                Check((levels[0].tiles[static_cast<size_t>(x)][static_cast<size_t>(y - 1)] & 2) != 0,
                      "Move should set the monster bit at the new tile");
            } else {
                Check(!moved, "Move into a blocked tile should fail");
                Check(m.x == x && m.y == y, "a blocked Move must not change position");
            }

            // IsStairwayTile: cross-check every generated level's 4
            // canonical stairway coordinates against an independently
            // transcribed copy of the (priority-order-sensitive)
            // cascading logic, plus an arbitrary corner that should
            // never read as a stairway.
            int stairwayTruesSeen = 0;
            for (const auto& level : levels) {
                for (auto coord : {std::pair{17, 5}, std::pair{30, 17}, std::pair{17, 30}, std::pair{5, 17}}) {
                    bool expected = ExpectedIsStairwayTile(level, coord.first, coord.second);
                    bool actual = MonsterRuntime::IsStairwayTile(level, coord.first, coord.second);
                    if (expected != actual) {
                        std::printf("  FAIL: IsStairwayTile mismatch level=%d (%d,%d) up=%d down=%d expected=%d actual=%d\n",
                                    level.number, coord.first, coord.second, level.stairsUpDir, level.stairsDownDir,
                                    expected, actual);
                        g_ok = false;
                    }
                    if (expected) stairwayTruesSeen++;
                }
                Check(!MonsterRuntime::IsStairwayTile(level, 1, 1), "an arbitrary corner should never be a stairway tile");
            }
            Check(stairwayTruesSeen > 0, "expected at least one (level, coordinate) pair to be a real stairway tile");
        }

        // --- Chase: 1-in-5 movement cadence ---
        {
            int x = 0, y = 0;
            Check(FindInteriorWalkable(levels[0], &x, &y), "need an interior walkable hub tile");

            MonsterState m = MonsterRuntime::Spawn(2, 1, 1, monsters);
            m.x = static_cast<int8_t>(x);
            m.y = static_cast<int8_t>(y);
            dawnstar::JavaRandom rng(4242);

            bool expectedPattern[10] = {true, false, false, false, false, true, false, false, false, false};
            for (int i = 0; i < 10; i++) {
                bool moved = MonsterRuntime::Chase(m, x + 100, y + 100, levels, rng);
                Check(moved == expectedPattern[i], "Chase should move on a 1-in-5 cadence");
            }
        }

        // --- OnDeath: guaranteed drop always produces a valid record ---
        {
            MonsterState m = MonsterRuntime::Spawn(3, 1, 2, monsters);
            m.x = 4;
            m.y = 9;
            dawnstar::JavaRandom rng(55);
            MonsterRuntime::DeathDrop drop =
                MonsterRuntime::OnDeath(m, monsters, items, levels[1].tier, /*guaranteed=*/true, 500, rng);

            Check(drop.dropped, "a guaranteed death drop should always drop");
            Check(drop.record[0] == 4 && drop.record[1] == 9, "the drop record should carry the monster's position");
            Check((drop.record[6] & 1) != 0, "the drop record's flags byte should always have bit 0 set");
            Check((drop.record[6] & 4) != 0, "a guaranteed drop's flags byte should have bit 2 set");
            int realItemId = drop.record[2] & 0xFF;
            Check(realItemId >= 1 && realItemId <= items.ItemCount(), "the dropped item id should be in range");
            int16_t spawnIdBack = static_cast<int16_t>((drop.record[3] << 8) | drop.record[4]);
            Check(spawnIdBack == 500, "the drop record should carry the given spawnId");
        }

        // --- PlayerAttack / MonsterTick integration, against a real
        // character and a real monster ---
        {
            dawnstar::JavaRandom creationRng(2468);
            PlayerState player = dawnstar::PlayerCreation::CreateCharacter(0, "Fighter", charData, items, creationRng);
            player.coreStats[2] = player.coreStats[3];  // full HP
            player.coreStats[6] = 100;                  // plenty of fatigue

            MonsterState monster = MonsterRuntime::Spawn(9001, 2, 2, monsters);

            dawnstar::JavaRandom combatRng(13579);
            int startingHp = static_cast<uint8_t>(monster.hp);
            bool anyMonsterDamage = false;
            int16_t fatigueBefore = player.coreStats[6];
            bool fatigueEverDropped = false;

            for (int i = 0; i < 30 && static_cast<uint8_t>(monster.hp) > 0; i++) {
                int16_t before = player.coreStats[6];
                dawnstar::CombatResolution::PlayerAttack(player, monster, charData, items, monsters, combatRng);
                if (static_cast<uint8_t>(monster.hp) < startingHp) anyMonsterDamage = true;
                if (player.coreStats[6] < before) fatigueEverDropped = true;
                Check(player.coreStats[6] >= 0, "player fatigue must never go negative");
                Check(static_cast<uint8_t>(monster.hp) <= 255, "monster hp stays in byte range");
            }
            Check(player.combatTargetSpawnId == monster.spawnId, "PlayerAttack should set combatTargetSpawnId");
            Check(anyMonsterDamage, "at least one of 30 attack attempts should have landed some damage");
            Check(fatigueEverDropped, "at least one landed attack should have cost fatigue");
            (void)fatigueBefore;

            MonsterState attacker = MonsterRuntime::Spawn(9002, 2, 2, monsters);
            dawnstar::JavaRandom tickRng(97531);
            bool anyPlayerDamage = false;
            bool anyAilmentSet = false;
            int16_t hpBefore = player.coreStats[2];
            for (int i = 0; i < 60; i++) {
                dawnstar::CombatResolution::MonsterTick(attacker, player, charData, items, monsters, i * 900LL, tickRng);
                Check(player.coreStats[2] >= 0, "player HP must never go negative");
                if (player.coreStats[2] < hpBefore) anyPlayerDamage = true;
                if (player.ailmentMask != 0) anyAilmentSet = true;
            }
            Check(anyPlayerDamage, "at least one of 60 monster ticks (spanning many 800ms windows) should land damage");
            std::printf("  after combat: monster hp=%d player hp=%d ailmentMask=0x%x\n", static_cast<uint8_t>(monster.hp),
                        player.coreStats[2], static_cast<uint8_t>(player.ailmentMask));
            (void)anyAilmentSet;
        }

        if (!g_ok) {
            std::fprintf(stderr, "m15_monster_combat_smoke: FAILED\n");
            return 1;
        }
        std::printf("all monster/combat checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m15_monster_combat_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
