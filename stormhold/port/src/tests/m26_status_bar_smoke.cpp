// M26 smoke test: StatusBarPlan (GameCanvas.paintStatusBars()'s own
// HP/Magicka/Fatigue width computations) and GameRenderer::
// RenderStatusBars, the second whole vertical slice this port's renderer
// has (after M25's corridor floor+walls) -- the cheapest remaining paint
// method, per M25's own "what's next" note: entirely self-contained in
// PlayerState + CharacterData, no new asset loading, no world state.
//
// No JVM ground truth possible for a pixel-level check (same reasoning
// M6/M21/M25's own tests already give) -- verified via hand-picked
// PlayerState values for exact width-computation checks (including the
// real, confirmed hpWidth/magickaWidth vs. fatigueWidth clamp asymmetry),
// a real integration check against a freshly created M9 character (should
// render all three bars completely full), and a pixel-exact
// RenderStatusBars check against a synthetic Backbuffer.
#include <cstdio>
#include <string>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/item_database.h"
#include "player/player_combat_stats.h"
#include "player/player_creation.h"
#include "render/game_renderer.h"
#include "render/status_bar_plan.h"

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

void TestPlanBasicWidths(const CharacterData& charData) {
    std::printf("-- StatusBarPlan::Plan: basic width computation --\n");
    PlayerState p;
    p.coreStats[2] = 50;
    p.coreStats[3] = 100;  // HP: 50/100 -> 50*38/100 = 19
    p.coreStats[4] = 30;
    p.coreStats[5] = 60;  // Magicka: 30/60 -> 30*38/60 = 19
    p.coreStats[6] = 10;
    p.coreStats[7] = 40;  // Fatigue: 10/40 -> 10*38/40 = 9

    StatusBarPlan plan = StatusBarPlan::Plan(p, charData);
    Expect(plan.hpWidth == 19, "hpWidth should be effectiveStat(2)*38/coreStats[3]");
    Expect(plan.magickaWidth == 19, "magickaWidth should be effectiveStat(4)*38/coreStats[5]");
    Expect(plan.fatigueWidth == 9, "fatigueWidth should be effectiveStat(6)*38/coreStats[7]");
}

void TestPlanFullStatsGiveFullWidth(const CharacterData& charData) {
    std::printf("-- StatusBarPlan::Plan: current == max gives the full 38px width --\n");
    PlayerState p;
    p.coreStats[2] = p.coreStats[3] = 75;
    p.coreStats[4] = p.coreStats[5] = 40;
    p.coreStats[6] = p.coreStats[7] = 55;

    StatusBarPlan plan = StatusBarPlan::Plan(p, charData);
    Expect(plan.hpWidth == 38, "HP at max should fill the whole 38px track");
    Expect(plan.magickaWidth == 38, "Magicka at max should fill the whole 38px track");
    Expect(plan.fatigueWidth == 38, "Fatigue at max should fill the whole 38px track (well under the 40 clamp)");
}

void TestFatigueOnlyClampAsymmetry(const CharacterData& charData) {
    std::printf("-- StatusBarPlan::Plan: fatigueWidth's own >40 clamp, hpWidth/magickaWidth have none --\n");
    PlayerState p;
    // Current above max, with effect 23 NOT active -- EffectiveStat only
    // clamps to the max stat while that effect is active (see
    // PlayerCombatStats::EffectiveStat's own header comment), so an
    // out-of-range current value passes straight through unclamped here,
    // exactly like the original. Same construction for all 3 stats to
    // confirm the clamp really is fatigueWidth-only, not a general safety
    // net this port added to all three.
    p.coreStats[2] = 200;
    p.coreStats[3] = 10;  // effectiveStat(2)=200 -> 200*38/10 = 760, NOT clamped
    p.coreStats[4] = 200;
    p.coreStats[5] = 10;  // effectiveStat(4)=200 -> 760, NOT clamped
    p.coreStats[6] = 200;
    p.coreStats[7] = 10;  // effectiveStat(6)=200 -> 760, clamped to 40

    StatusBarPlan plan = StatusBarPlan::Plan(p, charData);
    Expect(plan.hpWidth == 760, "hpWidth has no clamp at all -- matches the original's own missing bounds check");
    Expect(plan.magickaWidth == 760, "magickaWidth has no clamp at all either");
    Expect(plan.fatigueWidth == 40, "fatigueWidth is the ONLY one of the three clamped, to 40 -- a real, confirmed "
                                     "asymmetry preserved from the original, not a bug introduced by this port");
}

void TestFreshCharacterRendersAllBarsFull(const CharacterData& charData, const ItemDatabase& items) {
    std::printf("-- integration: a freshly created M9 character renders all 3 bars full --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Test", /*spawnId=*/1, charData, items);
    StatusBarPlan plan = StatusBarPlan::Plan(p, charData);
    Expect(plan.hpWidth == 38, "a fresh character should start at full HP -- a full-width bar");
    Expect(plan.magickaWidth == 38, "a fresh character should start at full Magicka -- a full-width bar");
    Expect(plan.fatigueWidth == 38, "a fresh character should start at full Fatigue -- a full-width bar");
}

void TestRenderStatusBarsPixels() {
    std::printf("-- GameRenderer::RenderStatusBars: pixel-exact synthetic check --\n");
    StatusBarPlan plan;
    plan.hpWidth = 20;
    plan.magickaWidth = 0;
    plan.fatigueWidth = 38;

    Backbuffer bb;
    uint16_t sentinel = PackRGB565(1, 2, 3);
    bb.Fill(sentinel);
    GameRenderer::RenderStatusBars(bb, plan);

    uint16_t track = PackRGB565(255, 255, 0);
    uint16_t red = PackRGB565(255, 0, 0);
    uint16_t green = PackRGB565(0, 255, 0);
    uint16_t blue = PackRGB565(0, 0, 255);

    // HP track (y=130..136), fill (6,131)..(6+20,136).
    Expect(bb.Data()[130 * Backbuffer::kWidth + 5] == track, "HP track's own top-left corner should be yellow");
    Expect(bb.Data()[131 * Backbuffer::kWidth + 6] == red, "HP fill's first column should be red");
    Expect(bb.Data()[131 * Backbuffer::kWidth + (6 + 19)] == red, "HP fill's last (20th) column should still be red");
    Expect(bb.Data()[131 * Backbuffer::kWidth + (6 + 20)] == track,
           "just past the 20px HP fill, the track's own yellow should show through");

    // Magicka track (y=138..144), zero-width fill -> the WHOLE track stays yellow.
    Expect(bb.Data()[139 * Backbuffer::kWidth + 6] == track, "a zero-width Magicka fill should leave the track fully yellow");
    (void)green;

    // Fatigue track (y=146..152), full 38px fill.
    Expect(bb.Data()[147 * Backbuffer::kWidth + 6] == blue, "Fatigue fill's first column should be blue");
    Expect(bb.Data()[147 * Backbuffer::kWidth + (6 + 37)] == blue, "Fatigue fill's last (38th) column should still be blue");
    Expect(bb.Data()[147 * Backbuffer::kWidth + (6 + 38)] == track,
           "just past the 38px Fatigue fill (within the 40px track), yellow should show through");

    // Nothing outside the 3 bars' own rows should be touched.
    Expect(bb.Data()[0] == sentinel, "a pixel far from any status bar should be untouched");
    Expect(bb.Data()[129 * Backbuffer::kWidth + 5] == sentinel, "the row just above the HP track should be untouched");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assets(root);
        CharacterData charData = CharacterData::Load(assets);
        ItemDatabase items = ItemDatabase::Load(assets);

        TestPlanBasicWidths(charData);
        TestPlanFullStatsGiveFullWidth(charData);
        TestFatigueOnlyClampAsymmetry(charData);
        TestFreshCharacterRendersAllBarsFull(charData, items);
        TestRenderStatusBarsPixels();

        if (!g_ok) {
            std::fprintf(stderr, "m26_status_bar_smoke: FAILED\n");
            return 1;
        }

        std::printf("all checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m26_status_bar_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
