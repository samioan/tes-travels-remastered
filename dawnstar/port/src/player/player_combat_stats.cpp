#include "player/player_combat_stats.h"

namespace dawnstar {

namespace {

// Item.column(1, itemId)/column(3, itemId): category/questFlags, 1-based
// itemId matching ItemDatabase's own index0(itemId)=itemId-1 convention.
int ItemCategory(const ItemDatabase& items, int itemId) { return JavaAbs(items.category[static_cast<size_t>(itemId - 1)]); }
int ItemMagnitude(const ItemDatabase& items, int itemId) { return items.questFlags[static_cast<size_t>(itemId - 1)]; }

}  // namespace

int PlayerCombatStats::SkillValue(const PlayerState& p, const CharacterData& charData, int skillIndex,
                                   bool withAttributeBonus) {
    int value = p.skills[static_cast<size_t>(skillIndex)][0];
    if (withAttributeBonus) {
        int attrSlot = 1 + charData.skillAttributeIndex[static_cast<size_t>(skillIndex)];
        value += p.attributes[static_cast<size_t>(attrSlot)] / 3;
    }

    if (skillIndex == 11 && IsEffectActive(p, 3)) {
        value += p.skills[1][0];
    }

    if (p.coreStats[6] < 7) value--;
    if (p.starFrostBonusActive) value += 4;

    return value;
}

int PlayerCombatStats::WeaponSkillValue(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items,
                                         bool withAttributeBonus) {
    if (p.equippedItems[1] == 0) return 0;
    int category = ItemCategory(items, p.equippedItems[1]);
    return category == 5 ? SkillValue(p, charData, 5, withAttributeBonus) : SkillValue(p, charData, 7, withAttributeBonus);
}

int PlayerCombatStats::BaseEvasion(const PlayerState& p, const ItemDatabase& items) {
    if (p.equippedItems[1] == 0) return 20;
    int category = ItemCategory(items, p.equippedItems[1]);
    return category == 5 ? SkillBonus(p, 5) : SkillBonus(p, 7);
}

int PlayerCombatStats::BestArmorSkillIndex(const PlayerState& p, const CharacterData& charData) {
    int best = 0;
    int bestValue = SkillValue(p, charData, 0, false);
    int candidate = SkillValue(p, charData, 2, false);
    if (candidate > bestValue) {
        bestValue = candidate;
        best = 2;
    }

    candidate = SkillValue(p, charData, 8, false);
    if (candidate > bestValue) {
        bestValue = candidate;
        best = 8;
    }

    candidate = SkillValue(p, charData, 12, false);
    if (candidate > bestValue) {
        best = 12;
    }

    return best;
}

int PlayerCombatStats::ActiveWeaponSkillIndex(const PlayerState& p, const CharacterData& charData,
                                               const ItemDatabase& items) {
    if (IsEffectActive(p, 6)) return BestArmorSkillIndex(p, charData);

    if (p.equippedItems[0] == 0) return -1;
    int category = ItemCategory(items, p.equippedItems[0]);
    if (category == 1) return 0;
    if (category == 2) return 2;
    return category == 3 ? 8 : 12;
}

int PlayerCombatStats::OffhandSkillIndex(const PlayerState& p, const ItemDatabase& items) {
    if (p.equippedItems[1] == 0) return -1;
    int category = ItemCategory(items, p.equippedItems[1]);
    return category == 5 ? 5 : 7;
}

int PlayerCombatStats::AttackPower(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items,
                                    bool withAttributeBonus) {
    if (IsEffectActive(p, 14)) return 5 + SkillValue(p, charData, 4, false);

    if (IsEffectActive(p, 6)) {
        int skillIdx = BestArmorSkillIndex(p, charData);
        return SkillValue(p, charData, skillIdx, withAttributeBonus);
    }

    int value = 0;
    if (p.equippedItems[0] != 0) {
        int category = ItemCategory(items, p.equippedItems[0]);
        if (category == 1) {
            value = SkillValue(p, charData, 0, withAttributeBonus);
        } else if (category == 2) {
            value = SkillValue(p, charData, 2, withAttributeBonus);
        } else if (category == 3) {
            value = SkillValue(p, charData, 8, withAttributeBonus);
        } else {
            value = SkillValue(p, charData, 12, withAttributeBonus);
        }
    }

    if (IsEffectActive(p, 5)) value += SkillValue(p, charData, 1, false);

    return value;
}

int PlayerCombatStats::AttackAccuracy(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items) {
    if (IsEffectActive(p, 6) || IsEffectActive(p, 14)) {
        int skillIdx = BestArmorSkillIndex(p, charData);
        return SkillBonus(p, skillIdx);
    }

    if (p.equippedItems[0] == 0) return 20;
    int category = ItemCategory(items, p.equippedItems[0]);
    if (category == 1) return SkillBonus(p, 0);
    if (category == 2) return SkillBonus(p, 2);
    return category == 3 ? SkillBonus(p, 8) : SkillBonus(p, 12);
}

int PlayerCombatStats::WeaponDamage(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items) {
    int damage = 0;
    if (IsEffectActive(p, 14)) {
        damage = 5 + SkillValue(p, charData, 4, false);
    } else if (IsEffectActive(p, 6)) {
        damage = 20 + SkillValue(p, charData, 3, false);
    } else if (p.equippedItems[0] != 0) {
        damage = ItemMagnitude(items, p.equippedItems[0]);
    }

    if (IsEffectActive(p, 1)) damage += 10 + SkillValue(p, charData, 1, false);
    if (p.increaseHarmBuff) damage += 25;

    return damage;
}

int PlayerCombatStats::ArmorValue(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items) {
    int total = 0;
    if (p.equippedItems[1] != 0) total += 4 * ItemMagnitude(items, p.equippedItems[1]);
    if (p.equippedItems[2] != 0) total += 2 * ItemMagnitude(items, p.equippedItems[2]);
    if (p.equippedItems[3] != 0) total += 2 * ItemMagnitude(items, p.equippedItems[3]);
    if (p.equippedItems[4] != 0) total += ItemMagnitude(items, p.equippedItems[4]);
    if (p.equippedItems[5] != 0) total += ItemMagnitude(items, p.equippedItems[5]);
    // equippedItems[6] is NOT included -- see the header comment.

    total /= 10;

    if (IsEffectActive(p, 2)) total += 10 + SkillValue(p, charData, 1, false);
    if (IsEffectActive(p, 17)) total += p.tempArmorBonus;
    if (p.increaseArmorBuff) total += 15;

    return total;
}

bool PlayerCombatStats::IsEffectActive(const PlayerState& p, int effectId) {
    int8_t duration = p.effectDurations[static_cast<size_t>(effectId - 1)];
    if (duration == -1) return true;
    return duration == -2 ? p.combatTargetSpawnId != 0 : duration > 0;
}

int PlayerCombatStats::EffectiveStat(const PlayerState& p, const CharacterData& charData, int index) {
    int value = p.coreStats[static_cast<size_t>(index)];
    if (IsEffectActive(p, 23)) {
        if (index == 2) {
            value += SkillValue(p, charData, 10, false);
            if (value > p.coreStats[3]) value = p.coreStats[3];
        } else if (index == 6) {
            value += SkillValue(p, charData, 10, false);
            if (value > p.coreStats[7]) value = p.coreStats[7];
        } else if (index == 4) {
            value += SkillValue(p, charData, 10, false);
            if (value > p.coreStats[5]) value = p.coreStats[5];
        }
    }
    return value;
}

void PlayerCombatStats::GainSkillExp(PlayerState& p, const CharacterData& charData, int skillIndex, int amount) {
    if (skillIndex < 0 || skillIndex >= 14) return;

    p.skills[static_cast<size_t>(skillIndex)][2] =
        static_cast<int16_t>(p.skills[static_cast<size_t>(skillIndex)][2] + amount);
    while (p.skills[static_cast<size_t>(skillIndex)][2] > 10) {
        p.skills[static_cast<size_t>(skillIndex)][2] =
            static_cast<int16_t>(p.skills[static_cast<size_t>(skillIndex)][2] - 10);
        p.skills[static_cast<size_t>(skillIndex)][0]++;

        int16_t attrIdx = charData.skillAttributeIndex[static_cast<size_t>(skillIndex)];
        int bit = attrIdx / 2;
        p.attributeIncreaseFlags = static_cast<int8_t>(p.attributeIncreaseFlags | (1 << bit));

        p.coreStats[1]++;
    }

    if (p.coreStats[1] >= 10) {
        p.coreStats[0]++;
        p.levelUpPending = true;
    }
}

std::vector<std::string> PlayerCombatStats::KnownSkillsSummary(const PlayerState& p, const CharacterData& charData) {
    std::vector<std::string> out;
    for (int i = 0; i < 14; i++) {
        if (p.skills[static_cast<size_t>(i)][0] > 0) {
            out.push_back(charData.skillNames[static_cast<size_t>(i)] + ": " +
                          std::to_string(p.skills[static_cast<size_t>(i)][0]));
        }
    }
    return out;
}

int PlayerCombatStats::NthKnownSkillIndex(const PlayerState& p, int index) {
    int seen = 0;
    for (int i = 0; i < 14; i++) {
        if (p.skills[static_cast<size_t>(i)][0] > 0) {
            if (seen == index) return i;
            seen++;
        }
    }
    return -1;
}

std::string PlayerCombatStats::SkillTooltip(const PlayerState& p, const CharacterData& charData, int skillIndex) {
    return charData.skillNames[static_cast<size_t>(skillIndex)] + "\nRank: " +
           std::to_string(p.skills[static_cast<size_t>(skillIndex)][0]) + "\nExp: " +
           std::to_string(p.skills[static_cast<size_t>(skillIndex)][2]) + "/10";
}

PlayerCombatStats::RollResult PlayerCombatStats::RollOutcome(int atkChance, int defChance, JavaRandom& globalRng) {
    int atkRoll = LingoRandomInt(globalRng, 100);
    int defRoll = LingoRandomInt(globalRng, 100);
    bool defHit = atkRoll <= defChance;
    bool crit = defRoll <= atkChance;

    int outcome;
    if (crit && !defHit) {
        outcome = 3;
    } else if (crit && defHit) {
        outcome = defRoll >= atkRoll ? 2 : 1;
    } else if (crit || defHit) {
        outcome = 0;
    } else {
        outcome = defRoll >= atkRoll ? 2 : 1;
    }

    return {outcome, crit};
}

}  // namespace dawnstar
