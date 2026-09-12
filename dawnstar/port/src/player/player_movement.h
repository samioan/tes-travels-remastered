#pragma once
#include <array>
#include <cstdint>
#include <vector>

#include "assets/item_database.h"
#include "dungeon/dungeon_runtime.h"
#include "player/player_state.h"
#include "world/dungeon_generator.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Player.java's movement and
// hub/camp positioning: computeMoveTarget()/commitMove()/move()/
// isWalkable()/hasCampMark()/resetToHubPosition()/
// markCampAndReturnToTown()/warpToCampMark() (the latter four added in
// M18 for combat/combat_resolution.h's UseItem -- see
// docs/PORT_ROADMAP.md). `levels` mirrors ESGame.dungeons[] -- one
// GeneratedLevel per dungeon level, indexed by levelNumber-1 (see
// world/dungeon_generator.h). `world` (M22's dungeon/dungeon_runtime.h)
// is the live per-level monster/dropped-item registry Move()/
// CommitMove() now actually read and write -- this is the third module
// (after combat/ and dungeon/ itself) that needs two of the existing
// sibling modules at once, here player + dungeon (which itself already
// depends on world + monster); no cycle results, since neither world/
// nor monster/ depends back on player/.
//
// SIMPLIFIED versus the original (each is a real behavioral gap, not
// just an implementation detail -- see docs/PORT_ROADMAP.md's M13/M23
// entries):
//  - Every level in `levels` is treated as always populated
//    (Dungeon.java's lazy per-level population flag has no equivalent
//    here -- this port always generates every level it holds upfront).
//  - computeMoveTarget's own out-of-bounds neighbor-level lookup (used
//    to recenter the coordinate perpendicular to the crossed edge) is
//    NOT guarded in the original against a <=0 neighbor -- it relies on
//    the implicit level-design invariant that no walkable tile ever
//    borders a "no neighbor" edge, and would throw
//    ArrayIndexOutOfBoundsException in Java if that invariant were ever
//    violated. This port guards it anyway (skipping the recentering it
//    would have computed) since C++ has no equivalent safety net for an
//    out-of-bounds vector index; CommitMove's own `level <= 0` check
//    still rejects the move either way, exactly as the original does.
//  - MarkCampAndReturnToTown/ResetToHubPosition/WarpToCampMark (M18)
//    still skip the chest/NPC-visibility refresh calls (rendering, not
//    ported) -- unrelated to M23's registry wiring below.
//
// M23 closed the three gaps M13/M18 had left open pending a live
// registry: CommitMove's dropped-item auto-loot-on-arrival now really
// runs (PlayerInventory::AddItem into the first free slot, removing the
// looted record from `world` and clearing the tile's presence bit once
// every record there is gone); CommitMove's instant-lethal-tile (bit 8)
// case now really calls MarkCampAndReturnToTown(false) instead of only
// committing position/facing; and ComputeMoveTarget's "remove roaming
// gehen on level change" cleanup now really searches the LEAVING
// level's live monster registry for type 41 and removes it via
// DungeonRuntime::RemoveMonster, matching Player.java's own use of
// `this.currentLevel` (the old level, not the new one) there.
class PlayerMovement {
public:
    // direction: 1=forward, 2=backward, 3=turn right, 4=turn left.
    // `strafe`+3/4 sidesteps (turn, step, turn back) instead of turning
    // in place. Returns whether anything actually committed (a
    // successful step OR turn). `world`/`items` are M23's addition, for
    // CommitMove's now-real dropped-item auto-loot and
    // ComputeMoveTarget's now-real roaming-monster cleanup below.
    static bool Move(PlayerState& p, int direction, bool strafe, std::vector<GeneratedLevel>& levels,
                      WorldRegistry& world, const ItemDatabase& items);

    // Wall(bit0)/blocked(bit5)/monster(bit1) test for a move target tile.
    static bool IsWalkable(uint8_t tileBits);

    // Player.java's hasCampMark(): whether markCampAndReturnToTown has
    // ever bookmarked a camp point.
    static bool HasCampMark(const PlayerState& p) { return p.campLevel > 0; }

    // Player.java's resetToHubPosition(altSpawn): repositions to one of
    // two fixed level-1 entry points (the normal spawn, or the
    // "returning from camp" alt-spawn just inside the hub's door),
    // cleans up the type-41 "roaming" special monster on the level
    // being left if one is still tracked as alive (M23 -- shares
    // CleanupRoamingMonsterIfPresent with ComputeMoveTarget below), and
    // refreshes the corridor view. SIMPLIFIED: skips the chest/NPC-
    // visibility refresh calls (rendering, not ported).
    static void ResetToHubPosition(PlayerState& p, bool altSpawn, std::vector<GeneratedLevel>& levels,
                                    WorldRegistry& world);

    // Player.java's markCampAndReturnToTown(skipMark): bookmarks the
    // current position (unless skipMark, a path never actually
    // exercised in the original either -- always called with false) and
    // returns to the hub via ResetToHubPosition(true).
    static void MarkCampAndReturnToTown(PlayerState& p, bool skipMark, std::vector<GeneratedLevel>& levels,
                                         WorldRegistry& world);

    // Player.java's warpToCampMark(): warps to the bookmarked camp
    // point. No roaming-monster cleanup in the original here (only
    // ResetToHubPosition/ComputeMoveTarget have it). SIMPLIFIED: skips
    // the chest/NPC-visibility refresh calls, same as ResetToHubPosition.
    static void WarpToCampMark(PlayerState& p, const std::vector<GeneratedLevel>& levels);

    // Player.java's npcInFront(): the shop id (5-8 for the named
    // shopkeepers on levels 3/12/21/30, or Shop.hubShopAt's lookup in
    // the hub town) of the NPC that a forward step would land on, or
    // -1. Re-derives the look-ahead tile via ComputeMoveTarget(1,...)
    // exactly like the original -- including its same real,
    // faithfully-preserved quirk: since RefreshNpcInSight (below) is
    // only ever called right after a move has already committed, this
    // recomputes a move target from the ALREADY-NEW position, which can
    // in principle cross yet another level boundary and re-trigger
    // CleanupRoamingMonsterIfPresent. Harmless in practice: that cleanup
    // is idempotent (guarded by p.roamingSpecialMonsterPresent, already
    // cleared by the real move's own call if it fired), so calling it
    // twice for one tick changes nothing -- ported as-is rather than
    // "fixed" into a single call, matching the original's own shape.
    //
    // Shop.SHOP_X/Y[5..8] (overwritten by DungeonGenerator with each
    // named shopkeeper's real generated position) has no static
    // equivalent here -- read directly from
    // GeneratedLevel::specialShopX/Y instead, exactly as
    // player/visible_objects.cpp's own NPC-tagging already does.
    static int NpcInFront(PlayerState& p, std::vector<GeneratedLevel>& levels, WorldRegistry& world);

    // Player.java's refreshNpcInSight(): sets p.npcInSight from the tile
    // directly in front (tile bit 5/32, the "blocked marker" shop-room
    // tiles carry) via NpcInFront above. SIMPLIFIED: the original also
    // shows the shop's greeting in the message popup the moment it comes
    // into sight (showMessage/messagePriority) and prints a console
    // diagnostic on the "tile says NPC but npcInFront() disagrees" case
    // -- neither is ported (no message-popup system exists yet, see
    // docs/PORT_ROADMAP.md's flagged hotbar/message-popup/minimap
    // entry; no stdout channel is used by any other module either, see
    // CleanupRoamingMonsterIfPresent's own doc comment above).
    static void RefreshNpcInSight(PlayerState& p, std::vector<GeneratedLevel>& levels, WorldRegistry& world);

    // Player.java's chestInFront(): the registered chest record at the
    // tile a forward step would land on, or nullptr -- same
    // ComputeMoveTarget(1,...) re-derivation (and the same real,
    // harmless double-cleanup quirk) as NpcInFront above. M30:
    // GameCanvas.refreshChestInSight()'s own "chest != null" half lives
    // in main.cpp instead of a RefreshChestInSight method here, since
    // the other half (showMessage(MSG_CHEST,1)) needs
    // render/message_popup.h, which dawnstar_player cannot depend on
    // without cycling back through dawnstar_render.
    static const std::array<uint8_t, 8>* ChestInFront(PlayerState& p, std::vector<GeneratedLevel>& levels,
                                                        WorldRegistry& world);

    // Player.java's nearestAttackableMonster(): the registered monster
    // record at the tile a forward step would land on, or nullptr --
    // same ComputeMoveTarget(1,...) re-derivation (and the same real,
    // harmless double-cleanup quirk) as ChestInFront/NpcInFront above.
    // SIMPLIFIED, but not lossy: the original decodes a NEW Monster
    // object from the record on every call (a value snapshot it then
    // holds as GameCanvas.targetMonster across a tick); this port's
    // WorldRegistry stores the raw bytes directly, so there's no
    // separate "live Monster object" layer to snapshot -- this just
    // returns a pointer straight into the live record, and M32's
    // main.cpp orchestration decodes/mutates/re-encodes it as needed.
    // Non-const (unlike ChestInFront's own return) since combat needs to
    // write the mutated record back in place.
    static std::array<uint8_t, 28>* MonsterInFront(PlayerState& p, std::vector<GeneratedLevel>& levels,
                                                     WorldRegistry& world);

private:
    struct PendingMove {
        int level = 0;
        int tileX = 0;
        int tileY = 0;
        int facing = 0;
        bool levelChanged = false;
    };

    // Shared by ComputeMoveTarget and ResetToHubPosition (both of
    // Player.java's own real call sites for this exact block): if
    // p.roamingSpecialMonsterPresent, searches `world`'s registry for
    // the LEAVING level (p.currentLevel, read before either caller
    // updates it) for a type-41 monster and removes it via
    // DungeonRuntime::RemoveMonster, clearing the flag. The original's
    // "Remove roaming gehen failed" console message on a flag left set
    // isn't ported (no stdout channel any other module uses for this).
    static void CleanupRoamingMonsterIfPresent(PlayerState& p, std::vector<GeneratedLevel>& levels,
                                                WorldRegistry& world);

    static PendingMove ComputeMoveTarget(PlayerState& p, int direction, std::vector<GeneratedLevel>& levels,
                                          WorldRegistry& world);
    // `outLevelChanged` mirrors Player.java's `this.levelChanged` field
    // -- move()'s strafe handling reads and restores it around the
    // turn-back step, so it has to survive across sequential
    // CommitMove calls within one Move() rather than being purely local.
    static bool CommitMove(PlayerState& p, int direction, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                            const ItemDatabase& items, bool& outLevelChanged);
    static void RefreshCorridorView(PlayerState& p, const std::vector<GeneratedLevel>& levels);
};

}  // namespace dawnstar
