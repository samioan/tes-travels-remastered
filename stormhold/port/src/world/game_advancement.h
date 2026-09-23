#pragma once
#include <functional>

#include "world/dungeon_generator.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/ESGame.java's zone-gating
// system: getGameAdvancementLevel()/checkOpenAndPopulateDungeons(),
// confirmed real and confirmed still unmodeled by this port's own
// GeneratedLevel::populated header comment (that field was an
// always-true placeholder until this milestone, since nothing had ever
// flipped it any other way -- see that comment's own history) and by
// player/player_movement.h's/player_inventory.h's own "SKIPPED, no live
// ESGame session object exists yet" comments at every one of the 3 real
// giftPointsFound-increment call sites this class finally gives a target
// to call.
//
// **Confirmed real gate, not a cosmetic flag:** Player.commitMove()
// reads `ESGame.dungeons[pendingLevel-1].populated` directly (src/
// Player.java:809-811) and returns false -- exactly like a wall -- the
// instant it's false, for ANY move, including a same-level step (a
// same-level step's own `target` IS `dungeons[currentLevel-1]`, i.e. the
// level the player already occupies). See player/player_movement.h's own
// CommitMove header comment for the confirmed consequence this has for
// GameSave::Load.
//
// **Deliberately NOT ported: `openAndPopulateAllUpTo()`/
// `enterCurrentZone()`** (ESGame's own "open every zone up to the
// loaded player's advancement" catch-up pair, meant to run right after a
// load). Traced via the exact same "grep every call site" method M51's
// own Shop.reset() finding used: `enterCurrentZone()` (the ONLY caller
// `openAndPopulateAllUpTo()` has) itself has ZERO callers anywhere in
// `src/*.java` -- confirmed dead code, not merely unported. (There IS an
// `enterCurrentZoneStatic()`, called once from ESGame's own static
// initializer -- a same-prefix, otherwise UNRELATED method that only
// calls `buildHubTileTemplate()`, nothing to do with zone-opening at
// all; easy to conflate by name alone, so flagged explicitly here.)
// Building a live zone-catch-up call into GameSave::Load anyway would be
// STRICTLY MORE correct than the shipped game -- the exact "behavioral
// gain, not reimplementation" trap M51's own note already warns future
// work away from -- so this port's own GameSave::Load does not call
// anything from this class either, faithfully reproducing the real,
// confirmed consequence: a fresh-launch Continue Game resumed past zone
// 0 finds every other zone's `populated` back at its freshly-booted
// default (false), and since even a same-level step's own `target` gate
// checks THAT level's own `populated` bit, the player cannot move AT
// ALL, not even in place -- a genuine softlock in the original engine,
// preserved here rather than "fixed".
class GameAdvancement {
public:
    // Same shape as player/player_movement.h's own LevelLookup (declared
    // independently here rather than reused directly -- stormhold_world
    // is the lowest layer in this port's own one-directional dependency
    // graph and can't depend on stormhold_player, same reasoning dungeon/
    // dungeon_runtime.h's own LevelLookup comment already gives for
    // itself; MUTABLE here, unlike that CONST one, since OpenZone needs
    // to write `populated`).
    using LevelLookup = std::function<GeneratedLevel&(int levelNumber)>;

    // ESGame.getGameAdvancementLevel(giftPoints): buckets total gift
    // points collected into a 0-8 zone index. Confirmed name from the
    // original's own debug println.
    static int Level(int giftPoints);

    // ESGame.checkOpenAndPopulateDungeons(gameAdvLevel): opens (marks
    // `populated`) every level belonging to zone `gameAdvLevel` ONLY --
    // NOT every zone up to it (that's the OpenUpTo-shaped variant this
    // class deliberately doesn't implement, see this class's own header
    // comment). Called every time giftPointsFound crosses a new
    // advancement bucket during live play (3 real call sites: Player.
    // commitMove()'s single-item/multi-item dropped-item pickup branches,
    // and Player.collectChestItem()) plus once at new-game creation for
    // zone 0. A real, confirmed asymmetry follows from opening only the
    // CURRENT zone: a single pickup whose giftPoints jump skips an entire
    // zone bucket (e.g. bucket 0 straight to bucket 2) leaves the SKIPPED
    // zone locked for the rest of the session, since nothing else ever
    // revisits it. Preserved, not "fixed" -- same discipline every
    // milestone in this port holds to. This port's own BuildWorld already
    // eagerly generates/registers every level's tiles/spawns up front
    // (unlike the original's lazy per-level construction, see
    // GeneratedLevel::populated's own header comment), so there's no
    // `dungeons[idx].populate()` call to reproduce here -- opening a zone
    // is just the `populated` flip itself.
    static void OpenZone(int zone, const LevelLookup& levels);
};

}  // namespace stormhold
