#pragma once
#include <cstddef>
#include <string>

#include "dungeon/dungeon_runtime.h"
#include "player/player_state.h"
#include "world/shop_state.h"
#include "world/warden.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/ESGame.java's saveGameState()/
// loadGameState() -- the file-I/O layer M49's own "what's next" note
// flagged as ready to build once WorldSave existed. `PlayerSave::ToBytes`/
// `FromBytes` (M20), `WorldSave::ToBytes`/`FromBytes` (M49), and `Shop::
// WriteTo`/`ReadFrom` + `WardenState::WriteTo`/`ReadFrom` (M55, covering
// `writeMasterLists()`/`readMasterLists()`'s own `Shop`-owned fields --
// see world/shop_state.h's own header comment) already do all the real
// field-level work; this module is just the "one plain file holding
// every blob, back to back" framing around them, plus the existence
// check `MenuFlow`'s own Confirm() (ui/menu_flow.cpp) needs for its
// "Continue Game" item.
//
// **M55: file layout changed** -- the world blob now gets its own
// length prefix (previously read to EOF, which only worked because it
// was always the LAST thing in the file) so the new shop/warden sections
// can follow it. Old save files written before this milestone are not
// forward-compatible with this reader (their own "world blob" bytes
// would be misread as a length prefix) -- unremarkable for a dev-only,
// from-scratch reimplementation with no shipped save format yet, same
// "behavioral reimplementation, not byte-exact" precedent this project's
// own "Decisions carried through every milestone" section already
// commits to; not treated as a compatibility break worth guarding
// against.
//
// **Deliberately NOT ported here, both confirmed real, separate gaps:**
// `Item.nextSpawnId`/`Monster.nextSpawnIdCounter` (the other half of the
// original's own `writeMasterLists()`/`readMasterLists()` record) --
// this port has never modeled either as a single persistent global
// counter the way the original does (see player/player_creation.h's own
// `CreateCharacter` doc comment: every spawn site already uses its own
// local counter instead, a confirmed divergence made peace with since
// M6/M9) -- unifying them into one save-able counter is a bigger,
// separate lift, not attempted here. And `saveProgressUI`/
// `loadProgressUI`'s own animated percent-complete screen -- this port's
// save/load runs synchronously on the UI thread with no progress screen
// at all, the same simplification this port's own New Game creation
// already makes (`PlayerCreation::CreateCharacter` runs instantly too,
// no `newGameProgressUI` equivalent).
//
// **A confirmed simplification, not a guess:** the real save system
// names its RecordStore with a random numeric suffix
// (`generateUniqueSaveName`) purely to avoid colliding with an existing
// store name, immediately picks whichever store has the newest
// `getLastModified()` to load from (`findMostRecentSaveName`), and prunes
// every OTHER store right after a successful save (`deleteOtherSaves`).
// Read together, those three methods can never actually leave more than
// one save on disk at a time -- `deleteOtherSaves` runs unconditionally
// at the end of every `saveGameState()` call, so the "pick the newest of
// several candidate names" dance is pure MIDP RecordStore-collision
// plumbing, not a real multi-save-slot feature (confirmed by reading all
// three methods together in ESGame.java; nothing else in the whole file
// ever lists or offers a choice between save names to the player). This
// module skips reproducing the random-suffix/enumerate/prune dance
// entirely and just uses one fixed path -- the same "behavioral
// reimplementation, not byte-exact" call this project's own "Decisions
// carried through every milestone" section already commits to, applied
// here because the observable behavior (exactly one save persists,
// always the most recent) is identical either way.
//
// **NOT modeled here: tile-flag resync.** Same as `WorldSave::FromBytes`
// itself (world_save.h's own doc comment) -- the real load path's next
// step, `Dungeon.refreshTileFlagsFromRegistries()`, is the CALLER's job
// (`DungeonRuntime::RefreshTileFlags`, already ported at M16) to run once
// per level right after `Load` below returns, same as world_save.h's own
// note already says.
class GameSave {
public:
    // True if `path` names an existing, readable file -- ESGame's own
    // `findMostRecentSaveName() != null` reduced to a single fixed path
    // (see this class's own header comment for why that reduction is
    // faithful). `MenuFlow::Confirm`'s own "Continue Game" branch calls
    // this to decide between the real load path and `NoSavedGame`
    // (matching `loadGameState()`'s own `name == null` early failure).
    static bool Exists(const std::string& path);

    // ESGame.saveGameState(): `PlayerSave::ToBytes(player)`, then
    // `WorldSave::ToBytes(world)`, then `Shop::WriteTo`/`WardenState::
    // WriteTo` (M55), written back to back to one file. A leading 4-byte
    // (big-endian, matching every other length this port writes via
    // BinaryWriter) length prefix precedes BOTH the player blob and the
    // world blob (the world blob's own prefix is new at M55, needed now
    // that it's no longer the last thing in the file); shop/warden are
    // both fixed-size and need no prefix of their own. Returns false
    // (matching `saveGameState()`'s own boolean return) on any I/O
    // failure, including `path` being empty (this port's own "no
    // launcher, no persisted state" convention -- see main.cpp's
    // identical `ResolveUserDir()`/`OpenLogFile` pairing for the log
    // file's own equivalent no-op).
    static bool Save(const std::string& path, const PlayerState& player, const WorldRegistry& world,
                      const ShopState& shop, const WardenState& warden);

    // ESGame.loadGameState(): the inverse of Save above. `levelCount`
    // must match whatever `world` had when `Save` was called (see
    // WorldSave::FromBytes's own doc comment -- same hardcoded-37
    // assumption this port already makes everywhere a level count is
    // needed). Returns false, leaving every `out*` parameter unspecified,
    // on any I/O failure or malformed file (matching `loadGameState()`'s
    // own boolean return and its own catch-all `catch (Exception e)`).
    static bool Load(const std::string& path, size_t levelCount, PlayerState& outPlayer, WorldRegistry& outWorld,
                      ShopState& outShop, WardenState& outWarden);
};

}  // namespace stormhold
