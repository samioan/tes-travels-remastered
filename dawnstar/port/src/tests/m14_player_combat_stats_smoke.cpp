// M14 smoke test: PlayerCombatStats against real CharacterData/
// ItemDatabase (M11's PlayerCreation builds real starting characters) and
// hand-traced expectations from ../../../src/Player.java's source, same
// methodology as M12/M13 (no JVM ground truth available -- see
// player_movement.h's header comment for why that's still true here).
#include <cstdio>
#include <cstdlib>
#include <string>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/item_database.h"
#include "player/player_combat_stats.h"
#include "player/player_creation.h"
#include "util/java_random.h"

namespace {

using dawnstar::CharacterData;
using dawnstar::ItemDatabase;
using dawnstar::PlayerCombatStats;
using dawnstar::PlayerState;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

// Independent transcription of Item.column(1,id)'s category dispatch
// (1->skill0/AttackAccuracy col 0, 2->skill2, 3->skill8, else->skill12),
// written straight from Player.java rather than copied from
// player_combat_stats.cpp, to actually cross-check that file's mapping.
int ExpectedWeaponSkillIndex(const ItemDatabase& items, int itemId) {
    int category = std::abs(static_cast<int>(items.category[static_cast<size_t>(itemId - 1)]));
    if (category == 1) return 0;
    if (category == 2) return 2;
    return category == 3 ? 8 : 12;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        CharacterData charData = CharacterData::Load(archive);
        ItemDatabase items = ItemDatabase::Load(archive);

        // --- isEffectActive / clearEffect ---
        {
            PlayerState p;
            p.effectDurations[4] = -1;  // effect id 5: "until cured"
            p.effectDurations[6] = -2;  // effect id 7: "until in combat"
            p.effectDurations[9] = 3;   // effect id 10: counting down
            p.effectDurations[13] = 0;  // effect id 14: inactive
            p.combatTargetSpawnId = 0;

            Check(PlayerCombatStats::IsEffectActive(p, 5) == true, "effectDurations==-1 should be active");
            Check(PlayerCombatStats::IsEffectActive(p, 7) == false, "effectDurations==-2 with no combat target should be inactive");
            Check(PlayerCombatStats::IsEffectActive(p, 10) == true, "effectDurations>0 should be active");
            Check(PlayerCombatStats::IsEffectActive(p, 14) == false, "effectDurations==0 should be inactive");

            p.combatTargetSpawnId = 42;
            Check(PlayerCombatStats::IsEffectActive(p, 7) == true, "effectDurations==-2 with a combat target should be active");

            PlayerCombatStats::ClearEffect(p, 10);
            Check(p.effectDurations[9] == 0, "clearEffect should zero the duration");
        }

        // --- hasAilment ---
        {
            PlayerState p;
            p.ailmentMask = static_cast<int8_t>((1 << 0) | (1 << 5));
            Check(PlayerCombatStats::HasAilment(p, 1) == true, "hasAilment bit 0");
            Check(PlayerCombatStats::HasAilment(p, 6) == true, "hasAilment bit 5");
            Check(PlayerCombatStats::HasAilment(p, 2) == false, "hasAilment bit 1 should be clear");
        }

        // --- gainSkillExp: rank-up, attribute-flag, and level-up mechanics ---
        {
            PlayerState p;
            p.skills[3][0] = 5;
            p.skills[3][2] = 8;
            p.coreStats[1] = 8;  // levelExp, one point short of leveling
            p.coreStats[0] = 1;
            p.attributeIncreaseFlags = 0;

            // +25 exp: 8+25=33 -> 3 rank-ups (33 -> 23 -> 13 -> 3), skill rank 5->8,
            // levelExp 8+3=11 >= 10 -> levels up.
            PlayerCombatStats::GainSkillExp(p, charData, 3, 25);
            Check(p.skills[3][0] == 8, "gainSkillExp should rank up 3 times for +25 exp from 8");
            Check(p.skills[3][2] == 3, "gainSkillExp should leave the exp remainder (33 mod 10 = 3)");
            Check(p.coreStats[1] == 11, "gainSkillExp should add 1 levelExp point per rank-up");
            Check(p.coreStats[0] == 2, "levelExp reaching >=10 should level the character up");
            Check(p.levelUpPending == true, "levelUpPending should be set alongside the level-up");

            int16_t attrIdx = charData.skillAttributeIndex[3];
            int bit = attrIdx / 2;
            Check((p.attributeIncreaseFlags & (1 << bit)) != 0, "gainSkillExp should flag the governing attribute");
        }

        // --- rollOutcome: deterministic branch coverage (chances chosen
        // so the outcome doesn't depend on the actual dice, except where
        // noted -- see the file header comment) ---
        {
            dawnstar::JavaRandom rng(2024);
            auto r = PlayerCombatStats::RollOutcome(100, 0, rng);
            Check(r.outcome == 3 && r.crit == true, "rollOutcome(100,0) should always be a critical hit (outcome 3)");
        }
        {
            dawnstar::JavaRandom rng(2024);
            auto r = PlayerCombatStats::RollOutcome(0, 100, rng);
            Check(r.outcome == 0 && r.crit == false, "rollOutcome(0,100) should always be outcome 0 (defended, no crit)");
        }
        {
            dawnstar::JavaRandom probe(555);
            int atkRoll = dawnstar::LingoRandomInt(probe, 100);
            int defRoll = dawnstar::LingoRandomInt(probe, 100);
            int expected = defRoll >= atkRoll ? 2 : 1;

            dawnstar::JavaRandom rng(555);
            auto r = PlayerCombatStats::RollOutcome(100, 100, rng);
            Check(r.crit == true, "rollOutcome(100,100) should always be a crit roll");
            Check(r.outcome == expected, "rollOutcome(100,100)'s outcome should match the hand-traced roll comparison");

            dawnstar::JavaRandom rng2(555);
            auto r2 = PlayerCombatStats::RollOutcome(0, 0, rng2);
            Check(r2.crit == false, "rollOutcome(0,0) should never crit");
            Check(r2.outcome == expected, "rollOutcome(0,0)'s outcome should match the same roll comparison");
        }

        // --- equipment-dependent stats, using each class's real starting
        // gear (M11's PlayerCreation), no active effects/buffs ---
        dawnstar::JavaRandom globalRng(31337);
        for (int classIdx = 0; classIdx < charData.ClassCount(); classIdx++) {
            PlayerState p = dawnstar::PlayerCreation::CreateCharacter(classIdx, "Combatant", charData, items, globalRng);

            int weaponId = p.equippedItems[0];
            int offhandId = p.equippedItems[1];

            int expectedActiveSkill = weaponId == 0 ? -1 : ExpectedWeaponSkillIndex(items, weaponId);
            int actualActiveSkill = PlayerCombatStats::ActiveWeaponSkillIndex(p, charData, items);
            Check(actualActiveSkill == expectedActiveSkill, "ActiveWeaponSkillIndex should match the weapon-category mapping");

            int expectedDamage = weaponId == 0 ? 0 : items.questFlags[static_cast<size_t>(weaponId - 1)];
            int actualDamage = PlayerCombatStats::WeaponDamage(p, charData, items);
            Check(actualDamage == expectedDamage, "WeaponDamage with no buffs should be the weapon's raw magnitude column");

            int expectedAccuracy;
            if (weaponId == 0) {
                expectedAccuracy = 20;
            } else {
                expectedAccuracy = PlayerCombatStats::SkillBonus(p, expectedActiveSkill);
            }
            int actualAccuracy = PlayerCombatStats::AttackAccuracy(p, charData, items);
            Check(actualAccuracy == expectedAccuracy, "AttackAccuracy should match the weapon-category skillBonus");

            int expectedEvasion = offhandId == 0 ? 20
                                                  : (std::abs(static_cast<int>(items.category[static_cast<size_t>(offhandId - 1)])) == 5
                                                         ? PlayerCombatStats::SkillBonus(p, 5)
                                                         : PlayerCombatStats::SkillBonus(p, 7));
            int actualEvasion = PlayerCombatStats::BaseEvasion(p, items);
            Check(actualEvasion == expectedEvasion, "BaseEvasion should match the offhand-category skillBonus");

            int expectedArmor = 0;
            if (p.equippedItems[1] != 0) expectedArmor += 4 * items.questFlags[static_cast<size_t>(p.equippedItems[1] - 1)];
            if (p.equippedItems[2] != 0) expectedArmor += 2 * items.questFlags[static_cast<size_t>(p.equippedItems[2] - 1)];
            if (p.equippedItems[3] != 0) expectedArmor += 2 * items.questFlags[static_cast<size_t>(p.equippedItems[3] - 1)];
            if (p.equippedItems[4] != 0) expectedArmor += items.questFlags[static_cast<size_t>(p.equippedItems[4] - 1)];
            if (p.equippedItems[5] != 0) expectedArmor += items.questFlags[static_cast<size_t>(p.equippedItems[5] - 1)];
            expectedArmor /= 10;
            int actualArmor = PlayerCombatStats::ArmorValue(p, charData, items);
            Check(actualArmor == expectedArmor, "ArmorValue with no buffs should match the weighted-sum/10 formula");

            std::printf("class[%d] %-12s weapon=%s dmg=%d acc=%d evasion=%d armor=%d\n", classIdx,
                        charData.classNames[static_cast<size_t>(classIdx)].c_str(),
                        weaponId != 0 ? items.name[static_cast<size_t>(weaponId - 1)].c_str() : "(none)", actualDamage,
                        actualAccuracy, actualEvasion, actualArmor);
        }

        // --- effect/buff-conditional bonuses, on top of a real character ---
        {
            PlayerState p =
                dawnstar::PlayerCreation::CreateCharacter(0, "BuffTest", charData, items, globalRng);

            int baseDamage = PlayerCombatStats::WeaponDamage(p, charData, items);
            p.increaseHarmBuff = true;
            Check(PlayerCombatStats::WeaponDamage(p, charData, items) == baseDamage + 25,
                  "increaseHarmBuff should add a flat +25 weapon damage");
            p.increaseHarmBuff = false;

            p.effectDurations[0] = -1;  // effect 1: "until cured" bonus damage
            int expectedBonus = 10 + PlayerCombatStats::SkillValue(p, charData, 1, false);
            Check(PlayerCombatStats::WeaponDamage(p, charData, items) == baseDamage + expectedBonus,
                  "effect 1 should add its 10+skillValue(1) bonus damage");
            p.effectDurations[0] = 0;

            int baseArmor = PlayerCombatStats::ArmorValue(p, charData, items);
            p.increaseArmorBuff = true;
            Check(PlayerCombatStats::ArmorValue(p, charData, items) == baseArmor + 15,
                  "increaseArmorBuff should add a flat +15 armor");
            p.increaseArmorBuff = false;

            p.effectDurations[16] = -1;  // effect 17: tempArmorBonus buff
            p.tempArmorBonus = 7;
            Check(PlayerCombatStats::ArmorValue(p, charData, items) == baseArmor + 7,
                  "effect 17 should add tempArmorBonus to armor");
            p.effectDurations[16] = 0;

            p.effectDurations[13] = -1;  // effect 14: attack-power/accuracy override
            int expectedPower = 5 + PlayerCombatStats::SkillValue(p, charData, 4, false);
            Check(PlayerCombatStats::AttackPower(p, charData, items, false) == expectedPower,
                  "effect 14 should override AttackPower with 5+skillValue(4)");
            int bestSkill = PlayerCombatStats::BestArmorSkillIndex(p, charData);
            Check(PlayerCombatStats::AttackAccuracy(p, charData, items) == PlayerCombatStats::SkillBonus(p, bestSkill),
                  "effect 14 should override AttackAccuracy with the best armor skill's bonus");
        }

        if (!g_ok) {
            std::fprintf(stderr, "m14_player_combat_stats_smoke: FAILED\n");
            return 1;
        }
        std::printf("all combat-stat checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m14_player_combat_stats_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
