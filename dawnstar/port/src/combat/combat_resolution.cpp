#include "combat/combat_resolution.h"

#include <algorithm>
#include <cstdlib>

#include "monster/monster_runtime.h"
#include "player/player_combat_stats.h"
#include "player/player_inventory.h"
#include "player/player_movement.h"
#include "player/player_spellcasting.h"

namespace dawnstar {

void CombatResolution::PlayerAttack(PlayerState& player, MonsterState& target, const CharacterData& charData,
                                     const ItemDatabase& items, const MonsterDatabase& monsterDb,
                                     JavaRandom& globalRng) {
    player.combatTargetSpawnId = target.spawnId;
    // byte type = target.monsterType; -- read but never used in the
    // original (Player.java's attack()), not ported.

    int power = PlayerCombatStats::WeaponDamage(player, charData, items);
    int targetDef = MonsterRuntime::Stat(target, monsterDb, 7);
    int diff = power - targetDef;
    diff = std::min(diff, static_cast<int>(MonsterRuntime::Stat(target, monsterDb, 2)));

    if (PlayerCombatStats::IsEffectActive(player, 10)) {
        if (target.scratch[8] == 0) {
            PlayerCombatStats::ClearEffect(player, 10);
        } else {
            diff += target.scratch[8];
        }
    }

    int defChance = static_cast<int>(MonsterRuntime::Stat(target, monsterDb, 6)) - diff * 5;
    int atkChance = PlayerCombatStats::AttackAccuracy(player, charData, items) + diff * 5;
    defChance = std::min(std::max(defChance, 10), 95);
    atkChance = std::min(std::max(atkChance, 10), 95);

    PlayerCombatStats::RollResult roll = PlayerCombatStats::RollOutcome(atkChance, defChance, globalRng);
    if (roll.outcome == 0) return;

    int damage = PlayerCombatStats::WeaponDamage(player, charData, items);
    int targetArmor = static_cast<int>(MonsterRuntime::Stat(target, monsterDb, 8));

    if (PlayerCombatStats::IsEffectActive(player, 13)) {
        if (target.scratch[5] == 0) {
            player.effectDurations[12] = 0;
        } else {
            targetArmor -= target.scratch[5];
        }
    }

    if (roll.outcome == 1) {
        targetArmor = 2 * targetArmor;
    } else if (roll.outcome == 3) {
        damage = 2 * damage;
    }

    int rawDamage = damage - targetArmor;
    rawDamage = std::max(rawDamage, 4);
    int scaled = rawDamage * static_cast<int>(MonsterRuntime::Stat(target, monsterDb, 14)) / 100;
    MonsterRuntime::TakeDamage(target, scaled);
    // target.store(): SKIPPED, see monster/monster_runtime.h's class comment.

    if (PlayerCombatStats::IsEffectActive(player, 7)) {
        if (target.scratch[1] == 0) {
            PlayerCombatStats::ClearEffect(player, 7);
        } else {
            int fatigueDmg = target.scratch[1];
            fatigueDmg = std::max(fatigueDmg, 4);
            scaled = fatigueDmg * static_cast<int>(MonsterRuntime::Stat(target, monsterDb, 14)) / 100;
            MonsterRuntime::TakeDamage(target, scaled);
        }
    }

    if (roll.outcome >= 2) {
        int skillIdx = PlayerCombatStats::ActiveWeaponSkillIndex(player, charData, items);
        PlayerCombatStats::GainSkillExp(player, charData, skillIdx, 1);
    }

    if (!PlayerCombatStats::IsEffectActive(player, 7)) {
        int16_t newFatigue =
            static_cast<int16_t>(player.coreStats[6] - 7 * PlayerCombatStats::FatigueCostMultiplier(player));
        player.coreStats[6] = newFatigue < 0 ? int16_t{0} : newFatigue;
    }

    if (PlayerCombatStats::HasAilment(player, 6)) {
        int drain = 2 * player.coreStats[3] / 100;
        if (drain < 1) drain = 1;
        player.coreStats[2] = static_cast<int16_t>(player.coreStats[2] - drain);
    }
}

bool CombatResolution::MonsterTick(MonsterState& m, PlayerState& player, const CharacterData& charData,
                                    const ItemDatabase& items, const MonsterDatabase& monsterDb, int64_t now,
                                    JavaRandom& globalRng, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                                    int16_t& spawnIdCounter) {
    bool act = false;
    if (m.aiPhase == 0) {
        m.timestamp = now;
        m.aiPhase = 1;
    } else if (m.aiPhase == 1 && now - m.timestamp > 800) {
        act = true;
    }

    if (!act) return false;

    m.aiPhase = 2;
    m.timestamp = now;

    int detectionStat = MonsterRuntime::RawStat(m, monsterDb, 4);
    int stealth = PlayerCombatStats::WeaponSkillValue(player, charData, items, true);
    int diff = stealth - detectionStat;
    diff = std::min(diff, static_cast<int>(MonsterRuntime::RawStat(m, monsterDb, 2)));

    int chanceA = MonsterRuntime::RawStat(m, monsterDb, 3) - diff * 5;
    int chanceB = PlayerCombatStats::BaseEvasion(player, items) + diff * 5;
    chanceA = std::min(std::max(chanceA, 10), 95);
    chanceB = std::min(std::max(chanceB, 10), 95);

    int rollA = LingoRandomInt(globalRng, 100);
    int rollB = LingoRandomInt(globalRng, 100);
    bool detectedA = rollA <= chanceA;
    bool detectedB = rollB <= chanceB;

    int outcome;
    if (detectedA && !detectedB) {
        outcome = 3;
    } else if (detectedA && detectedB) {
        outcome = rollA >= rollB ? 2 : 1;
    } else if (detectedA || detectedB) {
        outcome = 0;
    } else {
        outcome = rollA >= rollB ? 2 : 1;
    }

    if (outcome == 0) {
        m.aiPhase = 1;
        return false;
    }

    int attackStat = MonsterRuntime::RawStat(m, monsterDb, 5);
    int defense = PlayerCombatStats::ArmorValue(player, charData, items);
    if (outcome == 1) defense = 2 * defense;

    int power = attackStat - defense;
    power = std::max(power, 4);
    int damage = power * player.coreStats[3] / 100;
    int16_t newHp = static_cast<int16_t>(player.coreStats[2] - damage);
    player.coreStats[2] = newHp < 0 ? int16_t{0} : newHp;

    if (detectedB) {
        int skillIdx = PlayerCombatStats::ActiveWeaponSkillIndex(player, charData, items);
        PlayerCombatStats::GainSkillExp(player, charData, skillIdx, 1);
    }

    if (outcome < 3) {
        m.aiPhase = 1;
        return true;
    }

    if (LingoRandomInt(globalRng, 100) <= 30) {
        int ailment = MonsterRuntime::RawStat(m, monsterDb, 11);
        if (ailment > 0) {
            int bit = ailment - 1;
            player.ailmentMask = static_cast<int8_t>(player.ailmentMask | (1 << bit));
            if (ailment != 1) {
                if (ailment == 2) {
                    // Dungeon.populateRandomMonsters(3), read off the
                    // ATTACKING monster's own dungeonLevel -- see this
                    // method's own doc comment in combat_resolution.h.
                    DungeonRuntime::PopulateRandomMonsters(levels, world, m.dungeonLevel - 1, 3, globalRng,
                                                            monsterDb, spawnIdCounter);
                } else if (ailment != 3) {
                    if (ailment == 4) {
                        player.trollThirstTimer = 30000;
                    } else if (ailment == 5) {
                        player.glacierCurseTimer = 30000;
                    }
                    // ailment 6/7/8: no-op in the original too.
                }
            }
        }
    }

    m.aiPhase = 1;
    return true;
}

void CombatResolution::CastOnMonster(PlayerState& player, MonsterState& target, const CharacterData& charData,
                                      const ItemDatabase& items, const MonsterDatabase& monsterDb,
                                      const SpellDatabase& spells, JavaRandom& globalRng) {
    int skillIdx = PlayerSpellcasting::SpellSkillIndexFor(player.selectedSpellId);
    int atkSkill = PlayerCombatStats::SkillValue(player, charData, skillIdx, true);
    int atkBonus = PlayerCombatStats::SkillBonus(player, skillIdx);
    int targetEvasion = MonsterRuntime::Stat(target, monsterDb, 10);
    int targetDef = MonsterRuntime::Stat(target, monsterDb, 9);
    const Spell& spell = spells.ById(player.selectedSpellId);
    int8_t power = spell.magickaCost;
    // spell.power ("school") is read here in the original too (as a
    // local named "school") but never actually used in castOnMonster --
    // Magicka is spent scaled by `power` (magickaCost) below instead.
    // Not ported, same unused-read pattern as CastOnSelf's `power`/
    // player/player_spellcasting.cpp's doc comment on it.

    int diff = atkSkill - targetEvasion;
    int targetHp = MonsterRuntime::Stat(target, monsterDb, 2);
    diff = std::min(diff, targetHp);
    int atkChance = atkBonus + diff * 5;
    int defChance = targetDef - diff * 5;
    atkChance = std::min(std::max(atkChance, 10), 95);
    defChance = std::min(std::max(defChance, 10), 95);

    PlayerCombatStats::RollResult roll = PlayerCombatStats::RollOutcome(atkChance, defChance, globalRng);
    int outcome = roll.outcome;
    int multiplier = 1;
    if (outcome == 0) {
        player.coreStats[4] = static_cast<int16_t>(player.coreStats[4] - 3 * power);
    } else if (outcome == 1) {
        player.coreStats[4] = static_cast<int16_t>(player.coreStats[4] - 3 * power / 2);
    } else if (outcome == 2) {
        player.coreStats[4] = static_cast<int16_t>(player.coreStats[4] - power);
    } else if (outcome == 3) {
        player.coreStats[4] = static_cast<int16_t>(player.coreStats[4] - power);
        multiplier = 2;
    }
    player.coreStats[4] = std::max(player.coreStats[4], int16_t{0});

    if (outcome >= 2) {
        PlayerCombatStats::GainSkillExp(player, charData, skillIdx, 1);
    }

    int spellId = player.selectedSpellId;
    switch (spellId) {
        case 4:
            target.scratch[9] = -2;
            // target.store(): SKIPPED, see monster/monster_runtime.h's class comment.
            break;
        case 7: {
            int val7 = 10 + PlayerCombatStats::SkillValue(player, charData, 3, false);
            target.scratch[1] = static_cast<int8_t>(val7);
            break;
        }
        case 8: {
            int val8 = PlayerCombatStats::SkillValue(player, charData, 3, false);
            int dmg8 = 12 + 2 * val8;
            MonsterRuntime::TakeDamage(target, dmg8);
            player.coreStats[6] = static_cast<int16_t>(player.coreStats[6] + val8);
            player.coreStats[6] = std::min(player.coreStats[6], player.coreStats[7]);
            player.coreStats[2] = static_cast<int16_t>(player.coreStats[2] + val8);
            player.coreStats[2] = std::min(player.coreStats[2], player.coreStats[3]);
            player.coreStats[4] = static_cast<int16_t>(player.coreStats[4] + 12);
            player.coreStats[4] = std::min(player.coreStats[4], player.coreStats[5]);
            break;
        }
        case 9: {
            if (MonsterRuntime::IsUndead(target)) {
                int base9 = 60 * multiplier;
                int rawDmg9 = base9 - MonsterRuntime::Stat(target, monsterDb, 8);
                rawDmg9 = std::max(rawDmg9, 4);
                int scaled9 = rawDmg9 * MonsterRuntime::Stat(target, monsterDb, 14) / 100;
                MonsterRuntime::TakeDamage(target, scaled9);
            }
            break;
        }
        case 10:
            player.effectDurations[static_cast<size_t>(spellId - 1)] = -2;
            target.scratch[8] = static_cast<int8_t>(2 * multiplier);
            break;
        case 11: {
            int base11 = 25 + PlayerCombatStats::SkillValue(player, charData, 4, false);
            int raw11 = base11 * multiplier;
            int dmg11 = raw11 - MonsterRuntime::Stat(target, monsterDb, 8);
            dmg11 = std::max(dmg11, 4);
            int scaled11 = dmg11 * MonsterRuntime::Stat(target, monsterDb, 14) / 100;
            MonsterRuntime::TakeDamage(target, scaled11);
            // target.store(): SKIPPED.
            break;
        }
        case 12: {
            player.effectDurations[static_cast<size_t>(spellId - 1)] = -2;
            int val12 = multiplier * (10 + PlayerCombatStats::SkillValue(player, charData, 4, false));
            val12 = std::min(val12, 255);
            target.scratch[4] = static_cast<int8_t>(val12);
            // target.store(): SKIPPED.
            break;
        }
        case 13: {
            player.effectDurations[static_cast<size_t>(spellId - 1)] = -2;
            int val13 = multiplier * (10 + PlayerCombatStats::SkillValue(player, charData, 4, false));
            val13 = std::min(val13, 255);
            target.scratch[5] = static_cast<int8_t>(val13);
            // target.store(): SKIPPED.
            break;
        }
        case 14:
            player.effectDurations[static_cast<size_t>(spellId - 1)] = -1;
            PlayerAttack(player, target, charData, items, monsterDb, globalRng);
            player.effectDurations[static_cast<size_t>(spellId - 1)] = 0;
            break;
        case 15:
            player.effectDurations[static_cast<size_t>(spellId - 1)] = -2;
            target.scratch[2] = 1;
            // target.store(): SKIPPED.
            break;
        case 16: {
            int val16 = 10 - targetEvasion;
            if (val16 > 0) {
                val16 = multiplier * val16;
                player.effectDurations[static_cast<size_t>(spellId - 1)] = static_cast<int8_t>(val16);
                target.scratch[6] = 1;
            }
            break;
        }
        case 17:
            player.effectDurations[static_cast<size_t>(spellId - 1)] = -2;
            player.tempArmorBonus =
                static_cast<int16_t>(10 + PlayerCombatStats::SkillValue(player, charData, 6, false));
            break;
        case 18:
            player.effectDurations[static_cast<size_t>(spellId - 1)] = -2;
            target.scratch[0] = static_cast<int8_t>(3 * multiplier);
            break;
        case 19: {
            player.effectDurations[static_cast<size_t>(spellId - 1)] = -2;
            int val19 = multiplier * (60 - 5 * targetEvasion);
            val19 = std::min(std::max(val19, 0), 100);
            target.scratch[3] = static_cast<int8_t>(val19);
            break;
        }
        case 20: {
            int base20 = 80 - 5 * targetEvasion;
            int raw20 = base20 * multiplier;
            int dmg20 = raw20 - MonsterRuntime::Stat(target, monsterDb, 8);
            dmg20 = std::max(dmg20, 4);
            int scaled20 = dmg20 * MonsterRuntime::Stat(target, monsterDb, 14) / 100;
            MonsterRuntime::TakeDamage(target, scaled20);
            break;
        }
        default:
            break;
    }

    player.coreStats[6] =
        static_cast<int16_t>(player.coreStats[6] - 5 * PlayerCombatStats::FatigueCostMultiplier(player));
    player.coreStats[6] = std::max(player.coreStats[6], int16_t{0});

    if (PlayerCombatStats::HasAilment(player, 6)) {
        int drain = 2 * player.coreStats[3] / 100;
        if (drain < 1) drain = 1;
        player.coreStats[2] = static_cast<int16_t>(player.coreStats[2] - drain);
    }
}

void CombatResolution::UseItem(PlayerState& player, int slot, MonsterState* target, const ItemDatabase& items,
                                const MonsterDatabase& monsterDb, std::vector<GeneratedLevel>& levels,
                                WorldRegistry& world, JavaRandom& globalRng) {
    int itemId = std::abs(static_cast<int>(player.inventoryItemIds[slot]));
    int8_t category = items.category[static_cast<size_t>(itemId - 1)];
    if (category != 13) return;

    bool consume = true;
    switch (itemId) {
        case 87:
            if (player.currentLevel == 1 && PlayerMovement::HasCampMark(player)) {
                PlayerMovement::WarpToCampMark(player, levels);
                break;
            }
            PlayerMovement::MarkCampAndReturnToTown(player, false, levels, world);
            break;
        case 88:
            PlayerSpellcasting::CureRandomAilment(player, globalRng);
            break;
        case 89:
            player.coreStats[2] = player.coreStats[3];
            break;
        case 90:
            player.coreStats[4] = player.coreStats[5];
            break;
        case 91:
            player.coreStats[6] = static_cast<int16_t>(player.coreStats[6] + 3 * player.coreStats[5]);
            break;
        case 92:
            player.coreStats[1]++;
            break;
        case 93:
            player.coreStats[2] = player.coreStats[3];
            player.coreStats[4] = player.coreStats[5];
            break;
        case 94:
            player.increaseHarmBuff = true;
            break;
        case 95:
            player.increaseArmorBuff = true;
            break;
        case 96:
            player.safeCampingBuff = true;
            consume = false;
            break;
        case 97:
            if (target != nullptr) {
                int def = MonsterRuntime::Stat(*target, monsterDb, 4);
                int evasion = MonsterRuntime::Stat(*target, monsterDb, 10);
                if (def <= 13 && evasion <= 13) target->hp = 0;
                // target.store(): SKIPPED, see this method's doc comment.
            }
            break;
        case 98:
            if (target != nullptr) {
                int def = MonsterRuntime::Stat(*target, monsterDb, 4);
                int evasion = MonsterRuntime::Stat(*target, monsterDb, 10);
                if (def <= 22 && evasion <= 22) target->hp = 0;
                // target.store(): SKIPPED.
            }
            break;
        case 99:
            if (target != nullptr) {
                int def = MonsterRuntime::Stat(*target, monsterDb, 4);
                int evasion = MonsterRuntime::Stat(*target, monsterDb, 10);
                if (def <= 29 && evasion <= 29) target->hp = 0;
                // target.store(): SKIPPED.
            }
            break;
        default:
            break;
    }

    if (consume) PlayerInventory::RemoveSlot(player, items, slot);
}

}  // namespace dawnstar
