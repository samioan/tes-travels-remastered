#include "player/player_combat_stats.h"

#include <algorithm>
#include <cstdlib>

#include "player/player_inventory.h"

namespace stormhold {

int PlayerCombatStats::SkillValue(const PlayerState& p, const CharacterData& charData, int skillIndex,
                                   bool includeBonus) {
    int v = p.skills[static_cast<size_t>(skillIndex)][0];
    if (includeBonus) {
        int attrIdx = 1 + charData.skillAttributeIndex[static_cast<size_t>(skillIndex)];
        v += p.attributes[static_cast<size_t>(attrIdx)] / 3;
    }

    if (skillIndex == 11 && IsEffectActive(p, 3)) {
        v += p.skills[1][0];
    }

    if (p.coreStats[6] < 7) {
        v--;
    }

    return v;
}

int PlayerCombatStats::DefenseSkillValue(const PlayerState& p, const CharacterData& charData,
                                          const ItemDatabase& items, bool includeBonus) {
    if (p.equippedItems[1] == 0) return 0;
    int cat = std::abs(items.category[static_cast<size_t>(p.equippedItems[1] - 1)]);
    return cat == 5 ? SkillValue(p, charData, 5, includeBonus) : SkillValue(p, charData, 7, includeBonus);
}

int PlayerCombatStats::BaseEvasion(const PlayerState& p, const ItemDatabase& items) {
    if (p.equippedItems[1] == 0) return 20;
    int cat = std::abs(items.category[static_cast<size_t>(p.equippedItems[1] - 1)]);
    return cat == 5 ? SkillBonus(p, 5) : SkillBonus(p, 7);
}

int PlayerCombatStats::BestArmorSkillIndex(const PlayerState& p, const CharacterData& charData) {
    int best = 0;
    int bestVal = SkillValue(p, charData, 0, false);

    int v = SkillValue(p, charData, 2, false);
    if (v > bestVal) {
        bestVal = v;
        best = 2;
    }

    v = SkillValue(p, charData, 8, false);
    if (v > bestVal) {
        bestVal = v;
        best = 8;
    }

    v = SkillValue(p, charData, 12, false);
    if (v > bestVal) {
        best = 12;
    }

    return best;
}

int PlayerCombatStats::ActiveWeaponSkillIndex(const PlayerState& p, const CharacterData& charData,
                                               const ItemDatabase& items) {
    if (IsEffectActive(p, 6)) return BestArmorSkillIndex(p, charData);
    if (p.equippedItems[0] == 0) return -1;

    int cat = std::abs(items.category[static_cast<size_t>(p.equippedItems[0] - 1)]);
    if (cat == 1) return 0;
    if (cat == 2) return 2;
    return cat == 3 ? 8 : 12;
}

int PlayerCombatStats::DefenseSkillIndex(const PlayerState& p, const ItemDatabase& items) {
    if (p.equippedItems[1] == 0) return -1;
    int cat = std::abs(items.category[static_cast<size_t>(p.equippedItems[1] - 1)]);
    return cat == 5 ? 5 : 7;
}

int PlayerCombatStats::AttackPower(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items,
                                    bool includeBonus) {
    if (IsEffectActive(p, 14)) return 5 + SkillValue(p, charData, 4, false);
    if (IsEffectActive(p, 6)) {
        int skill = BestArmorSkillIndex(p, charData);
        return SkillValue(p, charData, skill, includeBonus);
    }

    int v = 0;
    if (p.equippedItems[0] != 0) {
        int cat = std::abs(items.category[static_cast<size_t>(p.equippedItems[0] - 1)]);
        if (cat == 1) {
            v = SkillValue(p, charData, 0, includeBonus);
        } else if (cat == 2) {
            v = SkillValue(p, charData, 2, includeBonus);
        } else if (cat == 3) {
            v = SkillValue(p, charData, 8, includeBonus);
        } else {
            v = SkillValue(p, charData, 12, includeBonus);
        }
    }

    if (IsEffectActive(p, 5)) v += SkillValue(p, charData, 1, false);
    return v;
}

int PlayerCombatStats::AttackAccuracy(const PlayerState& p, const CharacterData& charData,
                                       const ItemDatabase& items) {
    if (IsEffectActive(p, 6) || IsEffectActive(p, 14)) {
        return SkillBonus(p, BestArmorSkillIndex(p, charData));
    }

    if (p.equippedItems[0] == 0) return 20;
    int cat = std::abs(items.category[static_cast<size_t>(p.equippedItems[0] - 1)]);
    if (cat == 1) return SkillBonus(p, 0);
    if (cat == 2) return SkillBonus(p, 2);
    return cat == 3 ? SkillBonus(p, 8) : SkillBonus(p, 12);
}

int PlayerCombatStats::WeaponDamage(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items) {
    int v;
    if (IsEffectActive(p, 14)) {
        v = 5 + SkillValue(p, charData, 4, false);
    } else if (IsEffectActive(p, 6)) {
        v = 20 + SkillValue(p, charData, 3, false);
    } else if (p.equippedItems[0] != 0) {
        // Item.column(3, itemId) -- the questFlags COLUMN doubles as the
        // equipped weapon's raw damage magnitude here, same "column
        // reuse" dawnstar's own Item.java documents for its analogous
        // armorValue()/weaponDamage(). Not a mistake: `items.questFlags`
        // really is the right array for this read.
        v = items.questFlags[static_cast<size_t>(p.equippedItems[0] - 1)];
    } else {
        v = 0;
    }

    if (IsEffectActive(p, 1)) v += 10 + SkillValue(p, charData, 1, false);
    if (p.increaseHarmBuff) v += 25;
    return v;
}

int PlayerCombatStats::ArmorValue(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items) {
    // Item.column(3, itemId) again (questFlags-as-magnitude, see
    // WeaponDamage's own comment above) -- weighted 4/2/2/1/1 across
    // equip slots 1-5. Slot 6 (equippedItems[6]) is NOT summed here,
    // ported exactly as the original, not "fixed".
    int v = 0;
    if (p.equippedItems[1] != 0) v += 4 * items.questFlags[static_cast<size_t>(p.equippedItems[1] - 1)];
    if (p.equippedItems[2] != 0) v += 2 * items.questFlags[static_cast<size_t>(p.equippedItems[2] - 1)];
    if (p.equippedItems[3] != 0) v += 2 * items.questFlags[static_cast<size_t>(p.equippedItems[3] - 1)];
    if (p.equippedItems[4] != 0) v += items.questFlags[static_cast<size_t>(p.equippedItems[4] - 1)];
    if (p.equippedItems[5] != 0) v += items.questFlags[static_cast<size_t>(p.equippedItems[5] - 1)];

    v /= 10;
    if (IsEffectActive(p, 2)) v += 10 + SkillValue(p, charData, 1, false);
    if (IsEffectActive(p, 17)) v += p.spellArmorBonus;
    if (p.increaseArmorBuff) v += 15;
    return v;
}

bool PlayerCombatStats::IsEffectActive(const PlayerState& p, int effectId) {
    int8_t duration = p.effectDurations[static_cast<size_t>(effectId - 1)];
    if (duration == -1) return true;
    if (duration == -2) return p.lastCombatTargetId != 0;
    return duration > 0;
}

int PlayerCombatStats::EffectiveStat(const PlayerState& p, const CharacterData& charData, int statIndex) {
    int v = p.coreStats[static_cast<size_t>(statIndex)];
    if (!IsEffectActive(p, 23)) return v;

    if (statIndex == 2) {
        v += SkillValue(p, charData, 10, false);
        if (v > p.coreStats[3]) v = p.coreStats[3];
    } else if (statIndex == 6) {
        v += SkillValue(p, charData, 10, false);
        if (v > p.coreStats[7]) v = p.coreStats[7];
    } else if (statIndex == 4) {
        v += SkillValue(p, charData, 10, false);
        if (v > p.coreStats[5]) v = p.coreStats[5];
    }

    return v;
}

int PlayerCombatStats::RollOutcome(JavaRandom& rng, int atkChance, int defChance) {
    int atkRoll = RandomInt1Based(rng, 100);
    int defRoll = RandomInt1Based(rng, 100);
    bool atkHit = atkRoll <= defChance;
    bool lastDefenseRollHit = defRoll <= atkChance;

    if (lastDefenseRollHit && !atkHit) return 3;
    if (lastDefenseRollHit && atkHit) return defRoll >= atkRoll ? 2 : 1;
    if (lastDefenseRollHit || atkHit) return 0;
    return defRoll >= atkRoll ? 2 : 1;
}

int PlayerCombatStats::RollShopOutcome(const PlayerState& p, const CharacterData& charData, int action,
                                        int interactionCount, JavaRandom& rng) {
    int skill = SkillValue(p, charData, 13, true);
    if (action == 3) skill += 3;

    int diff = skill - interactionCount;
    int a = 20 - diff * 5;
    int b = 20 + p.attributes[12] / 2 + diff * 5;
    a = std::min(std::max(a, 10), 95);
    b = std::min(std::max(b, 10), 95);
    return RollOutcome(rng, b, a);
}

void PlayerCombatStats::TickStatusCountdowns(PlayerState& p, int64_t deltaMs, bool monsterRenderedThisFrame) {
    if (HasAilment(p, 4)) {
        p.vampirismTimer = static_cast<int16_t>(p.vampirismTimer - deltaMs);
        if (p.vampirismTimer < 0) {
            p.vampirismTimer = 0;
            p.ailmentMask = static_cast<int8_t>(p.ailmentMask & ~(1 << 3));
        }
    }

    if (HasAilment(p, 5)) {
        p.manaBurnTimer = static_cast<int16_t>(p.manaBurnTimer - deltaMs);
        if (p.manaBurnTimer < 0) {
            p.manaBurnTimer = 0;
            p.ailmentMask = static_cast<int8_t>(p.ailmentMask & ~(1 << 4));
        }
    }

    if (HasAilment(p, 7) && monsterRenderedThisFrame) {
        p.terrifiedTimer = static_cast<int16_t>(p.terrifiedTimer - deltaMs);
        if (p.terrifiedTimer < 0) {
            p.terrifiedTimer = 0;
            p.ailmentMask = static_cast<int8_t>(p.ailmentMask & ~(1 << 6));
        }
    }
}

void PlayerCombatStats::TickPerSecond(PlayerState& p, const ItemDatabase& items) {
    for (int i = 0; i < 25; i++) {
        if (p.effectDurations[static_cast<size_t>(i)] > 0) {
            p.effectDurations[static_cast<size_t>(i)]--;
            if (p.effectDurations[static_cast<size_t>(i)] <= 0) {
                p.effectDurations[static_cast<size_t>(i)] = 0;
                if (i == 5) {
                    int slot = PlayerInventory::FindEquippedSlotForItem(p, 109);
                    PlayerInventory::RemoveInventorySlot(p, slot, items);
                }
            }
        }
    }

    if (HasAilment(p, 4)) {
        int drain = 2 * p.coreStats[3] / 100;
        drain = std::max(drain, 0);
        p.coreStats[2] = static_cast<int16_t>(p.coreStats[2] - drain);
    }

    if (HasAilment(p, 5)) {
        int regen = p.coreStats[5] / 10;
        p.coreStats[4] = static_cast<int16_t>(p.coreStats[4] + regen);
        if (p.coreStats[4] >= p.coreStats[5]) {
            p.coreStats[4] = 0;
            int burn = p.coreStats[5] / 10;
            p.coreStats[2] = static_cast<int16_t>(p.coreStats[2] - burn);
        }
    }
}

void PlayerCombatStats::ApplyRestRecovery(PlayerState& p, bool fullyRested, const ItemDatabase& items,
                                           JavaRandom& rng) {
    int missingHp = p.coreStats[3] - p.coreStats[2];
    int missingMagicka = p.coreStats[5] - p.coreStats[4];
    int missingFatigue = p.coreStats[7] - p.coreStats[6];
    if (!fullyRested) {
        missingHp = 2 * missingHp / 3;
        missingMagicka = 2 * missingMagicka / 3;
        missingFatigue = 2 * missingFatigue / 3;
    }

    if (HasAilment(p, 8)) {
        missingHp = 3 * missingHp / 4;
        missingMagicka = 3 * missingMagicka / 4;
        missingFatigue = 3 * missingFatigue / 4;
    }

    p.coreStats[2] = static_cast<int16_t>(p.coreStats[2] + missingHp);
    p.coreStats[4] = static_cast<int16_t>(p.coreStats[4] + missingMagicka);
    p.coreStats[6] = static_cast<int16_t>(p.coreStats[6] + missingFatigue);
    p.increaseHarmBuff = false;
    p.increaseArmorBuff = false;
    p.safeCampingBuff = false;

    if (RandomInt1Based(rng, 100) <= 10) {
        for (int slot = 0; slot < p.inventoryCount; slot++) {
            int itemId = std::abs(static_cast<int>(p.inventoryItemIds[static_cast<size_t>(slot)]));
            if (itemId == 96) {
                PlayerInventory::RemoveInventorySlot(p, slot, items);
                break;
            }
        }
    }

    for (int bit = 0; bit < 8; bit++) {
        int ailmentId = bit + 1;
        if (ailmentId != 4 && ailmentId != 5) {
            if (RandomInt1Based(rng, 100) <= 25) {
                p.ailmentMask = static_cast<int8_t>(p.ailmentMask & ~(1 << bit));
            }
        }
    }
}

}  // namespace stormhold
