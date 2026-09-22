#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "dungeon/dungeon_runtime.h"
#include "player/player_state.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace dawnstar {

// ESGame.java's own `Shop.*` static state that `writeOtherStateInfoToBytes()`/
// `readOtherStateInfo()` persist alongside the character record and the world
// registries -- one flat struct here rather than the save format reading/
// writing `ShopState` (npc/shop_interaction.h's own M45 port of `Shop.java`'s
// live state) directly, since main.cpp's own `syncOtherStateFromLive`/
// `syncLiveFromOtherState` already have to bridge the two around every real
// save/load anyway (this struct's own field layout matches the ORIGINAL's
// save format exactly, not `ShopState`'s in-memory shape). Every one of the
// 26 values below is written and read back in the exact field order/width
// the original uses.
//
// Defaults reproduce `Shop.reset()`'s own post-condition exactly (all 9
// `firstVisit` entries true, everything else zero/false) -- that's the state a
// brand-new character starts in, so `Reset()` below is what main.cpp constructs
// for a fresh game rather than a value-initialized (all-false) struct. Note
// `Item.nextSpawnId`/`Monster.nextSpawnIdCounter` are the two global spawn-id
// counters `Item.nextSpawnId()`/`Monster.nextSpawnId()` hand out from; the
// modules that would advance them live in this port still take their spawn id as
// an explicit parameter instead (see e.g. `DungeonRuntime::PopulateRandomMonsters`'s
// own `spawnIdCounter` parameter and main.cpp's own `nextDropSpawnId` local), so
// nothing here advances them yet -- saving/loading them is real format fidelity
// whose live wiring arrives with whichever milestone ports those call sites.
struct OtherStateInfo {
    // `Item.nextSpawnId`
    int16_t itemNextSpawnId = 0;
    // `Monster.nextSpawnIdCounter`
    int16_t monsterNextSpawnIdCounter = 0;
    std::array<bool, 9> shopFirstVisit{};
    std::array<int16_t, 4> shopInteractionCount{};
    std::array<int16_t, 4> shopRewardsGiven{};
    std::array<int8_t, 4> shopQuestState1{};
    std::array<int8_t, 4> shopQuestState2{};
    bool shopShowDeathGreeting = false;

    // `Shop.reset()`'s own post-condition -- see this struct's own comment.
    static OtherStateInfo Reset();
};

// ESGame.java's own character+world persistence: `saveGameState()`,
// `loadGameState()`, and every helper those two call --
// `writeMasterListsToRecordStore()`, `readMasterListRecords()`,
// `writeOtherStateInfoToBytes()`, `readOtherStateInfo()`, `maxWriteSize()`,
// `getRSNameNotInUse()`, `getLastGoodRSName()`, `cleanupRecordStores()`,
// `resumeGame()`, `openAndRepopulateDungeons()`, and
// `getGameAdvancementLevel()`. This is the real machinery behind the Options
// menu's own "Save Game"/"Load Game" entries (secondaryParam==31, index 5/6),
// which M39 deliberately left as silent deferred no-ops pending exactly this
// module (see ui/options_menu.h's own class comment).
//
// THE RECORD-STORE SUBSTITUTION (a port decision, not a simplification of
// anything recoverable): the original persists through MIDP's
// `javax.microedition.rms.RecordStore` -- a phone-private database of named
// stores, each holding an ordered list of opaque byte[] records, with
// `listRecordStores()`/`getLastModified()`/`deleteRecordStore()` and no
// filesystem at all. This port has no RMS, so one store == one plain file in a
// caller-supplied directory, and one record == a 4-byte big-endian length prefix
// followed by that many raw bytes (`SerializeRecordStore`/`DeserializeRecordStore`
// below). Every record's own CONTENTS are byte-identical to the original's, and
// the record COUNT/ORDER (111 records, see kRecordCount) is reproduced exactly,
// so `ReadMasterListRecords`' own "return the id of the next record after the
// master lists" arithmetic -- which is how `loadGameState()` finds the
// other-state-info record at all -- carries over unchanged. The file's own NAME
// is exactly the store name the original would have used ("es_gamestate<N>"),
// with no added extension.
//
// DELIBERATELY NOT REPRODUCED -- `maxRecordSize()`'s hardcoded `return 1500`,
// the fixed read buffer `readMasterListRecords()` sizes from it. That constant is
// an RMS-era buffer-sizing detail with no counterpart here (each record is read
// into its own exactly-sized byte vector), and reproducing it would reproduce a
// genuine latent bug rather than any real game behavior: `RecordStore.getRecord(id,
// buffer, 0)` THROWS when a record is longer than the buffer, so any level whose
// monster record exceeds 1500 bytes (4 + 53*28 == 1488 still fits; 4 + 54*28 ==
// 1516 does not) makes `loadGameState()` throw -> return false -> "No game is
// available for loading", permanently, since `getLastGoodRSName()` only checks
// that a store opens and has the newest timestamp -- it never validates that its
// records still fit. The WRITE side has no such limit at all (`addRecord` accepts
// records up to RMS's own much larger cap, and `maxWriteSize()` only sizes a
// self-growing ByteArrayOutputStream), so the original can write a save it can
// never read back. m42_game_save_smoke.cpp measures the real generated world's
// own largest monster/chest/dropped-item record against that boundary.
//
// ALSO NOT REPRODUCED, each with its own reason:
//  - `saveGameState()`/`loadGameState()` run on a background `Thread` in the
//    original (`helperThreadState` 5/6, started right after
//    `setCurrentDisplay(saveGameUI/loadGameUI)`), driving `LoadingScreen.percent`
//    + `repaint()`/`serviceRepaints()` as they go. This port runs them
//    synchronously and hands the same percent values to a caller-supplied
//    callback instead (see ProgressCallback below); main.cpp presents the
//    LoadingScreen frame at every callback, which is the same observable
//    sequence of frames without needing a second thread.
//  - `dungeons[i].populated = true` in `openAndRepopulateDungeons()` -- see
//    world/dungeon_view.h's own class comment: this port generates all 37 levels
//    upfront, so every level is always "populated" and that flag has no
//    counterpart to set. Only `refreshTileFlags()` has one
//    (`DungeonRuntime::RefreshTileFlags`).
//  - `openAndRepopulateDungeons(var0)`'s own `var0` (the game-advancement level
//    `resumeGame()` computes and passes) is never read by the method body -- the
//    loop runs over all 37 levels regardless. A real dead parameter, reproduced
//    as one (the parameter is still taken, so the call graph and the
//    `getGameAdvancementLevel()` computation it feeds stay intact).
//  - `Shop.loadDialogue()` in `loadGameState()`'s `mainMenuUI != null` branch --
//    this port loads npcstrings.dat once at startup and never unloads it, so
//    there is nothing to reload. (That branch is also only reachable when loading
//    from the MAIN MENU's own "Load Game"; from the Options menu `mainMenuUI` is
//    null and the original skips it entirely.)
//  - `this.loadingDungeonID = this.character.currentLevel;` + the
//    `imgloadRunning`/`reloadGame` dance in `run()`'s helperThreadState==6 tail
//    -- this port has no per-level image loading (all textures are decoded once
//    at startup), so there is nothing to reload after a load.
class GameSave {
public:
    // Invoked exactly where the original assigns `saveGameUI.percent`/
    // `loadGameUI.percent` AND then calls `repaint()`/`serviceRepaints()` --
    // i.e. the callee is expected to repaint, which is what makes the new
    // percent visible. Note this deliberately does NOT include each method's own
    // opening `percent = 0` assignment: the original never repaints from inside
    // saveGameState()/loadGameState() for that one (the LoadingScreen's own first
    // 0-percent paint came from `setCurrentDisplay()` before the thread even
    // started), so the caller paints percent 0 itself before calling.
    using ProgressCallback = std::function<void(int percent)>;

    // One MIDP RecordStore's worth of records, in record-id order (index 0 ==
    // record id 1). See this class's own comment on the file substitution.
    using RecordStore = std::vector<std::vector<uint8_t>>;

    // The full save layout, all 111 records:
    //   1        -- the character, `Player.toBytes(true)` (M12's PlayerSave)
    //   2..37    -- levels 1..36's own monster lists (level 0, the hub, has no
    //               monster spawns at all -- DungeonGenerator::BuildHubLevel
    //               places none -- so the original never saves index 0 here)
    //   38..73   -- levels 1..36's own chest lists (same reasoning)
    //   74..110  -- levels 0..36's own dropped-item lists (ALL 37 -- a dropped
    //               item can land on the hub level too)
    //   111      -- the OtherStateInfo record (38 bytes)
    static constexpr int kRecordCount = 111;
    static constexpr int kFirstMasterListRecord = 2;
    static constexpr int kOtherStateRecord = 111;
    // Record-store name prefix, exactly the original's own literal.
    static constexpr const char* kStoreNamePrefix = "es_gamestate";

    // `writeOtherStateInfoToBytes()`/`readOtherStateInfo()`: 2 shorts, 9
    // booleans, 4 shorts, 4 shorts, 4 bytes, 4 bytes, 1 boolean == 38 bytes.
    // (The original's `new ByteArrayOutputStream(60)` is only an initial-capacity
    // hint, not the real size.) ReadOtherStateInfo throws on a short record,
    // matching the DataInputStream EOFException the original would hit -- which
    // loadGameState() catches and turns into a `false` return.
    static std::vector<uint8_t> WriteOtherStateInfoToBytes(const OtherStateInfo& s);
    static OtherStateInfo ReadOtherStateInfo(const std::vector<uint8_t>& data);

    // `maxWriteSize()`: the largest single master-list record this world would
    // produce, plus the original's own +50 slack -- 4 + n*28 for monsters,
    // 4 + n*8 for chests, 4 + n*7 for dropped items, over levels 1..36 / 1..36 /
    // 0..36 respectively. Only ever used to size a ByteArrayOutputStream in the
    // original (which grows anyway); ported because it's a real method with real
    // per-record-size arithmetic worth pinning down, and because m42's own test
    // uses it to measure the 1500-byte-buffer finding above.
    static int MaxWriteSize(const WorldRegistry& world);

    // `writeMasterListsToRecordStore(rs)`: APPENDS the 109 master-list records
    // (36 monster + 36 chest + 37 dropped-item) to `store`, which the caller must
    // have already given record 1 (the character bytes) -- `addRecord` appends,
    // and that's exactly what the original does
    // too. Per-record contents: a 4-byte big-endian count, then that many
    // `Monster.writeTo()` 28-byte entries / 8-byte chest records / 7-byte
    // dropped-item records. The order monsters and chests are written in is
    // unspecified in BOTH implementations (the original enumerates a Hashtable,
    // this port an unordered_map) and doesn't matter: both are keyed by position
    // on read. Dropped items ARE order-sensitive (a Vector in the original, a
    // std::vector here) and are written/read in registration order exactly.
    static void WriteMasterListsToRecordStore(const WorldRegistry& world, RecordStore& store,
                                              const ProgressCallback& onProgress);

    // `readMasterListRecords(rs, firstRecordId)`: clears and refills `world`'s own
    // monster/chest/dropped-item registries from records `firstRecordId` onward,
    // returning the id of the NEXT record (kOtherStateRecord for a complete save
    // -- that's how loadGameState() finds the other-state record). Faithfully
    // reproduces the original's two easy-to-miss details: each monster is stored
    // via `Monster.store()`, which keys by the monster's OWN `dungeonLevel` field
    // (`monsters[dungeonLevel-1]`), NOT by the record's own level index -- so a
    // record whose monsters disagree about their level puts them where their own
    // field says; and an out-of-range `dungeonLevel` (<1 or >37) is a hard error
    // here, matching the ArrayIndexOutOfBoundsException the original's own
    // `monsters[this.dungeonLevel - 1]` subscript would throw (caught by
    // loadGameState(), so the load fails).
    static int ReadMasterListRecords(const RecordStore& store, int firstRecordId, WorldRegistry& world,
                                     const ProgressCallback& onProgress);

    // The file container itself -- see this class's own comment. Each record is a
    // 4-byte big-endian length prefix + its bytes; DeserializeRecordStore throws
    // on a truncated/garbage file, which LoadGameState turns into `false` (the
    // same "Exception in loadGameState" outcome the original's own
    // RecordStoreException/EOFException paths produce).
    static std::vector<uint8_t> SerializeRecordStore(const RecordStore& store);
    static RecordStore DeserializeRecordStore(const std::vector<uint8_t>& bytes);

    // `getGameAdvancementLevel(giftPointsFound)` -- 0/1/2/3/4/5 at the original's
    // own 17/29/38/49/62 gift-point thresholds. This class's own thin wrapper
    // around `util/game_advancement.h`'s free function (`resumeGame()`'s real
    // call site); npc/shop_interaction.cpp's own quest-gating call (M45)
    // reuses that same shared header directly rather than this wrapper --
    // see game_advancement.h's own doc comment.
    static int GetGameAdvancementLevel(int giftPointsFound);

    // `openAndRepopulateDungeons(advancementLevel)` -- see this class's own
    // comment on why `advancementLevel` is a real dead parameter. Runs
    // `DungeonRuntime::RefreshTileFlags` over all 37 levels, reporting
    // `100 * (i+1) / 37` (clamped to 100, as the original clamps) at each step.
    static void OpenAndRepopulateDungeons(int advancementLevel, std::vector<GeneratedLevel>& levels,
                                          const WorldRegistry& world, const ProgressCallback& onProgress);

    // `resumeGame()` -- `openAndRepopulateDungeons(getGameAdvancementLevel(
    // character.giftPointsFound))`. Called by the original's own `run()`
    // helperThreadState==6 branch right AFTER loadGameState() returns true (not
    // from inside it), so LoadGameState below deliberately doesn't call this.
    static void ResumeGame(const PlayerState& player, std::vector<GeneratedLevel>& levels, const WorldRegistry& world,
                           const ProgressCallback& onProgress);

    // `getRSNameNotInUse()` -- "es_gamestate" + `Util.randomInt(10000)` (i.e.
    // `LingoRandomInt(rng, 10000)`: 1..10000 inclusive, the same 1-based Lingo
    // idiom util/java_random.h documents), re-rolled until it names a store that
    // doesn't already exist. `rng` is ESGame.r's own counterpart -- the original's
    // `Util.randomInt(bound)` routes through `ESGame.lingoRandomInt(bound)`,
    // which uses that same single global Random.
    static std::string GetRsNameNotInUse(const std::string& dir, JavaRandom& rng);

    // `getLastGoodRSName()` -- the newest-modified "es_gamestate*" store in `dir`,
    // or nullopt if there is none (the original returns null, which
    // loadGameState() turns into `throw new Exception("No valid record store!")`).
    // `getLastModified()`'s counterpart is the file's own last-write time; ties
    // keep the first one seen, exactly like the original's own strict `>`.
    static std::optional<std::string> GetLastGoodRsName(const std::string& dir);

    // `cleanupRecordStores()` -- deletes every "es_gamestate*" store except the
    // last-good one. Called right after a successful save closes its own store,
    // so the just-written file (newest) survives and every older one goes: this
    // game keeps exactly ONE save slot.
    static void CleanupRecordStores(const std::string& dir);

    // `saveGameState()`. Creates `dir` if needed, writes a brand-new store file
    // under a fresh GetRsNameNotInUse() name, then cleans up the older ones.
    // Returns false (and deletes the partial file, exactly like the original's
    // own catch-block `RecordStore.deleteRecordStore(name)`) on any failure --
    // an unwritable directory, a serialization error, anything.
    static bool SaveGameState(const std::string& dir, const PlayerState& player, const WorldRegistry& world,
                              const OtherStateInfo& other, JavaRandom& rng, const ProgressCallback& onProgress);

    // `loadGameState()`. Finds the last-good store, replaces `outPlayer` wholesale
    // (`Player.fromBytes(bytes, true)`) and refills `world` from the master-list
    // records, and fills `outOther`. Returns false on ANY failure -- no store at
    // all, a truncated/garbage file, a bad monster dungeonLevel -- matching the
    // original's single catch-all. The caller then does what `run()`'s own
    // helperThreadState==6 branch does: ResumeGame() on success, or show
    // `noSavedGameUI` on failure.
    static bool LoadGameState(const std::string& dir, PlayerState& outPlayer, WorldRegistry& world,
                              OtherStateInfo& outOther, const ProgressCallback& onProgress);
};

}  // namespace dawnstar

