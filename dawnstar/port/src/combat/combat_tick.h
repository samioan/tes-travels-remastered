#pragma once
#include <cstdint>
#include <vector>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/spell_database.h"
#include "dungeon/dungeon_runtime.h"
#include "player/player_state.h"
#include "render/message_popup.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace dawnstar {

// Renamed-source counterpart of GameCanvas.processAttack()/
// refreshTargetMonster()/resolveMonsterDeath()/processSpellCast()/
// cycleSelectedSpell() -- the player-initiated attack and spellcasting
// actions, plus dispatchTickActions()'s own always-run-every-tick
// target-refresh/death-resolution tail. Kept in its own module (the
// same reason combat/combat_resolution.h itself is) because it needs
// BOTH dawnstar_combat (CombatResolution::PlayerAttack/CastOnMonster/
// MonsterRuntime::OnDeath) and dawnstar_render (render/message_popup.h,
// for the "Creature is dead!"/"Not enough magicka!"/etc. popups) --
// neither depends on the other, so combining them anywhere else would
// either cycle or force main.cpp to duplicate untested logic inline.
// See docs/PORT_ROADMAP.md's M32/M33 entries.
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

    // GameCanvas.processSpellCast(): casts player.selectedSpellId,
    // gated by the same "poll the held key every tick, rely on the
    // original's own internal 500ms cooldown to throttle it" shape
    // ProcessAttack above already established for a discrete-keydown-
    // event-plus-cooldown action (GameCanvas.keyPressed()'s own
    // `key == 51` handler sets castSpellRequested unconditionally,
    // unlike attack's own hotbarContext-gated key). Order matches the
    // original exactly: invalid spell id -> no-op; not enough Magicka
    // -> message (does NOT advance the cooldown); cooldown not yet
    // elapsed -> no-op; then, if offensive, either "No monster here!"
    // or CombatResolution::CastOnMonster on whatever's directly in
    // front (re-derived fresh via PlayerMovement::MonsterInFront, same
    // "SIMPLIFIED but not lossy" reasoning as ProcessAttack above --
    // action dispatch is mutually exclusive per tick, so the front tile
    // can't have moved since player.monsterTargeted was last refreshed);
    // otherwise CombatResolution::CastOnSelf. A real preserved quirk:
    // `lastSpellCastTimeMs` advances even when an offensive cast finds
    // no monster -- the original's own `this.lastSpellCastTime = now;`
    // sits OUTSIDE the monsterTargeted check, at the end of the same
    // branch that guards it.
    static void ProcessSpellCast(PlayerState& player, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                                  const MonsterDatabase& monsterDb, const ItemDatabase& items,
                                  const CharacterData& charData, const SpellDatabase& spells,
                                  MessagePopupState& messagePopup, JavaRandom& globalRng, int64_t nowMs,
                                  int64_t& lastSpellCastTimeMs, bool& spellHitFlash, bool& selfSpellFlash);

    // GameCanvas.cycleSelectedSpell(): unlike ProcessAttack/
    // ProcessSpellCast above, the original has no internal cooldown
    // here -- every physical keydown cycles exactly once, so main.cpp's
    // own caller edge-detects the key itself (a stand-in for
    // spellCycleRequested, set on keydown and consumed here) rather
    // than polling it every tick the way attack/cast do.
    static void CycleSpell(PlayerState& player, const SpellDatabase& spells, MessagePopupState& messagePopup,
                           int64_t nowMs);
};

}  // namespace dawnstar
