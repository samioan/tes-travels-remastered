// M9 smoke test: PlayerCreation against real CharacterData/ItemDatabase,
// for all 7 classes.
//
// No JVM ground truth possible here either (same RegisteredMIDlet static-
// initializer issue as M6/M7/M8), but character creation is deterministic
// (see player_creation.h's own header comment -- no RNG involved at all),
// so every expected value below is either cross-checked directly against
// the loaded CharacterData/ItemDatabase tables, or an independently
// derived constant (the known-spell mask/selectedSpellId, confirmed
// against real extracted/charin.dat via a standalone script, not
// re-derived by this test's own code from the same algorithm being
// tested).
#include <cmath>
#include <cstdio>
#include <string>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/item_database.h"
#include "player/player_creation.h"

namespace {

bool Expect(bool cond, const char* what) {
    if (!cond) std::printf("  FAIL: %s\n", what);
    return cond;
}

// Player.java's classStartingItems -- duplicated here (not reused from the
// port's internal table) so this test doesn't just check the port agrees
// with itself.
constexpr int kClassStartingItems[7][2] = {
    {1, 27}, {7, 27}, {7, 22}, {17, 27}, {12, 22}, {17, 27}, {12, 22},
};

// Confirmed against real extracted/charin.dat via a standalone script: all
// 5 spell-tier skills (Alteration/Conjuration/Destruction/Illusion/
// Restoration, skill indices 1/3/4/6/10) have a nonzero starting rank
// (35-45) for EVERY one of the 7 classes, not just caster archetypes --
// unlike dawnstar, where only Battlemage/Sorcerer/Spellsword-shaped
// classes start with any known spells at all. So every Stormhold class
// ends up with the exact same knownSpellsMask/selectedSpellId out of
// character creation. See docs/PORT_ROADMAP.md's M9 entry.
constexpr uint32_t kExpectedKnownSpellsMask = 0x108421;
constexpr int8_t kExpectedSelectedSpellId = 1;

bool CheckClass(int classIndex, const stormhold::CharacterData& charData, const stormhold::ItemDatabase& items) {
    bool ok = true;

    stormhold::PlayerState p =
        stormhold::PlayerCreation::CreateCharacter(classIndex, "Test", /*spawnId=*/7, charData, items);

    const auto& tmpl = charData.classTemplates[static_cast<size_t>(classIndex)];

    ok &= Expect(p.classIndex == classIndex, "classIndex mismatch");
    ok &= Expect(p.raceIndex == tmpl[1], "raceIndex should match classTemplates[classIndex][1]");

    for (int i = 0; i < 8; i++) {
        ok &= Expect(p.attributes[static_cast<size_t>(2 * i)] == tmpl[static_cast<size_t>(2 + i)],
                     "attribute base should match classTemplates column");
        ok &= Expect(p.attributes[static_cast<size_t>(2 * i + 1)] == 0, "attribute bonus should start at 0");
    }
    ok &= Expect(p.classMagickaFactor == tmpl[10], "classMagickaFactor mismatch");
    ok &= Expect(p.classUnknownPair[0] == tmpl[11] && p.classUnknownPair[1] == tmpl[12],
                 "classUnknownPair mismatch");

    ok &= Expect(p.coreStats[0] == 1, "level should start at 1");
    ok &= Expect(p.coreStats[1] == 0, "exp should start at 0");
    int16_t expectedMaxHp = static_cast<int16_t>((p.attributes[0] + p.attributes[10]) / 2);
    int16_t expectedMaxMagicka = static_cast<int16_t>(p.classMagickaFactor * p.attributes[2] / 4);
    int16_t expectedMaxFatigue =
        static_cast<int16_t>(p.attributes[0] + p.attributes[4] + p.attributes[6] + p.attributes[10]);
    ok &= Expect(p.coreStats[3] == expectedMaxHp, "maxHP derivation mismatch");
    ok &= Expect(p.coreStats[5] == expectedMaxMagicka, "maxMagicka derivation mismatch");
    ok &= Expect(p.coreStats[7] == expectedMaxFatigue, "maxFatigue derivation mismatch");
    ok &= Expect(p.coreStats[2] == p.coreStats[3], "curHP should start full");
    ok &= Expect(p.coreStats[4] == p.coreStats[5], "curMagicka should start full");
    ok &= Expect(p.coreStats[6] == p.coreStats[7], "curFatigue should start full");
    ok &= Expect(p.coreStats[8] == 0 && p.coreStats[9] == 0, "coreStats[8]/[9] should start at 0");
    ok &= Expect(p.levelUpAttributeFlags == 0, "levelUpAttributeFlags should start at 0");

    int col = 13;
    for (int i = 0; i < 14; i++) {
        int16_t threshold = tmpl[static_cast<size_t>(col++)];
        int16_t rank = tmpl[static_cast<size_t>(col++)];
        ok &= Expect(p.skills[static_cast<size_t>(i)][0] == threshold, "skill threshold mismatch");
        ok &= Expect(p.skills[static_cast<size_t>(i)][1] == rank, "skill rank mismatch");
        ok &= Expect(p.skills[static_cast<size_t>(i)][2] == 0, "skill exp should start at 0");
    }

    ok &= Expect(p.knownSpellsMask == kExpectedKnownSpellsMask, "knownSpellsMask should match all-classes constant");
    ok &= Expect(p.selectedSpellId == kExpectedSelectedSpellId, "selectedSpellId should match all-classes constant");

    // grantStartingItems(): 2 items granted and auto-equipped -- weapon
    // into equip slot 0, armor into equip slot 1 for every class
    // (confirmed against real itemsin.dat: every classStartingItems pair
    // is one weapon-category item + one armor-category item, always
    // resolving to two DISTINCT equip slots, so the equipItem() swap path
    // never actually triggers during character creation).
    ok &= Expect(p.inventoryCount == 2, "inventoryCount should be 2 after grantStartingItems");
    int weaponId = kClassStartingItems[classIndex][0];
    int armorId = kClassStartingItems[classIndex][1];
    ok &= Expect(std::abs(p.inventoryItemIds[0]) == weaponId, "slot 0 item id mismatch");
    ok &= Expect(std::abs(p.inventoryItemIds[1]) == armorId, "slot 1 item id mismatch");
    ok &= Expect(p.inventoryItemIds[0] < 0 && p.inventoryItemIds[1] < 0, "both starting items should be equipped");
    int weaponEquipSlot = items.EquipSlotOf(weaponId);
    int armorEquipSlot = items.EquipSlotOf(armorId);
    ok &= Expect(weaponEquipSlot != armorEquipSlot, "weapon/armor should occupy distinct equip slots");
    ok &= Expect(p.equippedItems[static_cast<size_t>(weaponEquipSlot)] == weaponId, "equippedItems weapon slot mismatch");
    ok &= Expect(p.equippedItems[static_cast<size_t>(armorEquipSlot)] == armorId, "equippedItems armor slot mismatch");
    // Both starting items share the SAME spawnId (grantStartingItems()
    // calls Item.nextSpawnId() once, not once per item) -- see
    // player_creation.h's own header comment.
    int32_t expectedPacked = static_cast<int32_t>(7) << 16;
    ok &= Expect(p.inventoryItemData[0] == expectedPacked && p.inventoryItemData[1] == expectedPacked,
                 "inventoryItemData should pack the shared spawnId with 0 charge");

    ok &= Expect(p.giftPointsFound == 0 && p.rumorRevealStep == 0 && p.wardenLoreStep == 0,
                 "gift/rumor/warden-lore counters should start at 0");
    ok &= Expect(p.ailmentMask == 0 && p.vampirismTimer == 0 && p.manaBurnTimer == 0 && p.terrifiedTimer == 0,
                 "ailment state should start clear");
    ok &= Expect(p.currentLevel == 1 && p.tileX == 9 && p.tileY == 10 && p.facing == 1,
                 "should spawn at the new-character hub position (9, 10)");
    ok &= Expect(p.pendingLevel == p.currentLevel && p.pendingTileX == p.tileX && p.pendingTileY == p.tileY &&
                     p.pendingFacing == p.facing,
                 "pending position should mirror actual position");
    ok &= Expect(p.campLevel == 0 && p.campX == 0 && p.campY == 0 && p.campFacing == 0,
                 "camp bookmark should start clear");
    ok &= Expect(!p.increaseHarmBuff && !p.increaseArmorBuff && !p.safeCampingBuff, "buffs should start clear");

    std::printf("  %-12s race=%-10s maxHP=%-4d maxMagicka=%-4d maxFatigue=%-4d weapon=%-16s armor=%-16s\n",
                charData.classNames[static_cast<size_t>(classIndex)].c_str(),
                charData.raceNames[static_cast<size_t>(p.raceIndex)].c_str(), p.coreStats[3], p.coreStats[5],
                p.coreStats[7], items.name[static_cast<size_t>(weaponId - 1)].c_str(),
                items.name[static_cast<size_t>(armorId - 1)].c_str());

    return ok;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        stormhold::AssetRoot assets(root);
        stormhold::CharacterData charData = stormhold::CharacterData::Load(assets);
        stormhold::ItemDatabase items = stormhold::ItemDatabase::Load(assets);

        bool ok = true;
        for (int classIndex = 0; classIndex < charData.ClassCount(); classIndex++) {
            if (!CheckClass(classIndex, charData, items)) ok = false;
        }

        if (!ok) {
            std::fprintf(stderr, "m9_player_creation_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m9_player_creation_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
