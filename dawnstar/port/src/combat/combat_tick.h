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
// cycleSelectedSpell(), plus (M36) Dungeon.tickNearbyMonsters() itself
// -- the player-initiated attack and spellcasting actions, the
// monster-initiated AI tick, and dispatchTickActions()'s own
// always-run-every-tick target-refresh/death-resolution tail. Kept in
// its own module (the same reason combat/combat_resolution.h itself is)
// because it needs BOTH dawnstar_combat (CombatResolution::PlayerAttack/
// CastOnMonster/MonsterTick/MonsterRuntime::OnDeath/Chase) and
// dawnstar_render (render/message_popup.h, for the "Creature is dead!"/
// "Not enough magicka!"/"Creature attacks!"/etc. popups) -- neither
// depends on the other, so combining them anywhere else would either
// cycle or force main.cpp to duplicate untested logic inline. See
// docs/PORT_ROADMAP.md's M32/M33/M36 entries.
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
    //
    // M51: `actionTaken` is set true whenever the same cooldown+target
    // gate that decides whether to attack at all passes (matching
    // GameCanvas.processAttack()'s own `actionTakenThisTick = true;`,
    // which sits BEFORE the hit roll) -- deliberately NOT the same as
    // this method's own `hit landed` return value: CombatResolution::
    // PlayerAttack's own `if (roll.outcome == 0) return;` means a missed
    // swing still counts as an action (suppresses passive fatigue regen
    // this tick) even though no damage landed.
    static bool ProcessAttack(PlayerState& player, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                              const MonsterDatabase& monsterDb, const ItemDatabase& items,
                              const CharacterData& charData, JavaRandom& globalRng, int64_t nowMs,
                              int64_t& lastAttackTimeMs, bool& actionTaken);

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
    //
    // M51: `actionTaken` is set true at exactly the point
    // GameCanvas.processSpellCast() sets `actionTakenThisTick = true;`
    // -- after the invalid-id/not-enough-magicka/cooldown gates have all
    // passed, regardless of whether an offensive cast then finds no
    // monster (that "No monster here!" case still consumes the cooldown
    // and counts as an action in the original, matching ProcessAttack's
    // own miss-still-counts reasoning above).
    static void ProcessSpellCast(PlayerState& player, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                                  const MonsterDatabase& monsterDb, const ItemDatabase& items,
                                  const CharacterData& charData, const SpellDatabase& spells,
                                  MessagePopupState& messagePopup, JavaRandom& globalRng, int64_t nowMs,
                                  int64_t& lastSpellCastTimeMs, bool& spellHitFlash, bool& selfSpellFlash,
                                  bool& actionTaken);

    // GameCanvas.cycleSelectedSpell(): unlike ProcessAttack/
    // ProcessSpellCast above, the original has no internal cooldown
    // here -- every physical keydown cycles exactly once, so main.cpp's
    // own caller edge-detects the key itself (a stand-in for
    // spellCycleRequested, set on keydown and consumed here) rather
    // than polling it every tick the way attack/cast do.
    static void CycleSpell(PlayerState& player, const SpellDatabase& spells, MessagePopupState& messagePopup,
                           int64_t nowMs);

    // Dungeon.tickNearbyMonsters(now, player): the monster-initiated AI
    // tick -- every monster registered on the player's own current level
    // within Manhattan distance 1-3 either attacks (distance 1, via
    // CombatResolution::MonsterTick, an 800ms wind-up then an action
    // phase) or takes one step toward the player (distance 2-3, via
    // MonsterRuntime::Chase, at most once every 5 calls per monster).
    // Sets player.minimapDirty on any monster step (GameCanvas.run()'s
    // own `(flags & 1) != 0` check) and shows the real "Creature
    // attacks!" popup on any landed attack (`(flags & 2) != 0`) --
    // folding both of run()'s own post-tickNearbyMonsters checks in here
    // rather than threading a return value back out to main.cpp, same
    // "no real caller needs the raw bits separately" reasoning as
    // RefreshAndResolveTargetMonster's own void return above.
    //
    // SIMPLIFIED, but not lossy: iterates the live WorldRegistry's own
    // monster map directly (filtering by distance from each entry's own
    // position key) rather than the original's 7x7 TILE scan that looks
    // the registry up only after finding a tile with bit 2 set -- exactly
    // equivalent, since every registered monster's own tile always
    // carries that same bit by construction (RegisterGeneratedSpawns/
    // MonsterRuntime::Move/DungeonRuntime::RemoveMonster all keep the
    // two in sync), so nothing is missed or double-counted either way.
    //
    // A successful Chase step is the FIRST live (in-tick-loop) caller
    // that actually relocates a registered monster -- MonsterRuntime::
    // Move() itself already updates the moved monster's tile bits, but
    // (per its own doc comment) was never able to update a live registry
    // key, since none existed until M22/M24. This method is what finally
    // re-keys the WorldRegistry entry (erase the old position, insert at
    // the new one) after a step actually lands, closing that gap.
    //
    // `spawnIdCounter` (M53) is threaded straight through to
    // CombatResolution::MonsterTick, for its own now-real ailment==2
    // ("curse of hunger") 3-monster spawn -- the SAME Monster.
    // nextSpawnIdCounter substitute camp/camp_tick.h's own
    // TrySpawnMonsterNear call already advances (main.cpp's
    // `nextMonsterSpawnId`), not a second independent one.
    static void TickNearbyMonsters(PlayerState& player, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                                    const CharacterData& charData, const ItemDatabase& items,
                                    const MonsterDatabase& monsterDb, int64_t nowMs, JavaRandom& globalRng,
                                    MessagePopupState& messagePopup, int16_t& spawnIdCounter);
};

}  // namespace dawnstar
