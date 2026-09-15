#include "save/game_save.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "assets/binary_reader.h"
#include "assets/binary_writer.h"
#include "monster/monster_runtime.h"
#include "player/player_save.h"
#include "util/game_advancement.h"

namespace dawnstar {

namespace {

// ESGame.java's own hardcoded level count -- `new Hashtable[37]` x2 +
// `new Vector[37]` in allocateMasterLists(), and the literal bound of every
// loop in writeMasterListsToRecordStore()/readMasterListRecords()/
// openAndRepopulateDungeons()/maxWriteSize() below.
constexpr int kLevelCount = 37;
// The three master-list record widths -- Monster.writeTo()'s 28 bytes (see
// monster/monster_runtime.h), the 8-byte chest records and 7-byte dropped-item
// records dungeon/dungeon_runtime.h's own WorldRegistry comment documents.
// These are also exactly the per-entry multipliers maxWriteSize() itself uses.
constexpr int kMonsterRecordBytes = 28;
constexpr int kChestRecordBytes = 8;
constexpr int kDroppedItemRecordBytes = 7;

std::string StorePath(const std::string& dir, const std::string& name) { return dir + "/" + name; }

// `String.startsWith("es_gamestate")` -- the filter both getLastGoodRSName() and
// cleanupRecordStores() apply to listRecordStores().
bool IsGameStateStoreName(const std::string& name) {
    return name.rfind(GameSave::kStoreNamePrefix, 0) == 0;
}

// `RecordStore.listRecordStores()`'s counterpart: every file in `dir`. A missing
// directory yields an empty list, matching the original's own `null` return
// (which both callers treat as "0 stores").
std::vector<std::string> ListStoreNames(const std::string& dir) {
    std::vector<std::string> names;
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) return names;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (entry.is_regular_file(ec)) names.push_back(entry.path().filename().string());
    }
    return names;
}

// `RecordStore.getRecord(id)`'s counterpart: 1-based, and an absent id is a hard
// error -- the original throws RecordStoreException there, which
// loadGameState()'s own catch-all turns into a `false` return.
const std::vector<uint8_t>& RecordAt(const GameSave::RecordStore& store, int oneBasedId) {
    if (oneBasedId < 1 || static_cast<size_t>(oneBasedId) > store.size()) {
        throw std::runtime_error("GameSave: record store has no record " + std::to_string(oneBasedId));
    }
    return store[static_cast<size_t>(oneBasedId - 1)];
}

std::vector<uint8_t> ReadWholeFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("GameSave: cannot open " + path);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

void WriteWholeFile(const std::string& path, const std::vector<uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("GameSave: cannot write " + path);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    out.flush();
    if (!out) throw std::runtime_error("GameSave: write failed for " + path);
}

}  // namespace

OtherStateInfo OtherStateInfo::Reset() {
    OtherStateInfo s;
    s.shopFirstVisit.fill(true);
    return s;
}

std::vector<uint8_t> GameSave::WriteOtherStateInfoToBytes(const OtherStateInfo& s) {
    std::vector<uint8_t> out;
    BinaryWriter w(out);
    w.WriteS16(s.itemNextSpawnId);
    w.WriteS16(s.monsterNextSpawnIdCounter);
    for (int i = 0; i < 9; i++) w.WriteBool(s.shopFirstVisit[static_cast<size_t>(i)]);
    for (int i = 0; i < 4; i++) w.WriteS16(s.shopInteractionCount[static_cast<size_t>(i)]);
    for (int i = 0; i < 4; i++) w.WriteS16(s.shopRewardsGiven[static_cast<size_t>(i)]);
    for (int i = 0; i < 4; i++) w.WriteS8(s.shopQuestState1[static_cast<size_t>(i)]);
    for (int i = 0; i < 4; i++) w.WriteS8(s.shopQuestState2[static_cast<size_t>(i)]);
    w.WriteBool(s.shopShowDeathGreeting);
    return out;
}

OtherStateInfo GameSave::ReadOtherStateInfo(const std::vector<uint8_t>& data) {
    std::string_view view(reinterpret_cast<const char*>(data.data()), data.size());
    std::istringstream in{std::string(view)};
    BinaryReader r(in);
    OtherStateInfo s;
    s.itemNextSpawnId = r.ReadS16();
    s.monsterNextSpawnIdCounter = r.ReadS16();
    for (int i = 0; i < 9; i++) s.shopFirstVisit[static_cast<size_t>(i)] = r.ReadU8() != 0;
    for (int i = 0; i < 4; i++) s.shopInteractionCount[static_cast<size_t>(i)] = r.ReadS16();
    for (int i = 0; i < 4; i++) s.shopRewardsGiven[static_cast<size_t>(i)] = r.ReadS16();
    for (int i = 0; i < 4; i++) s.shopQuestState1[static_cast<size_t>(i)] = r.ReadS8();
    for (int i = 0; i < 4; i++) s.shopQuestState2[static_cast<size_t>(i)] = r.ReadS8();
    s.shopShowDeathGreeting = r.ReadU8() != 0;
    return s;
}

int GameSave::MaxWriteSize(const WorldRegistry& world) {
    int largest = 0;
    for (int level = 1; level < kLevelCount; level++) {
        int size = 4 + static_cast<int>(world.monsters.at(static_cast<size_t>(level)).size()) * kMonsterRecordBytes;
        if (size > largest) largest = size;
    }
    for (int level = 1; level < kLevelCount; level++) {
        int size = 4 + static_cast<int>(world.chests.at(static_cast<size_t>(level)).size()) * kChestRecordBytes;
        if (size > largest) largest = size;
    }
    for (int level = 0; level < kLevelCount; level++) {
        int size =
            4 + static_cast<int>(world.droppedItems.at(static_cast<size_t>(level)).size()) * kDroppedItemRecordBytes;
        if (size > largest) largest = size;
    }
    return largest + 50;
}

void GameSave::WriteMasterListsToRecordStore(const WorldRegistry& world, RecordStore& store,
                                             const ProgressCallback& onProgress) {
    for (int level = 1; level < kLevelCount; level++) {
        std::vector<uint8_t> record;
        BinaryWriter w(record);
        const auto& monsters = world.monsters.at(static_cast<size_t>(level));
        w.WriteS32(static_cast<int32_t>(monsters.size()));
        for (const auto& entry : monsters) {
            // The original decodes each stored 28-byte record back into a Monster
            // and re-encodes it via writeTo() -- a byte-for-byte identity, but
            // reproduced through the same decode/encode pair rather than copying
            // the array straight across, so any future divergence between
            // ToBytes/FromBytes and WriteTo/ReadFrom shows up here too.
            MonsterRuntime::WriteTo(w, MonsterRuntime::FromBytes(entry.second));
        }
        store.push_back(record);
        if (onProgress) onProgress(20 + 30 * (level + 1) / kLevelCount);
    }

    for (int level = 1; level < kLevelCount; level++) {
        std::vector<uint8_t> record;
        BinaryWriter w(record);
        const auto& chests = world.chests.at(static_cast<size_t>(level));
        w.WriteS32(static_cast<int32_t>(chests.size()));
        for (const auto& entry : chests) {
            // writeBytesToDataOututStream(out, bytes, 8): 8 individual
            // writeByte() calls, i.e. the raw stored bytes verbatim.
            for (int i = 0; i < kChestRecordBytes; i++) w.WriteU8(entry.second[static_cast<size_t>(i)]);
        }
        store.push_back(record);
        if (onProgress) onProgress(50 + 30 * (level + 1) / kLevelCount);
    }

    for (int level = 0; level < kLevelCount; level++) {
        std::vector<uint8_t> record;
        BinaryWriter w(record);
        const auto& dropped = world.droppedItems.at(static_cast<size_t>(level));
        w.WriteS32(static_cast<int32_t>(dropped.size()));
        for (const auto& entry : dropped) {
            for (int i = 0; i < kDroppedItemRecordBytes; i++) w.WriteU8(entry[static_cast<size_t>(i)]);
        }
        store.push_back(record);
        if (onProgress) onProgress(80 + 19 * (level + 1) / kLevelCount);
    }
}

int GameSave::ReadMasterListRecords(const RecordStore& store, int firstRecordId, WorldRegistry& world,
                                    const ProgressCallback& onProgress) {
    int recordId = firstRecordId;

    for (int level = 1; level < kLevelCount; level++) {
        const std::vector<uint8_t>& record = RecordAt(store, recordId++);
        std::string_view view(reinterpret_cast<const char*>(record.data()), record.size());
        std::istringstream in{std::string(view)};
        BinaryReader r(in);
        // The record's OWN level is what gets cleared -- but each monster is
        // stored by its own `dungeonLevel` field below, exactly as
        // Monster.store() does (`monsters[this.dungeonLevel - 1]`), so the two
        // indices are deliberately not assumed to agree.
        world.monsters.at(static_cast<size_t>(level)).clear();
        int count = r.ReadS32();
        for (int i = 0; i < count; i++) {
            MonsterState m = MonsterRuntime::ReadFrom(r);
            int index = static_cast<int>(m.dungeonLevel) - 1;
            if (index < 0 || static_cast<size_t>(index) >= world.monsters.size()) {
                // The original's own `ESGame.monsters[this.dungeonLevel - 1]`
                // subscript throws ArrayIndexOutOfBoundsException here, which
                // loadGameState() catches -- a hard failure, not a skip.
                throw std::runtime_error("GameSave: monster dungeonLevel " + std::to_string(m.dungeonLevel) +
                                         " outside 1.." + std::to_string(kLevelCount));
            }
            world.monsters[static_cast<size_t>(index)][PackPosKey(m.x, m.y)] = MonsterRuntime::ToBytes(m);
        }
        if (onProgress) onProgress(20 + 30 * (level + 1) / kLevelCount);
    }

    for (int level = 1; level < kLevelCount; level++) {
        const std::vector<uint8_t>& record = RecordAt(store, recordId++);
        std::string_view view(reinterpret_cast<const char*>(record.data()), record.size());
        std::istringstream in{std::string(view)};
        BinaryReader r(in);
        auto& chests = world.chests.at(static_cast<size_t>(level));
        chests.clear();
        int count = r.ReadS32();
        for (int i = 0; i < count; i++) {
            std::array<uint8_t, kChestRecordBytes> chest{};
            for (int b = 0; b < kChestRecordBytes; b++) chest[static_cast<size_t>(b)] = r.ReadU8();
            // chests[level].put(Util.posKey(bytes[0], bytes[1]), bytes) -- the
            // key is re-derived from the record's own x/y, same as on save.
            int x = static_cast<int8_t>(chest[0]);
            int y = static_cast<int8_t>(chest[1]);
            chests[PackPosKey(x, y)] = chest;
        }
        if (onProgress) onProgress(50 + 30 * (level + 1) / kLevelCount);
    }

    for (int level = 0; level < kLevelCount; level++) {
        const std::vector<uint8_t>& record = RecordAt(store, recordId++);
        std::string_view view(reinterpret_cast<const char*>(record.data()), record.size());
        std::istringstream in{std::string(view)};
        BinaryReader r(in);
        auto& dropped = world.droppedItems.at(static_cast<size_t>(level));
        dropped.clear();
        int count = r.ReadS32();
        for (int i = 0; i < count; i++) {
            std::array<uint8_t, kDroppedItemRecordBytes> item{};
            for (int b = 0; b < kDroppedItemRecordBytes; b++) item[static_cast<size_t>(b)] = r.ReadU8();
            dropped.push_back(item);
        }
        if (onProgress) onProgress(80 + 19 * (level + 1) / kLevelCount);
    }

    return recordId;
}

std::vector<uint8_t> GameSave::SerializeRecordStore(const RecordStore& store) {
    std::vector<uint8_t> out;
    BinaryWriter w(out);
    for (const auto& record : store) {
        w.WriteU32(static_cast<uint32_t>(record.size()));
        out.insert(out.end(), record.begin(), record.end());
    }
    return out;
}

GameSave::RecordStore GameSave::DeserializeRecordStore(const std::vector<uint8_t>& bytes) {
    RecordStore store;
    std::string_view view(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    std::istringstream in{std::string(view)};
    BinaryReader r(in);
    // Reads until the underlying stream runs out -- a 4-byte length prefix whose
    // promised bytes aren't there is a hard error (BinaryReader throws on EOF),
    // which LoadGameState turns into its own `false`.
    while (in.peek() != std::char_traits<char>::eof()) {
        uint32_t size = r.ReadU32();
        std::vector<uint8_t> record(static_cast<size_t>(size));
        for (uint32_t i = 0; i < size; i++) record[static_cast<size_t>(i)] = r.ReadU8();
        store.push_back(std::move(record));
    }
    return store;
}

int GameSave::GetGameAdvancementLevel(int giftPointsFound) {
    // M45: the real formula now lives in util/game_advancement.h (a second
    // consumer, npc/shop_interaction.h, needed it too) -- this stays as a
    // thin forwarding wrapper so existing callers (main.cpp's ResumeGame
    // path, m42_game_save_smoke.cpp) don't need to change.
    return ::dawnstar::GetGameAdvancementLevel(giftPointsFound);
}

void GameSave::OpenAndRepopulateDungeons(int advancementLevel, std::vector<GeneratedLevel>& levels,
                                         const WorldRegistry& world, const ProgressCallback& onProgress) {
    // A real dead parameter -- see this class's own header comment. Kept in the
    // signature so resumeGame()'s own getGameAdvancementLevel() call stays
    // intact, exactly as the original's does.
    (void)advancementLevel;
    // The original loops `var2 < 37` over its own fixed `dungeons[]`; this port
    // iterates its own level vector (37 entries for the real game) and keeps the
    // original's literal 37 as the percent denominator, including its own
    // clamp-at-100.
    for (size_t i = 0; i < levels.size(); i++) {
        DungeonRuntime::RefreshTileFlags(levels[i], world, static_cast<int>(i));
        int percent = 100 * static_cast<int>(i + 1) / kLevelCount;
        if (percent > 100) percent = 100;
        if (onProgress) onProgress(percent);
    }
}

void GameSave::ResumeGame(const PlayerState& player, std::vector<GeneratedLevel>& levels, const WorldRegistry& world,
                          const ProgressCallback& onProgress) {
    OpenAndRepopulateDungeons(GetGameAdvancementLevel(player.giftPointsFound), levels, world, onProgress);
}

std::string GameSave::GetRsNameNotInUse(const std::string& dir, JavaRandom& rng) {
    // listRecordStores() is snapshotted ONCE, before the re-roll loop -- the
    // original does the same (its `var1`/`var2` are read once at the top), so the
    // loop re-rolls against that same fixed list rather than re-listing.
    std::vector<std::string> existing = ListStoreNames(dir);
    while (true) {
        std::string name = std::string(kStoreNamePrefix) + std::to_string(LingoRandomInt(rng, 10000));
        if (std::find(existing.begin(), existing.end(), name) == existing.end()) return name;
    }
}

std::optional<std::string> GameSave::GetLastGoodRsName(const std::string& dir) {
    std::optional<std::string> best;
    std::filesystem::file_time_type bestTime{};
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) return best;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        std::string name = entry.path().filename().string();
        if (!IsGameStateStoreName(name)) continue;
        // The original wraps openRecordStore()/getNumRecords()/getLastModified()
        // in a try/catch that silently skips any store it can't inspect; this
        // port's equivalent is skipping any file whose timestamp can't be read.
        std::error_code timeEc;
        std::filesystem::file_time_type modified = entry.last_write_time(timeEc);
        if (timeEc) continue;
        if (!best || modified > bestTime) {
            best = name;
            bestTime = modified;
        }
    }
    return best;
}

void GameSave::CleanupRecordStores(const std::string& dir) {
    std::optional<std::string> lastGood = GetLastGoodRsName(dir);
    // Collected first, then deleted -- deleting while iterating a
    // directory_iterator is unspecified enough across platforms that this port
    // doesn't rely on it (the original's own RecordStore.deleteRecordStore()
    // iterates a snapshot String[] for the same reason).
    std::vector<std::string> stale;
    std::error_code ec;
    if (std::filesystem::is_directory(dir, ec)) {
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            std::string name = entry.path().filename().string();
            if (!IsGameStateStoreName(name)) continue;
            if (lastGood && *lastGood == name) continue;
            stale.push_back(name);
        }
    }
    for (const auto& name : stale) {
        std::error_code removeEc;
        std::filesystem::remove(StorePath(dir, name), removeEc);
    }
}

bool GameSave::SaveGameState(const std::string& dir, const PlayerState& player, const WorldRegistry& world,
                             const OtherStateInfo& other, JavaRandom& rng, const ProgressCallback& onProgress) {
    std::string name;
    // `catch (...)` mirrors the original's own `catch (Throwable var18)` -- a
    // save failure of ANY kind is reported the same way, and the partial store is
    // discarded either way.
    try {
        // RecordStore.openRecordStore(name, true) creates the store; this port's
        // equivalent is making sure the directory exists first (which also makes
        // GetRsNameNotInUse's own listing see a real directory).
        std::filesystem::create_directories(dir);
        name = GetRsNameNotInUse(dir, rng);

        RecordStore store;
        store.push_back(PlayerSave::ToBytes(player));
        if (onProgress) onProgress(20);
        WriteMasterListsToRecordStore(world, store, onProgress);
        store.push_back(WriteOtherStateInfoToBytes(other));
        // closeRecordStore() -- the point at which the save actually becomes
        // durable, and after which cleanupRecordStores() can safely treat it as
        // the newest store.
        WriteWholeFile(StorePath(dir, name), SerializeRecordStore(store));
        CleanupRecordStores(dir);
        if (onProgress) onProgress(100);
        return true;
    } catch (...) {
        if (!name.empty()) {
            std::error_code ec;
            std::filesystem::remove(StorePath(dir, name), ec);
        }
        return false;
    }
}

bool GameSave::LoadGameState(const std::string& dir, PlayerState& outPlayer, WorldRegistry& world,
                             OtherStateInfo& outOther, const ProgressCallback& onProgress) {
    try {
        std::optional<std::string> name = GetLastGoodRsName(dir);
        if (!name) throw std::runtime_error("GameSave: no valid record store");
        RecordStore store = DeserializeRecordStore(ReadWholeFile(StorePath(dir, *name)));

        // `this.character = Player.fromBytes(var6, true)` happens BEFORE the
        // master lists are read, so a load that fails partway through the master
        // lists (or at the other-state record) has ALREADY replaced the live
        // character -- the original's own real ordering, reproduced deliberately
        // rather than "fixed" into an all-or-nothing swap. The caller shows
        // noSavedGameUI and returns to the Options menu, where the game then
        // continues with that already-swapped character.
        outPlayer = PlayerSave::FromBytes(RecordAt(store, 1));
        if (onProgress) onProgress(20);

        int nextRecord = ReadMasterListRecords(store, kFirstMasterListRecord, world, onProgress);
        // readOtherStateInfo() writes straight into Shop's statics, so outOther is
        // only touched once the record itself parsed -- a failure before this
        // point leaves the caller's existing values untouched, as in the original.
        outOther = ReadOtherStateInfo(RecordAt(store, nextRecord));
        return true;
    } catch (...) {
        return false;
    }
}

}  // namespace dawnstar

