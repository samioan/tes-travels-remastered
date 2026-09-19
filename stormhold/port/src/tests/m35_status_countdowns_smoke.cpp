// M35 smoke test: PlayerCombatStats::TickStatusCountdowns --
// GameCanvas.tickStatusCountdowns() (was decompiled/e.java's c(long)), a
// throw-stub in ../../../src/GameCanvas.java until this session. Per-tick
// ailment-timer countdowns for 3 specific ailments (vampirismTimer/
// manaBurnTimer/terrifiedTimer), gated on Player.hasAilment(id) -- NOT
// Player.isEffectActive(id), a similarly-shaped but genuinely different
// check (decompiled/j.java's own k(int) reads ailmentMask directly,
// t(int) reads effectDurations[]) -- confirmed by reading the decompiled
// bytecode directly, not assumed from the two methods' similar shape.
//
// No JVM ground truth possible (same reasoning every other data-model
// milestone's own tests already give) -- verified via hand-derived
// synthetic PlayerState combinations covering every branch: the
// ailment-gate itself, the clamp-to-zero-and-clear-bit transition, and
// (ailment 7 only) the real, confirmed monsterRenderedThisFrame coupling
// (GameCanvas's own unconfirmed_A, the same flag render/
// visible_object_renderer.h's RenderMonsters, M28, already returns).
#include <cstdio>

#include "player/player_combat_stats.h"

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

void TestNoAilmentLeavesTimerUntouched() {
    std::printf("-- no ailment set -> every timer stays untouched --\n");
    PlayerState p;
    p.vampirismTimer = 500;
    p.manaBurnTimer = 500;
    p.terrifiedTimer = 500;
    p.ailmentMask = 0;

    PlayerCombatStats::TickStatusCountdowns(p, 250, /*monsterRenderedThisFrame=*/true);

    Expect(p.vampirismTimer == 500, "vampirismTimer should be untouched without ailment 4");
    Expect(p.manaBurnTimer == 500, "manaBurnTimer should be untouched without ailment 5");
    Expect(p.terrifiedTimer == 500, "terrifiedTimer should be untouched without ailment 7");
}

void TestVampirismCountdownAndExpiry() {
    std::printf("-- ailment 4 (vampirism): countdown, then clamp+clear on expiry --\n");
    PlayerState p;
    p.ailmentMask = 1 << 3;  // bit index 3 = ailment 4
    p.vampirismTimer = 600;

    PlayerCombatStats::TickStatusCountdowns(p, 250, false);
    Expect(p.vampirismTimer == 350, "vampirismTimer should decrement by deltaMs while still positive");
    Expect(PlayerCombatStats::HasAilment(p, 4), "ailment 4 should still be set while the timer hasn't expired");

    PlayerCombatStats::TickStatusCountdowns(p, 250, false);
    Expect(p.vampirismTimer == 100, "vampirismTimer should keep decrementing");
    Expect(PlayerCombatStats::HasAilment(p, 4), "ailment 4 should still be set");

    PlayerCombatStats::TickStatusCountdowns(p, 250, false);
    Expect(p.vampirismTimer == 0, "vampirismTimer should clamp to exactly 0, not go negative");
    Expect(!PlayerCombatStats::HasAilment(p, 4), "ailment 4 should clear once the timer expires");
}

void TestManaBurnCountdownAndExpiry() {
    std::printf("-- ailment 5 (mana burn): same shape, independent bit --\n");
    PlayerState p;
    p.ailmentMask = 1 << 4;  // bit index 4 = ailment 5
    p.manaBurnTimer = 100;

    PlayerCombatStats::TickStatusCountdowns(p, 250, false);
    Expect(p.manaBurnTimer == 0, "manaBurnTimer should clamp to 0 (100 - 250 < 0)");
    Expect(!PlayerCombatStats::HasAilment(p, 5), "ailment 5 should clear once expired");
}

void TestTerrifiedGatedOnMonsterRenderedThisFrame() {
    std::printf("-- ailment 7 (terrified): ALSO gated on monsterRenderedThisFrame --\n");
    PlayerState p;
    p.ailmentMask = 1 << 6;  // bit index 6 = ailment 7
    p.terrifiedTimer = 500;

    PlayerCombatStats::TickStatusCountdowns(p, 250, /*monsterRenderedThisFrame=*/false);
    Expect(p.terrifiedTimer == 500, "terrifiedTimer should NOT tick down when no monster was rendered last frame");
    Expect(PlayerCombatStats::HasAilment(p, 7), "ailment 7 should still be set");

    PlayerCombatStats::TickStatusCountdowns(p, 250, /*monsterRenderedThisFrame=*/true);
    Expect(p.terrifiedTimer == 250, "terrifiedTimer should tick down once a monster WAS rendered");
    Expect(PlayerCombatStats::HasAilment(p, 7), "ailment 7 should still be set (not yet expired)");

    PlayerCombatStats::TickStatusCountdowns(p, 300, /*monsterRenderedThisFrame=*/true);
    Expect(p.terrifiedTimer == 0, "terrifiedTimer should clamp to 0 on expiry (250 - 300 < 0)");
    Expect(!PlayerCombatStats::HasAilment(p, 7), "ailment 7 should clear once expired");
}

void TestIndependentAilmentsDoNotInterfere() {
    std::printf("-- all 3 ailments active at once: independent timers/bits --\n");
    PlayerState p;
    p.ailmentMask = static_cast<int8_t>((1 << 3) | (1 << 4) | (1 << 6));
    p.vampirismTimer = 10;
    p.manaBurnTimer = 500;
    p.terrifiedTimer = 10;

    PlayerCombatStats::TickStatusCountdowns(p, 20, /*monsterRenderedThisFrame=*/true);

    Expect(p.vampirismTimer == 0 && !PlayerCombatStats::HasAilment(p, 4), "ailment 4 should expire independently");
    Expect(p.manaBurnTimer == 480 && PlayerCombatStats::HasAilment(p, 5), "ailment 5 should keep counting down, untouched by ailment 4's expiry");
    Expect(p.terrifiedTimer == 0 && !PlayerCombatStats::HasAilment(p, 7), "ailment 7 should expire independently too");
}

}  // namespace

int main() {
    TestNoAilmentLeavesTimerUntouched();
    TestVampirismCountdownAndExpiry();
    TestManaBurnCountdownAndExpiry();
    TestTerrifiedGatedOnMonsterRenderedThisFrame();
    TestIndependentAilmentsDoNotInterfere();

    if (!g_ok) {
        std::fprintf(stderr, "m35_status_countdowns_smoke: FAILED\n");
        return 1;
    }

    std::printf("all checks passed\n");
    return 0;
}
