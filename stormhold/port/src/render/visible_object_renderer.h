#pragma once
#include "graphics/backbuffer.h"
#include "player/player_state.h"
#include "render/visible_object_assets.h"

namespace stormhold {

// M28: renamed-source counterpart of GameCanvas.paintObjects()/
// paintMonsters() (../../../src/GameCanvas.java, M22) -- the sprite
// drawing that consumes M27's now-populated PlayerState::visibleObjects,
// the natural "selection logic done, now draw it" pairing (M21->M25,
// now M27->M28).
//
// Both methods share the same zone-priority draw order this port
// reproduces exactly: far (slots 8-12), then mid (4-6), then near (1) --
// nearer sprites paint OVER farther ones wherever their fixed screen
// rects overlap, matching the original's own statement order.
class VisibleObjectRenderer {
public:
    // GameCanvas.paintObjects() (was decompiled/e.java's b(Graphics)) --
    // draws visible chests (VisibleSlotKind::Chest)/dropped items
    // (VisibleSlotKind::DroppedItem) only; monsters/Warden are
    // RenderMonsters' own job.
    static void RenderObjects(Backbuffer& bb, const VisibleObjectAssets& assets, const PlayerState& p);

    // GameCanvas.paintMonsters() (was decompiled/e.java's g(Graphics)).
    // Returns whether any MONSTER sprite was drawn (GameCanvas's own
    // `unconfirmed_A`, reset false at the top of the original method
    // every call and set true only by a real monster render, NEVER by a
    // Warden one -- confirmed by reading the whole method; no confirmed
    // reader anywhere in ../../../src/ yet, exposed as a return value
    // here rather than inventing mutable static state for it).
    //
    // Does NOT separately re-check a monster's own "seen" flag
    // (Player.java's `record[6] != 0`, the original's own way of telling
    // a real monster record apart from an Integer sentinel/String in the
    // same untyped Vector slot) -- this port's `VisibleSlotKind::Monster`
    // is ALREADY exactly that same guarantee, by construction:
    // `VisibleObjects::Refresh` (M27) only ever tags a slot `Monster`
    // in the same call where it also overwrites that monster's
    // `unconfirmedFlag` true, so the two conditions can never actually
    // disagree here.
    static bool RenderMonsters(Backbuffer& bb, const VisibleObjectAssets& assets, const PlayerState& p);
};

}  // namespace stormhold
