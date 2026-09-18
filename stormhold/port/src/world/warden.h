#pragma once
#include "world/dungeon_generator.h"

namespace stormhold {

// Stormhold-only NPC world-event, no dawnstar equivalent at all: Varus
// (Shop.java's shop index 6, `Shop.NAMES[6]`) is tied to a "Warden" that
// periodically visits/leaves the hub town, flipping dungeon tile bit 32 at
// his own world position (`Shop.SHOP_X[6]`/`SHOP_Y[6]` = (9,9)). Confirmed
// directly from `../../../src/Shop.java`'s `wardenArrives()`/
// `wardenLeaves()`/`shouldWardenVisit()`/`wardenVisitCount`/`wardenPresent`.
//
// Deliberately scoped to just this world-event state machine, not the rest
// of Shop.java's dialogue() dispatcher (the 7-NPC roster, quest-turn-in
// economy, Beneca/Helga's bespoke branches) -- that needs a live Player/
// Item-inventory/dungeon-tile-mutation model this port doesn't have yet
// (see docs/PORT_ROADMAP.md's "what's next" -- player state is its own
// later milestone).
class WardenState {
public:
    // Shop.SHOP_X[6]/SHOP_Y[6] -- Varus's fixed hub-town position.
    static constexpr int kShopX = 9;
    static constexpr int kShopY = 9;

    int visitCount = 0;
    bool present = false;

    // Shop.shouldWardenVisit(elapsedCounter): true once `elapsedCounter`
    // (exact unit/source unconfirmed in the original -- Shop.java's own
    // header comment calls it "some kind of playtime/turn counter", passed
    // in by an unconfirmed external caller) crosses the next escalating
    // threshold (13/26/39) for the CURRENT visitCount, provided the Warden
    // isn't already present. Matches the original's own if/else chain
    // exactly: there's no 4th threshold, so once visitCount reaches 3 this
    // always returns false regardless of elapsedCounter, same as the Java.
    bool ShouldVisit(int elapsedCounter) const;

    // Shop.wardenArrives(): flips bit 32 on at (kShopX, kShopY) on the HUB
    // level's own tile grid, increments visitCount, sets present.
    void Arrive(GeneratedLevel& hub);

    // Shop.wardenLeaves() -- **preserves a real, confirmed bug rather than
    // "fixing" it, same documentation discipline as M6's dead chest-record
    // byte.** The original reads the CURRENT tile byte from
    // `ESGame.dungeons[1]` (level INDEX 1 = level NUMBER 2, a real
    // procedurally-generated dungeon, NOT the hub) at Varus's position,
    // clears bit 32 on THAT value, then writes the result into
    // `ESGame.dungeons[0]` (the hub)'s tile at the same position --
    // instead of reading/clearing/writing the hub's own existing tile like
    // wardenArrives() does. Near-certainly a copy-paste index bug
    // (`dungeons[1]` should read `dungeons[0]`): the real effect is that
    // "the Warden leaving" doesn't restore the hub tile's actual prior
    // value at all -- it overwrites it with whatever level 2 happens to
    // have generated at the same (x, y) coordinate, with bit 32 cleared.
    // `level2` here is level NUMBER 2's own generated tiles (`PopulateLevel
    // (2, ...)`'s output), matching that real indexing exactly.
    void Leave(GeneratedLevel& hub, const GeneratedLevel& level2);
};

}  // namespace stormhold
