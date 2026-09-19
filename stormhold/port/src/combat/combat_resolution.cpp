#include "combat/combat_resolution.h"

#include <algorithm>

#include "monster/monster_runtime.h"
#include "player/player_combat_stats.h"
#include "player/player_leveling.h"

namespace stormhold {

void CombatResolution::PlayerAttack(PlayerState& player, MonsterState& target, const CharacterData& charData,
                                     const ItemDatabase& items, const MonsterDatabase& monsterDb,
                                     JavaRandom& globalRng, WorldRegistry& world) {
    player.lastCombatTargetId = target.spawnId;
    // byte type = target.typeIndex: read but never used in the original
    // (Player.java's attack()), not ported.

    int offense = PlayerCombatStats::AttackPower(player, charData, items, true);
    int defense = MonsterRuntime::Stat(target, monsterDb, 7);
    int diff = offense - defense;
    diff = std::min(diff, static_cast<int>(MonsterRuntime::Stat(target, monsterDb, 2)));

    if (PlayerCombatStats::IsEffectActive(player, 10)) {
        if (target.scratch[8] == 0) {
            PlayerCombatStats::ClearEffect(player, 10);
        } else {
            diff += target.scratch[8];
        }
    }

    int monsterDefendChance = static_cast<int>(MonsterRuntime::Stat(target, monsterDb, 6)) - diff * 5;
    int playerAttackChance = PlayerCombatStats::AttackAccuracy(player, charData, items) + diff * 5;
    monsterDefendChance = std::min(std::max(monsterDefendChance, 10), 95);
    playerAttackChance = std::min(std::max(playerAttackChance, 10), 95);

    int tier = PlayerCombatStats::RollOutcome(globalRng, playerAttackChance, monsterDefendChance);
    if (tier == 0) return;

    int power = PlayerCombatStats::WeaponDamage(player, charData, items);
    int armor = MonsterRuntime::Stat(target, monsterDb, 8);
    if (PlayerCombatStats::IsEffectActive(player, 13)) {
        if (target.scratch[5] == 0) {
            player.effectDurations[12] = 0;
        } else {
            armor -= target.scratch[5];
        }
    }

    if (tier == 1) {
        armor = 2 * armor;
    } else if (tier == 3) {
        power = 2 * power;
    }

    int dmg = power - armor;
    dmg = std::max(dmg, 4);
    int scaled = dmg * static_cast<int>(MonsterRuntime::Stat(target, monsterDb, 14)) / 100;
    MonsterRuntime::TakeDamage(target, scaled);
    DungeonRuntime::StoreMonster(world, target);  // target.store()

    if (PlayerCombatStats::IsEffectActive(player, 7)) {
        if (target.scratch[1] == 0) {
            PlayerCombatStats::ClearEffect(player, 7);
        } else {
            int raw = target.scratch[1];
            raw = std::max(raw, 4);
            scaled = raw * static_cast<int>(MonsterRuntime::Stat(target, monsterDb, 14)) / 100;
            MonsterRuntime::TakeDamage(target, scaled);
        }
    }

    if (tier >= 2) {
        int skillIdx = PlayerCombatStats::ActiveWeaponSkillIndex(player, charData, items);
        PlayerLeveling::GainSkillExp(player, skillIdx, 1);
    }

    if (!PlayerCombatStats::IsEffectActive(player, 7)) {
        int fatigueCostMultiplier = (player.ailmentMask & 1) ? 3 : 1;
        int16_t newFatigue = static_cast<int16_t>(player.coreStats[6] - 7 * fatigueCostMultiplier);
        player.coreStats[6] = std::max(newFatigue, int16_t{0});
    }

    if (PlayerCombatStats::HasAilment(player, 6)) {
        int selfDmg = 2 * player.coreStats[3] / 100;
        if (selfDmg < 1) selfDmg = 1;
        player.coreStats[2] = static_cast<int16_t>(player.coreStats[2] - selfDmg);
    }
}

void CombatResolution::MonsterTick(MonsterState& m, PlayerState& player, const CharacterData& charData,
                                    const ItemDatabase& items, const MonsterDatabase& monsterDb, int64_t now,
                                    JavaRandom& globalRng, GeneratedLevel& level, WorldRegistry& world,
                                    JavaRandom& ambushRng, int16_t& spawnIdCounter) {
    m.aiPhase = 2;
    m.unconfirmedTimestamp = now;

    int baseDefense = MonsterRuntime::RawStat(m, monsterDb, 4);
    int playerOffense = PlayerCombatStats::DefenseSkillValue(player, charData, items, true);
    int diff = playerOffense - baseDefense;
    diff = std::min(diff, static_cast<int>(MonsterRuntime::RawStat(m, monsterDb, 2)));

    int attackChance = MonsterRuntime::RawStat(m, monsterDb, 3) - diff * 5;
    int defendChance = PlayerCombatStats::BaseEvasion(player, items) + diff * 5;
    attackChance = std::min(std::max(attackChance, 10), 95);
    defendChance = std::min(std::max(defendChance, 10), 95);

    int rollA = RandomInt1Based(globalRng, 100);
    int rollB = RandomInt1Based(globalRng, 100);
    bool hitA = rollA <= attackChance;
    bool hitB = rollB <= defendChance;

    int tier;
    if (hitA && !hitB) {
        tier = 3;
    } else if (hitA && hitB) {
        tier = rollA >= rollB ? 2 : 1;
    } else if (hitA || hitB) {
        tier = 0;
    } else {
        tier = rollA >= rollB ? 2 : 1;
    }

    if (tier == 0) {
        m.aiPhase = 1;
        return;
    }

    int basePower = MonsterRuntime::RawStat(m, monsterDb, 5);
    int armor = PlayerCombatStats::ArmorValue(player, charData, items);
    if (tier == 1) armor = 2 * armor;

    int damage = basePower - armor;
    damage = std::max(damage, 4);
    int scaled = damage * player.coreStats[3] / 100;
    int16_t newHp = static_cast<int16_t>(player.coreStats[2] - scaled);
    player.coreStats[2] = std::max(newHp, int16_t{0});

    if (hitB) {
        int skillIdx = PlayerCombatStats::DefenseSkillIndex(player, items);
        PlayerLeveling::GainSkillExp(player, skillIdx, 1);
    }

    if (tier < 3) {
        m.aiPhase = 1;
        return;
    }

    if (RandomInt1Based(globalRng, 100) <= 30) {
        int ailmentId = MonsterRuntime::RawStat(m, monsterDb, 11);
        if (ailmentId > 0) {
            int bit = ailmentId - 1;
            player.ailmentMask = static_cast<int8_t>(player.ailmentMask | (1 << bit));
            if (ailmentId != 1) {
                if (ailmentId == 2) {
                    // Dungeon.spawnAmbushMonsters(3) -- see this method's
                    // own declaration comment for why `level`/`ambushRng`
                    // are separate parameters from `world`/`globalRng`.
                    DungeonRuntime::SpawnAmbushMonsters(level, world, 3, ambushRng, monsterDb, spawnIdCounter);
                } else if (ailmentId != 3) {
                    if (ailmentId == 4) {
                        player.vampirismTimer = 30000;
                    } else if (ailmentId == 5) {
                        player.manaBurnTimer = 30000;
                    }
                    // ailment 6/7/8: no-op in the original too.
                }
            }
        }
    }

    m.aiPhase = 1;
}

bool CombatResolution::TickMonstersOnLevel(WorldRegistry& world, GeneratedLevel& level, PlayerState& player,
                                            const CharacterData& charData, const ItemDatabase& items,
                                            const MonsterDatabase& monsterDb, std::vector<GeneratedLevel>& levels,
                                            int64_t now, JavaRandom& globalRng, JavaRandom& ambushRng,
                                            int16_t& spawnIdCounter) {
    bool showAttackMessage = false;
    size_t levelIndex = static_cast<size_t>(level.number - 1);

    std::vector<int16_t> spawnIds;
    spawnIds.reserve(world.monsters[levelIndex].size());
    for (const auto& entry : world.monsters[levelIndex]) spawnIds.push_back(entry.first);

    for (int16_t spawnId : spawnIds) {
        auto it = world.monsters[levelIndex].find(spawnId);
        if (it == world.monsters[levelIndex].end()) continue;
        MonsterState m = MonsterRuntime::FromBytes(it->second);

        if (MonsterRuntime::IsAdjacent(m, player.tileX, player.tileY)) {
            if (m.aiPhase == 0) {
                m.unconfirmedTimestamp = now;
                m.aiPhase = 1;
            } else if (m.aiPhase == 1 && now - m.unconfirmedTimestamp > 800) {
                MonsterTick(m, player, charData, items, monsterDb, now, globalRng, level, world, ambushRng,
                            spawnIdCounter);
                showAttackMessage = true;
            } else if (now - m.unconfirmedTimestamp > 800) {
                MonsterTick(m, player, charData, items, monsterDb, now, globalRng, level, world, ambushRng,
                            spawnIdCounter);
            }
        } else {
            MonsterRuntime::Chase(m, player.tileX, player.tileY, levels, globalRng);
        }

        DungeonRuntime::StoreMonster(world, m);
    }

    return showAttackMessage;
}

bool CombatResolution::ResolveAttackInput(PlayerState& player, std::optional<MonsterState>& target,
                                           bool& attackRequested, int64_t now, int64_t& lastAttackTimeMs,
                                           const CharacterData& charData, const ItemDatabase& items,
                                           const MonsterDatabase& monsterDb, JavaRandom& globalRng,
                                           WorldRegistry& world) {
    bool attacked = false;
    if (now - lastAttackTimeMs >= 500 && target.has_value()) {
        PlayerAttack(player, *target, charData, items, monsterDb, globalRng, world);
        lastAttackTimeMs = now;
        attacked = true;
    }

    attackRequested = false;
    return attacked;
}

}  // namespace stormhold
