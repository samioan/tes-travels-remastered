// M39 smoke test: CombatResolution::ResolveAttackInput --
// GameCanvas.resolveAttackInput() (was decompiled/e.java's d(long)), a
// new method this session (no GameCanvas.java-side stub existed before
// now -- found by reading the decompiled source directly while mapping
// out tickMovementAndAI/e(long)'s own dispatch branches, same
// reconnaissance M38's own a()/m() came from).
//
// No JVM ground truth possible (same reasoning every other data-model
// milestone's own tests already give) -- verified via hand-derived
// synthetic scenarios covering every branch: the 500ms cooldown gate,
// the no-target gate, attackRequested clearing either way, and a real
// successful attack actually reaching PlayerAttack (cross-checked
// against player.lastCombatTargetId, the same invariant M14's own
// PlayerAttack test already relies on).
#include <cstdio>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "combat/combat_resolution.h"
#include "player/player_creation.h"
#include "util/java_random.h"

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

void TestNoTargetNeverAttacks(const CharacterData& charData, const ItemDatabase& items,
                                const MonsterDatabase& monsters) {
    std::printf("-- no target set: never attacks, attackRequested still clears --\n");
    PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    std::optional<MonsterState> target;  // nullopt
    bool attackRequested = true;
    int64_t lastAttackTimeMs = 0;
    WorldRegistry world(37);
    JavaRandom rng(1);

    bool attacked = CombatResolution::ResolveAttackInput(player, target, attackRequested, 10000, lastAttackTimeMs,
                                                           charData, items, monsters, rng, world);
    Expect(!attacked, "no target should mean no attack");
    Expect(!attackRequested, "attackRequested should clear even when no attack happened");
    Expect(lastAttackTimeMs == 0, "lastAttackTimeMs should stay untouched when no attack happened");
}

void TestCooldownGate(const CharacterData& charData, const ItemDatabase& items, const MonsterDatabase& monsters) {
    std::printf("-- 500ms cooldown gate --\n");
    PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    std::optional<MonsterState> target = MonsterRuntime::Spawn(5, 1, 2, monsters);
    bool attackRequested = true;
    int64_t lastAttackTimeMs = 1000;
    WorldRegistry world(37);
    JavaRandom rng(1);

    // Only 499ms have passed -- still on cooldown.
    bool attacked = CombatResolution::ResolveAttackInput(player, target, attackRequested, 1499, lastAttackTimeMs,
                                                           charData, items, monsters, rng, world);
    Expect(!attacked, "an attack within the 500ms cooldown should be dropped");
    Expect(lastAttackTimeMs == 1000, "a dropped attack should NOT re-stamp lastAttackTimeMs");
    Expect(!attackRequested, "attackRequested should still clear on a dropped, mid-cooldown attempt");

    // Exactly 500ms have passed -- the original's own >= check allows it.
    attackRequested = true;
    attacked = CombatResolution::ResolveAttackInput(player, target, attackRequested, 1500, lastAttackTimeMs,
                                                      charData, items, monsters, rng, world);
    Expect(attacked, "an attack at exactly the 500ms boundary should connect (>=, not >)");
    Expect(lastAttackTimeMs == 1500, "a successful attack should re-stamp lastAttackTimeMs to `now`");
}

void TestSuccessfulAttackReachesPlayerAttack(const CharacterData& charData, const ItemDatabase& items,
                                               const MonsterDatabase& monsters) {
    std::printf("-- a successful attack really reaches PlayerAttack --\n");
    PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    std::optional<MonsterState> target = MonsterRuntime::Spawn(6, 1, 2, monsters);
    int16_t targetSpawnId = target->spawnId;
    bool attackRequested = true;
    int64_t lastAttackTimeMs = 0;
    WorldRegistry world(37);
    JavaRandom rng(7);

    bool attacked = CombatResolution::ResolveAttackInput(player, target, attackRequested, 5000, lastAttackTimeMs,
                                                           charData, items, monsters, rng, world);
    Expect(attacked, "a fresh target with no cooldown should attack");
    Expect(player.lastCombatTargetId == targetSpawnId,
           "PlayerAttack should have run for real -- lastCombatTargetId set to the target's own spawnId");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assets(root);
        ItemDatabase items = ItemDatabase::Load(assets);
        MonsterDatabase monsters = MonsterDatabase::Load(assets);
        CharacterData charData = CharacterData::Load(assets);

        TestNoTargetNeverAttacks(charData, items, monsters);
        TestCooldownGate(charData, items, monsters);
        TestSuccessfulAttackReachesPlayerAttack(charData, items, monsters);

        if (!g_ok) {
            std::fprintf(stderr, "m39_attack_input_smoke: FAILED\n");
            return 1;
        }

        std::printf("all checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m39_attack_input_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
