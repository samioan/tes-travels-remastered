#include "combat/combat_tick.h"

#include <algorithm>

#include "combat/combat_resolution.h"
#include "monster/monster_runtime.h"
#include "player/player_combat_stats.h"
#include "player/player_movement.h"

namespace dawnstar {

bool CombatTick::ProcessAttack(PlayerState& player, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                                const MonsterDatabase& monsterDb, const ItemDatabase& items,
                                const CharacterData& charData, JavaRandom& globalRng, int64_t nowMs,
                                int64_t& lastAttackTimeMs) {
    auto* record = PlayerMovement::MonsterInFront(player, levels, world);
    if (record == nullptr || nowMs - lastAttackTimeMs < 500) return false;

    MonsterState target = MonsterRuntime::FromBytes(*record);
    int8_t hpBefore = target.hp;
    CombatResolution::PlayerAttack(player, target, charData, items, monsterDb, globalRng);
    lastAttackTimeMs = nowMs;
    *record = MonsterRuntime::ToBytes(target);
    return hpBefore > target.hp;
}

void CombatTick::RefreshAndResolveTargetMonster(PlayerState& player, std::vector<GeneratedLevel>& levels,
                                                 WorldRegistry& world, const MonsterDatabase& monsterDb,
                                                 const ItemDatabase& items, MessagePopupState& messagePopup,
                                                 JavaRandom& globalRng, int64_t nowMs, int16_t& nextDropSpawnId) {
    // refreshTargetMonster()
    auto* record = PlayerMovement::MonsterInFront(player, levels, world);
    player.monsterTargeted = record != nullptr;
    if (record == nullptr) return;

    MonsterState target = MonsterRuntime::FromBytes(*record);
    if (target.hp > 0) return;

    // resolveMonsterDeath()
    if (target.monsterType == 41) {
        player.specialEncounterResolved = true;
        player.roamingSpecialMonsterPresent = false;
    }

    if (target.monsterType != 42) {
        GeneratedLevel& monsterLevel = levels[static_cast<size_t>(target.dungeonLevel - 1)];
        MonsterRuntime::DeathDrop drop =
            MonsterRuntime::OnDeath(target, monsterDb, items, monsterLevel.tier, false, nextDropSpawnId++, globalRng);
        if (drop.dropped) {
            DungeonRuntime::AddDroppedItem(monsterLevel, world, drop.record);
        }
    }
    // type 42's own end-of-game-UI transition isn't ported (no menu/
    // end-of-game screen exists yet) -- the rest of this cleanup still
    // runs unconditionally either way, matching the original's own
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
}

}  // namespace dawnstar
