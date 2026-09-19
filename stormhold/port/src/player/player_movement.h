#pragma once
#include <cstdint>
#include <functional>

#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "dungeon/dungeon_runtime.h"
#include "player/player_state.h"
#include "world/dungeon_generator.h"
#include "world/warden.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/Player.java's core movement
// pipeline: computeMoveTarget() (facing-relative position math + cross-
// level boundary stitching, including the hub-town (19x19) <-> standard-
// level (35x35) recentering) + isWalkableTileBits() + commitMove() +
// move()'s turn/step/turn-back strafe wrapper.
//
// **M17: `CommitMove` now wires the dropped-item auto-loot side effect**
// (`world`/`items` params, using M16's `DungeonRuntime`/`WorldRegistry`
// and M12's `PlayerInventory::TryPickUpItem`) -- see `CommitMove`'s own
// declaration comment for the exact behavior, including a real confirmed
// asymmetry between the original's "exactly one item on this tile" and
// "several items on this tile" branches, preserved rather than
// "corrected".
//
// **M19: `CommitMove` now also wires the two side effects M17 had left
// out on purpose** (`monsterDb`/`warden` params): `Shop.wardenPresent`'s
// on-any-STEP clear, and the level-37-entry forced-respawn of the
// type-41 "roaming" monster -- see `CommitMove`'s own declaration
// comment for both, including a real confirmed inconsistency between
// this method's own Warden-clearing path and `WardenState::Leave`'s.
//
// **M25: `CommitMove` now also wires `refreshCorridorView()`** (see
// `RefreshCorridorView` below) -- `refreshNearbyMonsterFlags()` (the
// OTHER rendering-adjacent side effect this class header used to lump in
// alongside it) is still skipped; it touches monster-registry state this
// class doesn't otherwise depend on, a separate follow-up.
class PlayerMovement {
public:
    // Looks up a GeneratedLevel by its 1-based level number. This port has
    // no persistent world-state object yet (M6/M8's own precedent of
    // taking explicit GeneratedLevel arguments rather than inventing one)
    // -- the caller supplies whatever generated/cached levels are needed.
    // Returns a MUTABLE reference: CommitMove marks the target level
    // `visited` on a successful move (see GeneratedLevel::visited's own
    // header comment -- NOT the same concept as GeneratedLevel::populated).
    using LevelLookup = std::function<GeneratedLevel&(int levelNumber)>;

    // Player.computeMoveTarget(dir): pure position/facing math from `p`'s
    // current state. dir: 1=step forward, 2=step backward, 3=turn one way,
    // 4=turn the other way (no position change for a turn).
    //
    // Throws std::runtime_error if a step would cross a map edge with NO
    // neighbor level (a DungeonGeomRow neighbor <= 0). The real Java has
    // NO guard here at all -- it would throw ArrayIndexOutOfBoundsException
    // fetching a negative-indexed ESGame.dungeons[] slot. Believed
    // unreachable in real play (every walkable edge tile should border a
    // real neighbor, by construction of how stairways/corridors are
    // carved toward the level's own confirmed neighbors) but not proven
    // exhaustively across all 37 levels -- surfaced loudly here rather
    // than silently producing a nonsense pending position, same
    // discipline as AssetRoot::OpenFile/RawImage's palette-index guard.
    static void ComputeMoveTarget(PlayerState& p, int dir, const LevelLookup& levels);

    // Player.isWalkableTileBits(tileBits): not a wall (bit 1), not
    // blocked (bit 32, hub-town shop tiles), not monster-occupied (bit 2).
    static bool IsWalkableTileBits(uint8_t tileBits);

    // M25: Player.refreshCorridorView() -- `this.currentDungeon().
    // sampleCorridorView(this.tileX, this.tileY, this.facing,
    // this.corridorView)`, via DungeonRuntime::SampleCorridorView (M21).
    // `level` is the player's CURRENT level (post-move, when called from
    // CommitMove below) -- the caller's job to pass the right one, same
    // "caller supplies/owns world state" pattern this whole module
    // already uses. Wraps `levels` in a const-returning adaptor internally
    // since DungeonRuntime::LevelLookup's own signature returns `const
    // GeneratedLevel&` (read-only lookups only), unlike this class's own
    // mutable-returning LevelLookup alias.
    //
    // Exposed standalone, not just inlined into CommitMove, because
    // Player.java's own refreshCorridorView() has THREE more call sites
    // this port doesn't wire yet -- resetState() (character creation),
    // markCampAndReturnToTown(), warpToCampMark() (Player.java lines
    // 339/2493/2501) -- all real, all deliberately deferred: none of
    // those three port-side equivalents (PlayerCreation::CreateCharacter,
    // PlayerInventory::MarkCampAndReturnToTown/WarpToCampMark) take a
    // LevelLookup today, and threading one through just for this is a
    // separate, smaller follow-up, not a blocker for this milestone's
    // actual goal (a live render pass off CommitMove's own corridorView).
    static void RefreshCorridorView(PlayerState& p, const GeneratedLevel& level, const LevelLookup& levels);

    // Player.commitMove(dir) -- see class header comment for what's
    // deliberately not modeled. `world`/`items` are the M17 addition: once
    // position/facing/fatigue are committed (matching the original's own
    // control flow -- the position update happens BEFORE this block runs,
    // same as `Player.java`'s own commitMove()), if the landed tile
    // carries the dropped-item bit (4):
    //
    //  - exactly one item on the tile (`Dungeon.countDroppedItemsAt()==1`):
    //    if it's locked (record[6] bit 4), sets `p.pendingLockedItemFlag`
    //    and returns true immediately (matching the original's own early
    //    `return true` -- the auto-camp-mark check below never runs in
    //    this case, faithfully). Otherwise tries to pick it up
    //    (`PlayerInventory::TryPickUpItem`); on success, removes it from
    //    `world` and, **only when the item had NOT been possessed before**
    //    (record[6] bit 2 CLEAR) and its category is 11 ("gift"), adds its
    //    subtype value to `p.giftPointsFound`.
    //  - more than one item on the tile: same per-record loop (locked
    //    check first, same early return), BUT the gift-points condition is
    //    the OPPOSITE bit test -- **only when the item HAD been possessed
    //    before** (record[6] bit 2 SET). Confirmed directly from `Player.
    //    java`'s own commitMove(): the single-item branch tests
    //    `(record[6] & 2) == 0`, the multi-item branch tests `(record[6] &
    //    2) != 0` -- a real, asymmetric condition between the two branches
    //    that reads like a copy-paste inversion bug, not something this
    //    port introduced. Preserved exactly, not "fixed" to be consistent.
    //
    // `ESGame.getGameAdvancementLevel()`/`checkOpenAndPopulateDungeons()`
    // (the progressive zone-opening side effect a gift-point gain can
    // trigger) is SKIPPED entirely -- no live `ESGame` session object
    // exists in this port to open zones on, same class of gap as this
    // header's own class comment. `p.giftPointsFound` itself is still
    // accumulated (that part is pure `PlayerState` arithmetic, no `ESGame`
    // needed), only the dungeon-opening side effect is dropped.
    //
    // Also wires `Player.autoMarkCampOnTile()`: on a successful STEP (not
    // a turn) landing on a bit-8 tile, calls
    // `PlayerInventory::MarkCampAndReturnToTown` then immediately clears
    // `justMarkedCamp` right back to false, matching the original's own
    // "mark then immediately un-flag" sequence exactly.
    //
    // M19 additions, both from `Player.java`'s own `commitMove()` body,
    // ported in the exact position they run at relative to everything
    // above:
    //
    //  - Entering level 37 from anywhere else (`p.pendingLevel == 37 &&
    //    p.currentLevel != 37`) -- checked right after the walkability
    //    gate, BEFORE the position/facing/fatigue commit above -- scans
    //    EVERY monster already registered on level 37 in `world` (M18's
    //    `RegisterGeneratedSpawns` is what puts the type-41 "roaming"
    //    monster there in the first place) and, for the one with
    //    `typeIndex == 41`, resets `currentHp` back to the type's own
    //    max-HP column (`MonsterRuntime::Stat(..., 14)`, matching the
    //    original's own `(byte) m.stat(14)` cast exactly -- masking to
    //    `uint8_t` then narrowing back to `int8_t` always reproduces the
    //    same bit pattern `RawStat` would read directly, so this is
    //    equivalent to a full heal) and re-stores it via
    //    `DungeonRuntime::StoreMonster`. A "level 37's boss is always at
    //    full health when you arrive" mechanic.
    //  - On a successful STEP (not a turn), if `warden.present` is true,
    //    clears it straight to false. **Confirmed NOT the same thing as
    //    `WardenState::Leave`:** this is a direct, unconditional
    //    `Shop.wardenPresent = false;` in the original -- no
    //    `wardenLeaves()` call, no tile mutation at all. That means
    //    taking a single step ANYWHERE in the game while the Warden
    //    happens to be visiting silently makes `warden.present` read
    //    false again, while the hub's own tile bit 32 at Varus's
    //    position (set by `WardenState::Arrive`) stays SET until
    //    whatever caller actually drives `WardenState::Leave` on its own
    //    schedule eventually runs (that caller isn't recovered yet --
    //    same class of gap as `Monster.tick()`'s own missing driver).
    //    Reads like the original intends `wardenPresent` as a
    //    lightweight "has the player acted since he arrived" gate
    //    distinct from the tile's own visible state, not a bug this port
    //    should paper over.
    static bool CommitMove(PlayerState& p, int dir, const LevelLookup& levels, WorldRegistry& world,
                            const ItemDatabase& items, const MonsterDatabase& monsterDb, WardenState& warden);

    // Player.move(dir, strafe): dir 1=forward/2=backward for a plain step;
    // when strafe is true, dir 3/4 are strafe-left/right (turn, step, turn
    // back). Unlike dawnstar's own Player.move (which guards the turn-back
    // step with a one-shot `suppressStrafeAdjust` flag -- see dawnstar's
    // own PlayerState comment), Stormhold's version has no such guard at
    // all: confirmed by reading move() directly, the turn-back always runs
    // unconditionally, and its return value OVERWRITES the sideways step's
    // own result (so a strafe move's return value reflects whether the
    // final turn-back succeeded -- which, since turning never fails
    // walkability, is almost always true -- not whether the actual step
    // landed). Preserved exactly; not a transcription gap on this port's
    // side, and dawnstar's own port preserves the identical overwrite
    // shape for its own equivalent method.
    static bool Move(PlayerState& p, int dir, bool strafe, const LevelLookup& levels, WorldRegistry& world,
                      const ItemDatabase& items, const MonsterDatabase& monsterDb, WardenState& warden);
};

}  // namespace stormhold
