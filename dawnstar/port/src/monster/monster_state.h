#pragma once
#include <array>
#include <cstdint>

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Monster.java's per-instance
// runtime state (as opposed to assets/monster_database.h, the static
// per-type stat table Monster.load() builds once at startup).
struct MonsterState {
    int16_t spawnId = 0;
    int8_t monsterType = 0;
    int8_t hp = 0;
    int8_t x = 0;
    int8_t y = 0;
    // Collected/looted marker -- Monster.java's own field comment
    // doesn't say more than that; set true once this monster's corpse
    // (or its drop) has been dealt with.
    bool flag = false;
    int8_t dungeonLevel = 0;
    // 10 bytes of monster-type-specific combat/status scratch space
    // (e.g. Player.attack()'s reads of scratch[1]/[5]/[8] for specific
    // ongoing-effect magnitudes -- see player/player_combat_stats.h's
    // callers once Player.attack() itself is ported).
    std::array<int8_t, 10> scratch{};
    // AI pacing: chase() moves once every 5 ticks (0..4).
    int8_t moveCooldown = 0;
    // AI phase: 0=idle/just noticed, 1=wound-up (waiting), 2=acted this
    // encounter -- see MonsterRuntime::Tick.
    int8_t aiPhase = 0;
    // Wall-clock ms timestamp (System.currentTimeMillis() in the
    // original) marking the start of the current aiPhase's wait.
    int64_t timestamp = 0;
};

}  // namespace dawnstar
