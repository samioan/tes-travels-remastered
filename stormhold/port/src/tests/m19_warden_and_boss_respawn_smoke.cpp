// M19 smoke test: the two side effects player/player_movement.h's
// CommitMove had left flagged-but-skipped since M10, now wired --
// Shop.wardenPresent's on-any-step clear, and the level-37-entry
// forced-respawn of the type-41 "roaming" monster.
#include <cstdio>
#include <map>
#include <string>

#include "assets/asset_root.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "player/player_movement.h"

namespace {

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

stormhold::GeneratedLevel MakeLevel(int number, int width = 35, int height = 35) {
    stormhold::GeneratedLevel level;
    level.number = number;
    level.width = width;
    level.height = height;
    level.tiles.assign(static_cast<size_t>(width), std::vector<uint8_t>(static_cast<size_t>(height), 0));
    return level;
}

void TestLevel37EntryHealsForcedRoamingMonster(const stormhold::MonsterDatabase& monsters,
                                                const stormhold::ItemDatabase& items) {
    std::printf("-- entering level 37 from elsewhere fully heals the registered type-41 monster --\n");
    stormhold::GeneratedLevel level20 = MakeLevel(20);
    stormhold::GeneratedLevel level37 = MakeLevel(37);
    stormhold::WorldRegistry world(37);

    // A damaged type-41 "roaming" monster, plus a damaged type-1 monster
    // that should NOT be touched by this mechanic.
    stormhold::MonsterState boss = stormhold::MonsterRuntime::Spawn(1, 41, 37, monsters);
    int8_t bossMaxHp = boss.currentHp;
    boss.currentHp = 1;  // damaged
    boss.tileX = 10;
    boss.tileY = 10;
    stormhold::DungeonRuntime::StoreMonster(world, boss);

    stormhold::MonsterState other = stormhold::MonsterRuntime::Spawn(2, 1, 37, monsters);
    other.currentHp = 1;
    other.tileX = 20;
    other.tileY = 20;
    stormhold::DungeonRuntime::StoreMonster(world, other);

    std::map<int, stormhold::GeneratedLevel> cache;
    cache[20] = level20;
    cache[37] = level37;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& { return cache.at(n); };

    stormhold::PlayerState p;
    p.currentLevel = 20;
    p.tileX = 17;
    p.tileY = 34;
    p.facing = 3;  // south, straight into level 37 (using a synthetic neighbor)
    p.coreStats[6] = 100;
    cache[20].neighborSouth = 37;

    stormhold::WardenState warden{};
    bool moved = stormhold::PlayerMovement::CommitMove(p, 1, lookup, world, items, monsters, warden);
    Expect(moved, "crossing into level 37 should succeed");
    Expect(p.currentLevel == 37, "the player should have actually landed on level 37");

    stormhold::MonsterState healedBoss =
        stormhold::MonsterRuntime::FromBytes(world.monsters[36].at(1));
    Expect(healedBoss.currentHp == bossMaxHp, "the type-41 monster's HP should reset to the type's own max-HP column");

    stormhold::MonsterState untouchedOther =
        stormhold::MonsterRuntime::FromBytes(world.monsters[36].at(2));
    Expect(untouchedOther.currentHp == 1, "a non-type-41 monster on the same level should NOT be healed");
}

void TestMovingWithinLevel37DoesNotReheal(const stormhold::MonsterDatabase& monsters,
                                           const stormhold::ItemDatabase& items) {
    std::printf("-- moving WITHIN level 37 (not entering it) does not re-trigger the heal --\n");
    stormhold::GeneratedLevel level37 = MakeLevel(37);
    stormhold::WorldRegistry world(37);

    stormhold::MonsterState boss = stormhold::MonsterRuntime::Spawn(1, 41, 37, monsters);
    boss.currentHp = 1;
    boss.tileX = 10;
    boss.tileY = 10;
    stormhold::DungeonRuntime::StoreMonster(world, boss);

    std::map<int, stormhold::GeneratedLevel> cache;
    cache[37] = level37;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& { return cache.at(n); };

    stormhold::PlayerState p;
    p.currentLevel = 37;
    p.tileX = 17;
    p.tileY = 17;
    p.facing = 2;
    p.coreStats[6] = 100;
    stormhold::WardenState warden{};

    stormhold::PlayerMovement::CommitMove(p, 1, lookup, world, items, monsters, warden);
    stormhold::MonsterState stillDamaged = stormhold::MonsterRuntime::FromBytes(world.monsters[36].at(1));
    Expect(stillDamaged.currentHp == 1,
           "the original's own guard is `currentLevel != 37` -- moving around WITHIN level 37 should never reheal");
}

void TestWardenClearedOnStepButTileUntouched(const stormhold::MonsterDatabase& monsters,
                                              const stormhold::ItemDatabase& items) {
    std::printf("-- a step clears warden.present directly, WITHOUT touching any tile (unlike WardenState::Leave) --\n");
    stormhold::GeneratedLevel level5 = MakeLevel(5);
    stormhold::WorldRegistry world(37);
    std::map<int, stormhold::GeneratedLevel> cache;
    cache[5] = level5;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& { return cache.at(n); };

    stormhold::WardenState warden;
    warden.present = true;
    warden.visitCount = 1;

    stormhold::PlayerState p;
    p.currentLevel = 5;
    p.tileX = 17;
    p.tileY = 17;
    p.facing = 2;
    p.coreStats[6] = 100;

    bool moved = stormhold::PlayerMovement::CommitMove(p, 1, lookup, world, items, monsters, warden);
    Expect(moved, "the step itself should succeed independent of the Warden mechanic");
    Expect(!warden.present, "a successful STEP should clear warden.present directly");
    Expect(warden.visitCount == 1,
           "this is a direct flag write, not WardenState::Leave -- visitCount (only Leave/Arrive touch it) should "
           "be untouched");
    // No hub level is even reachable from here (currentLevel==5, not 1),
    // demonstrating this path never needs -- and never gets -- a
    // GeneratedLevel& for the hub at all, unlike WardenState::Leave which
    // requires one specifically to (buggily) mutate.
}

void TestTurnDoesNotClearWarden(const stormhold::ItemDatabase& items, const stormhold::MonsterDatabase& monsters) {
    std::printf("-- a TURN (not a step) should NOT clear warden.present --\n");
    stormhold::GeneratedLevel level5 = MakeLevel(5);
    stormhold::WorldRegistry world(37);
    std::map<int, stormhold::GeneratedLevel> cache;
    cache[5] = level5;
    stormhold::PlayerMovement::LevelLookup lookup = [&](int n) -> stormhold::GeneratedLevel& { return cache.at(n); };

    stormhold::WardenState warden;
    warden.present = true;

    stormhold::PlayerState p;
    p.currentLevel = 5;
    p.tileX = 17;
    p.tileY = 17;
    p.facing = 1;
    p.coreStats[6] = 100;

    stormhold::PlayerMovement::CommitMove(p, 3, lookup, world, items, monsters, warden);  // turn, not a step
    Expect(warden.present, "a turn (dir 3/4) should NOT clear warden.present -- only dir 1/2 (a real step) does");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        stormhold::AssetRoot assets(root);
        stormhold::ItemDatabase items = stormhold::ItemDatabase::Load(assets);
        stormhold::MonsterDatabase monsters = stormhold::MonsterDatabase::Load(assets);

        TestLevel37EntryHealsForcedRoamingMonster(monsters, items);
        TestMovingWithinLevel37DoesNotReheal(monsters, items);
        TestWardenClearedOnStepButTileUntouched(monsters, items);
        TestTurnDoesNotClearWarden(items, monsters);

        if (!g_ok) {
            std::fprintf(stderr, "m19_warden_and_boss_respawn_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m19_warden_and_boss_respawn_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
