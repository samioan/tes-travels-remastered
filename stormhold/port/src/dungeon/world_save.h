#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

#include "dungeon/dungeon_runtime.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/ESGame.java's
// writeAllLevelRegistries(RecordStore)/readPerLevelRecords(RecordStore,
// firstRecordId) -- the monsters[]/chests[]/droppedItems[] half of the
// real "RecordStore-based save format" (ESGame.java's own header comment),
// the piece docs/PORT_ROADMAP.md's "what's next" flagged as still missing
// after M20's PlayerSave. `MonsterRuntime::ReadFrom`/`WriteTo` (monster/
// monster_runtime.h) and `BinaryReader`/`BinaryWriter` (both M20) already
// exist and do all the real per-record work here -- this module is just
// the "count, then N records, per level" framing loop around them, plus
// the same loop inlined directly for chests/dropped-items (which have no
// live object to round-trip through, just raw byte records already).
//
// **Deliberately NOT ported here:** `writeMasterLists()`/
// `readMasterLists()` (Item.nextSpawnId/Monster.nextSpawnIdCounter plus
// most of Shop's own static per-shop quest-economy state). **M53/M55
// update:** `ShopState` (world/shop_state.h) now exists, and `Shop::
// WriteTo`/`ReadFrom` there plus `WardenState::WriteTo`/`ReadFrom`
// (world/warden.h) cover everything this port models of that record --
// wired into the actual save file by `player/game_save.h`'s own
// `GameSave::Save`/`Load`, not here (this module stays scoped to the
// per-level registries alone). `Item.nextSpawnId`/
// `Monster.nextSpawnIdCounter` are still NOT covered -- see game_save.h's
// own header comment for why (this port has never modeled either as a
// single persistent global counter). And the RecordStore-equivalent
// file I/O itself (multi-save-slot naming/discovery/deletion,
// `generateUniqueSaveName`/`findMostRecentSaveName`/`deleteOtherSaves`) --
// this port needs its own plain-file save mechanism, not a MIDP
// RecordStore, same "behavioral reimplementation, not byte-exact"
// decision this project's own "Decisions carried through every milestone"
// section already commits to; a later milestone's job, together with
// wiring `main.cpp`'s Main Menu "Continue Game" item (currently always
// takes the no-saved-game branch, M40) to any of this.
//
// **A real, confirmed dead-code finding, NOT mechanically re-ported:**
// `ESGame.java` declares TWO near-identical methods for reading this same
// data -- `readPerLevelRecords` (confirmed the ACTUAL one `loadGameState()`
// calls) and `readAllLevelRegistries` (grepped: declared, but with no
// caller anywhere in the whole file -- `loadGameState()`'s own header
// comment even NAMES `readAllLevelRegistries` as what it calls, but its
// actual body calls `readPerLevelRecords` instead, a stale/copy-pasted
// comment, not a second real call site). `FromBytes` below ports
// `readPerLevelRecords`'s real logic only; `readAllLevelRegistries` is
// confirmed unreachable, so reproducing its own near-identical body as a
// second, never-called C++ method would just be dead weight, unlike this
// project's usual "port a confirmed-dead BRANCH faithfully" treatment for
// in-method dead code (M40/M46/M47/M48's own dead-branch findings) --
// there's a real difference between preserving a dead branch INSIDE a
// live method (still exercised, still provably reachable in the
// surrounding control flow once you're already there) and hand-
// duplicating an entire dead METHOD with literally zero callers.
//
// **NOT modeled: tile-flag resync.** Neither `writeAllLevelRegistries`
// nor `readPerLevelRecords` touches `Dungeon`'s own tile bits at all
// (confirmed by reading both directly) -- `ToBytes`/`FromBytes` below
// don't either. The real load path's own next step,
// `Dungeon.refreshTileFlagsFromRegistries()`, is ALREADY ported
// (`DungeonRuntime::RefreshTileFlags`, dungeon_runtime.h, whose own doc
// comment already names "after a save/load" as its intended use) -- the
// caller is expected to run that, once per level, right after
// `FromBytes` below, same "caller supplies/owns world state" pattern
// this whole port already uses throughout.
class WorldSave {
public:
    // ESGame.writeAllLevelRegistries(RecordStore): monsters (levels 1..
    // world.monsters.size()-1 -- index 0, the hub, is skipped, matching
    // the confirmed "hub has no monster/chest spawns" convention M6/M18
    // already established), then chests (same range), then dropped items
    // (levels 0..world.droppedItems.size()-1, INCLUDING the hub -- a
    // player can drop an item there). Per level: a signed 32-bit count
    // (`writeInt`), then that many records back-to-back -- monsters via
    // `MonsterRuntime::FromBytes` (the registry's own packed-record
    // storage, matching `Monster.fromBytesShared(record)`) then
    // `MonsterRuntime::WriteTo` (matching the original's own
    // "unpack, then re-serialize through the OTHER format" round trip,
    // preserved exactly, not "optimized" into writing the packed bytes
    // directly); chests/dropped-items are already stored as raw byte
    // records, written 1 byte at a time (`WriteS8`), matching
    // `ESGame.writeBytes()`'s own trivial per-byte loop.
    //
    // Enumeration order within a level (which record comes first) is
    // whatever `std::unordered_map`/`std::vector` iteration happens to
    // produce -- the original's own `Hashtable`/`Vector` enumeration
    // order isn't confirmed to mean anything either (nothing reads
    // records back in a position-sensitive way; `FromBytes` below
    // rebuilds the exact same keyed/unordered containers regardless of
    // record order), so this is a harmless, unavoidable "behavioral
    // reimplementation, not byte-exact" divergence, not a bug.
    static std::vector<uint8_t> ToBytes(const WorldRegistry& world);

    // ESGame.readPerLevelRecords(RecordStore, firstRecordId) -- see this
    // class's own header comment for why NOT readAllLevelRegistries.
    // `levelCount` must match whatever `ToBytes` was called against
    // (WorldRegistry's own constructor takes the same parameter; nothing
    // in the byte stream itself records how many levels it covers, same
    // as the original's own hardcoded-37 loop bounds -- this port already
    // hardcodes 37 everywhere a level count is needed, main.cpp's own
    // `WorldRegistry world(37)` included).
    static WorldRegistry FromBytes(const std::vector<uint8_t>& data, size_t levelCount);
};

}  // namespace stormhold
