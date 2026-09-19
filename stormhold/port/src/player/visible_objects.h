#pragma once
#include "dungeon/dungeon_runtime.h"
#include "player/player_state.h"
#include "world/warden.h"

namespace stormhold {

// M27: renamed-source counterpart of ../../../src/Player.java's
// visibleObjects population pipeline -- refreshVisibleObjectSlots()/
// refreshVisibleObjects()/resolveVisibleObjectSlot()/placeVisibleObject()/
// facingAxisDistance() -- the system GameCanvas.paintObjects()/
// paintMonsters() (M22) paint from directly. Deliberately the DATA-MODEL
// half only, mirroring the M21/M25 split (M21 = corridor wall-segment
// SELECTION, M25 = actually drawing it): this produces a correctly-
// populated PlayerState::visibleObjects, with no pixel drawn anywhere --
// same scoping dawnstar's own equivalent M25 milestone already used for
// its own identical system.
//
// **Still no confirmed caller anywhere in ../../../src/:** unlike this
// class's own header comment's claim ("rebuilt every move"),
// `refreshVisibleObjects()` has NO call site in the currently-transcribed
// source at all -- same class of gap as the still-untranscribed
// tick-loop helpers (`tickMovementAndAI` etc, flagged repeatedly since
// M14/M15/M22). `PlayerMovement::CommitMove` does NOT call this port's
// `Refresh()` either, for the same reason. A future milestone wiring a
// real game loop will need to call it after every move, same as it will
// need to call `PlayerMovement::RefreshCorridorView` today.
class VisibleObjects {
public:
    // Java's Integer.MAX_VALUE, Player.facingAxisDistance()'s own
    // "not visible" sentinel for a negative-along-facing-axis distance.
    static constexpr int kNotVisible = 0x7FFFFFFF;

    // Player.facingAxisDistance(): distance from `p` to (tx, ty) along
    // p's own facing axis, or kNotVisible when that distance is
    // negative. Takes the target's tile position directly rather than a
    // polymorphic record/length-sniffed object the way the original
    // does -- Refresh() below already knows which record kind it's
    // looking at per call site, so there's nothing to dynamically
    // dispatch on in C++.
    //
    // **A real, confirmed dead computation, exposed anyway:**
    // Player.placeVisibleObjectIfSlotFree() computes this distance and
    // appears to branch on it (`kind != 4 && dist != 1`) -- but reading
    // the whole method shows BOTH branches of that if/else call
    // placeVisibleObject() and return true identically, so neither this
    // distance nor that branch has ANY effect on behavior. Not
    // reproduced as a branch anywhere in Refresh() below (there is
    // nothing to branch on), but kept standalone and documented so a
    // future pass re-deriving this can still see exactly what the
    // original computed here without needing to re-read Player.java.
    static int FacingAxisDistance(const PlayerState& p, int tx, int ty);

    // Player.resolveVisibleObjectSlot(): resolves (ox, oy) -- via
    // DungeonRuntime::RelativeViewOffset(p.tileX, p.tileY, p.facing, ox,
    // oy) -- against the fixed position/occlusion cascade into one of
    // the 13 visibleObjects slots. Returns the slot index (0-12) on
    // success, -1 when no position matches or the resolved slot is
    // occlusion-guarded by an already-Blocked/Shadowed neighbor. Does
    // NOT itself write `p.visibleObjects` -- Refresh() below does that
    // once it also knows which record (or the Warden marker) to store.
    static int ResolveSlot(const PlayerState& p, int ox, int oy);

    // Player.refreshVisibleObjectSlots(): resets all 13 slots to Empty,
    // marks each of the 13 fixed corridorView-relative positions
    // Blocked when a wall occludes it (bit 1 of
    // DungeonRuntime::ViewGridAt(p.corridorView, dx, dy)), then cascades
    // a fixed set of Shadowed implications. Ported as a literal
    // sequential statement list, not a fixed-point loop: later checks in
    // the original DO observe earlier checks' own Shadowed writes (e.g.
    // slot 9's own check can only ever trigger because slot 5's earlier
    // cascade already shadowed it), and this preserves that exact
    // left-to-right read-after-write ordering.
    static void RefreshSlots(PlayerState& p);

    // Player.refreshVisibleObjects(includeWarden): RefreshSlots(p), then
    // places every dropped item/chest/monster on `p`'s current level
    // (world's levelIndex = p.currentLevel - 1) into a slot when one
    // resolves, plus the Warden (Warden-kind slot, no record) when
    // `p.currentLevel == 1 && warden.present`.
    //
    // **`includeWarden` is a real, confirmed dead parameter, kept for
    // signature fidelity anyway:** the Warden placement above reads
    // `Shop.wardenPresent` directly, never `includeWarden`; the ONLY
    // other place the original threads it through is
    // placeVisibleObjectIfSlotFree's own third parameter -- which that
    // method's own source literally names `unused`, and which
    // FacingAxisDistance's own header comment above already confirms
    // has no effect either way. So this parameter's value never changes
    // this method's behavior, in this port or the original -- confirmed
    // independent of what any (currently unknown, see this class's own
    // header comment) real caller passes.
    //
    // Placing a monster or dropped item (NOT a chest -- Player.
    // placeVisibleObject()'s own switch has no `case 4`) ALSO writes
    // back into `world`: a monster's `unconfirmedFlag` is set
    // permanently true (Player.java's own `data[6] = 1`, an overwrite --
    // see GameCanvas.java's own header comment on why this really means
    // "has ever been seen", not "alive/renderable"), and a dropped
    // item's record[6] bit 0 is OR'd in -- both persisted straight back
    // into `world`'s own stored record (an explicit re-store, since this
    // port's WorldRegistry holds independent copies rather than the
    // original's aliased Java array references).
    static void Refresh(PlayerState& p, WorldRegistry& world, bool includeWarden, const WardenState& warden);
};

}  // namespace stormhold
