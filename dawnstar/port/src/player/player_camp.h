#pragma once
#include <vector>

#include "assets/item_database.h"
#include "dungeon/dungeon_runtime.h"
#include "player/player_state.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Player.java's rest(). Lives
// alongside player/player_movement.h's own WorldRegistry-touching
// methods (this needs `world`/`levels` too, for the roaming-monster
// cleanup below) rather than player/player_combat_stats.h, which has no
// such dependency.
class PlayerCamp {
public:
    // Player.rest(fullRest): restores HP/Magicka/Fatigue by the full
    // missing amount (fullRest) or 2/3 of it otherwise, further scaled
    // to 3/4 if ailment 8 ("Exhaustion") is active (the two scalings
    // compose exactly as the original's sequential reassignment does,
    // not as a single combined fraction); zeroes both level-exp
    // counters (coreStats[8]/[9]); clears the harm/armor/safe-camping
    // buff flags; a 10% chance to consume one "Safe Camping" scroll
    // (itemId 96) if the player is carrying one; and an independent 25%
    // roll PER non-4/5 ailment bit to clear it (ailments 4/"Vampirism"
    // and 5/"Lycanthropy" are permanent conditions, never rolled away by
    // resting -- matching the original's own `num != 4 && num != 5`
    // skip).
    //
    // Also runs Player.java's own roaming-special-monster cleanup this
    // method opens with -- see PlayerMovement::CleanupRoamingMonsterIfPresent's
    // own doc comment for why that's shared code, not duplicated here.
    static void Rest(PlayerState& p, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                      const ItemDatabase& items, JavaRandom& globalRng, bool fullRest);
};

}  // namespace dawnstar
