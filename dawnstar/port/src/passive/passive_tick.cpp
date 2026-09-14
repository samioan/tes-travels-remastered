#include "passive/passive_tick.h"

#include "player/player_combat_stats.h"
#include "player/player_inventory.h"

namespace dawnstar {

void PassiveTick::TickStatusCountdowns(PlayerState& player, int64_t elapsedMs, bool monsterAttacking) {
    // Transcribed from GameCanvas.java lines 1805-1832. Java's
    // `(short)(timer - elapsed)` long-arithmetic-then-narrow is
    // reproduced by the int16_t cast; the expiry bit-ORs are the idempotent
    // same-bit writes this method's own header doc comment documents --
    // kept as written rather than "cleaned up".
    if (PlayerCombatStats::HasAilment(player, 4)) {
        player.trollThirstTimer = static_cast<int16_t>(player.trollThirstTimer - elapsedMs);
        if (player.trollThirstTimer < 0) {
            player.trollThirstTimer = 0;
            player.ailmentMask = static_cast<int8_t>(player.ailmentMask | (1 << 3));
        }
    }

    if (PlayerCombatStats::HasAilment(player, 5)) {
        player.glacierCurseTimer = static_cast<int16_t>(player.glacierCurseTimer - elapsedMs);
        if (player.glacierCurseTimer < 0) {
            player.glacierCurseTimer = 0;
            player.ailmentMask = static_cast<int8_t>(player.ailmentMask | (1 << 4));
        }
    }

    if (PlayerCombatStats::HasAilment(player, 7) && monsterAttacking) {
        player.terrifiedTimer = static_cast<int16_t>(player.terrifiedTimer - elapsedMs);
        if (player.terrifiedTimer < 0) {
            player.terrifiedTimer = 0;
            player.ailmentMask = static_cast<int8_t>(player.ailmentMask | (1 << 6));
        }
    }
}

PassiveTick::PerSecondResult PassiveTick::TickPerSecond(PlayerState& player, std::vector<GeneratedLevel>& levels,
                                                        WorldRegistry& world, const ItemDatabase& items,
                                                        const MonsterDatabase& monsterDb, JavaRandom& globalRng,
                                                        int16_t& nextMonsterSpawnId, int64_t nowMs,
                                                        MessagePopupState& messagePopup) {
    // (1) `if (this.player.hasAilment(4))`: `drain = 2 * coreStats[3] /
    // 100; drain = Math.max(drain, 0); coreStats[2] -= drain` -- NOT
    // clamped at 0 below, exactly as the original.
    if (PlayerCombatStats::HasAilment(player, 4)) {
        int drain = 2 * player.coreStats[3] / 100;
        if (drain < 0) drain = 0;
        player.coreStats[2] = static_cast<int16_t>(player.coreStats[2] - drain);
    }

    // (2) `if (this.player.hasAilment(5))`: the overflow-resets-to-0
    // Magicka quirk, preserved exactly.
    if (PlayerCombatStats::HasAilment(player, 5)) {
        int regen = player.coreStats[5] / 10;
        player.coreStats[4] = static_cast<int16_t>(player.coreStats[4] + regen);
        if (player.coreStats[4] >= player.coreStats[5]) {
            player.coreStats[4] = 0;
            int drain = player.coreStats[5] / 10;
            player.coreStats[2] = static_cast<int16_t>(player.coreStats[2] - drain);
        }
    }

    // (3) The effectDurations[] per-second countdown, with its one
    // special case at index 5 (effect 6, "Safe Camping").
    for (int i = 0; i < 25; i++) {
        if (player.effectDurations[static_cast<size_t>(i)] > 0) {
            player.effectDurations[static_cast<size_t>(i)]--;
            if (player.effectDurations[static_cast<size_t>(i)] <= 0) {
                player.effectDurations[static_cast<size_t>(i)] = 0;
                if (i == 5) {
                    int slot = PlayerInventory::FindSlotOf(player, 101);
                    if (slot != -1) {
                        PlayerInventory::RemoveSlot(player, items, slot);
                    }
                }
            }
        }
    }

    // (4) GameCanvas.java lines 1872-1888: "levelMonsters = ESGame.
    // monsters[currentLevel-1]; if (levelMonsters != null) { ... while
    // (monsters.hasMoreElements()) { Monster.fromBytes(scratch, rec); if
    // (scratch.scratch[6] != 0) { scratch.scratch[7]--; ... } } }" -- a
    // real NO-OP in the original: it decodes each record into a
    // throwaway `scratch` Monster and decrements THAT copy's cooldown
    // bytes, but never stores the mutated record back into the registry,
    // so nothing observable ever changes (compare M36's real store-backed
    // tick, combat_tick.cpp's TickNearbyMonsters). Nothing to port beyond
    // this documentation -- a registry iteration that cannot change any
    // value would be pure cost.

    // (5) The ambush spawner. `if (this.player.ambushTimer >= 0) { int
    // elapsedSeconds = ++this.player.ambushTimer; ... }`.
    if (player.ambushTimer >= 0) {
        int elapsedSeconds = ++player.ambushTimer;
        int spawnType = -1;
        if (player.newGamePlus) {
            switch (elapsedSeconds) {
                case 3: spawnType = 4; break;
                case 20: spawnType = 16; break;
                case 35: spawnType = 7; break;
                case 38: spawnType = 18; break;
                case 53: spawnType = 12; break;
                case 68: spawnType = 20; break;
                case 70: spawnType = 22; break;
                case 85: spawnType = 24; break;
                case 100: spawnType = 26; break;
                case 115: spawnType = 28; break;
                case 117: spawnType = 30; break;
                case 127: spawnType = 31; break;
                case 132: spawnType = 32; break;
                default: break;
            }
        } else {
            switch (elapsedSeconds) {
                case 5: spawnType = 4; break;
                case 16: spawnType = 16; break;
                case 28: spawnType = 8; break;
                case 38: spawnType = 20; break;
                case 42: spawnType = 21; break;
                case 55: spawnType = 22; break;
                case 66: spawnType = 23; break;
                case 77: spawnType = 24; break;
                case 88: spawnType = 25; break;
                case 99: spawnType = 26; break;
                case 105: spawnType = 27; break;
                case 115: spawnType = 28; break;
                case 118: spawnType = 29; break;
                case 127: spawnType = 30; break;
                case 132: spawnType = 31; break;
                default: break;
            }
        }

        if (elapsedSeconds == 140) {
            spawnType = 42;
        }

        if (spawnType > 0) {
            int levelIndex = player.currentLevel - 1;
            // `int spawnX = 1 + Util.randomInt(17);` then the retry loop
            // (`for (spawnY = ...; !trySpawn(spawnX, spawnY, spawnType);
            // spawnY = ... ) { spawnX = ...; }`) -- X then Y before the
            // first attempt, then X (body) before Y (update) before every
            // retry, so the RNG call order below is exact.
            int spawnX = 1 + LingoRandomInt(globalRng, 17);
            int spawnY = 1 + LingoRandomInt(globalRng, 17);
            while (!DungeonRuntime::TrySpawnMonsterNear(levels, world, levelIndex, spawnX, spawnY, spawnType,
                                                        globalRng, monsterDb, nextMonsterSpawnId)) {
                spawnX = 1 + LingoRandomInt(globalRng, 17);
                spawnY = 1 + LingoRandomInt(globalRng, 17);
            }

            // `if (ESGame.monsters[currentLevel - 1].size() > 5) { ...
            // endOfGameUI ... } else if (this.showMessage(
            // MSG_ENEMY_ARRIVED, 3)) { ... }` -- the level's WHOLE
            // registry, newcomer included.
            if (world.monsters[static_cast<size_t>(levelIndex)].size() > 5) {
                return PerSecondResult::EndOfGame;
            }
            MessagePopup::Show(messagePopup, {"Enemy", "arrived!"}, 3, nowMs);
        }
    }
    return PerSecondResult::None;
}

}  // namespace dawnstar
