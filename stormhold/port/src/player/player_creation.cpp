#include "player/player_creation.h"

#include "player/player_inventory.h"

namespace stormhold {

namespace {

// Player.java's classStartingItems[classIndex][2] -- 2 starting item ids
// granted and auto-equipped by grantStartingItems(), one row per class.
constexpr int kClassStartingItems[7][2] = {
    {1, 27}, {7, 27}, {7, 22}, {17, 27}, {12, 22}, {17, 27}, {12, 22},
};

// Player.applyClassTemplate(int).
void ApplyClassTemplate(PlayerState& p, int classIndex, const CharacterData& charData) {
    const auto& tmpl = charData.classTemplates[static_cast<size_t>(classIndex)];
    p.classIndex = static_cast<int16_t>(classIndex);
    p.raceIndex = tmpl[1];

    for (int i = 0; i < 8; i++) {
        p.attributes[static_cast<size_t>(2 * i)] = tmpl[static_cast<size_t>(2 + i)];
        p.attributes[static_cast<size_t>(2 * i + 1)] = 0;
    }

    p.classMagickaFactor = tmpl[10];
    p.classUnknownPair[0] = tmpl[11];
    p.classUnknownPair[1] = tmpl[12];

    p.coreStats[0] = 1;
    p.coreStats[1] = 0;

    // computeDerivedStats().
    p.coreStats[3] = static_cast<int16_t>((p.attributes[0] + p.attributes[10]) / 2);
    p.coreStats[5] = static_cast<int16_t>(p.classMagickaFactor * p.attributes[2] / 4);
    p.coreStats[7] =
        static_cast<int16_t>(p.attributes[0] + p.attributes[4] + p.attributes[6] + p.attributes[10]);

    p.coreStats[2] = p.coreStats[3];
    p.coreStats[4] = p.coreStats[5];
    p.coreStats[6] = p.coreStats[7];
    p.coreStats[8] = 0;
    p.coreStats[9] = 0;
    p.levelUpAttributeFlags = 0;

    int col = 13;
    for (int i = 0; i < 14; i++) {
        p.skills[static_cast<size_t>(i)][0] = tmpl[static_cast<size_t>(col++)];
        p.skills[static_cast<size_t>(i)][1] = tmpl[static_cast<size_t>(col++)];
        p.skills[static_cast<size_t>(i)][2] = 0;
    }

    p.inventoryItemIds.fill(0);
    p.inventoryItemData.fill(0);
    p.equippedItems.fill(0);
    p.inventoryCount = 0;

    // computeStartingSpellMask(): builds the starting known-spell bitmask
    // from the class template's 14 threshold/rank column pairs (starting
    // at column 13, the same columns the skills loop above just read) --
    // `threshold` is read (to advance the column cursor correctly) but,
    // same as the real Java, never actually used for anything; only
    // `rank > 0` gates a bit. Bucket mapping (skill index 1->bit0,
    // 3->bit5, 4->bit10, 6->bit15, 10->bit20) matches
    // Player.spellSkillIndexFor()'s own bucket boundaries.
    int mask = 0;
    col = 13;
    int8_t selectedSpellId = 0;
    bool pickFirst = true;
    for (int i = 0; i < 14; i++) {
        int16_t threshold = tmpl[static_cast<size_t>(col++)];
        int16_t rank = tmpl[static_cast<size_t>(col++)];
        (void)threshold;

        int firstBit;
        switch (i) {
            case 1:
                firstBit = 0;
                break;
            case 3:
                firstBit = 5;
                break;
            case 4:
                firstBit = 10;
                break;
            case 6:
                firstBit = 15;
                break;
            case 10:
                firstBit = 20;
                break;
            default:
                firstBit = -1;
                break;
        }

        if (firstBit != -1 && rank > 0) {
            mask |= 1 << firstBit;
            if (pickFirst) {
                selectedSpellId = static_cast<int8_t>(firstBit + 1);
                pickFirst = false;
            }
        }
    }
    p.knownSpellsMask = static_cast<uint32_t>(mask);
    p.selectedSpellId = selectedSpellId;
}

// Player.resetState(classIndex, false)'s "new character" branch, minus
// grantStartingItems() (called separately below, once inventory state
// exists to grant into).
void ResetForNewCharacter(PlayerState& p) {
    p.giftPointsFound = 0;
    p.rumorRevealStep = 0;
    p.wardenLoreStep = 0;

    p.ailmentMask = 0;
    p.vampirismTimer = 0;
    p.manaBurnTimer = 0;
    p.terrifiedTimer = 0;

    // setHubSpawnPosition(false).
    p.currentLevel = p.pendingLevel = 1;
    p.tileX = p.pendingTileX = 9;
    p.tileY = p.pendingTileY = 10;
    p.facing = p.pendingFacing = 1;

    p.campLevel = 0;
    p.campX = 0;
    p.campY = 0;
    p.campFacing = 0;

    p.effectDurations.fill(0);

    p.lastCombatTargetId = 0;
    p.spellArmorBonus = 0;
    p.increaseHarmBuff = false;
    p.increaseArmorBuff = false;
    p.safeCampingBuff = false;
}

// Player.grantStartingItems(). Inventory add/equip logic now lives in
// PlayerInventory (M12) -- player_creation.cpp used to keep private
// duplicates of these, moved out so character creation and general
// inventory management share one real implementation.
void GrantStartingItems(PlayerState& p, int classIndex, int16_t spawnId, const ItemDatabase& items) {
    for (int itemId : kClassStartingItems[classIndex]) {
        PlayerInventory::AddInventoryItemRaw(p, itemId, spawnId, 0);
        int slot = p.inventoryCount - 1;
        PlayerInventory::EquipItem(p, slot, true, items);
    }
}

}  // namespace

PlayerState PlayerCreation::CreateCharacter(int classIndex, const std::string& name, int16_t spawnId,
                                             const CharacterData& charData, const ItemDatabase& items) {
    PlayerState p;
    p.name = name;
    ApplyClassTemplate(p, classIndex, charData);
    ResetForNewCharacter(p);
    GrantStartingItems(p, classIndex, spawnId, items);
    return p;
}

}  // namespace stormhold
