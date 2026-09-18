// M13 smoke test: PlayerCombatStats against real CharacterData/
// ItemDatabase, plus RollOutcome cross-checked against an independent
// Python reimplementation of java.util.Random (same standard as M5's own
// JVM-captured verification).
#include <cstdio>
#include <string>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/item_database.h"
#include "player/player_combat_stats.h"
#include "player/player_creation.h"

namespace {

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

void TestRollOutcomeAgainstIndependentReference() {
    std::printf("-- RollOutcome vs. an independent Python java.util.Random reimplementation --\n");
    // Computed independently (not from this port's own code): seed the
    // same 48-bit LCG, draw two RandomInt1Based(100) rolls, apply the
    // exact tier formula by hand.
    struct Case {
        int64_t seed;
        int atkChance, defChance;
        int expectedTier;
    };
    Case cases[] = {
        {0, 60, 40, 3}, {42, 60, 40, 1}, {12345, 60, 40, 2},
        {0, 95, 10, 3}, {42, 95, 10, 3}, {12345, 95, 10, 2},
    };
    for (const Case& c : cases) {
        stormhold::JavaRandom rng(c.seed);
        int tier = stormhold::PlayerCombatStats::RollOutcome(rng, c.atkChance, c.defChance);
        Expect(tier == c.expectedTier, "RollOutcome should match the independently-computed reference tier");
    }
}

void TestCombatStatsAgainstRealCharacter(const stormhold::CharacterData& charData,
                                          const stormhold::ItemDatabase& items) {
    std::printf("-- combat stats against a real created character --\n");
    // Barbarian: Miner Pick (Axe, category 1 -> skill 0) + Padded Cloth
    // (Light Armor, category 6 -> equip slot 1, not one of the 4 armor
    // skills at all -- category 6 isn't 5, so DefenseSkillValue/
    // BaseEvasion/DefenseSkillIndex all take their "else" (skill 7)
    // branch here).
    stormhold::PlayerState p = stormhold::PlayerCreation::CreateCharacter(0, "Test", 7, charData, items);

    int activeWeaponSkill = stormhold::PlayerCombatStats::ActiveWeaponSkillIndex(p, charData, items);
    Expect(activeWeaponSkill == 0, "Barbarian's equipped Axe should select skill index 0");

    int attackPower = stormhold::PlayerCombatStats::AttackPower(p, charData, items, true);
    int expectedAttackPower = stormhold::PlayerCombatStats::SkillValue(p, charData, 0, true);
    Expect(attackPower == expectedAttackPower, "AttackPower should equal SkillValue(0, true) with no effects active");

    int attackAccuracy = stormhold::PlayerCombatStats::AttackAccuracy(p, charData, items);
    Expect(attackAccuracy == stormhold::PlayerCombatStats::SkillBonus(p, 0),
           "AttackAccuracy should equal SkillBonus(0) for an Axe-equipped character with no effects active");

    int weaponDamage = stormhold::PlayerCombatStats::WeaponDamage(p, charData, items);
    Expect(weaponDamage == items.questFlags[0], "WeaponDamage should equal item 1's (Miner Pick) magnitude column");

    // Padded Cloth occupies equip slot 1 -- ArmorValue should be exactly
    // 4x its magnitude column, integer-divided by 10.
    int armorValue = stormhold::PlayerCombatStats::ArmorValue(p, charData, items);
    int expectedArmor = 4 * items.questFlags[static_cast<size_t>(27 - 1)] / 10;
    Expect(armorValue == expectedArmor, "ArmorValue should be 4x Padded Cloth's magnitude column, /10");

    int defenseIndex = stormhold::PlayerCombatStats::DefenseSkillIndex(p, items);
    Expect(defenseIndex == 7, "Padded Cloth (category 6) should select defense skill index 7, not 5");
    Expect(stormhold::PlayerCombatStats::BaseEvasion(p, items) == stormhold::PlayerCombatStats::SkillBonus(p, 7),
           "BaseEvasion should equal SkillBonus(7) to match DefenseSkillIndex");

    // No equipment at all: AttackAccuracy/AttackPower/BaseEvasion/
    // DefenseSkillIndex should all fall back to their "unequipped"
    // defaults.
    stormhold::PlayerState unequipped;
    Expect(stormhold::PlayerCombatStats::AttackAccuracy(unequipped, charData, items) == 20,
           "AttackAccuracy should default to 20 with nothing equipped");
    Expect(stormhold::PlayerCombatStats::AttackPower(unequipped, charData, items, true) == 0,
           "AttackPower should default to 0 with nothing equipped");
    Expect(stormhold::PlayerCombatStats::BaseEvasion(unequipped, items) == 20,
           "BaseEvasion should default to 20 with nothing equipped");
    Expect(stormhold::PlayerCombatStats::DefenseSkillIndex(unequipped, items) == -1,
           "DefenseSkillIndex should default to -1 with nothing equipped");
    Expect(stormhold::PlayerCombatStats::ActiveWeaponSkillIndex(unequipped, charData, items) == -1,
           "ActiveWeaponSkillIndex should default to -1 (unarmed) with nothing equipped");
}

void TestIsEffectActive() {
    std::printf("-- IsEffectActive's three duration codes --\n");
    stormhold::PlayerState p;

    p.effectDurations[0] = -1;  // effect 1: always active
    Expect(stormhold::PlayerCombatStats::IsEffectActive(p, 1), "-1 duration should always be active");

    p.effectDurations[1] = -2;  // effect 2: conditional on lastCombatTargetId
    p.lastCombatTargetId = 0;
    Expect(!stormhold::PlayerCombatStats::IsEffectActive(p, 2),
           "-2 duration should be inactive while lastCombatTargetId == 0");
    p.lastCombatTargetId = 5;
    Expect(stormhold::PlayerCombatStats::IsEffectActive(p, 2),
           "-2 duration should become active once lastCombatTargetId != 0");

    p.effectDurations[2] = 3;  // effect 3: counting down
    Expect(stormhold::PlayerCombatStats::IsEffectActive(p, 3), "a positive duration should be active");
    p.effectDurations[2] = 0;
    Expect(!stormhold::PlayerCombatStats::IsEffectActive(p, 3), "a zero duration should not be active");

    stormhold::PlayerCombatStats::ClearEffect(p, 1);
    Expect(!stormhold::PlayerCombatStats::IsEffectActive(p, 1), "ClearEffect should zero the duration");
}

void TestEffectiveStatClamping() {
    std::printf("-- EffectiveStat's Regeneration clamp --\n");
    // SkillValue(..., false) never reads charData.skillAttributeIndex
    // (only the includeBonus=true path does), so a default-constructed
    // CharacterData is fine for this isolated test.
    stormhold::CharacterData charData;

    stormhold::PlayerState p;
    p.coreStats[2] = 90;  // curHP
    p.coreStats[3] = 100;  // maxHP
    p.skills[10][0] = 30;  // skill 10 rank -- SkillValue(10, false) reads this directly

    Expect(stormhold::PlayerCombatStats::EffectiveStat(p, charData, 2) == 90,
           "EffectiveStat should return the raw value while effect 23 is inactive");

    p.effectDurations[22] = -1;  // effect 23: Regeneration, always active
    int boosted = stormhold::PlayerCombatStats::EffectiveStat(p, charData, 2);
    Expect(boosted == 100, "EffectiveStat should clamp the Regeneration bonus at maxHP (coreStats[3])");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        stormhold::AssetRoot assets(root);
        stormhold::CharacterData charData = stormhold::CharacterData::Load(assets);
        stormhold::ItemDatabase items = stormhold::ItemDatabase::Load(assets);

        TestRollOutcomeAgainstIndependentReference();
        TestCombatStatsAgainstRealCharacter(charData, items);
        TestIsEffectActive();
        TestEffectiveStatClamping();

        if (!g_ok) {
            std::fprintf(stderr, "m13_player_combat_stats_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m13_player_combat_stats_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
