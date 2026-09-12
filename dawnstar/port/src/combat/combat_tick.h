#pragma once
#include <cstdint>
#include <vector>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "dungeon/dungeon_runtime.h"
#include "player/player_state.h"
#include "render/message_popup.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace dawnstar {

// Renamed-source counterpart of GameCanvas.processAttack()/
// refreshTargetMonster()/resolveMonsterDeath() -- the player-initiated
// attack action, plus dispatchTickActions()'s own always-run-every-tick
// target-refresh/death-resolution tail. Kept in its own module (the
// same reason combat/combat_resolution.h itself is) because it needs
// BOTH dawnstar_combat (CombatResolution::PlayerAttack/MonsterRuntime::
// OnDeath) and dawnstar_render (render/message_popup.h, for the
// "Creature is dead!" popup) -- neither depends on the other, so
// combining them anywhere else would either cycle or force main.cpp to
// duplicate untested logic inline. See docs/PORT_ROADMAP.md's M32 entry.
class CombatTick {
public:
    // GameCanvas.processAttack(): resolves a player-initiated attack
    // against whatever monster is directly in front, gated by the same
    // 500ms cooldown the original uses (`lastAttackTimeMs`, threaded in
    // by reference since it's a GameCanvas field, not one of Player's
    // own -- same reasoning as render/message_popup.h's
    // MessagePopupState being kept out of PlayerState). Returns whether
    // the attack landed a hit (hpBefore > hpAfter), for the hotbar's
    // own action-flash icon (GameCanvas.paintActionFlashes()'s
    // monsterHitFlash case -- the only one of its 3 cases reachable
    // from this milestone; spellHitFlash/selfSpellFlash remain unported
    // pending the spellcasting-wiring milestone).
    //
    // SIMPLIFIED, but not lossy: re-derives the front monster FRESH here
    // (via PlayerMovement::MonsterInFront) rather than reusing a cached
    // GameCanvas.targetMonster set by the previous tick's
    // refreshTargetMonster() -- equivalent, not an approximation,
    // because attack and movement are mutually exclusive per tick
    // (dispatchTickActions' own if/else chain, mirrored by main.cpp's
    // own `attackActive` gate), so the player's front tile can't have
    // changed since the last refresh whenever this runs.
    static bool ProcessAttack(PlayerState& player, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                              const MonsterDatabase& monsterDb, const ItemDatabase& items,
                              const CharacterData& charData, JavaRandom& globalRng, int64_t nowMs,
                              int64_t& lastAttackTimeMs);

    // GameCanvas.refreshTargetMonster() + resolveMonsterDeath(), always
    // called back-to-back at the very end of dispatchTickActions --
    // every tick, regardless of which action (if any) fired that tick --
    // so combined into one call here since nothing else ever calls
    // either separately. Sets player.monsterTargeted (read every frame
    // by render/hotbar_renderer.h's ComputeHotbarContext); on a death,
    // rolls loot via MonsterRuntime::OnDeath, registers any drop via
    // DungeonRuntime::AddDroppedItem, removes the monster via
    // DungeonRuntime::RemoveMonster, applies the ailment-4 kill-heal
    // bonus, and shows the "Creature is dead!" popup.
    //
    // `nextDropSpawnId` substitutes for Item.nextSpawnId()'s global
    // counter, the same simplification player/player_creation.cpp's
    // GrantStartingItems already uses.
    static void RefreshAndResolveTargetMonster(PlayerState& player, std::vector<GeneratedLevel>& levels,
                                                WorldRegistry& world, const MonsterDatabase& monsterDb,
                                                const ItemDatabase& items, MessagePopupState& messagePopup,
                                                JavaRandom& globalRng, int64_t nowMs, int16_t& nextDropSpawnId);
};

}  // namespace dawnstar
