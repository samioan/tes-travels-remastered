#include "player/player_spellcasting.h"

#include <algorithm>
#include <cstdlib>

#include "player/player_combat_stats.h"
#include "player/player_inventory.h"

namespace dawnstar {

int PlayerSpellcasting::SpellSkillIndexFor(int spellId) {
    if (spellId <= 5) return 1;
    if (spellId <= 10) return 3;
    if (spellId <= 15) return 4;
    return spellId <= 20 ? 6 : 10;
}

void PlayerSpellcasting::CastOnSelf(PlayerState& p, const CharacterData& charData, const ItemDatabase& items,
                                     const SpellDatabase& spells, JavaRandom& globalRng) {
    int skillIdx = SpellSkillIndexFor(p.selectedSpellId);
    int atkSkill = PlayerCombatStats::SkillValue(p, charData, skillIdx, true);
    int atkBonus = PlayerCombatStats::SkillBonus(p, skillIdx);
    const Spell& spell = spells.ById(p.selectedSpellId);
    int8_t cost = spell.icon;
    int8_t durationMult = spell.durationMultiplier;
    // spell.magickaCost is read here in the original too (as a local
    // named "power") but never actually used -- castOnSelf spends
    // Magicka scaled by `school` (spell.power) below instead, not by
    // magickaCost. Not ported, same as M15's unused Monster.attack()
    // `type` read (see combat/combat_resolution.cpp's PlayerAttack).
    int8_t school = spell.power;

    int diff = atkSkill - cost;
    int atkChance = atkBonus + diff * 5;
    int defChance = durationMult - diff * 5;
    atkChance = std::min(std::max(atkChance, 10), 95);
    defChance = std::min(std::max(defChance, 10), 95);

    PlayerCombatStats::RollResult roll = PlayerCombatStats::RollOutcome(atkChance, defChance, globalRng);
    int outcome = roll.outcome;
    int8_t multiplier = 1;
    if (outcome == 0) {
        p.coreStats[4] = static_cast<int16_t>(p.coreStats[4] - 3 * school);
    } else if (outcome == 1) {
        p.coreStats[4] = static_cast<int16_t>(p.coreStats[4] - 3 * school / 2);
    } else if (outcome == 2) {
        p.coreStats[4] = static_cast<int16_t>(p.coreStats[4] - school);
    } else if (outcome == 3) {
        p.coreStats[4] = static_cast<int16_t>(p.coreStats[4] - school);
        multiplier = 2;
    }
    p.coreStats[4] = std::max(p.coreStats[4], int16_t{0});

    if (outcome >= 2) {
        PlayerCombatStats::GainSkillExp(p, charData, skillIdx, 1);
    }

    int spellId = p.selectedSpellId;
    switch (spellId) {
        case 1:
        case 2:
        case 3:
        case 5:
            p.effectDurations[static_cast<size_t>(spellId - 1)] = static_cast<int8_t>(durationMult * multiplier);
            break;
        case 6: {
            if (PlayerInventory::AddItem(p, 101, 0, 0) && PlayerInventory::EquipLastPickedUpItem(p, items, true)) {
                p.effectDurations[static_cast<size_t>(spellId - 1)] = static_cast<int8_t>(durationMult * multiplier);
            }
            break;
        }
        case 21: {
            int healAmount = 6 + PlayerCombatStats::SkillValue(p, charData, 10, false);
            p.coreStats[2] = static_cast<int16_t>(p.coreStats[2] + multiplier * healAmount);
            p.coreStats[2] = std::min(p.coreStats[2], p.coreStats[3]);
            break;
        }
        case 23:
            p.effectDurations[static_cast<size_t>(spellId - 1)] = -2;
            break;
        case 24:
            p.effectDurations[static_cast<size_t>(spellId - 1)] = -4;
            break;
        case 25:
            for (int i = 1; i <= multiplier; i++) {
                CureRandomAilment(p, globalRng);
            }
            break;
        default:
            break;
    }

    p.coreStats[6] = static_cast<int16_t>(p.coreStats[6] - 5 * PlayerCombatStats::FatigueCostMultiplier(p));
    p.coreStats[6] = std::max(p.coreStats[6], int16_t{0});

    if (PlayerCombatStats::HasAilment(p, 6)) {
        int drain = 2 * p.coreStats[3] / 100;
        if (drain < 1) drain = 1;
        p.coreStats[2] = static_cast<int16_t>(p.coreStats[2] - drain);
    }
}

int PlayerSpellcasting::ActiveAilmentCount(const PlayerState& p) {
    int count = 0;
    for (int i = 0; i < 8; i++) {
        if (((p.ailmentMask >> i) & 1) != 0) count++;
    }
    return count;
}

void PlayerSpellcasting::CureRandomAilment(PlayerState& p, JavaRandom& globalRng) {
    int active = ActiveAilmentCount(p);
    if (active <= 0) return;

    int pick = active == 1 ? 1 : LingoRandomInt(globalRng, active);
    int seen = 0;
    for (int i = 0; i < 8; i++) {
        int bit = (p.ailmentMask >> i) & 1;
        if (bit == 1) {
            if (++seen == pick) {
                p.ailmentMask = static_cast<int8_t>(p.ailmentMask & ~(1 << i));
                break;
            }
        }
    }
}

bool PlayerSpellcasting::CanLearnSpell(const PlayerState& p, const ItemDatabase& items, const SpellDatabase& spells,
                                        int slot) {
    int itemId = std::abs(static_cast<int>(p.inventoryItemIds[slot]));
    if (items.category[static_cast<size_t>(itemId - 1)] != 12) return false;

    int spellId = p.inventoryItemData[slot] & 0xFF;
    int8_t requiredSkill = spells.ById(spellId).skillRequired;
    if ((p.knownSpellsMask & (1u << (spellId - 1))) != 0) return false;
    return p.skills[static_cast<size_t>(requiredSkill)][0] > 0;
}

bool PlayerSpellcasting::LearnSpellFromScroll(PlayerState& p, const ItemDatabase& items, int slot) {
    // itemId/category are read in the original too, but never actually
    // used -- the scroll's spell id comes entirely from
    // inventoryItemData's low byte, not from the item table.
    int spellData = p.inventoryItemData[slot] & 0xFF;
    int spellBit = spellData - 1;
    p.knownSpellsMask |= (1u << spellBit);
    PlayerInventory::RemoveSlot(p, items, slot);
    return true;
}

std::vector<std::string> PlayerSpellcasting::KnownSpellsSummary(const PlayerState& p, const SpellDatabase& spells) {
    std::vector<std::string> out;
    for (int i = 0; i < spells.Count(); i++) {
        if ((p.knownSpellsMask & (1u << i)) != 0) {
            int spellId = i + 1;
            std::string line = spells.all[static_cast<size_t>(i)].name;
            if (spellId == p.selectedSpellId) line = "R: " + line;
            out.push_back(line);
        }
    }
    return out;
}

int PlayerSpellcasting::NthKnownSpellId(const PlayerState& p, const SpellDatabase& spells, int index) {
    int seen = 0;
    for (int i = 0; i < spells.Count(); i++) {
        if ((p.knownSpellsMask & (1u << i)) != 0) {
            if (seen == index) return i;
            seen++;
        }
    }
    return -1;
}

int PlayerSpellcasting::CycleSelectedSpell(const PlayerState& p, const SpellDatabase& spells) {
    if (!spells.IsValidId(p.selectedSpellId)) {
        int first = NthKnownSpellId(p, spells, 0);
        return first < 0 ? 0 : first + 1;
    }

    int start = p.selectedSpellId - 1;
    int i = start + 1;
    if (i == spells.Count()) i = 0;

    while (i != start) {
        if ((p.knownSpellsMask & (1u << i)) != 0) return i + 1;
        if (++i == spells.Count()) i = 0;
    }

    return p.selectedSpellId;
}

std::string PlayerSpellcasting::SpellTooltip(const CharacterData& charData, const SpellDatabase& spells, int index) {
    const Spell& spell = spells.all[static_cast<size_t>(index)];
    std::string text = spell.name + "\n";
    text += charData.skillNames[static_cast<size_t>(spell.skillRequired)] + "\n";
    text += "Cost: " + std::to_string(spell.magickaCost) + "\n";
    text += spell.description;
    return text;
}

}  // namespace dawnstar
