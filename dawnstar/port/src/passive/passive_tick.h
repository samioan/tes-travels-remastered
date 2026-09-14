#pragma once
#include <cstdint>
#include <vector>

#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "dungeon/dungeon_runtime.h"
#include "player/player_state.h"
#include "render/message_popup.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/GameCanvas.java's run()-tail
// timed systems (lines ~1407-1415): tickStatusCountdowns(elapsed), the
// per-iteration millisecond countdown of Player's 3 timed status-ailment
// timers, and tickPerSecond(), the once-per-accumulated-second passive
// tick -- HP drain/regen, the effectDurations[] countdown, and the
// scripted "overstayed in one place" ambush spawner M43's Reveal Traitor
// result arms (the only assignment of ambushTimer in the whole game).
// Its own module (the same "own small module, no cycle" shape as
// combat/interact/camp) because it needs dawnstar_player
// (PlayerCombatStats::HasAilment, PlayerInventory) + dawnstar_dungeon
// (DungeonRuntime::TrySpawnMonsterNear) + dawnstar_render
// (MessagePopup) all at once -- see docs/PORT_ROADMAP.md's M44 entry.
class PassiveTick {
public:
    // tickStatusCountdowns(elapsed), GameCanvas.java lines ~1805-1832:
    // counts down trollThirstTimer/glacierCurseTimer/terrifiedTimer while
    // their own ailments are active (hasAilment(4)/(5)/(7), bits 3/4/6),
    // zeroing each on expiry. A REAL, preserved finding: the original's
    // expiry arms -- `ailmentMask = Util.setBit(3/4/6, ailmentMask)` --
    // write the SAME bits their own hasAilment(4/5/7) branch conditions
    // already require (Util.setBit is 0-based, hasAilment(n) reads bit
    // n-1), so they are idempotent no-ops: these three ailments never
    // expire on their own, and the countdown's only real effect is
    // zeroing the timer. Ported exactly as written (the redundant bit-OR
    // included) rather than "cleaned up".
    //
    // `monsterAttacking` is GameCanvas's own static (the render-time
    // VisibleObjects::AnyMonsterAttacking value main.cpp already keeps,
    // same one-tick lag as the original's own repaint-then-countdown
    // ordering): bit 6 (Terrified) only counts down while it's true.
    static void TickStatusCountdowns(PlayerState& player, int64_t elapsedMs, bool monsterAttacking);

    enum class PerSecondResult {
        // No ambush checkpoint hit this second, or one hit and the level
        // held <= 5 monsters afterwards (the "Enemy/arrived!" popup is
        // shown inside either way).
        None,
        // A checkpoint monster spawned and the level now holds > 5
        // monsters: `this.game.endOfGameUI = this.game.newGameOverUI();
        // this.game.setCurrentDisplay(this.game.endOfGameUI);` -- the
        // caller (main.cpp) builds and shows the real "Game Over" screen.
        EndOfGame,
    };

    // tickPerSecond(), GameCanvas.java lines ~1840-2003, transcribed
    // block by block: (1) hasAilment(4) (Troll Thirst) drains HP by
    // max(2*maxHP/100, 0) per second -- NOT clamped at 0 below, exactly
    // as the original (HP can go negative; the death system handles
    // that); (2) hasAilment(5) (Glacier Curse) adds maxMagicka/10
    // Magicka per second, and on reaching maxMagicka RESETS current
    // Magicka to 0 and drains HP by maxMagicka/10 -- a real quirk
    // preserved as found (the "regen" overflows into a reset instead of
    // clamping); (3) the effectDurations[] per-second countdown, whose
    // only special case is index 5 (effect 6, "Safe Camping"): expiring
    // it removes the equipped item-101 StarFrost from the inventory via
    // PlayerInventory::FindSlotOf/RemoveSlot (which itself only matches
    // the EQUIPPED form -- see FindSlotOf's own header doc comment);
    // (4) the per-level monster scratch-cooldown block, a real NO-OP in
    // the original (see the .cpp's own comment) -- documented, nothing
    // observable to port; (5) the ambush spawner.
    //
    // The ambush spawner: while ambushTimer >= 0 (armed ONLY by the
    // Reveal Traitor result's Ok, M43), each call increments it and
    // checks the elapsed-second checkpoint tables -- two entirely
    // different schedules selected by newGamePlus (the correct-guess
    // flag), with elapsed 140 always spawning the type-42 end-game
    // monster on top of either schedule. The `spawnType` values are
    // TIER arguments for the same forcedTypeOrSentinel parameter M22
    // already traced (only 41/42 are literal forced types; every other
    // positive value picks a random monster appropriate for that
    // difficulty tier), NOT literal monster types. The spawn position is
    // 1+Util.randomInt(17) -- LingoRandomInt, 1-based inclusive, so
    // really 2..18 -- re-rolled in the original's exact X-then-Y order
    // until trySpawnMonsterNear succeeds. Then: >5 monsters on the
    // level (including the newcomer) -> EndOfGame; else the
    // MSG_ENEMY_ARRIVED {"Enemy", "arrived!"} popup at priority 3.
    static PerSecondResult TickPerSecond(PlayerState& player, std::vector<GeneratedLevel>& levels,
                                         WorldRegistry& world, const ItemDatabase& items,
                                         const MonsterDatabase& monsterDb, JavaRandom& globalRng,
                                         int16_t& nextMonsterSpawnId, int64_t nowMs,
                                         MessagePopupState& messagePopup);
};

}  // namespace dawnstar
