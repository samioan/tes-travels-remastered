#include "combat/combat_tick.h"

#include <algorithm>
#include <cstdlib>

#include "combat/combat_resolution.h"
#include "monster/monster_runtime.h"
#include "player/player_combat_stats.h"
#include "player/player_movement.h"
#include "player/player_spellcasting.h"

namespace dawnstar {

bool CombatTick::ProcessAttack(PlayerState& player, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                                const MonsterDatabase& monsterDb, const ItemDatabase& items,
                                const CharacterData& charData, JavaRandom& globalRng, int64_t nowMs,
                                int64_t& lastAttackTimeMs, bool& actionTaken) {
    auto* record = PlayerMovement::MonsterInFront(player, levels, world);
    if (record == nullptr || nowMs - lastAttackTimeMs < 500) return false;

    actionTaken = true;
    MonsterState target = MonsterRuntime::FromBytes(*record);
    int8_t hpBefore = target.hp;
    CombatResolution::PlayerAttack(player, target, charData, items, monsterDb, globalRng);
    lastAttackTimeMs = nowMs;
    *record = MonsterRuntime::ToBytes(target);
    return hpBefore > target.hp;
}

bool CombatTick::RefreshAndResolveTargetMonster(PlayerState& player, std::vector<GeneratedLevel>& levels,
                                                 WorldRegistry& world, const MonsterDatabase& monsterDb,
                                                 const ItemDatabase& items, MessagePopupState& messagePopup,
                                                 JavaRandom& globalRng, int64_t nowMs, int16_t& nextDropSpawnId) {
    // refreshTargetMonster()
    auto* record = PlayerMovement::MonsterInFront(player, levels, world);
    player.monsterTargeted = record != nullptr;
    if (record == nullptr) return false;

    MonsterState target = MonsterRuntime::FromBytes(*record);
    if (target.hp > 0) return false;

    // resolveMonsterDeath()
    if (target.monsterType == 41) {
        player.specialEncounterResolved = true;
        player.roamingSpecialMonsterPresent = false;
    }

    bool victoryTriggered = target.monsterType == 42;
    if (!victoryTriggered) {
        GeneratedLevel& monsterLevel = levels[static_cast<size_t>(target.dungeonLevel - 1)];
        MonsterRuntime::DeathDrop drop =
            MonsterRuntime::OnDeath(target, monsterDb, items, monsterLevel.tier, false, nextDropSpawnId++, globalRng);
        if (drop.dropped) {
            DungeonRuntime::AddDroppedItem(monsterLevel, world, drop.record);
        }
    }
    // type 42's own end-of-game-UI transition (M56) is signaled via this
    // method's return value -- the rest of this cleanup still runs
    // unconditionally either way, matching the original's own
    // fallthrough (both branches reach the same removeMonster/heal/
    // message/reset code below).

    // ESGame.removeMonster(this.player.currentLevel, ...): a REAL,
    // faithfully-preserved oddity, not a port bug -- the original uses
    // the PLAYER's current level here, not the monster's own
    // dungeonLevel (used just above for OnDeath's tier and
    // AddDroppedItem). The two can differ at a doorway tile (looking
    // one step into a neighboring level without having crossed into it
    // yet -- the same ComputeMoveTarget(1,...) cross-level case
    // NpcInFront/ChestInFront's own doc comments already flag), in
    // which case this call silently searches the WRONG level's
    // registry and removes nothing -- harmless in practice (the
    // monster is already dead; nothing else in this milestone depends
    // on it actually being removed for correctness), but preserved
    // exactly rather than "fixed" to use target.dungeonLevel instead.
    GeneratedLevel& playerLevel = levels[static_cast<size_t>(player.currentLevel - 1)];
    DungeonRuntime::RemoveMonster(playerLevel, world, target.x, target.y);

    if (PlayerCombatStats::HasAilment(player, 4)) {
        player.coreStats[2] = static_cast<int16_t>(player.coreStats[2] + 3 * player.coreStats[3] / 10);
        player.coreStats[2] = static_cast<int16_t>(std::min<int>(player.coreStats[2], player.coreStats[3]));
    }

    MessagePopup::Show(messagePopup, {"Creature", "is dead!"}, 1, nowMs);

    player.monsterTargeted = false;
    player.minimapDirty = true;
    return victoryTriggered;
}

void CombatTick::ProcessSpellCast(PlayerState& player, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                                   const MonsterDatabase& monsterDb, const ItemDatabase& items,
                                   const CharacterData& charData, const SpellDatabase& spells,
                                   MessagePopupState& messagePopup, JavaRandom& globalRng, int64_t nowMs,
                                   int64_t& lastSpellCastTimeMs, bool& spellHitFlash, bool& selfSpellFlash,
                                   bool& actionTaken) {
    int spellId = player.selectedSpellId;
    if (!spells.IsValidId(spellId)) return;

    if (spells.ById(spellId).magickaCost > PlayerCombatStats::EffectiveStat(player, charData, 4)) {
        MessagePopup::Show(messagePopup, {"Not enough", "magicka!"}, 3, nowMs);
        return;
    }

    if (nowMs - lastSpellCastTimeMs < 500) return;

    actionTaken = true;
    if (spells.IsOffensive(spellId)) {
        if (!player.monsterTargeted) {
            MessagePopup::Show(messagePopup, {"No monster", "here!"}, 1, nowMs);
        } else {
            auto* record = PlayerMovement::MonsterInFront(player, levels, world);
            if (record != nullptr) {
                MonsterState target = MonsterRuntime::FromBytes(*record);
                CombatResolution::CastOnMonster(player, target, charData, items, monsterDb, spells, globalRng);
                *record = MonsterRuntime::ToBytes(target);
                spellHitFlash = true;
            }
        }
    } else {
        PlayerSpellcasting::CastOnSelf(player, charData, items, spells, globalRng);
        selfSpellFlash = true;
    }

    lastSpellCastTimeMs = nowMs;
}

void CombatTick::CycleSpell(PlayerState& player, const SpellDatabase& spells, MessagePopupState& messagePopup,
                             int64_t nowMs) {
    int spellId = PlayerSpellcasting::CycleSelectedSpell(player, spells);
    if (spellId == 0) {
        MessagePopup::Show(messagePopup, {"No spells!", ""}, -1, nowMs);
    } else {
        player.selectedSpellId = static_cast<int8_t>(spellId);
        MessagePopup::Show(messagePopup, MessagePopup::WrapToTwoLines(spells.ById(spellId).name), -1, nowMs);
    }
}

void CombatTick::TickNearbyMonsters(PlayerState& player, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                                     const CharacterData& charData, const ItemDatabase& items,
                                     const MonsterDatabase& monsterDb, int64_t nowMs, JavaRandom& globalRng,
                                     MessagePopupState& messagePopup, int16_t& spawnIdCounter) {
    auto& monsterMap = world.monsters[static_cast<size_t>(player.currentLevel - 1)];

    // Snapshot the in-range keys first: a successful Chase step below
    // re-keys its own map entry (erase+insert), which would be undefined
    // behavior if done while range-iterating the same live map (the
    // exact bug M34's own test caught and fixed for a different map).
    std::vector<int> keysInRange;
    for (const auto& [key, record] : monsterMap) {
        int x = 0, y = 0;
        UnpackPosKey(key, &x, &y);
        int dist = std::abs(x - player.tileX) + std::abs(y - player.tileY);
        if (dist >= 1 && dist <= 3) keysInRange.push_back(key);
    }

    bool moved = false;
    bool attacked = false;
    for (int key : keysInRange) {
        auto it = monsterMap.find(key);
        if (it == monsterMap.end()) continue;  // defensive only: nothing above ever removes an entry this loop hasn't reached yet.

        MonsterState m = MonsterRuntime::FromBytes(it->second);
        int dist = std::abs(static_cast<int>(m.x) - player.tileX) + std::abs(static_cast<int>(m.y) - player.tileY);
        if (dist == 1) {
            bool landed = CombatResolution::MonsterTick(m, player, charData, items, monsterDb, nowMs, globalRng,
                                                         levels, world, spawnIdCounter);
            if (landed) attacked = true;
            // MonsterTick (M53: the ailment==2 "curse of hunger" case)
            // can insert new entries into THIS SAME monsterMap, which
            // may rehash it and invalidate `it` -- re-find rather than
            // reuse the iterator across that call. `key` itself (m's
            // position before this tick) is unaffected by attacking,
            // so it's still the right lookup.
            auto refreshed = monsterMap.find(key);
            if (refreshed != monsterMap.end()) refreshed->second = MonsterRuntime::ToBytes(m);
        } else {
            int oldX = m.x, oldY = m.y;
            if (MonsterRuntime::Chase(m, player.tileX, player.tileY, levels, globalRng)) {
                moved = true;
            }
            if (m.x != oldX || m.y != oldY) {
                monsterMap.erase(key);
                monsterMap[PackPosKey(m.x, m.y)] = MonsterRuntime::ToBytes(m);
            } else {
                it->second = MonsterRuntime::ToBytes(m);
            }
        }
    }

    if (moved) player.minimapDirty = true;
    if (attacked) MessagePopup::Show(messagePopup, {"Creature", "attacks!"}, 2, nowMs);
}

}  // namespace dawnstar
