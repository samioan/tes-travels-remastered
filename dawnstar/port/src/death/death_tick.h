#pragma once
#include <cstdint>
#include <vector>

#include "assets/item_database.h"
#include "dungeon/dungeon_runtime.h"
#include "npc/shop_interaction.h"
#include "player/player_state.h"
#include "render/message_popup.h"
#include "world/dungeon_generator.h"

namespace dawnstar {

// Renamed-source counterpart of GameCanvas's death/respawn system:
// processIdleTick()'s own `hp <= 0` check (which only sets
// `deathState = 2` -- that half stays in main.cpp, right next to
// `dispatchTickActions`, same spot the original calls it from) and
// run()'s own per-tick `deathState != 1` state machine, the sibling of
// campState's own (camp/camp_tick.h) sitting right after it in the
// original's if/else-if chain -- see docs/PORT_ROADMAP.md's M50 entry.
// Kept in its own module for the same reason camp/camp_tick.h and
// interact/interact_tick.h are: it needs dawnstar_player
// (PlayerInventory::IsEquipped/RemoveSlot, PlayerMovement::ResetState),
// dawnstar_render (the message popup) AND dawnstar_npc (ShopState::
// showDeathGreeting) all at once.
class DeathTick {
public:
    // GameCanvas.run()'s own per-tick `deathState != 1` branch --
    // returns whether the REST of this tick's normal per-tick work
    // should run at all, exactly like camp/camp_tick.h's own
    // TickCampState. Only meaningful to call when campState == 0 THIS
    // tick, BEFORE this call -- matching the original's if/else-if
    // priority (campState 1/2/3 takes over the tick outright; deathState
    // is never even examined that tick) -- so main.cpp only calls this
    // when TickCampState's own campState check found nothing to do; see
    // that call site's own doc comment.
    //
    // While `deathState == 1` (alive), this is a pure no-op returning
    // true, matching the original's own "neither elseif branch matched"
    // fallthrough. Once `deathState != 1`: the first tick after death
    // flips deathState 2 -> 3 and clears the message popup outright
    // (`messageVisible = false; messagePriority = 0`, not just hidden by
    // timeout); every tick after that returns false (freezing input,
    // same as camping) until 5 real seconds have passed since the death
    // began, at which point it resolves: `normalizeForSummary` applied
    // directly to the live coreStats (current = max HP/Magicka/Fatigue,
    // coreStats[8] zeroed), every unequipped inventory slot dropped
    // (equipped gear survives death), `starFrostBonusActive` cleared,
    // `PlayerMovement::ResetState(true, ...)` (ailments/timers cleared,
    // repositioned to the hub's alt spawn point), `shop.showDeathGreeting`
    // set so the next NPC talked to offers the "you're back" line
    // (npc/shop_interaction.cpp's own `showDeathGreeting` branches),
    // `player.minimapDirty` raised, and the new level's name shown as a
    // popup -- then returns true so the rest of the tick (movement input
    // etc.) resumes right away, also setting `suppressMoveThisTick`
    // (same GameCanvas.suppressMoveInput-for-one-tick reasoning as
    // TickCampState's own out-parameter) so a held direction key doesn't
    // immediately walk the freshly-respawned player forward.
    static bool TickDeathState(PlayerState& player, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                                const ItemDatabase& items, ShopState& shop, MessagePopupState& messagePopup,
                                int64_t& deathTimeMs, int64_t nowMs, bool& suppressMoveThisTick);
};

}  // namespace dawnstar
