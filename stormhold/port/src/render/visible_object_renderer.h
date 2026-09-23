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

    // M67: GameCanvas.paintUnknown_b() (was decompiled/e.java's
    // b(Graphics,int)) -- the NPC/shop-portrait icon overhead the
    // look-ahead tile, shown while a quest-turn-in shop (0-5) with an
    // unclaimed reward, or Varus/the Warden (6), sits directly ahead of
    // the player. `stat` is `Player.questShopAtPendingTile()`'s return
    // value, which -- now that phase-3 port M67 resolved the LOW
    // CONFIDENCE flag on that method (see ../../../docs/PORT_ROADMAP.md's
    // "what's next") -- is exactly PlayerMovement::ShopAheadOfPlayer's own
    // return value; callers should NOT call this when that's < 0, mirroring
    // the original's `if (unconfirmed_W) { ... }` gate (itself already
    // confirmed equivalent to `shopAhead >= 0`, M60). Cases 0-5 reuse
    // RenderMonsterOrIconSprite() with small literal indices instead of
    // real monster typeIndexes -- an NPC/shop-portrait sprite sheet
    // apparently laid out in the same row-index space as the monster
    // table, not independently confirmed beyond both falling in valid
    // table ranges (same caveat the original method's own header comment
    // already carried). Case 6 shows the Warden compass icon instead,
    // scaled by how many times the Warden has visited (`wardenVisitCount`,
    // capped at 3, matching RenderWardenCompassIcon's own tier parameter).
    static void RenderUnknownB(Backbuffer& bb, const VisibleObjectAssets& assets, int stat, int wardenVisitCount);

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
