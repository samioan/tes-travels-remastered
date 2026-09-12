#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace dawnstar {

// Renamed-source counterpart of a slice of ../../../src/Player.java's
// runtime instance state -- just what character creation
// (player_creation.h) touches: stats, attributes, skills, starting
// inventory/equipment, starting known spells, and hub-town position.
// Not the whole class -- movement, combat, spellcasting, dialogue, and
// the save format are all separate, later milestones (see
// docs/PORT_ROADMAP.md).
struct PlayerState {
    std::string name;
    int classIndex = 0;
    int raceIndex = 0;

    // level, levelExp, curHP, maxHP, curMagicka, maxMagicka, curFatigue,
    // maxFatigue, and two more slots ([8]/[9]) whose use is unconfirmed
    // in the Java source itself -- see Player.java's own field comment.
    std::array<int16_t, 10> coreStats{};
    int8_t attributeIncreaseFlags = 0;
    int gold = 0;
    // 8 attributes as base+bonus pairs (attributes[2*i]=base,
    // attributes[2*i+1]=bonus).
    std::array<int16_t, 16> attributes{};
    int16_t classMagickaFactor = 0;
    std::array<int16_t, 2> classUnknownPair{};
    // Skills as rank/bonus/exp-toward-next-rank triples.
    std::array<std::array<int16_t, 3>, 14> skills{};

    int inventoryCount = 0;
    // Item-type ids, negative = currently equipped.
    std::array<int8_t, 24> inventoryItemIds{};
    // Packed value/charge per inventory slot: (spawnIdOrPacked << 16) +
    // (int8_t)charge, matching Player.java's addInventoryItem() exactly.
    std::array<int32_t, 24> inventoryItemData{};
    // Equipped item-type per equip slot, indexed by Item's equip-slot
    // column (0=weapon,1=shield/offhand,2..6=armor slots).
    std::array<int8_t, 7> equippedItems{};

    // Bitmask of known spell ids (bit (id-1) set = known).
    uint32_t knownSpellsMask = 0;
    int8_t selectedSpellId = 0;

    // Rolled 0-3 at character creation -- the hidden "traitor" index the
    // mystery-subplot rumor system gradually reveals (Shop.java's
    // RUMOR_STRING_OFFSET[traitorIndex]).
    int8_t traitorIndex = 0;

    int currentLevel = 1;
    int tileX = 9;
    int tileY = 9;
    int facing = 1;
};

}  // namespace dawnstar
