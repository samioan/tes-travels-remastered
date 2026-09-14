// M42 smoke test: GameSave (save/game_save.h) + LoadingScreen
// (ui/loading_screen.h) + the Options menu's now-real "Save Game"/"Load
// Game" actions -- ESGame.java's own saveGameState()/loadGameState() and
// every helper those two call.
//
// No JVM ground truth is available (same reason as every prior
// milestone). Every real target is independently re-derived here from
// ../../../src/ESGame.java's own saveGameState/loadGameState/
// writeMasterListsToRecordStore/readMasterListRecords/
// writeOtherStateInfoToBytes/readOtherStateInfo/maxWriteSize/
// getRSNameNotInUse/getLastGoodRSName/cleanupRecordStores/resumeGame/
// openAndRepopulateDungeons/getGameAdvancementLevel, plus
// ../../../src/Monster.java's own writeTo() 28-byte layout and
// ../../../src/LoadingScreen.java's own renderProgress() -- never read
// back out of game_save.cpp. The 38-byte other-state record's exact byte
// layout, the 111-record store layout, and all three percent sequences
// are each reconstructed from scratch in this file with their own
// independent big-endian encoders, so this test can't merely be checking
// the implementation's arithmetic against itself. Text placement is
// cross-checked against BitmapFont::DrawString used as an oracle, the
// same technique M37-M41's own tests established.
#include <algorithm>
#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/help_text.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/shop_dialogue.h"
#include "assets/spell_database.h"
#include "dungeon/dungeon_runtime.h"
#include "graphics/backbuffer.h"
#include "graphics/bitmap_font.h"
#include "monster/monster_runtime.h"
#include "player/player_creation.h"
#include "player/player_save.h"
#include "player/player_state.h"
#include "save/game_save.h"
#include "ui/loading_screen.h"
#include "ui/options_menu.h"
#include "ui/screen.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::Backbuffer;
namespace BitmapFont = dawnstar::BitmapFont;
using dawnstar::CharacterData;
using dawnstar::DungeonRuntime;
using dawnstar::GameSave;
using dawnstar::GeneratedLevel;
using dawnstar::JavaRandom;
using dawnstar::LoadingScreen;
using dawnstar::LoadingScreenMode;
using dawnstar::MonsterRuntime;
using dawnstar::MonsterState;
using dawnstar::OptionsMenu;
using dawnstar::OptionsMenuAction;
using dawnstar::OtherStateInfo;
using dawnstar::PlayerState;
using dawnstar::Screen;
using dawnstar::WorldRegistry;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

uint16_t PixelAt(const Backbuffer& bb, int x, int y) {
    return bb.Data()[static_cast<size_t>(y) * Backbuffer::kWidth + x];
}

// Same oracle technique m37-m41's own tests established.
bool TextRenderedAt(const Backbuffer& bb, int x0, int y0, const std::string& text, uint16_t color) {
    Backbuffer scratch;
    scratch.Fill(0);
    BitmapFont::DrawString(scratch, 0, 0, text, 0xFFFF);
    bool sawOnPixel = false;
    int spanW = static_cast<int>(text.size()) * BitmapFont::kAdvance;
    for (int dy = 0; dy < BitmapFont::kGlyphHeight; dy++) {
        for (int dx = 0; dx < spanW; dx++) {
            if (PixelAt(scratch, dx, dy) == 0) continue;
            sawOnPixel = true;
            int px = x0 + dx;
            int py = y0 + dy;
            if (px < 0 || px >= Backbuffer::kWidth || py < 0 || py >= Backbuffer::kHeight) return false;
            if (PixelAt(bb, px, py) != color) return false;
        }
    }
    return sawOnPixel;
}

// --- This test's OWN independent big-endian encoders, deliberately not
// reusing assets/binary_writer.h (which is what game_save.cpp itself
// uses): a shared encoder would make every byte-layout check below a
// tautology. ---
void PutS16(std::vector<uint8_t>& out, int16_t v) {
    uint16_t u = static_cast<uint16_t>(v);
    out.push_back(static_cast<uint8_t>(u >> 8));
    out.push_back(static_cast<uint8_t>(u));
}
void PutS8(std::vector<uint8_t>& out, int8_t v) { out.push_back(static_cast<uint8_t>(v)); }
void PutBool(std::vector<uint8_t>& out, bool v) { out.push_back(v ? 1 : 0); }
void PutS32(std::vector<uint8_t>& out, int32_t v) {
    uint32_t u = static_cast<uint32_t>(v);
    out.push_back(static_cast<uint8_t>(u >> 24));
    out.push_back(static_cast<uint8_t>(u >> 16));
    out.push_back(static_cast<uint8_t>(u >> 8));
    out.push_back(static_cast<uint8_t>(u));
}

int16_t ReadS16At(const std::vector<uint8_t>& b, size_t i) {
    return static_cast<int16_t>(static_cast<uint16_t>((static_cast<uint16_t>(b[i]) << 8) | b[i + 1]));
}
int32_t ReadS32At(const std::vector<uint8_t>& b, size_t i) {
    return static_cast<int32_t>((static_cast<uint32_t>(b[i]) << 24) | (static_cast<uint32_t>(b[i + 1]) << 16) |
                                (static_cast<uint32_t>(b[i + 2]) << 8) | b[i + 3]);
}

// ESGame.java's own level count and the three master-list record widths,
// re-derived from writeMasterListsToRecordStore/maxWriteSize's own
// literals (`4 + var2 * 28`, `4 + var2 * 8`, `4 + var2 * 7`) rather than
// taken from game_save.h.
constexpr int kLevels = 37;
constexpr int kMonsterBytes = 28;
constexpr int kChestBytes = 8;
constexpr int kDroppedBytes = 7;
// readMasterListRecords' own hardcoded read-buffer size, via
// maxRecordSize()'s `return 1500` -- see section F below.
constexpr int kOriginalReadBuffer = 1500;

bool SameMonsters(std::vector<std::array<uint8_t, kMonsterBytes>> a,
                  std::vector<std::array<uint8_t, kMonsterBytes>> b) {
    // Order-insensitive on purpose: the original enumerates a Hashtable and
    // this port an unordered_map, so neither side's write order is
    // meaningful (both are keyed by position on read).
    auto less = [](const std::array<uint8_t, kMonsterBytes>& x, const std::array<uint8_t, kMonsterBytes>& y) {
        return x < y;
    };
    std::sort(a.begin(), a.end(), less);
    std::sort(b.begin(), b.end(), less);
    return a == b;
}

std::vector<std::array<uint8_t, kMonsterBytes>> RegistryMonsters(const WorldRegistry& world, int level) {
    std::vector<std::array<uint8_t, kMonsterBytes>> out;
    for (const auto& entry : world.monsters[static_cast<size_t>(level)]) out.push_back(entry.second);
    return out;
}

// The 28-byte chunks of one written monster record, skipping its own
// 4-byte count prefix.
std::vector<std::array<uint8_t, kMonsterBytes>> RecordMonsters(const std::vector<uint8_t>& record) {
    std::vector<std::array<uint8_t, kMonsterBytes>> out;
    int count = ReadS32At(record, 0);
    for (int i = 0; i < count; i++) {
        std::array<uint8_t, kMonsterBytes> m{};
        for (int b = 0; b < kMonsterBytes; b++) {
            m[static_cast<size_t>(b)] = record[static_cast<size_t>(4 + i * kMonsterBytes + b)];
        }
        out.push_back(m);
    }
    return out;
}

bool RegistryEquals(const WorldRegistry& a, const WorldRegistry& b) {
    for (int level = 0; level < kLevels; level++) {
        if (a.monsters[static_cast<size_t>(level)] != b.monsters[static_cast<size_t>(level)]) return false;
        if (a.chests[static_cast<size_t>(level)] != b.chests[static_cast<size_t>(level)]) return false;
        if (a.droppedItems[static_cast<size_t>(level)] != b.droppedItems[static_cast<size_t>(level)]) return false;
    }
    return true;
}

bool OtherStateEquals(const OtherStateInfo& a, const OtherStateInfo& b) {
    return a.itemNextSpawnId == b.itemNextSpawnId && a.monsterNextSpawnIdCounter == b.monsterNextSpawnIdCounter &&
           a.shopFirstVisit == b.shopFirstVisit && a.shopInteractionCount == b.shopInteractionCount &&
           a.shopRewardsGiven == b.shopRewardsGiven && a.shopQuestState1 == b.shopQuestState1 &&
           a.shopQuestState2 == b.shopQuestState2 && a.shopShowDeathGreeting == b.shopShowDeathGreeting;
}

std::vector<uint8_t> ReadFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

void WriteFile(const std::string& path, const std::vector<uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

bool Throws(const std::function<void()>& f) {
    try {
        f();
    } catch (...) {
        return true;
    }
    return false;
}

// The three percent sequences, each re-derived here from ESGame.java's own
// literals rather than collected from game_save.cpp: saveGameState()'s
// `20 + 30 * (var6 + 1) / 37` / `50 + 30 * (var30 + 1) / 37` /
// `80 + 19 * (var32 + 1) / 37` (monsters 1..36, chests 1..36, dropped
// items 0..36), readMasterListRecords' identical three loops, and
// openAndRepopulateDungeons' clamped `100 * var1 / 37`.
std::vector<int> ExpectedMasterListPercents() {
    std::vector<int> out;
    for (int level = 1; level < kLevels; level++) out.push_back(20 + 30 * (level + 1) / kLevels);
    for (int level = 1; level < kLevels; level++) out.push_back(50 + 30 * (level + 1) / kLevels);
    for (int level = 0; level < kLevels; level++) out.push_back(80 + 19 * (level + 1) / kLevels);
    return out;
}

std::vector<int> ExpectedRepopulatePercents() {
    std::vector<int> out;
    for (int i = 1; i <= kLevels; i++) {
        int percent = 100 * i / kLevels;
        if (percent > 100) percent = 100;
        out.push_back(percent);
    }
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";
    // One throwaway directory for the whole test -- the port's own
    // RecordStore substitute (see save/game_save.h's own class comment).
    std::string saveDir = (std::filesystem::temp_directory_path() / "dawnstar_m42_saves").string();

    try {
        std::error_code ec;
        std::filesystem::remove_all(saveDir, ec);

        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        dawnstar::MonsterDatabase monsterDb = dawnstar::MonsterDatabase::Load(archive);
        dawnstar::SpellDatabase spells = dawnstar::SpellDatabase::Load(archive);
        CharacterData charData = CharacterData::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);
        dawnstar::HelpText helpText = dawnstar::HelpText::Load(archive);
        dawnstar::ShopDialogue shopDialogue = dawnstar::ShopDialogue::Load(root + "/npcstrings.dat");
        Check(geometry.rows.size() == static_cast<size_t>(kLevels),
              "the real dungeon.dat should describe exactly 37 levels, the count every master-list loop hardcodes");

        // The real 37-level world, built exactly the way main.cpp's own
        // BuildWorld does -- a live WorldRegistry full of real generated
        // monster/chest spawns, not a synthetic fixture.
        std::vector<GeneratedLevel> levels;
        WorldRegistry world(geometry.rows.size());
        for (size_t i = 0; i < geometry.rows.size(); i++) {
            int levelNumber = static_cast<int>(i) + 1;
            levels.push_back(levelNumber == 1
                                  ? dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[0])
                                  : dawnstar::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[i], items,
                                                                              monsterDb));
            DungeonRuntime::RegisterGeneratedSpawns(levels.back(), world);
        }
        size_t totalMonsters = 0;
        size_t totalChests = 0;
        for (int level = 0; level < kLevels; level++) {
            totalMonsters += world.monsters[static_cast<size_t>(level)].size();
            totalChests += world.chests[static_cast<size_t>(level)].size();
        }
        Check(totalMonsters > 0 && totalChests > 0,
              "the real generated world should have registered both monsters and chests to save");
        std::printf("  real world: %zu levels, %zu monsters, %zu chests registered\n", levels.size(), totalMonsters,
                    totalChests);

        JavaRandom rng(20260914);
        PlayerState player = dawnstar::PlayerCreation::CreateCharacter(3, "M42 Tester", charData, items, rng);
        player.gold = 4321;
        player.giftPointsFound = 41;  // -> advancement level 3 (checked below)
        player.currentLevel = 7;

        // --- A: OtherStateInfo -- writeOtherStateInfoToBytes()/
        // readOtherStateInfo()'s exact 38-byte layout, re-encoded here from
        // scratch rather than read back out of game_save.cpp. ---
        OtherStateInfo reset = OtherStateInfo::Reset();
        bool allFirstVisitTrue = true;
        for (int i = 0; i < 9; i++) {
            allFirstVisitTrue = allFirstVisitTrue && reset.shopFirstVisit[static_cast<size_t>(i)];
        }
        Check(allFirstVisitTrue, "Shop.reset() leaves all 9 firstVisit entries true");
        Check(reset.itemNextSpawnId == 0 && reset.monsterNextSpawnIdCounter == 0 && !reset.shopShowDeathGreeting,
              "Shop.reset() leaves both spawn-id counters and showDeathGreeting at zero/false");

        OtherStateInfo s;  // deliberately NOT Reset(): distinct, edge-valued fields
        s.itemNextSpawnId = -32768;
        s.monsterNextSpawnIdCounter = 32767;
        s.shopFirstVisit = {true, false, true, false, true, false, true, false, true};
        s.shopInteractionCount = {-1, 2, 300, -300};
        s.shopRewardsGiven = {1, -2, 3, -4};
        s.shopQuestState1 = {-128, 0, 1, 127};
        s.shopQuestState2 = {5, -5, 100, -100};
        s.shopShowDeathGreeting = true;

        std::vector<uint8_t> otherBytes = GameSave::WriteOtherStateInfoToBytes(s);
        // 2 shorts + 9 booleans + 4 shorts + 4 shorts + 4 bytes + 4 bytes +
        // 1 boolean -- counted straight off the two methods' own loops.
        constexpr size_t kExpectedOtherStateBytes = 2 + 2 + 9 + 4 * 2 + 4 * 2 + 4 + 4 + 1;
        Check(otherBytes.size() == kExpectedOtherStateBytes,
              "the other-state record should be exactly 38 bytes (the original's ByteArrayOutputStream(60) is only a capacity hint)");

        std::vector<uint8_t> expectOther;
        PutS16(expectOther, s.itemNextSpawnId);
        PutS16(expectOther, s.monsterNextSpawnIdCounter);
        for (int i = 0; i < 9; i++) PutBool(expectOther, s.shopFirstVisit[static_cast<size_t>(i)]);
        for (int i = 0; i < 4; i++) PutS16(expectOther, s.shopInteractionCount[static_cast<size_t>(i)]);
        for (int i = 0; i < 4; i++) PutS16(expectOther, s.shopRewardsGiven[static_cast<size_t>(i)]);
        for (int i = 0; i < 4; i++) PutS8(expectOther, s.shopQuestState1[static_cast<size_t>(i)]);
        for (int i = 0; i < 4; i++) PutS8(expectOther, s.shopQuestState2[static_cast<size_t>(i)]);
        PutBool(expectOther, s.shopShowDeathGreeting);
        Check(otherBytes == expectOther,
              "the other-state record's bytes should match this test's own independently encoded layout exactly");
        Check(ReadS16At(otherBytes, 0) == -32768 && ReadS16At(otherBytes, 2) == 32767,
              "both spawn-id counters should be big-endian shorts at offsets 0 and 2");
        Check(otherBytes[4] == 1 && otherBytes[5] == 0 && otherBytes[12] == 1,
              "the 9 firstVisit booleans should start at offset 4, one byte each");

        OtherStateInfo readBack = GameSave::ReadOtherStateInfo(otherBytes);
        Check(OtherStateEquals(readBack, s), "reading the other-state record back should restore all 26 fields exactly");
        Check(Throws([&] {
                  GameSave::ReadOtherStateInfo(std::vector<uint8_t>(otherBytes.begin(), otherBytes.end() - 1));
              }),
              "a one-byte-short other-state record should throw (the EOFException loadGameState catches)");

        // getGameAdvancementLevel()'s own 5 thresholds, checked at both sides
        // of each boundary.
        const int giftPoints[] = {0, 16, 17, 28, 29, 37, 38, 48, 49, 61, 62, 200};
        const int expectedAdvancement[] = {0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5};
        bool advancementOk = true;
        for (int i = 0; i < 12; i++) {
            if (GameSave::GetGameAdvancementLevel(giftPoints[i]) != expectedAdvancement[i]) advancementOk = false;
        }
        Check(advancementOk, "getGameAdvancementLevel should return 0/1/2/3/4/5 at its own 17/29/38/49/62 thresholds");
        Check(GameSave::GetGameAdvancementLevel(player.giftPointsFound) == 3,
              "this test's own 41 gift points should map to advancement level 3 (38 <= 41 < 49)");

        // --- B: maxWriteSize() -- the largest single master-list record plus
        // the original's own +50 slack, recomputed here from the live
        // registry's own per-level container sizes. ---
        int expectedMaxWriteSize = 0;
        for (int level = 1; level < kLevels; level++) {
            int size = 4 + static_cast<int>(world.monsters[static_cast<size_t>(level)].size()) * kMonsterBytes;
            expectedMaxWriteSize = std::max(expectedMaxWriteSize, size);
        }
        for (int level = 1; level < kLevels; level++) {
            int size = 4 + static_cast<int>(world.chests[static_cast<size_t>(level)].size()) * kChestBytes;
            expectedMaxWriteSize = std::max(expectedMaxWriteSize, size);
        }
        for (int level = 0; level < kLevels; level++) {
            int size = 4 + static_cast<int>(world.droppedItems[static_cast<size_t>(level)].size()) * kDroppedBytes;
            expectedMaxWriteSize = std::max(expectedMaxWriteSize, size);
        }
        expectedMaxWriteSize += 50;
        Check(GameSave::MaxWriteSize(world) == expectedMaxWriteSize,
              "maxWriteSize should be the largest per-level record (4+n*28 / 4+n*8 / 4+n*7) plus 50");
        Check(GameSave::MaxWriteSize(WorldRegistry(static_cast<size_t>(kLevels))) == 54,
              "an empty world's maxWriteSize should be 4 + 50 (the count prefix alone)");
        std::printf("  maxWriteSize(real world) = %d\n", GameSave::MaxWriteSize(world));

        // --- C: the 111-record store layout, both master-list write loops,
        // and their exact percent sequences. ---
        GameSave::RecordStore store;
        store.push_back(dawnstar::PlayerSave::ToBytes(player));
        std::vector<int> writePercents;
        GameSave::WriteMasterListsToRecordStore(world, store, [&](int p) { writePercents.push_back(p); });
        Check(store.size() == 110,
              "the character record plus the 109 master-list records (36 monster + 36 chest + 37 dropped-item)");
        // saveGameState()'s own final addRecord: the other-state record, id 111.
        store.push_back(GameSave::WriteOtherStateInfoToBytes(s));
        Check(store.size() == 111,
              "a complete save should be exactly 111 records: 1 character + 109 master-list + 1 other-state");
        Check(writePercents.size() == 109, "the master-list loops should report one percent each: 36 + 36 + 37");
        Check(writePercents == ExpectedMasterListPercents(),
              "the save's own percent sequence should be exactly 20+30*(L+1)/37, then 50+30*(L+1)/37, then "
              "80+19*(L+1)/37");

        // Monster records: record id level+1 (so ids 2..37), a 4-byte count
        // then that level's own 28-byte records.
        bool monsterRecordsOk = true;
        for (int level = 1; level < kLevels; level++) {
            const std::vector<uint8_t>& record = store[static_cast<size_t>(level)];
            size_t n = world.monsters[static_cast<size_t>(level)].size();
            if (ReadS32At(record, 0) != static_cast<int32_t>(n)) monsterRecordsOk = false;
            if (record.size() != 4 + n * static_cast<size_t>(kMonsterBytes)) monsterRecordsOk = false;
            if (!SameMonsters(RecordMonsters(record), RegistryMonsters(world, level))) monsterRecordsOk = false;
        }
        Check(monsterRecordsOk,
              "every monster record should be a 4-byte count plus exactly that level's own 28-byte monster records");
        Check(store[1].size() == 4 + world.monsters[1].size() * static_cast<size_t>(kMonsterBytes),
              "level 0 (the hub) should have NO monster record at all -- ids start at 2, for level 1");

        // Chest records: ids 38..73 (levels 1..36), 8 bytes each.
        bool chestRecordsOk = true;
        for (int level = 1; level < kLevels; level++) {
            const std::vector<uint8_t>& record = store[static_cast<size_t>(36 + level)];
            const auto& chests = world.chests[static_cast<size_t>(level)];
            if (ReadS32At(record, 0) != static_cast<int32_t>(chests.size())) chestRecordsOk = false;
            if (record.size() != 4 + chests.size() * static_cast<size_t>(kChestBytes)) chestRecordsOk = false;
            for (const auto& entry : chests) {
                bool found = false;
                for (size_t i = 0; i < chests.size() && !found; i++) {
                    bool same = true;
                    for (int b = 0; b < kChestBytes; b++) {
                        if (record[4 + i * static_cast<size_t>(kChestBytes) + static_cast<size_t>(b)] !=
                            entry.second[static_cast<size_t>(b)]) {
                            same = false;
                        }
                    }
                    if (same) found = true;
                }
                if (!found) chestRecordsOk = false;
            }
        }
        Check(chestRecordsOk, "every chest record should hold exactly that level's own 8-byte chest records verbatim");

        // Dropped-item records: ids 74..110 (levels 0..36), 7 bytes each --
        // ORDER-sensitive, unlike the two keyed registries above (a Vector in
        // the original, a std::vector here).
        bool droppedRecordsOk = true;
        for (int level = 0; level < kLevels; level++) {
            const std::vector<uint8_t>& record = store[static_cast<size_t>(73 + level)];
            const auto& dropped = world.droppedItems[static_cast<size_t>(level)];
            if (ReadS32At(record, 0) != static_cast<int32_t>(dropped.size())) droppedRecordsOk = false;
            if (record.size() != 4 + dropped.size() * static_cast<size_t>(kDroppedBytes)) droppedRecordsOk = false;
            for (size_t i = 0; i < dropped.size(); i++) {
                for (int b = 0; b < kDroppedBytes; b++) {
                    if (record[4 + i * static_cast<size_t>(kDroppedBytes) + static_cast<size_t>(b)] !=
                        dropped[i][static_cast<size_t>(b)]) {
                        droppedRecordsOk = false;
                    }
                }
            }
        }
        Check(droppedRecordsOk,
              "every dropped-item record should hold that level's own 7-byte records in registration order");

        // Reading the master lists back: the returned record id is how
        // loadGameState() finds the other-state record at all.
        WorldRegistry reloaded(static_cast<size_t>(kLevels));
        std::vector<int> readPercents;
        int nextRecord = GameSave::ReadMasterListRecords(store, 2, reloaded, [&](int p) { readPercents.push_back(p); });
        Check(nextRecord == 111,
              "readMasterListRecords should return the id of the record after the master lists (111) -- the "
              "other-state record");
        Check(readPercents == ExpectedMasterListPercents(),
              "the load's own percent sequence should be the same 109 values the save reports");
        Check(RegistryEquals(reloaded, world),
              "reading the master lists back should restore every level's monsters/chests/dropped items exactly");

        // Monster.store()'s own keying quirk: a monster is stored by its OWN
        // dungeonLevel field (`monsters[this.dungeonLevel - 1]`), NOT by the
        // record it was read from -- so a record whose monster claims a
        // different level puts it there, while its own record's level is
        // merely cleared.
        {
            MonsterState m{};
            m.spawnId = 77;
            m.monsterType = 3;
            m.hp = 12;
            m.x = 4;
            m.y = 9;
            m.dungeonLevel = 2;  // claims level 2...
            std::array<uint8_t, kMonsterBytes> monsterBytes = MonsterRuntime::ToBytes(m);
            Check(MonsterRuntime::FromBytes(monsterBytes).dungeonLevel == 2,
                  "ToBytes/FromBytes should carry dungeonLevel (byte 7 of Monster.writeTo's own 28-byte layout)");
            std::vector<uint8_t> engineered;
            PutS32(engineered, 1);
            engineered.insert(engineered.end(), monsterBytes.begin(), monsterBytes.end());

            // A store whose LEVEL 5 monster record (id 6) holds that monster,
            // every other master-list record empty. Level 2's own record (id 3)
            // is read first, so monsters[1] is cleared before the monster lands
            // there and is never cleared again -- the relocation survives.
            WorldRegistry emptyWorld(static_cast<size_t>(kLevels));
            GameSave::RecordStore quirkStore;
            quirkStore.push_back(dawnstar::PlayerSave::ToBytes(player));
            GameSave::WriteMasterListsToRecordStore(emptyWorld, quirkStore, nullptr);
            std::vector<uint8_t> emptyRecord = quirkStore[5];
            quirkStore[5] = engineered;  // record id 6 == level 5

            WorldRegistry quirkWorld(static_cast<size_t>(kLevels));
            GameSave::ReadMasterListRecords(quirkStore, 2, quirkWorld, nullptr);
            Check(quirkWorld.monsters[1].size() == 1,
                  "...but read from level 5's record, so it should land at monsters[1], exactly as Monster.store() keys it");
            Check(quirkWorld.monsters[5].empty(),
                  "level 5's own registry -- the record it came from -- should have been cleared, not written to");
            Check(quirkWorld.monsters[1].count(static_cast<int>(dawnstar::PackPosKey(4, 9))) == 1,
                  "the relocated monster should be keyed by its own x/y via Util.posKey");

            // The same relocation in the OTHER direction is self-clobbering, and
            // the original is too: a monster in level 2's record claiming level 5
            // is stored at monsters[4], which level 5's own later record then
            // clears. Reproduced, not "fixed".
            MonsterState up = m;
            up.dungeonLevel = 5;
            std::array<uint8_t, kMonsterBytes> upBytes = MonsterRuntime::ToBytes(up);
            std::vector<uint8_t> upRecord;
            PutS32(upRecord, 1);
            upRecord.insert(upRecord.end(), upBytes.begin(), upBytes.end());
            GameSave::RecordStore upStore = quirkStore;
            upStore[5] = emptyRecord;
            upStore[2] = upRecord;  // record id 3 == level 2
            WorldRegistry upWorld(static_cast<size_t>(kLevels));
            GameSave::ReadMasterListRecords(upStore, 2, upWorld, nullptr);
            Check(upWorld.monsters[4].empty(),
                  "a monster relocated to a HIGHER level than its own record is wiped by that level's own later "
                  "clear() -- the original's own behavior too");
            Check(upWorld.monsters[1].empty(), "...and nothing is left at its own record's level either");

            // An out-of-range dungeonLevel is the original's own
            // ArrayIndexOutOfBoundsException -- a hard failure, not a skip.
            for (int8_t badLevel : {static_cast<int8_t>(0), static_cast<int8_t>(38), static_cast<int8_t>(-1)}) {
                std::vector<uint8_t> bad = engineered;
                bad[4 + 7] = static_cast<uint8_t>(badLevel);  // byte 7 == dungeonLevel
                GameSave::RecordStore badStore = quirkStore;
                badStore[5] = bad;
                WorldRegistry badWorld(static_cast<size_t>(kLevels));
                Check(Throws([&] { GameSave::ReadMasterListRecords(badStore, 2, badWorld, nullptr); }),
                      "a monster whose dungeonLevel is outside 1..37 should throw, failing the whole load");
            }
        }

        // --- D: saveGameState()/loadGameState() end to end, through the real
        // file substitute. ---
        OtherStateInfo liveOther = OtherStateInfo::Reset();
        liveOther.itemNextSpawnId = 4242;
        liveOther.monsterNextSpawnIdCounter = 909;
        liveOther.shopFirstVisit[3] = false;
        liveOther.shopInteractionCount[1] = 7;
        liveOther.shopRewardsGiven[2] = -3;
        liveOther.shopQuestState1[0] = 2;
        liveOther.shopQuestState2[3] = -1;
        liveOther.shopShowDeathGreeting = true;

        std::vector<int> savePercents;
        Check(GameSave::SaveGameState(saveDir, player, world, liveOther, rng, [&](int p) { savePercents.push_back(p); }),
              "saving a real character in a real world should succeed");
        std::vector<int> expectedSavePercents{20};
        for (int p : ExpectedMasterListPercents()) expectedSavePercents.push_back(p);
        expectedSavePercents.push_back(100);
        Check(savePercents == expectedSavePercents,
              "saveGameState should report 20, then the 109 master-list values, then 100 -- and never 0, since the "
              "original's own percent=0 assignment is the one it never repaints");

        std::vector<std::string> files;
        for (const auto& entry : std::filesystem::directory_iterator(saveDir)) {
            files.push_back(entry.path().filename().string());
        }
        Check(files.size() == 1, "a successful save should leave exactly one store file behind");
        Check(!files.empty() && files[0].rfind(GameSave::kStoreNamePrefix, 0) == 0,
              "the store's own name should be the original's own es_gamestate<N> literal, with no added extension");
        int nameSuffix = std::stoi(files[0].substr(std::string(GameSave::kStoreNamePrefix).size()));
        Check(nameSuffix >= 1 && nameSuffix <= 10000,
              "the name's own number should be Util.randomInt(10000) -- 1..10000 inclusive, Lingo's own 1-based idiom");

        GameSave::RecordStore fileStore = GameSave::DeserializeRecordStore(ReadFile(saveDir + "/" + files[0]));
        Check(fileStore.size() == 111, "the file should hold exactly the 111 records the save built");
        Check(fileStore[0] == dawnstar::PlayerSave::ToBytes(player),
              "record 1 should be the character's own M12 full-save bytes");
        Check(fileStore[110] == GameSave::WriteOtherStateInfoToBytes(liveOther),
              "record 111 should be the other-state record (its own byte layout verified independently in section A)");

        PlayerState loadedPlayer;
        loadedPlayer.name = "STALE";
        loadedPlayer.gold = -1;
        WorldRegistry loadedWorld(static_cast<size_t>(kLevels));
        OtherStateInfo loadedOther;  // NOT Reset(): a partial read would show
        std::vector<int> loadPercents;
        Check(GameSave::LoadGameState(saveDir, loadedPlayer, loadedWorld, loadedOther,
                                      [&](int p) { loadPercents.push_back(p); }),
              "loading the save just written should succeed");
        std::vector<int> expectedLoadPercents{20};
        for (int p : ExpectedMasterListPercents()) expectedLoadPercents.push_back(p);
        Check(loadPercents == expectedLoadPercents,
              "loadGameState should report 20 then the 109 master-list values -- the 100 comes from run()'s own tail, "
              "after resumeGame(), not from loadGameState itself");
        Check(dawnstar::PlayerSave::ToBytes(loadedPlayer) == dawnstar::PlayerSave::ToBytes(player),
              "the loaded character should be byte-identical to the saved one through M12's own full-save format");
        Check(loadedPlayer.name == "M42 Tester" && loadedPlayer.gold == 4321 && loadedPlayer.currentLevel == 7 &&
                  loadedPlayer.giftPointsFound == 41,
              "the loaded character's own fields should be the saved ones, not the stale placeholders");
        Check(RegistryEquals(loadedWorld, world), "the loaded world registries should match the saved ones exactly");
        Check(OtherStateEquals(loadedOther, liveOther), "the loaded other-state record should match what was saved");

        // resumeGame() -> openAndRepopulateDungeons(getGameAdvancementLevel(
        // giftPointsFound)), called by run()'s own tail rather than from inside
        // loadGameState().
        std::vector<int> repopulatePercents;
        GameSave::ResumeGame(loadedPlayer, levels, loadedWorld, [&](int p) { repopulatePercents.push_back(p); });
        Check(repopulatePercents == ExpectedRepopulatePercents(),
              "openAndRepopulateDungeons should report 100*(i+1)/37 clamped at 100, once per level");
        Check(repopulatePercents.size() == static_cast<size_t>(kLevels) && repopulatePercents.back() == 100,
              "the repopulate pass should cover all 37 levels and end at 100 percent");

        // A second save replaces the first: cleanupRecordStores() keeps exactly
        // the newest store, so this game has ONE save slot.
        std::string firstSaveName = files[0];
        player.gold = 5555;
        Check(GameSave::SaveGameState(saveDir, player, world, liveOther, rng, nullptr), "a second save should succeed");
        files.clear();
        for (const auto& entry : std::filesystem::directory_iterator(saveDir)) {
            files.push_back(entry.path().filename().string());
        }
        Check(files.size() == 1, "cleanupRecordStores should leave exactly ONE store -- the newest");
        Check(files[0] != firstSaveName,
              "the second save's own name should differ: getRSNameNotInUse re-rolls past any name already in use");
        Check(GameSave::GetLastGoodRsName(saveDir) == files[0], "getLastGoodRSName should pick the newest store");
        PlayerState secondLoad;
        Check(GameSave::LoadGameState(saveDir, secondLoad, loadedWorld, loadedOther, nullptr),
              "loading after the second save should succeed");
        Check(secondLoad.gold == 5555, "...and should return the SECOND character, not the first");

        // Failure paths -- the original's single catch-all, each returning false.
        std::string emptyDir = saveDir + "_empty";
        std::filesystem::remove_all(emptyDir, ec);
        std::filesystem::create_directories(emptyDir);
        PlayerState noSavePlayer;
        WorldRegistry noSaveWorld(static_cast<size_t>(kLevels));
        OtherStateInfo noSaveOther;
        Check(!GameSave::LoadGameState(emptyDir, noSavePlayer, noSaveWorld, noSaveOther, nullptr),
              "loading with no store present should fail (getLastGoodRSName's own null -> \"No valid record store!\")");
        Check(!GameSave::GetLastGoodRsName(emptyDir), "getLastGoodRSName on an empty directory should return nullopt");
        Check(!GameSave::GetLastGoodRsName(saveDir + "_missing"),
              "a directory that does not exist at all should behave like an empty one, not throw");

        std::string savePath = saveDir + "/" + files[0];
        std::vector<uint8_t> wholeFile = ReadFile(savePath);
        WriteFile(savePath, std::vector<uint8_t>(wholeFile.begin(), wholeFile.begin() + 10));
        PlayerState truncatedPlayer;
        WorldRegistry truncatedWorld(static_cast<size_t>(kLevels));
        OtherStateInfo truncatedOther;
        Check(!GameSave::LoadGameState(saveDir, truncatedPlayer, truncatedWorld, truncatedOther, nullptr),
              "a truncated store file should fail the load rather than half-parse silently");
        WriteFile(savePath, wholeFile);

        Check(!GameSave::SaveGameState(savePath, player, world, liveOther, rng, nullptr),
              "saving into a path that is an existing FILE should fail (create_directories throws)");

        // The original's own real ordering quirk: `this.character =
        // Player.fromBytes(var6, true)` runs BEFORE readMasterListRecords, so a
        // load that fails partway through the master lists has ALREADY replaced
        // the live character -- reproduced deliberately, not "fixed".
        {
            // Walk the file's own length-prefixed records to find the first
            // monster record (ids 2..37) that actually holds a monster.
            size_t offset = 0;
            int recordId = 0;
            size_t corruptAt = 0;
            int corruptLevel = -1;
            while (offset + 4 <= wholeFile.size()) {
                int32_t len = ReadS32At(wholeFile, offset);
                if (len < 0 || offset + 4 + static_cast<size_t>(len) > wholeFile.size()) break;
                recordId++;
                if (corruptLevel < 0 && recordId >= 2 && recordId <= 37 && ReadS32At(wholeFile, offset + 4) > 0) {
                    // Its own count prefix is at offset+4, the first monster at
                    // offset+8, and dungeonLevel is byte 7 of that monster.
                    corruptAt = offset + 8 + 7;
                    corruptLevel = recordId - 1;
                }
                offset += 4 + static_cast<size_t>(len);
            }
            Check(corruptLevel >= 0, "the real world should have at least one monster record to corrupt");
            Check(wholeFile[corruptAt] >= 1 && wholeFile[corruptAt] <= 37,
                  "that byte should hold a valid dungeonLevel before it is corrupted");

            std::vector<uint8_t> corruptFile = wholeFile;
            corruptFile[corruptAt] = 0;  // -> monsters[-1]: the original's own ArrayIndexOutOfBoundsException
            WriteFile(savePath, corruptFile);

            PlayerState swapPlayer;
            swapPlayer.name = "STALE";
            swapPlayer.gold = -1;
            WorldRegistry swapWorld(static_cast<size_t>(kLevels));
            OtherStateInfo swapOther;
            swapOther.itemNextSpawnId = 1234;
            swapOther.shopShowDeathGreeting = true;
            Check(!GameSave::LoadGameState(saveDir, swapPlayer, swapWorld, swapOther, nullptr),
                  "a corrupt monster dungeonLevel should fail the load");
            Check(swapPlayer.name == player.name && swapPlayer.gold == 5555,
                  "...but the live character should ALREADY have been replaced -- the original's own real ordering "
                  "(character first, master lists second)");
            Check(swapOther.itemNextSpawnId == 1234 && swapOther.shopShowDeathGreeting,
                  "the other-state record is read LAST, so a failed load leaves the caller's own values untouched");
            Check(swapWorld.monsters[static_cast<size_t>(corruptLevel)].empty(),
                  "the failing record's own level should still have been cleared before the throw");
            WriteFile(savePath, wholeFile);
        }

        // --- E: getRSNameNotInUse()'s own re-roll, against a twin JavaRandom at
        // the same seed so the expected names come from Lingo's own sequence
        // rather than from the implementation. ---
        std::string nameDir = saveDir + "_names";
        std::filesystem::remove_all(nameDir, ec);
        std::filesystem::create_directories(nameDir);
        JavaRandom nameRng(987654321);
        JavaRandom twinRng(987654321);
        std::string expectedName = "es_gamestate" + std::to_string(dawnstar::LingoRandomInt(twinRng, 10000));
        Check(GameSave::GetRsNameNotInUse(nameDir, nameRng) == expectedName,
              "the store name should be \"es_gamestate\" + Util.randomInt(10000), drawn from ESGame.r's own sequence");
        WriteFile(nameDir + "/" + expectedName, std::vector<uint8_t>());
        std::string rerolledName;
        while (true) {
            std::string candidate = "es_gamestate" + std::to_string(dawnstar::LingoRandomInt(twinRng, 10000));
            if (candidate != expectedName) {
                rerolledName = candidate;
                break;
            }
        }
        Check(GameSave::GetRsNameNotInUse(nameDir, nameRng) == rerolledName,
              "a name already in use should be re-rolled past, continuing the same Random sequence");

        // --- F: the original's own hardcoded 1500-byte read buffer
        // (maxRecordSize()'s `return 1500`, which sizes readMasterListRecords'
        // own fixed byte[]), MEASURED against the real world rather than
        // assumed. Deliberately not reproduced -- see save/game_save.h's own
        // class comment on the RecordStore.getRecord(id, buffer, 0) overflow it
        // causes -- but measured here so that decision rests on a fact. ---
        int largestMonsterRecord = 0;
        int largestChestRecord = 0;
        int largestDroppedRecord = 0;
        for (int level = 1; level < kLevels; level++) {
            largestMonsterRecord =
                std::max(largestMonsterRecord,
                         4 + kMonsterBytes * static_cast<int>(world.monsters[static_cast<size_t>(level)].size()));
            largestChestRecord = std::max(
                largestChestRecord, 4 + kChestBytes * static_cast<int>(world.chests[static_cast<size_t>(level)].size()));
        }
        for (int level = 0; level < kLevels; level++) {
            largestDroppedRecord =
                std::max(largestDroppedRecord,
                         4 + kDroppedBytes * static_cast<int>(world.droppedItems[static_cast<size_t>(level)].size()));
        }
        int largestFileRecord = 0;
        for (size_t i = 1; i < fileStore.size(); i++) {
            largestFileRecord = std::max(largestFileRecord, static_cast<int>(fileStore[i].size()));
        }
        Check(largestFileRecord == std::max(largestMonsterRecord, std::max(largestChestRecord, largestDroppedRecord)),
              "the largest record actually written should equal the largest computed per-level record size");
        // The boundary itself, independent of any particular world's contents:
        // 4 + 53*28 == 1488 fits, 4 + 54*28 == 1516 does not.
        Check(4 + 53 * kMonsterBytes <= kOriginalReadBuffer && 4 + 54 * kMonsterBytes > kOriginalReadBuffer,
              "the original's 1500-byte buffer holds at most 53 monsters per level -- 54+ would make its own load throw");
        std::printf("  largest real records: monster %d, chest %d, dropped-item %d bytes (original's buffer %d)\n",
                    largestMonsterRecord, largestChestRecord, largestDroppedRecord, kOriginalReadBuffer);
        std::printf("  -> the real world's largest record %s that buffer, so the original %s\n",
                    largestFileRecord > kOriginalReadBuffer ? "EXCEEDS" : "fits inside",
                    largestFileRecord > kOriginalReadBuffer
                        ? "could never load this save back"
                        : "can load this save (overflow needs 54+ monsters on one level)");

        // --- G: LoadingScreen's own renderProgress() pixel layout, all four
        // modes x three percents. ---
        // 2510210 decimal == 0x264D82 == RGB(38, 77, 130) -- the same blue
        // Screen.java's own render methods use (see ui/screen.cpp's own
        // kBackgroundColor, derived from the very same literal).
        constexpr uint16_t kLoadingBackground = dawnstar::PackRGB565(38, 77, 130);
        constexpr uint16_t kWhite = dawnstar::PackRGB565(255, 255, 255);
        constexpr uint16_t kBarBlue = dawnstar::PackRGB565(0, 0, 255);              // g.setColor(255)
        const LoadingScreenMode loadingModes[4] = {LoadingScreenMode::CreatingNewGame, LoadingScreenMode::LoadingGame,
                                                   LoadingScreenMode::SavingGame, LoadingScreenMode::LoadingDungeon};
        const char* loadingTitles[4] = {"Creating New Game", "Loading Game", "Saving Game", "Loading Dungeon"};
        for (int i = 0; i < 4; i++) {
            LoadingScreen loading(loadingModes[i]);
            Check(loading.Percent() == 0, "a fresh LoadingScreen should start at 0 percent");
            for (int percent : {0, 37, 100}) {
                loading.SetPercent(percent);
                Check(loading.Percent() == percent, "SetPercent should store the percent unmodified (no clamp)");
                Backbuffer bb;
                bb.Fill(0);
                loading.Render(bb);
                Check(PixelAt(bb, 0, 0) == kLoadingBackground && PixelAt(bb, 175, 207) == kLoadingBackground &&
                          PixelAt(bb, 88, 100) == kLoadingBackground,
                      "renderProgress should fill the whole 176x208 screen with 2510210");
                Check(TextRenderedAt(bb, Screen::width() / 2 - BitmapFont::StringWidth(loadingTitles[i]) / 2, 30,
                                     loadingTitles[i], kWhite),
                      "the mode's own action line should be white and centered at y=30");
                Check(TextRenderedAt(bb, Screen::width() / 2 - BitmapFont::StringWidth("Please Wait") / 2, 45,
                                     "Please Wait", kWhite),
                      "\"Please Wait\" should be white and centered at y=45");
                bool boxOk = true;
                for (int x = 43; x < 43 + 90; x++) {
                    if (PixelAt(bb, x, 60) != kWhite || PixelAt(bb, x, 79) != kWhite) boxOk = false;
                }
                for (int y = 60; y < 80; y++) {
                    if (PixelAt(bb, 43, y) != kWhite || PixelAt(bb, 132, y) != kWhite) boxOk = false;
                }
                Check(boxOk, "the bar's own outline box should be white, 90x20 at ((176-90)/2, 60)");
                int barWidth = percent * 88 / 100;
                bool barOk = true;
                for (int x = 44; x < 44 + barWidth; x++) {
                    for (int y = 61; y < 79; y++) {
                        if (PixelAt(bb, x, y) != kBarBlue) barOk = false;
                    }
                }
                Check(barOk, "the bar should be pure blue, percent*88/100 wide and 18 tall, inset 1px inside the box");
                if (barWidth < 88) {
                    Check(PixelAt(bb, 44 + barWidth, 70) == kWhite,
                          "past the bar's own right edge the box's own white should still show");
                }
            }
        }
        Check(37 * 88 / 100 == 32, "37 percent should draw a 32px-wide bar (integer division, as the original does)");

        // --- H: the Options menu's now-real "Save Game"/"Load Game" actions and
        // their two failure screens (secondaryParam==31's own case 5/6, plus
        // run()'s own two else-branches). ---
        OptionsMenu menu(helpText, shopDialogue);
        for (int i = 0; i < 10; i++) menu.OnUp();
        for (int i = 0; i < 5; i++) menu.OnDown();  // index 5 == "Save Game"
        Check(menu.OnSelect(player, charData, items, spells, levels, world) == OptionsMenuAction::SaveGame,
              "\"Save Game\" (secondaryParam==31's own case 5) should hand the real save back to main.cpp");
        Backbuffer optionsBb;
        optionsBb.Fill(0);
        menu.Render(optionsBb);
        Check(TextRenderedAt(optionsBb, Screen::width() / 2 - BitmapFont::StringWidth("Options") / 2, 0, "Options",
                             kWhite),
              "...and should leave the Options list itself showing -- the LoadingScreen swap is ESGame's own job");
        for (int i = 0; i < 10; i++) menu.OnUp();
        for (int i = 0; i < 6; i++) menu.OnDown();  // index 6 == "Load Game"
        Check(menu.OnSelect(player, charData, items, spells, levels, world) == OptionsMenuAction::LoadGame,
              "\"Load Game\" (case 6) should hand the real load back to main.cpp");

        // The Save Error screen: GenericInfoUI secondaryParam 499 -> exit().
        menu.ShowSaveError();
        Backbuffer errorBb;
        errorBb.Fill(0);
        menu.Render(errorBb);
        Check(TextRenderedAt(errorBb, Screen::width() / 2 - BitmapFont::StringWidth("Save Error") / 2, 0, "Save Error",
                             kWhite),
              "the failed-save screen should show the real \"Save Error\" title on the shared info Screen");
        Check(menu.OnCancel() == OptionsMenuAction::None,
              "a mode-4 message screen has no Cancel command at all -- Cancel is a real no-op");
        Check(menu.OnSelect(player, charData, items, spells, levels, world) == OptionsMenuAction::Exit,
              "Ok on \"Save Error\" should EXIT the game: secondaryParam==499's own dispatch is an unconditional exit()");

        // The "Unavailable" screen: secondaryParam 305 -> back to OptionsUI.
        menu.ShowNoSavedGame();
        Backbuffer noSaveBb;
        noSaveBb.Fill(0);
        menu.Render(noSaveBb);
        Check(TextRenderedAt(noSaveBb, Screen::width() / 2 - BitmapFont::StringWidth("Unavailable") / 2, 0,
                             "Unavailable", kWhite),
              "the failed-load screen should show the real \"Unavailable\" title on its own separate Screen");
        Check(menu.OnCancel() == OptionsMenuAction::None, "its own mode-4 Cancel is a real no-op too");
        Check(menu.OnSelect(player, charData, items, spells, levels, world) == OptionsMenuAction::None,
              "Ok on \"Unavailable\" is not a main.cpp-level action...");
        Backbuffer backBb;
        backBb.Fill(0);
        menu.Render(backBb);
        Check(TextRenderedAt(backBb, Screen::width() / 2 - BitmapFont::StringWidth("Options") / 2, 0, "Options", kWhite),
              "...it returns to the Options menu (case 6's own backTarget), despite the message's own text claiming "
              "the main menu");

        std::filesystem::remove_all(saveDir, ec);
        std::filesystem::remove_all(emptyDir, ec);
        std::filesystem::remove_all(nameDir, ec);
    } catch (const std::exception& e) {
        std::printf("  EXCEPTION: %s\n", e.what());
        g_ok = false;
    }

    std::printf(g_ok ? "M42 game-save smoke: OK\n" : "M42 game-save smoke: FAILURES\n");
    return g_ok ? 0 : 1;
}
