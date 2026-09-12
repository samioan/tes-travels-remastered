#include "combat/combat_resolution.h"

#include <algorithm>

#include "monster/monster_runtime.h"
#include "player/player_combat_stats.h"

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
                                    JavaRandom& globalRng) {
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
                    // Dungeon.populateRandomMonsters(3): SKIPPED, see
                    // this method's doc comment in combat_resolution.h.
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

}  // namespace dawnstar
