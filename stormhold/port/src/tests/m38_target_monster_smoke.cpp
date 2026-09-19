// M38 smoke test: PlayerMovement::MonsterInFront --
// Player.monsterInFront() (was decompiled/j.java's n()), a new method
// this session, needed for GameCanvas.refreshTargetMonster() (was
// decompiled/e.java's a(), also new this session -- see
// ../../../src/GameCanvas.java's own header comment for why its real
// caller, tickMovementAndAI/e(long), remains a stub while this piece is
// still confirmed and transcribed on its own).
//
// No JVM ground truth possible (same reasoning every other data-model
// milestone's own tests already give) -- verified via hand-derived
// synthetic scenarios: a real monster directly ahead is found; nothing
// ahead returns nullopt; and the no-neighbor edge case (a real,
// believed-unreachable landmine `ComputeMoveTarget` itself already
// guards by throwing) returns nullopt gracefully here instead --
// confirming MonsterInFront's own catch-and-translate actually works,
// not just that it compiles.
#include <cstdio>
#include <map>

#include "player/player_movement.h"

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

void TestMonsterDirectlyAheadIsFound() {
    std::printf("-- a real monster directly ahead is found, PlayerState pending fields updated --\n");
    GeneratedLevel level = MakeOpenLevel(5);
    WorldRegistry world(37);

    MonsterState m;
    m.spawnId = 9;
    m.typeIndex = 3;
    m.dungeonLevel = 5;
    m.tileX = 10;
    m.tileY = 9;  // one tile north of the player below
    level.tiles[10][9] |= 2;
    DungeonRuntime::StoreMonster(world, m);

    std::map<int, GeneratedLevel> cache;
    cache[5] = level;
    PlayerMovement::LevelLookup lookup = [&](int n) -> GeneratedLevel& { return cache.at(n); };

    PlayerState p;
    p.currentLevel = 5;
    p.tileX = 10;
    p.tileY = 10;
    p.facing = 1;  // north

    auto found = PlayerMovement::MonsterInFront(p, lookup, world);
    Expect(found.has_value(), "a real monster directly ahead should be found");
    if (found.has_value()) {
        Expect(found->spawnId == 9, "the found monster should be the real one, by spawnId");
    }
    Expect(p.pendingTileX == 10 && p.pendingTileY == 9, "computeMoveTarget(1)'s own side effect should still run");
}

void TestNothingAheadReturnsNullopt() {
    std::printf("-- nothing ahead -> nullopt --\n");
    GeneratedLevel level = MakeOpenLevel(6);
    WorldRegistry world(37);
    std::map<int, GeneratedLevel> cache;
    cache[6] = level;
    PlayerMovement::LevelLookup lookup = [&](int n) -> GeneratedLevel& { return cache.at(n); };

    PlayerState p;
    p.currentLevel = 6;
    p.tileX = 10;
    p.tileY = 10;
    p.facing = 1;

    auto found = PlayerMovement::MonsterInFront(p, lookup, world);
    Expect(!found.has_value(), "an empty tile directly ahead should return nullopt");
}

void TestNoNeighborEdgeReturnsNulloptGracefully() {
    std::printf("-- no-neighbor edge: nullopt, NOT a thrown exception --\n");
    GeneratedLevel level = MakeOpenLevel(7);  // neighborNorth left at 0
    WorldRegistry world(37);
    std::map<int, GeneratedLevel> cache;
    cache[7] = level;
    PlayerMovement::LevelLookup lookup = [&](int n) -> GeneratedLevel& { return cache.at(n); };

    PlayerState p;
    p.currentLevel = 7;
    p.tileX = 17;
    p.tileY = 0;
    p.facing = 1;  // north, straight off the map edge, no real neighbor

    bool threw = false;
    std::optional<MonsterState> found;
    try {
        found = PlayerMovement::MonsterInFront(p, lookup, world);
    } catch (const std::exception&) {
        threw = true;
    }
    Expect(!threw, "MonsterInFront should catch the no-neighbor exception, not let it propagate");
    Expect(!found.has_value(), "the no-neighbor edge should resolve to nullopt");
}

}  // namespace

int main() {
    TestMonsterDirectlyAheadIsFound();
    TestNothingAheadReturnsNullopt();
    TestNoNeighborEdgeReturnsNulloptGracefully();

    if (!g_ok) {
        std::fprintf(stderr, "m38_target_monster_smoke: FAILED\n");
        return 1;
    }

    std::printf("all checks passed\n");
    return 0;
}
