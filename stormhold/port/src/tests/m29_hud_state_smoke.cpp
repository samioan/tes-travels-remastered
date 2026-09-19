// M29 smoke test: HudState/IsAdjacentToVarus/IsNpcDialogueDue/
// ResolveHudIconSet -- GameCanvas.paintHud()'s own SELECTION logic
// (resolveHudIconSet()) plus the isNpcDialogueDue() helper it depends
// on, data only, no pixels yet -- same "selection logic first, pixels
// later" split M21/M27 already used, deferred here specifically because
// paintHud() itself needs two primitives this port doesn't have yet
// (a filled rounded-rect and character-glyph text rendering).
//
// No JVM ground truth possible (same reasoning every other data-model
// milestone's own tests already give) -- verified via hand-derived
// synthetic PlayerState/HudState/TargetMonsterInfo combinations covering
// every branch of both methods' own control flow.
#include <cstdio>
#include <optional>

#include "render/hud_state.h"

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

void TestIsAdjacentToVarus() {
    std::printf("-- IsAdjacentToVarus --\n");
    Expect(!IsAdjacentToVarus(2, WardenState::kShopX + 1, WardenState::kShopY),
           "off the hub level (currentLevel != 1) should always be false, even if adjacent by position");

    Expect(IsAdjacentToVarus(1, WardenState::kShopX + 1, WardenState::kShopY), "one tile east should be adjacent");
    Expect(IsAdjacentToVarus(1, WardenState::kShopX - 1, WardenState::kShopY), "one tile west should be adjacent");
    Expect(IsAdjacentToVarus(1, WardenState::kShopX, WardenState::kShopY + 1), "one tile south should be adjacent");
    Expect(IsAdjacentToVarus(1, WardenState::kShopX, WardenState::kShopY - 1), "one tile north should be adjacent");

    Expect(!IsAdjacentToVarus(1, WardenState::kShopX, WardenState::kShopY),
           "standing exactly ON Varus's tile (distance 0) should NOT count as adjacent");
    Expect(!IsAdjacentToVarus(1, WardenState::kShopX + 1, WardenState::kShopY + 1),
           "a diagonal neighbor (Manhattan distance 2) should NOT count as adjacent");
}

void TestIsNpcDialogueDueViaVarus() {
    std::printf("-- IsNpcDialogueDue: Varus branch --\n");
    PlayerState p;
    p.currentLevel = 1;
    p.tileX = WardenState::kShopX + 1;
    p.tileY = WardenState::kShopY;

    Expect(IsNpcDialogueDue(p, std::nullopt), "adjacent to Varus with no target monster should still be true");

    TargetMonsterInfo unrelated{100, 100, 5};
    Expect(IsNpcDialogueDue(p, unrelated),
           "adjacent to Varus should short-circuit true regardless of any target monster");
}

void TestIsNpcDialogueDueViaTargetMonster() {
    std::printf("-- IsNpcDialogueDue: level-37 type-41 target monster branch --\n");
    PlayerState p;
    p.currentLevel = 37;
    p.tileX = 20;
    p.tileY = 20;

    Expect(!IsNpcDialogueDue(p, std::nullopt), "no target monster and not adjacent to Varus should be false");

    TargetMonsterInfo wrongType{20, 21, 5};
    Expect(!IsNpcDialogueDue(p, wrongType), "an adjacent target monster of the WRONG typeIndex should be false");

    TargetMonsterInfo rightTypeAdjacent{20, 21, 41};
    Expect(IsNpcDialogueDue(p, rightTypeAdjacent),
           "an adjacent typeIndex-41 target monster on level 37 should be true");

    TargetMonsterInfo rightTypeFar{25, 25, 41};
    Expect(!IsNpcDialogueDue(p, rightTypeFar), "a non-adjacent typeIndex-41 target monster should be false");

    p.currentLevel = 12;
    Expect(!IsNpcDialogueDue(p, rightTypeAdjacent),
           "the same adjacent typeIndex-41 monster should be false off level 37");
}

void TestResolveHudIconSet() {
    std::printf("-- ResolveHudIconSet: all 4 branches --\n");
    PlayerState p;
    p.currentLevel = 5;  // never adjacent to Varus, no target monster below -> IsNpcDialogueDue() always false here

    HudState hud;
    Expect(ResolveHudIconSet(hud, p, std::nullopt) == 0, "every flag false, dialogue not due -> icon set 0");

    hud.unconfirmedW = true;
    Expect(ResolveHudIconSet(hud, p, std::nullopt) == 2, "only unconfirmedW set, dialogue not due -> icon set 2");

    p.currentLevel = 1;
    p.tileX = WardenState::kShopX + 1;
    p.tileY = WardenState::kShopY;
    Expect(ResolveHudIconSet(hud, p, std::nullopt) == 0,
           "unconfirmedW set, but adjacent to Varus (dialogue due) -> icon set 0, not 2");

    hud = HudState{};
    p.currentLevel = 5;
    hud.unconfirmedM = true;
    Expect(ResolveHudIconSet(hud, p, std::nullopt) == 2, "unconfirmedM alone -> icon set 2");

    hud = HudState{};
    hud.unconfirmedR = true;
    Expect(ResolveHudIconSet(hud, p, std::nullopt) == 2, "unconfirmedR alone -> icon set 2");

    hud.unconfirmedAa = true;
    Expect(ResolveHudIconSet(hud, p, std::nullopt) == 1,
           "unconfirmedAa takes priority over everything else -> icon set 1");
}

}  // namespace

int main() {
    TestIsAdjacentToVarus();
    TestIsNpcDialogueDueViaVarus();
    TestIsNpcDialogueDueViaTargetMonster();
    TestResolveHudIconSet();

    if (!g_ok) {
        std::fprintf(stderr, "m29_hud_state_smoke: FAILED\n");
        return 1;
    }

    std::printf("all checks passed\n");
    return 0;
}
