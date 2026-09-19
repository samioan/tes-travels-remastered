// M37 smoke test: CombatResolution::TickMonstersOnLevel --
// GameCanvas.tickMonsterAI() (was decompiled/e.java's b(long)), a
// throw-stub in ../../../src/GameCanvas.java until this session, and the
// long-flagged, previously-unrecovered caller for Monster.tick()/
// Monster.chase() (see ../../docs/ROADMAP.md, open since phase-3
// M14/M15).
//
// No JVM ground truth possible (same reasoning every other data-model
// milestone's own tests already give) -- verified via hand-derived
// synthetic scenarios covering every branch: the non-adjacent chase path
// (and its confirmed aiPhase-reset side effect), the 3-phase adjacent
// wind-up (idle->winding-up->attack, with the attack-message trigger
// firing ONLY on that first transition), the repeat-attack path (no
// further message), and a real end-to-end integration check against a
// real generated level + real MonsterDatabase/CharacterData/ItemDatabase.
#include <cstdio>
#include <string>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "combat/combat_resolution.h"
#include "monster/monster_runtime.h"
#include "player/player_creation.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace {

using namespace stormhold;

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

GeneratedLevel MakeOpenLevel(int number, int width = 35, int height = 35) {
    GeneratedLevel level;
    level.number = number;
    level.width = width;
    level.height = height;
    level.tiles.assign(static_cast<size_t>(width), std::vector<uint8_t>(static_cast<size_t>(height), 0));
    level.populated = true;
    return level;
}

void TestNonAdjacentMonsterChasesAndResetsAiPhase(const CharacterData& charData, const ItemDatabase& items,
                                                   const MonsterDatabase& monsters) {
    std::printf("-- non-adjacent monster: chases, resets aiPhase to 0, no attack message --\n");
    GeneratedLevel level = MakeOpenLevel(5);
    WorldRegistry world(37);
    std::vector<GeneratedLevel> levels(37);
    levels[4] = level;

    MonsterState m;
    m.spawnId = 1;
    m.typeIndex = 1;  // a real type -- typeIndex 0 would OOB-read monsterDb.typeStats[-1]
    m.dungeonLevel = 5;
    m.tileX = 20;
    m.tileY = 20;
    m.aiPhase = 2;  // pretend it was mid-attack-cadence before -- should reset
    DungeonRuntime::StoreMonster(world, m);

    PlayerState p;
    p.tileX = 10;
    p.tileY = 10;

    JavaRandom combatRng(1);
    JavaRandom ambushRng(2);
    int16_t spawnIdCounter = 1000;

    bool showMessage = CombatResolution::TickMonstersOnLevel(world, levels[4], p, charData, items, monsters, levels,
                                                               1000, combatRng, ambushRng, spawnIdCounter);
    Expect(!showMessage, "a non-adjacent monster should never trigger the attack message");

    auto it = world.monsters[4].find(1);
    Expect(it != world.monsters[4].end(), "the monster should still be registered (stored back)");
    if (it != world.monsters[4].end()) {
        MonsterState stored = MonsterRuntime::FromBytes(it->second);
        Expect(stored.aiPhase == 0, "a non-adjacent monster's aiPhase should reset to 0 (IsAdjacent's own side effect)");
    }
}

void TestAdjacentFirstCallStartsWindup(const CharacterData& charData, const ItemDatabase& items,
                                        const MonsterDatabase& monsters) {
    std::printf("-- adjacent monster, aiPhase 0: starts the 800ms wind-up, no message yet --\n");
    GeneratedLevel level = MakeOpenLevel(6);
    WorldRegistry world(37);
    std::vector<GeneratedLevel> levels(37);
    levels[5] = level;

    MonsterState m;
    m.spawnId = 2;
    m.typeIndex = 1;
    m.dungeonLevel = 6;
    m.tileX = 10;
    m.tileY = 11;  // adjacent to player below
    m.aiPhase = 0;
    DungeonRuntime::StoreMonster(world, m);

    PlayerState p;
    p.tileX = 10;
    p.tileY = 10;

    JavaRandom combatRng(1);
    JavaRandom ambushRng(2);
    int16_t spawnIdCounter = 1000;

    bool showMessage = CombatResolution::TickMonstersOnLevel(world, levels[5], p, charData, items, monsters, levels,
                                                               5000, combatRng, ambushRng, spawnIdCounter);
    Expect(!showMessage, "starting the wind-up (aiPhase 0->1) should NOT show the attack message yet");

    MonsterState stored = MonsterRuntime::FromBytes(world.monsters[5].at(2));
    Expect(stored.aiPhase == 1, "aiPhase should advance to 1 (winding up)");
    Expect(stored.unconfirmedTimestamp == 5000, "unconfirmedTimestamp should be stamped with `now`");
}

void TestAdjacentWindupCompletesAndShowsMessage(const CharacterData& charData, const ItemDatabase& items,
                                                 const MonsterDatabase& monsters) {
    std::printf("-- adjacent monster, aiPhase 1 past 800ms: attacks AND shows the message --\n");
    GeneratedLevel level = MakeOpenLevel(7);
    WorldRegistry world(37);
    std::vector<GeneratedLevel> levels(37);
    levels[6] = level;

    MonsterState m;
    m.spawnId = 3;
    m.typeIndex = 1;
    m.dungeonLevel = 7;
    m.tileX = 10;
    m.tileY = 11;
    m.aiPhase = 1;
    m.unconfirmedTimestamp = 1000;
    DungeonRuntime::StoreMonster(world, m);

    PlayerState p;
    p.tileX = 10;
    p.tileY = 10;
    p.coreStats[2] = 100;  // HP, so a hit has room to register
    p.coreStats[3] = 100;

    JavaRandom combatRng(1);
    JavaRandom ambushRng(2);
    int16_t spawnIdCounter = 1000;

    // now - unconfirmedTimestamp = 1801 > 800.
    bool showMessage = CombatResolution::TickMonstersOnLevel(world, levels[6], p, charData, items, monsters, levels,
                                                               2801, combatRng, ambushRng, spawnIdCounter);
    Expect(showMessage, "the FIRST wind-up-to-attack transition should show the attack message");

    MonsterState stored = MonsterRuntime::FromBytes(world.monsters[6].at(3));
    Expect(stored.unconfirmedTimestamp == 2801, "MonsterTick should have re-stamped unconfirmedTimestamp to `now`");
}

void TestAdjacentRepeatAttackShowsNoMessage(const CharacterData& charData, const ItemDatabase& items,
                                             const MonsterDatabase& monsters) {
    std::printf("-- adjacent monster, aiPhase already 2, past 800ms: repeat attack, NO message --\n");
    GeneratedLevel level = MakeOpenLevel(8);
    WorldRegistry world(37);
    std::vector<GeneratedLevel> levels(37);
    levels[7] = level;

    MonsterState m;
    m.spawnId = 4;
    m.typeIndex = 1;
    m.dungeonLevel = 8;
    m.tileX = 10;
    m.tileY = 11;
    m.aiPhase = 2;  // already past its first attack
    m.unconfirmedTimestamp = 1000;
    DungeonRuntime::StoreMonster(world, m);

    PlayerState p;
    p.tileX = 10;
    p.tileY = 10;
    p.coreStats[2] = 100;
    p.coreStats[3] = 100;

    JavaRandom combatRng(1);
    JavaRandom ambushRng(2);
    int16_t spawnIdCounter = 1000;

    bool showMessage = CombatResolution::TickMonstersOnLevel(world, levels[7], p, charData, items, monsters, levels,
                                                               2801, combatRng, ambushRng, spawnIdCounter);
    Expect(!showMessage, "a repeat attack (aiPhase already left at 2) should NOT show the message again");
}

void TestRealIntegration(const std::string& root) {
    std::printf("-- integration: a real generated level + real monster/character data --\n");
    AssetRoot assets(root);
    ItemDatabase items = ItemDatabase::Load(assets);
    MonsterDatabase monsterDb = MonsterDatabase::Load(assets);
    CharacterData charData = CharacterData::Load(assets);
    DungeonGeometry geometry = DungeonGeometry::Load(assets);

    GeneratedLevel level = DungeonGenerator::PopulateLevel(2, geometry.rows[1], items, monsterDb);
    WorldRegistry world(37);
    DungeonRuntime::RegisterGeneratedSpawns(level, world, monsterDb);
    std::vector<GeneratedLevel> levels(37);
    levels[1] = level;

    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    p.currentLevel = 2;
    p.tileX = 17;
    p.tileY = 17;

    JavaRandom combatRng(42);
    JavaRandom ambushRng(43);
    int16_t spawnIdCounter = 10000;

    bool threw = false;
    try {
        for (int64_t tick = 0; tick < 20; tick++) {
            CombatResolution::TickMonstersOnLevel(world, levels[1], p, charData, items, monsterDb, levels,
                                                   tick * 250, combatRng, ambushRng, spawnIdCounter);
        }
    } catch (const std::exception&) {
        threw = true;
    }
    Expect(!threw, "20 ticks against a real generated level's real monster population should never crash");
    std::printf("  ran 20 ticks against level 2's real monster population (%zu monsters) without crashing\n",
                world.monsters[1].size());
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assets(root);
        ItemDatabase items = ItemDatabase::Load(assets);
        MonsterDatabase monsters = MonsterDatabase::Load(assets);
        CharacterData charData = CharacterData::Load(assets);

        TestNonAdjacentMonsterChasesAndResetsAiPhase(charData, items, monsters);
        TestAdjacentFirstCallStartsWindup(charData, items, monsters);
        TestAdjacentWindupCompletesAndShowsMessage(charData, items, monsters);
        TestAdjacentRepeatAttackShowsNoMessage(charData, items, monsters);
        TestRealIntegration(root);

        if (!g_ok) {
            std::fprintf(stderr, "m37_monster_ai_tick_smoke: FAILED\n");
            return 1;
        }

        std::printf("all checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m37_monster_ai_tick_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
