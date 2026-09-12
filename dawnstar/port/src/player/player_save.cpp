#include "player/player_save.h"

#include <array>
#include <sstream>

#include "assets/binary_reader.h"
#include "assets/binary_writer.h"
#include "player/player_creation.h"

namespace dawnstar {

std::vector<uint8_t> PlayerSave::ToBytes(const PlayerState& p) {
    std::vector<uint8_t> bytes;
    BinaryWriter out(bytes);

    out.WriteUTF(p.name);
    out.WriteS16(static_cast<int16_t>(p.classIndex));
    out.WriteS16(static_cast<int16_t>(p.raceIndex));

    for (int i = 0; i < 10; i++) out.WriteS16(p.coreStats[i]);
    out.WriteS8(p.attributeIncreaseFlags);

    out.WriteS32(p.gold);

    for (int i = 0; i < 16; i++) out.WriteS16(p.attributes[i]);

    out.WriteS16(p.classMagickaFactor);
    out.WriteS16(p.classUnknownPair[0]);
    out.WriteS16(p.classUnknownPair[1]);

    for (int i = 0; i < 14; i++) {
        for (int c = 0; c < 3; c++) out.WriteS16(p.skills[i][c]);
    }

    out.WriteS8(static_cast<int8_t>(p.inventoryCount));
    for (int i = 0; i < 24; i++) out.WriteS8(p.inventoryItemIds[i]);
    for (int i = 0; i < 24; i++) out.WriteS32(p.inventoryItemData[i]);
    for (int i = 0; i < 7; i++) out.WriteS8(p.equippedItems[i]);

    out.WriteS32(static_cast<int32_t>(p.knownSpellsMask));
    out.WriteS8(p.selectedSpellId);

    out.WriteS16(p.giftPointsFound);
    out.WriteS16(p.rumorRevealStep);
    out.WriteS8(p.ailmentMask);
    out.WriteS16(p.trollThirstTimer);
    out.WriteS16(p.glacierCurseTimer);
    out.WriteS16(p.terrifiedTimer);
    out.WriteBool(p.unconfirmedZ);
    out.WriteS8(static_cast<int8_t>(p.currentLevel));
    out.WriteS8(static_cast<int8_t>(p.tileX));
    out.WriteS8(static_cast<int8_t>(p.tileY));
    out.WriteS8(static_cast<int8_t>(p.facing));
    out.WriteS8(p.campLevel);
    out.WriteS8(p.campX);
    out.WriteS8(p.campY);
    out.WriteS8(p.campFacing);

    for (int i = 0; i < 25; i++) out.WriteS8(p.effectDurations[i]);

    out.WriteS16(p.combatTargetSpawnId);
    out.WriteS16(p.tempArmorBonus);
    out.WriteBool(p.increaseHarmBuff);
    out.WriteBool(p.increaseArmorBuff);
    out.WriteBool(p.safeCampingBuff);

    // Player.java's toBytes(true):
    //   packed = (byte)(this.traitorIndex << 2 + this.traitorSuspicionCount);
    // Java's `+` binds tighter than `<<`, so this is actually
    //   traitorIndex << (2 + traitorSuspicionCount)
    // NOT the (traitorIndex << 2) + traitorSuspicionCount the read-back
    // side clearly assumes (fromBytes: traitorSuspicionCount = packed %
    // 4; traitorIndex = (packed >> 2) % 4 -- a "2 bits each" scheme). A
    // real original-game operator-precedence bug found in the decompiled
    // source (not a decompiler/rename artifact), preserved faithfully
    // here rather than "fixed" -- on real hardware this data is already
    // lossy across a save/load round-trip for most (traitorIndex,
    // traitorSuspicionCount) pairs. See docs/PORT_ROADMAP.md's M12 entry
    // for a hand-traced example. The +16/+32 flag bits below are then
    // added on top of whatever that shift produced, exactly as the
    // original does, rather than into guaranteed-clear bits.
    int packed = static_cast<int8_t>(p.traitorIndex << (2 + p.traitorSuspicionCount));
    if (p.specialEncounterResolved) packed = static_cast<int8_t>(packed + 16);
    if (p.roamingSpecialMonsterPresent) packed = static_cast<int8_t>(packed + 32);
    out.WriteS8(static_cast<int8_t>(packed));

    int flagIdx = 0;
    while (flagIdx < 96) {
        uint8_t b = p.eventFlags[flagIdx++] ? 0x80 : 0;
        b = static_cast<uint8_t>(b | (p.eventFlags[flagIdx++] ? 0x40 : 0));
        b = static_cast<uint8_t>(b | (p.eventFlags[flagIdx++] ? 0x20 : 0));
        b = static_cast<uint8_t>(b | (p.eventFlags[flagIdx++] ? 0x10 : 0));
        b = static_cast<uint8_t>(b | (p.eventFlags[flagIdx++] ? 0x08 : 0));
        b = static_cast<uint8_t>(b | (p.eventFlags[flagIdx++] ? 0x04 : 0));
        b = static_cast<uint8_t>(b | (p.eventFlags[flagIdx++] ? 0x02 : 0));
        b = static_cast<uint8_t>(b | (p.eventFlags[flagIdx++] ? 0x01 : 0));
        out.WriteU8(b);
    }

    return bytes;
}

PlayerState PlayerSave::FromBytes(const std::vector<uint8_t>& data) {
    std::string raw(reinterpret_cast<const char*>(data.data()), data.size());
    std::istringstream stream(raw);
    BinaryReader in(stream);

    PlayerState p;
    p.name = in.ReadUTF();
    p.classIndex = in.ReadS16();
    p.raceIndex = in.ReadS16();

    for (int i = 0; i < 10; i++) p.coreStats[i] = in.ReadS16();
    p.attributeIncreaseFlags = in.ReadS8();

    p.gold = in.ReadS32();

    for (int i = 0; i < 16; i++) p.attributes[i] = in.ReadS16();

    p.classMagickaFactor = in.ReadS16();
    p.classUnknownPair[0] = in.ReadS16();
    p.classUnknownPair[1] = in.ReadS16();

    for (int i = 0; i < 14; i++) {
        for (int c = 0; c < 3; c++) p.skills[i][c] = in.ReadS16();
    }

    p.inventoryCount = in.ReadS8();
    for (int i = 0; i < 24; i++) p.inventoryItemIds[i] = in.ReadS8();
    for (int i = 0; i < 24; i++) p.inventoryItemData[i] = in.ReadS32();
    for (int i = 0; i < 7; i++) p.equippedItems[i] = in.ReadS8();

    p.knownSpellsMask = static_cast<uint32_t>(in.ReadS32());
    p.selectedSpellId = in.ReadS8();

    p.giftPointsFound = in.ReadS16();
    p.rumorRevealStep = in.ReadS16();
    p.ailmentMask = in.ReadS8();
    p.trollThirstTimer = in.ReadS16();
    p.glacierCurseTimer = in.ReadS16();
    p.terrifiedTimer = in.ReadS16();
    p.unconfirmedZ = in.ReadS8() != 0;
    p.currentLevel = in.ReadS8();
    p.tileX = in.ReadS8();
    p.tileY = in.ReadS8();
    p.facing = in.ReadS8();
    p.campLevel = in.ReadS8();
    p.campX = in.ReadS8();
    p.campY = in.ReadS8();
    p.campFacing = in.ReadS8();

    for (int i = 0; i < 25; i++) p.effectDurations[i] = in.ReadS8();

    p.combatTargetSpawnId = in.ReadS16();
    p.tempArmorBonus = in.ReadS16();
    p.increaseHarmBuff = in.ReadS8() != 0;
    p.increaseArmorBuff = in.ReadS8() != 0;
    p.safeCampingBuff = in.ReadS8() != 0;

    // See ToBytes's comment: this packed byte is decoded with the "2
    // bits each" scheme the write side's operator-precedence bug doesn't
    // actually implement, so traitorIndex/traitorSuspicionCount are
    // faithfully *not* guaranteed to survive a round-trip unchanged.
    // `packed` is deliberately `int` (not `uint8_t`) so `%`/`>>` match
    // Java's byte->int sign-extending promotion.
    int packed = in.ReadS8();
    p.roamingSpecialMonsterPresent = (packed & 32) == 32;
    p.specialEncounterResolved = (packed & 16) == 16;
    p.traitorSuspicionCount = static_cast<int8_t>(packed % 4);
    p.traitorIndex = static_cast<int8_t>((packed >> 2) % 4);

    int flagIdx = 0;
    while (flagIdx < 96) {
        uint8_t b = static_cast<uint8_t>(in.ReadS8());
        p.eventFlags[flagIdx++] = (b & 0x80) != 0;
        p.eventFlags[flagIdx++] = (b & 0x40) != 0;
        p.eventFlags[flagIdx++] = (b & 0x20) != 0;
        p.eventFlags[flagIdx++] = (b & 0x10) != 0;
        p.eventFlags[flagIdx++] = (b & 0x08) != 0;
        p.eventFlags[flagIdx++] = (b & 0x04) != 0;
        p.eventFlags[flagIdx++] = (b & 0x02) != 0;
        p.eventFlags[flagIdx++] = (b & 0x01) != 0;
    }

    return p;
}

std::vector<uint8_t> PlayerSave::ToBytesSummary(PlayerState& p, const CharacterData& charData) {
    std::vector<uint8_t> bytes;
    BinaryWriter out(bytes);

    out.WriteUTF(p.name);
    out.WriteS16(static_cast<int16_t>(p.classIndex));
    out.WriteS16(static_cast<int16_t>(p.raceIndex));

    // Player.java's normalizeForSummary(): current=max for HP/Magicka/
    // Fatigue, coreStats[8] zeroed -- applied to a COPY, unlike
    // computeStartingSpellMask() below.
    std::array<int16_t, 10> summary = p.coreStats;
    summary[2] = summary[3];
    summary[4] = summary[5];
    summary[6] = summary[7];
    summary[8] = 0;
    for (int i = 0; i < 10; i++) out.WriteS16(summary[i]);

    out.WriteS32(p.gold);
    for (int i = 0; i < 16; i++) out.WriteS16(p.attributes[i]);

    out.WriteS16(p.classMagickaFactor);
    out.WriteS16(p.classUnknownPair[0]);
    out.WriteS16(p.classUnknownPair[1]);

    for (int i = 0; i < 14; i++) {
        for (int c = 0; c < 3; c++) out.WriteS16(p.skills[i][c]);
    }

    // See this method's header doc comment: this mutates p.selectedSpellId
    // and discards any spells learned beyond the class's starting set.
    uint32_t mask = PlayerCreation::ComputeStartingSpellMask(p, charData);
    out.WriteS32(static_cast<int32_t>(mask));

    return bytes;
}

PlayerState PlayerSave::FromBytesSummary(const std::vector<uint8_t>& data, const CharacterData& charData,
                                          const ItemDatabase& items, JavaRandom& globalRng) {
    std::string raw(reinterpret_cast<const char*>(data.data()), data.size());
    std::istringstream stream(raw);
    BinaryReader in(stream);

    std::string name = in.ReadUTF();
    int classIndex = in.ReadS16();

    // Player.java's applyClassTemplate(classIndex)+resetState(false):
    // rolls a fresh traitorIndex, grants starting items, and places the
    // character at the hub spawn -- none of which the summary format
    // itself carries.
    PlayerState p = PlayerCreation::CreateCharacter(classIndex, name, charData, items, globalRng);

    p.raceIndex = in.ReadS16();
    for (int i = 0; i < 10; i++) p.coreStats[i] = in.ReadS16();

    p.gold = in.ReadS32();
    for (int i = 0; i < 16; i++) p.attributes[i] = in.ReadS16();

    p.classMagickaFactor = in.ReadS16();
    p.classUnknownPair[0] = in.ReadS16();
    p.classUnknownPair[1] = in.ReadS16();

    for (int i = 0; i < 14; i++) {
        for (int c = 0; c < 3; c++) p.skills[i][c] = in.ReadS16();
    }

    p.knownSpellsMask = static_cast<uint32_t>(in.ReadS32());

    return p;
}

}  // namespace dawnstar
