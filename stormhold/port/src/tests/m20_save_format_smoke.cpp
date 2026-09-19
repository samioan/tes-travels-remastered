// M20 smoke test: assets/binary_writer.h (new this milestone),
// monster/monster_runtime.h's ReadFrom/WriteTo (Monster's second stream
// serialization), and player/player_save.h (Player's own full=true save
// format) -- against real created characters, not just synthetic structs.
#include <cstdio>
#include <sstream>
#include <string>

#include "assets/asset_root.h"
#include "assets/binary_reader.h"
#include "assets/binary_writer.h"
#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "monster/monster_runtime.h"
#include "player/player_creation.h"
#include "player/player_save.h"

namespace {

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

void TestBinaryWriterReaderRoundTrip() {
    std::printf("-- BinaryWriter/BinaryReader round trip --\n");
    std::ostringstream obuf;
    stormhold::BinaryWriter out(obuf);

    out.WriteU8(0xFF);
    out.WriteS8(-1);
    out.WriteU16(0xBEEF);
    out.WriteS16(-12345);
    out.WriteU32(0xDEADBEEFu);
    out.WriteS32(-2000000000);
    out.WriteBool(true);
    out.WriteBool(false);
    out.WriteUTF("Stormhold");

    std::istringstream ibuf(obuf.str());
    stormhold::BinaryReader in(ibuf);
    Expect(in.ReadU8() == 0xFF, "WriteU8/ReadU8 should round-trip");
    Expect(in.ReadS8() == -1, "WriteS8/ReadS8 should round-trip a negative byte");
    Expect(in.ReadU16() == 0xBEEF, "WriteU16/ReadU16 should round-trip");
    Expect(in.ReadS16() == -12345, "WriteS16/ReadS16 should round-trip a negative short");
    Expect(in.ReadU32() == 0xDEADBEEFu, "WriteU32/ReadU32 should round-trip");
    Expect(in.ReadS32() == -2000000000, "WriteS32/ReadS32 should round-trip a large negative int");
    Expect(in.ReadU8() != 0, "WriteBool(true) should round-trip");
    Expect(in.ReadU8() == 0, "WriteBool(false) should round-trip");
    Expect(in.ReadUTF() == "Stormhold", "WriteUTF/ReadUTF should round-trip");
}

void TestMonsterReadFromWriteToMatchesToBytesFromBytes() {
    std::printf("-- Monster ReadFrom/WriteTo produce the EXACT SAME 28 bytes as ToBytes/FromBytes --\n");
    stormhold::MonsterState m;
    m.spawnId = -12345;
    m.typeIndex = 5;
    m.currentHp = -3;
    m.tileX = 17;
    m.tileY = 29;
    m.unconfirmedFlag = true;
    m.dungeonLevel = 9;
    m.chaseCadence = 3;
    m.aiPhase = 2;
    m.unconfirmedTimestamp = 1234567890123LL;
    for (int i = 0; i < 10; i++) m.scratch[static_cast<size_t>(i)] = static_cast<int8_t>(i - 5);

    std::array<uint8_t, 28> packed = stormhold::MonsterRuntime::ToBytes(m);

    std::ostringstream obuf;
    stormhold::BinaryWriter out(obuf);
    stormhold::MonsterRuntime::WriteTo(out, m);
    std::string streamed = obuf.str();

    Expect(streamed.size() == 28, "WriteTo should produce exactly 28 bytes, same as the packed record");
    bool identical = true;
    for (size_t i = 0; i < 28 && i < streamed.size(); i++) {
        if (static_cast<uint8_t>(streamed[i]) != packed[i]) identical = false;
    }
    Expect(identical, "WriteTo's byte stream should be BYTE-FOR-BYTE identical to ToBytes()'s packed array -- "
                      "confirming Stormhold's readFrom/writeTo encode the exact same 28 fields in the exact "
                      "same order, unlike dawnstar's own Monster");

    std::istringstream ibuf(streamed);
    stormhold::BinaryReader in(ibuf);
    stormhold::MonsterState back = stormhold::MonsterRuntime::ReadFrom(in);
    Expect(back.spawnId == m.spawnId, "ReadFrom should round-trip spawnId, including negative values");
    Expect(back.typeIndex == m.typeIndex, "ReadFrom should round-trip typeIndex");
    Expect(back.currentHp == m.currentHp, "ReadFrom should round-trip a negative currentHp");
    Expect(back.tileX == m.tileX && back.tileY == m.tileY, "ReadFrom should round-trip position");
    Expect(back.unconfirmedFlag == m.unconfirmedFlag, "ReadFrom should round-trip unconfirmedFlag");
    Expect(back.dungeonLevel == m.dungeonLevel, "ReadFrom should round-trip dungeonLevel");
    Expect(back.chaseCadence == m.chaseCadence, "ReadFrom should round-trip chaseCadence");
    Expect(back.aiPhase == m.aiPhase, "ReadFrom should round-trip aiPhase");
    Expect(back.unconfirmedTimestamp == m.unconfirmedTimestamp, "ReadFrom should round-trip a large 64-bit timestamp");
    Expect(back.scratch == m.scratch, "ReadFrom should round-trip scratch[10]");
}

void TestPlayerSaveFullRoundTrip(const stormhold::CharacterData& charData, const stormhold::ItemDatabase& items) {
    std::printf("-- PlayerSave full-format round trip against a real created character --\n");
    stormhold::PlayerState p = stormhold::PlayerCreation::CreateCharacter(2, "Ser Aldric", 42, charData, items);

    // Mutate every field the full save format actually touches to a
    // varied, non-default value.
    p.classIndex = 2;
    p.raceIndex = 4;
    p.coreStats = {5, 120, -30, 40, 0, 55, 70, 80, -1, 2};
    p.levelUpAttributeFlags = 0x2A;
    p.unconfirmedIntField = -123456789;
    for (size_t i = 0; i < p.attributes.size(); i++) p.attributes[i] = static_cast<int16_t>(i * 3 - 10);
    p.classMagickaFactor = 7;
    p.classUnknownPair = {11, -22};
    for (size_t i = 0; i < p.skills.size(); i++) {
        p.skills[i][0] = static_cast<int16_t>(i + 1);
        p.skills[i][1] = static_cast<int16_t>(i + 2);
        p.skills[i][2] = static_cast<int16_t>(i + 3);
    }
    p.inventoryCount = 3;
    p.inventoryItemIds = {-1, 27, 50};
    p.inventoryItemData[0] = 70000;
    p.inventoryItemData[1] = -5;
    p.equippedItems = {1, 0, 0, 0, 0, 0, 27};
    p.knownSpellsMask = 0x108421;
    p.selectedSpellId = 6;
    p.giftPointsFound = 17;
    p.rumorRevealStep = 3;
    p.wardenLoreStep = 2;
    p.ailmentMask = 0x15;
    p.vampirismTimer = 12345;
    p.manaBurnTimer = -1;
    p.terrifiedTimer = 999;
    p.unconfirmedFlag2 = true;
    p.currentLevel = 14;
    p.tileX = 22;
    p.tileY = 5;
    p.facing = 3;
    p.campLevel = 9;
    p.campX = 1;
    p.campY = 2;
    p.campFacing = 4;
    for (size_t i = 0; i < p.effectDurations.size(); i++) p.effectDurations[i] = static_cast<int8_t>(i - 12);
    p.lastCombatTargetId = -7;
    p.spellArmorBonus = 33;
    p.increaseHarmBuff = true;
    p.increaseArmorBuff = false;
    p.safeCampingBuff = true;

    // Also vary the transient fields the save format should NOT carry --
    // if FromBytes ever accidentally round-trips one of these, it means
    // this milestone over-serialized something.
    p.pendingLevel = 30;
    p.pendingTileX = 11;
    p.pendingTileY = 12;
    p.pendingFacing = 2;
    p.prevTileX = 8;
    p.prevTileY = 9;
    p.crossingLevelBoundary = true;
    p.enteredNewLevelZone = true;
    p.leftLevelZone = true;
    p.justMarkedCamp = true;
    p.pendingLockedItemFlag = true;

    std::vector<uint8_t> bytes = stormhold::PlayerSave::ToBytes(p);
    stormhold::PlayerState back = stormhold::PlayerSave::FromBytes(bytes);

    Expect(back.name == p.name, "name should round-trip");
    Expect(back.classIndex == p.classIndex, "classIndex should round-trip");
    Expect(back.raceIndex == p.raceIndex, "raceIndex should round-trip");
    Expect(back.coreStats == p.coreStats, "coreStats should round-trip, including negative values");
    Expect(back.levelUpAttributeFlags == p.levelUpAttributeFlags, "levelUpAttributeFlags should round-trip");
    Expect(back.unconfirmedIntField == p.unconfirmedIntField, "unconfirmedIntField should round-trip a large negative int");
    Expect(back.attributes == p.attributes, "attributes should round-trip");
    Expect(back.classMagickaFactor == p.classMagickaFactor, "classMagickaFactor should round-trip");
    Expect(back.classUnknownPair == p.classUnknownPair, "classUnknownPair should round-trip");
    Expect(back.skills == p.skills, "skills should round-trip");
    Expect(back.inventoryCount == p.inventoryCount, "inventoryCount should round-trip");
    Expect(back.inventoryItemIds == p.inventoryItemIds, "inventoryItemIds should round-trip");
    Expect(back.inventoryItemData == p.inventoryItemData, "inventoryItemData should round-trip");
    Expect(back.equippedItems == p.equippedItems, "equippedItems should round-trip");
    Expect(back.knownSpellsMask == p.knownSpellsMask, "knownSpellsMask should round-trip");
    Expect(back.selectedSpellId == p.selectedSpellId, "selectedSpellId should round-trip");
    Expect(back.giftPointsFound == p.giftPointsFound, "giftPointsFound should round-trip");
    Expect(back.rumorRevealStep == p.rumorRevealStep, "rumorRevealStep should round-trip");
    Expect(back.wardenLoreStep == p.wardenLoreStep, "wardenLoreStep should round-trip");
    Expect(back.ailmentMask == p.ailmentMask, "ailmentMask should round-trip");
    Expect(back.vampirismTimer == p.vampirismTimer, "vampirismTimer should round-trip");
    Expect(back.manaBurnTimer == p.manaBurnTimer, "manaBurnTimer should round-trip a negative value");
    Expect(back.terrifiedTimer == p.terrifiedTimer, "terrifiedTimer should round-trip");
    Expect(back.unconfirmedFlag2 == p.unconfirmedFlag2, "unconfirmedFlag2 should round-trip");
    Expect(back.currentLevel == p.currentLevel, "currentLevel should round-trip");
    Expect(back.tileX == p.tileX && back.tileY == p.tileY, "tileX/tileY should round-trip");
    Expect(back.facing == p.facing, "facing should round-trip");
    Expect(back.campLevel == p.campLevel && back.campX == p.campX && back.campY == p.campY &&
               back.campFacing == p.campFacing,
           "the camp bookmark should round-trip");
    Expect(back.effectDurations == p.effectDurations, "effectDurations should round-trip");
    Expect(back.lastCombatTargetId == p.lastCombatTargetId, "lastCombatTargetId should round-trip a negative value");
    Expect(back.spellArmorBonus == p.spellArmorBonus, "spellArmorBonus should round-trip");
    Expect(back.increaseHarmBuff == p.increaseHarmBuff, "increaseHarmBuff should round-trip");
    Expect(back.increaseArmorBuff == p.increaseArmorBuff, "increaseArmorBuff should round-trip");
    Expect(back.safeCampingBuff == p.safeCampingBuff, "safeCampingBuff should round-trip");

    // The full save format does NOT carry any of these -- FromBytes
    // should leave them at PlayerState's own default member initializers
    // regardless of what the original had.
    Expect(back.pendingLevel == 1, "pendingLevel is transient, NOT part of the save format -- should be the default");
    Expect(back.pendingTileX == 9 && back.pendingTileY == 10, "pendingTileX/Y should be the default, not carried");
    Expect(back.pendingFacing == 1, "pendingFacing should be the default, not carried");
    Expect(back.prevTileX == 0 && back.prevTileY == 0, "prevTileX/Y should be the default, not carried");
    Expect(!back.crossingLevelBoundary, "crossingLevelBoundary should be the default, not carried");
    Expect(!back.enteredNewLevelZone && !back.leftLevelZone, "enteredNewLevelZone/leftLevelZone should be the default");
    Expect(!back.justMarkedCamp, "justMarkedCamp should be the default, not carried");
    Expect(!back.pendingLockedItemFlag, "pendingLockedItemFlag should be the default, not carried");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        stormhold::AssetRoot assets(root);
        stormhold::CharacterData charData = stormhold::CharacterData::Load(assets);
        stormhold::ItemDatabase items = stormhold::ItemDatabase::Load(assets);

        TestBinaryWriterReaderRoundTrip();
        TestMonsterReadFromWriteToMatchesToBytesFromBytes();
        TestPlayerSaveFullRoundTrip(charData, items);

        if (!g_ok) {
            std::fprintf(stderr, "m20_save_format_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m20_save_format_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
