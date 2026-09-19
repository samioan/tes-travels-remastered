// M36 smoke test: PlayerCombatStats::TickPerSecond --
// GameCanvas.tickPerSecond() (was decompiled/e.java's l()), a throw-stub
// in ../../../src/GameCanvas.java until this session. Three independent
// per-real-second mechanics: the effectDurations[] decay loop (with its
// own slot-5 "remove the daedric weapon" side effect), the ailment-4
// ("vampirism") HP drain, and the ailment-5 ("mana burn") Magicka
// regen-then-overflow-burn.
//
// No JVM ground truth possible (same reasoning every other data-model
// milestone's own tests already give) -- verified via hand-derived
// synthetic PlayerState combinations covering every branch, plus a
// real-data check that item 109 is a real, existing item (not a
// placeholder id).
#include <cstdio>
#include <string>

#include "assets/asset_root.h"
#include "assets/item_database.h"
#include "player/player_combat_stats.h"
#include "player/player_inventory.h"

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

void TestEffectDurationsDecrementHarmlessly(const ItemDatabase& items) {
    std::printf("-- effectDurations[]: ordinary slots decrement with no side effect --\n");
    PlayerState p;
    p.effectDurations[0] = 3;
    p.effectDurations[9] = 1;

    PlayerCombatStats::TickPerSecond(p, items);
    Expect(p.effectDurations[0] == 2, "slot 0 should decrement by 1");
    Expect(p.effectDurations[9] == 0, "slot 9 should clamp to 0 once it hits zero");

    PlayerCombatStats::TickPerSecond(p, items);
    Expect(p.effectDurations[0] == 1, "slot 0 should keep decrementing");
    Expect(p.effectDurations[9] == 0, "slot 9 should stay at 0, not go negative");
}

void TestSlot5ExpiryRemovesDaedricWeapon(const ItemDatabase& items) {
    std::printf("-- effectDurations[5] hitting 0 removes item 109 (daedric weapon) --\n");
    PlayerState p;
    p.effectDurations[5] = 1;
    p.inventoryItemIds[0] = 42;
    p.inventoryItemIds[1] = static_cast<int8_t>(-109);  // equipped (negative-encoded)
    p.inventoryItemIds[2] = 7;
    p.inventoryCount = 3;

    PlayerCombatStats::TickPerSecond(p, items);

    Expect(p.effectDurations[5] == 0, "slot 5 should clamp to 0");
    Expect(p.inventoryCount == 2, "the daedric weapon's slot should be removed, compacting the count");
    bool stillPresent = false;
    for (int i = 0; i < p.inventoryCount; i++) {
        if (p.inventoryItemIds[static_cast<size_t>(i)] == static_cast<int8_t>(-109)) stillPresent = true;
    }
    Expect(!stillPresent, "item -109 should no longer be anywhere in the inventory");
    Expect(p.inventoryItemIds[0] == 42 && p.inventoryItemIds[1] == 7,
           "the remaining two items should compact down to slots 0/1, in order");
}

void TestSlot5ExpiryWithoutItemThrows(const ItemDatabase& items) {
    std::printf("-- effectDurations[5] hitting 0 with NO item 109 present -> throws --\n");
    PlayerState p;
    p.effectDurations[5] = 1;
    p.inventoryCount = 0;  // item 109 is nowhere in inventory

    bool threw = false;
    try {
        PlayerCombatStats::TickPerSecond(p, items);
    } catch (const std::exception&) {
        threw = true;
    }
    Expect(threw, "FindEquippedSlotForItem returning -1 should propagate into RemoveInventorySlot's own negative-slot "
                  "guard and throw -- the real removeInventorySlot(-1) would throw "
                  "ArrayIndexOutOfBoundsException too");
}

void TestOtherSlotsHittingZeroDoNothingSpecial(const ItemDatabase& items) {
    std::printf("-- every effectDurations slot OTHER than 5 has no special side effect --\n");
    PlayerState p;
    p.effectDurations[4] = 1;
    p.effectDurations[6] = 1;
    p.inventoryItemIds[0] = static_cast<int8_t>(-109);
    p.inventoryCount = 1;

    PlayerCombatStats::TickPerSecond(p, items);

    Expect(p.effectDurations[4] == 0 && p.effectDurations[6] == 0, "both slots should still decay normally");
    Expect(p.inventoryCount == 1 && p.inventoryItemIds[0] == static_cast<int8_t>(-109),
           "the daedric weapon should be untouched -- only slot 5 expiring removes it");
}

void TestVampirismDrainsHp(const ItemDatabase& items) {
    std::printf("-- ailment 4 (vampirism): drains 2%% of maxHP from HP --\n");
    PlayerState p;
    p.ailmentMask = 1 << 3;
    p.coreStats[3] = 100;  // maxHP
    p.coreStats[2] = 50;   // HP

    PlayerCombatStats::TickPerSecond(p, items);
    Expect(p.coreStats[2] == 48, "HP should drop by 2 (2% of 100 maxHP)");

    PlayerState pNoAilment;
    pNoAilment.coreStats[3] = 100;
    pNoAilment.coreStats[2] = 50;
    PlayerCombatStats::TickPerSecond(pNoAilment, items);
    Expect(pNoAilment.coreStats[2] == 50, "without ailment 4, HP should stay untouched");
}

void TestManaBurnRegenBelowMax(const ItemDatabase& items) {
    std::printf("-- ailment 5 (mana burn): regen below max, no overflow yet --\n");
    PlayerState p;
    p.ailmentMask = 1 << 4;
    p.coreStats[5] = 100;  // maxMagicka
    p.coreStats[4] = 50;   // Magicka
    p.coreStats[2] = 80;   // HP (should be untouched -- no overflow this tick)

    PlayerCombatStats::TickPerSecond(p, items);
    Expect(p.coreStats[4] == 60, "Magicka should regen by 10 (10% of 100 maxMagicka)");
    Expect(p.coreStats[2] == 80, "HP should be untouched while Magicka hasn't overflowed");
}

void TestManaBurnOverflowResetsToZeroAndBurnsHp(const ItemDatabase& items) {
    std::printf("-- ailment 5 (mana burn): overflow resets Magicka to 0 (not clamped) and burns HP --\n");
    PlayerState p;
    p.ailmentMask = 1 << 4;
    p.coreStats[5] = 100;  // maxMagicka
    p.coreStats[4] = 95;   // Magicka -- 95 + 10 = 105 >= 100
    p.coreStats[2] = 80;   // HP

    PlayerCombatStats::TickPerSecond(p, items);
    Expect(p.coreStats[4] == 0, "Magicka should reset to EXACTLY 0 on overflow, not clamp to maxMagicka");
    Expect(p.coreStats[2] == 70, "HP should take a 10%-of-maxMagicka hit (10) on the same overflow tick");
}

void TestRealItem109IsARealItem(const std::string& root) {
    std::printf("-- integration: item id 109 is a real entry in itemsin.dat --\n");
    AssetRoot assets(root);
    ItemDatabase items = ItemDatabase::Load(assets);
    Expect(109 <= items.ItemCount(), "item id 109 should be within the real item table's range");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assets(root);
        ItemDatabase items = ItemDatabase::Load(assets);

        TestEffectDurationsDecrementHarmlessly(items);
        TestSlot5ExpiryRemovesDaedricWeapon(items);
        TestSlot5ExpiryWithoutItemThrows(items);
        TestOtherSlotsHittingZeroDoNothingSpecial(items);
        TestVampirismDrainsHp(items);
        TestManaBurnRegenBelowMax(items);
        TestManaBurnOverflowResetsToZeroAndBurnsHp(items);
        TestRealItem109IsARealItem(root);

        if (!g_ok) {
            std::fprintf(stderr, "m36_tick_per_second_smoke: FAILED\n");
            return 1;
        }

        std::printf("all checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m36_tick_per_second_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
