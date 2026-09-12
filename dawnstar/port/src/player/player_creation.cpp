#include "player/player_creation.h"

namespace dawnstar {

namespace {

// Player.java's STARTING_ITEMS: per-class starting item id pairs,
// indexed by classIndex.
const int kStartingItems[7][2] = {{1, 27}, {7, 27}, {7, 22}, {17, 27}, {12, 22}, {17, 27}, {12, 22}};

// Player.java's recalcMaxStats().
void RecalcMaxStats(PlayerState& p) {
    p.coreStats[3] = static_cast<int16_t>((p.attributes[0] + p.attributes[10]) / 2);
    p.coreStats[5] = static_cast<int16_t>(p.classMagickaFactor * p.attributes[2] / 4);
    p.coreStats[7] = static_cast<int16_t>(p.attributes[0] + p.attributes[4] + p.attributes[6] + p.attributes[10]);
}

// Player.java's computeStartingSpellMask(): for 5 specific skill slots
// (mapped to bit positions 0/5/10/15/20, i.e. spell ids 1/6/11/16/21), a
// nonzero class-template threshold grants that tier's first spell. Also
// sets selectedSpellId to the first one granted.
uint32_t ComputeStartingSpellMask(PlayerState& p, const CharacterData& charData) {
    uint32_t mask = 0;
    int col = 13;
    bool first = true;

    for (int i = 0; i < 14; i++) {
        int16_t threshold = charData.classTemplates[p.classIndex][col];
        col += 2;  // the second column read per iteration is unused in the original too.

        int bit = -1;
        switch (i) {
            case 1:
                bit = 0;
                break;
            case 3:
                bit = 5;
                break;
            case 4:
                bit = 10;
                break;
            case 6:
                bit = 15;
                break;
            case 10:
                bit = 20;
                break;
            default:
                bit = -1;
                break;
        }

        if (bit != -1 && threshold > 0) {
            mask |= 1u << bit;
            if (first) {
                p.selectedSpellId = static_cast<int8_t>(bit + 1);
                first = false;
            }
        }
    }

    return mask;
}

// Player.java's equipItem(slot, autoUnequipConflict=true)'s auto-unequip
// path: unequipItemInSlot() -- finds whichever slot currently occupies
// `equipSlot` and unequips it. Never actually exercised by
// GrantStartingItems below (STARTING_ITEMS' pairs are curated to not
// conflict), but ported faithfully rather than assumed unreachable.
void UnequipItemInSlot(PlayerState& p, const ItemDatabase& items, int equipSlot) {
    for (int slot = 0; slot < p.inventoryCount; slot++) {
        int itemId = p.inventoryItemIds[slot] < 0 ? -p.inventoryItemIds[slot] : p.inventoryItemIds[slot];
        if (itemId != 0 && items.EquipSlotOf(itemId) == equipSlot) {
            p.equippedItems[equipSlot] = 0;
            p.inventoryItemIds[slot] = static_cast<int8_t>(itemId);  // flip back positive
        }
    }
}

// Player.java's addInventoryItem(itemId, spawnIdOrPacked, charge).
bool AddInventoryItem(PlayerState& p, int itemId, int spawnIdOrPacked, int charge) {
    if (p.inventoryCount >= 24) return false;
    p.inventoryItemIds[p.inventoryCount] = static_cast<int8_t>(itemId);
    int32_t packed = (spawnIdOrPacked << 16) + static_cast<int8_t>(charge);
    p.inventoryItemData[p.inventoryCount] = packed;
    p.inventoryCount++;
    return true;
}

// Player.java's equipItem(slot, autoUnequipConflict).
bool EquipItem(PlayerState& p, const ItemDatabase& items, int slot, bool autoUnequipConflict) {
    int8_t itemId = p.inventoryItemIds[slot];
    if (itemId < 0) return false;
    if (!items.IsEquippable(itemId)) return false;

    int equipSlot = items.EquipSlotOf(itemId);
    if (p.equippedItems[equipSlot] != 0) {
        if (!autoUnequipConflict) return false;
        UnequipItemInSlot(p, items, equipSlot);
    }

    p.equippedItems[equipSlot] = itemId;
    p.inventoryItemIds[slot] = static_cast<int8_t>(-itemId);
    return true;
}

// Player.java's grantStartingItems(): grants classIndex's starting item
// pair and auto-equips each one.
void GrantStartingItems(PlayerState& p, const ItemDatabase& items) {
    // Item.nextSpawnId() is a pure counter with no bearing on generation
    // correctness -- same simplification DungeonGenerator's chest
    // placement made (see world/dungeon_generator.cpp): a fixed
    // placeholder rather than the real cross-character-creation running
    // total, since nothing reads it yet.
    int spawnId = 1;
    const int(&startingItems)[2] = kStartingItems[p.classIndex];

    for (int itemId : startingItems) {
        AddInventoryItem(p, itemId, spawnId, 0);
        int slot = p.inventoryCount - 1;
        EquipItem(p, items, slot, true);
    }
}

}  // namespace

PlayerState PlayerCreation::CreateCharacter(int characterClass, const std::string& name,
                                            const CharacterData& charData, const ItemDatabase& items,
                                            JavaRandom& globalRng) {
    PlayerState p;
    p.name = name;

    // --- Player.applyClassTemplate(characterClass) ---
    p.classIndex = characterClass;
    p.raceIndex = charData.classTemplates[p.classIndex][1];

    for (int i = 0; i < 8; i++) {
        p.attributes[2 * i] = charData.classTemplates[p.classIndex][2 + i];
        p.attributes[2 * i + 1] = 0;
    }

    p.classMagickaFactor = charData.classTemplates[p.classIndex][10];
    p.classUnknownPair[0] = charData.classTemplates[p.classIndex][11];
    p.classUnknownPair[1] = charData.classTemplates[p.classIndex][12];
    p.coreStats[0] = 1;  // level
    p.coreStats[1] = 0;  // levelExp
    RecalcMaxStats(p);
    p.coreStats[2] = p.coreStats[3];  // curHP = maxHP
    p.coreStats[4] = p.coreStats[5];  // curMagicka = maxMagicka
    p.coreStats[6] = p.coreStats[7];  // curFatigue = maxFatigue
    p.coreStats[8] = 0;
    p.coreStats[9] = 0;
    p.attributeIncreaseFlags = 0;
    p.gold = 50;
    p.traitorIndex = static_cast<int8_t>(LingoRandomInt(globalRng, 4) - 1);

    int col = 13;
    for (int i = 0; i < 14; i++) {
        p.skills[i][0] = charData.classTemplates[p.classIndex][col++];
        p.skills[i][1] = charData.classTemplates[p.classIndex][col++];
        p.skills[i][2] = 0;
    }

    p.inventoryItemIds.fill(0);
    p.inventoryItemData.fill(0);
    p.equippedItems.fill(0);
    p.inventoryCount = 0;

    p.knownSpellsMask = ComputeStartingSpellMask(p, charData);

    // --- Player.resetState(false) [character-creation path] ---
    // (ailment/effect/camp-state resets omitted: PlayerState doesn't
    // model those fields yet, they're all-zero by construction anyway on
    // a freshly-built PlayerState. The roaming-special-monster cleanup
    // in resetToHubPosition() is also omitted -- always a no-op for a
    // brand-new character, whose roamingSpecialMonsterPresent-equivalent
    // starts false and has no way to have been set yet.)
    p.currentLevel = 1;
    p.tileX = 9;
    p.tileY = 9;
    p.facing = 1;

    GrantStartingItems(p, items);

    return p;
}

}  // namespace dawnstar
