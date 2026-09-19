#include "player/player_save.h"

#include <sstream>

#include "assets/binary_reader.h"
#include "assets/binary_writer.h"

namespace stormhold {

// Field order transcribed directly from Player.java's toBytes(true)/
// fromBytes(data, true) -- see player_save.h's own class comment for what
// this deliberately does NOT include.
std::vector<uint8_t> PlayerSave::ToBytes(const PlayerState& p) {
    std::ostringstream buf;
    BinaryWriter out(buf);

    out.WriteUTF(p.name);
    out.WriteS16(p.classIndex);
    out.WriteS16(p.raceIndex);

    for (int16_t v : p.coreStats) out.WriteS16(v);
    out.WriteS8(p.levelUpAttributeFlags);

    out.WriteS32(p.unconfirmedIntField);

    for (int16_t v : p.attributes) out.WriteS16(v);

    out.WriteS16(p.classMagickaFactor);
    out.WriteS16(p.classUnknownPair[0]);
    out.WriteS16(p.classUnknownPair[1]);

    for (const auto& skill : p.skills) {
        for (int16_t v : skill) out.WriteS16(v);
    }

    out.WriteS8(p.inventoryCount);
    for (int8_t v : p.inventoryItemIds) out.WriteS8(v);
    for (int32_t v : p.inventoryItemData) out.WriteS32(v);
    for (int8_t v : p.equippedItems) out.WriteS8(v);

    out.WriteS32(static_cast<int32_t>(p.knownSpellsMask));
    out.WriteS8(p.selectedSpellId);

    out.WriteS16(p.giftPointsFound);
    out.WriteS16(p.rumorRevealStep);
    out.WriteS16(p.wardenLoreStep);
    out.WriteS8(p.ailmentMask);
    out.WriteS16(p.vampirismTimer);
    out.WriteS16(p.manaBurnTimer);
    out.WriteS16(p.terrifiedTimer);
    out.WriteBool(p.unconfirmedFlag2);
    out.WriteS8(p.currentLevel);
    out.WriteS8(p.tileX);
    out.WriteS8(p.tileY);
    out.WriteS8(p.facing);
    out.WriteS8(p.campLevel);
    out.WriteS8(p.campX);
    out.WriteS8(p.campY);
    out.WriteS8(p.campFacing);

    for (int8_t v : p.effectDurations) out.WriteS8(v);

    out.WriteS16(p.lastCombatTargetId);
    out.WriteS16(p.spellArmorBonus);
    out.WriteBool(p.increaseHarmBuff);
    out.WriteBool(p.increaseArmorBuff);
    out.WriteBool(p.safeCampingBuff);

    std::string str = buf.str();
    return std::vector<uint8_t>(str.begin(), str.end());
}

PlayerState PlayerSave::FromBytes(const std::vector<uint8_t>& data) {
    std::istringstream buf(std::string(data.begin(), data.end()));
    BinaryReader in(buf);
    PlayerState p;

    p.name = in.ReadUTF();
    p.classIndex = in.ReadS16();
    p.raceIndex = in.ReadS16();

    for (int16_t& v : p.coreStats) v = in.ReadS16();
    p.levelUpAttributeFlags = in.ReadS8();

    p.unconfirmedIntField = in.ReadS32();

    for (int16_t& v : p.attributes) v = in.ReadS16();

    p.classMagickaFactor = in.ReadS16();
    p.classUnknownPair[0] = in.ReadS16();
    p.classUnknownPair[1] = in.ReadS16();

    for (auto& skill : p.skills) {
        for (int16_t& v : skill) v = in.ReadS16();
    }

    p.inventoryCount = in.ReadS8();
    for (int8_t& v : p.inventoryItemIds) v = in.ReadS8();
    for (int32_t& v : p.inventoryItemData) v = in.ReadS32();
    for (int8_t& v : p.equippedItems) v = in.ReadS8();

    p.knownSpellsMask = static_cast<uint32_t>(in.ReadS32());
    p.selectedSpellId = in.ReadS8();

    p.giftPointsFound = in.ReadS16();
    p.rumorRevealStep = in.ReadS16();
    p.wardenLoreStep = in.ReadS16();
    p.ailmentMask = in.ReadS8();
    p.vampirismTimer = in.ReadS16();
    p.manaBurnTimer = in.ReadS16();
    p.terrifiedTimer = in.ReadS16();
    p.unconfirmedFlag2 = in.ReadU8() != 0;
    p.currentLevel = in.ReadS8();
    p.tileX = in.ReadS8();
    p.tileY = in.ReadS8();
    p.facing = in.ReadS8();
    p.campLevel = in.ReadS8();
    p.campX = in.ReadS8();
    p.campY = in.ReadS8();
    p.campFacing = in.ReadS8();

    for (int8_t& v : p.effectDurations) v = in.ReadS8();

    p.lastCombatTargetId = in.ReadS16();
    p.spellArmorBonus = in.ReadS16();
    p.increaseHarmBuff = in.ReadU8() != 0;
    p.increaseArmorBuff = in.ReadU8() != 0;
    p.safeCampingBuff = in.ReadU8() != 0;

    return p;
}

}  // namespace stormhold
