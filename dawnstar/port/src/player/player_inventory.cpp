#include "player/player_inventory.h"

#include <array>
#include <cstdint>
#include <cstdlib>

#include "player/player_combat_stats.h"

namespace dawnstar {

namespace {

// Player.java's private unequipItemInSlot(equipSlot): finds whichever
// inventory slot currently occupies `equipSlot` and unequips it.
void UnequipItemInSlot(PlayerState& p, const ItemDatabase& items, int equipSlot) {
    for (int slot = 0; slot < p.inventoryCount; slot++) {
        int itemId = std::abs(static_cast<int>(p.inventoryItemIds[slot]));
        if (items.EquipSlotOf(itemId) == equipSlot) {
            PlayerInventory::UnequipSlot(p, items, slot);
        }
    }
}

// Item.java's own specialEffectText: flavor text for the 13 "gift"/
// special-consumable item ids 87-99, in order (confirmed 1:1 against
// Player.useItem's switch on those exact ids -- see CLASS_MAP.md). A
// hardcoded literal table in the Java source too, not data loaded from
// itemsin.dat, so it's kept here (the one place that needs it) rather
// than added to ItemDatabase itself -- same "small hardcoded table lives
// next to its one real consumer" precedent as world/dungeon_generator.cpp's
// own kDifficultyTierLookup/kMonsterTable.
const char* const kSpecialEffectText[13] = {
    "Warp to camp",       "Cures ailment",          "Restores Health",
    "Restores Magicka",   " ",                      "Grants level experience",
    "Health & Magicka",   "Increase harm",          "Increase armor",
    "Safe camping",       "Kills weak monster",     "Kills normal monster",
    "Kills strong monster",
};

}  // namespace

bool PlayerInventory::AddItem(PlayerState& p, int itemId, int spawnIdOrPacked, int charge) {
    if (p.inventoryCount >= 24) return false;
    p.inventoryItemIds[p.inventoryCount] = static_cast<int8_t>(itemId);
    int32_t packed = (spawnIdOrPacked << 16) + static_cast<int8_t>(charge);
    p.inventoryItemData[p.inventoryCount] = packed;
    p.inventoryCount++;
    return true;
}

bool PlayerInventory::IsEquipped(const PlayerState& p, const ItemDatabase& items, int slot) {
    int8_t itemId = p.inventoryItemIds[slot];
    if (!items.IsEquippable(std::abs(static_cast<int>(itemId)))) return false;
    return itemId < 0;
}

bool PlayerInventory::CanEquipOrUnequip(const PlayerState& p, const ItemDatabase& items, int slot) {
    int itemId = std::abs(static_cast<int>(p.inventoryItemIds[slot]));
    int8_t category = items.category[itemId - 1];
    switch (category) {
        case 1: case 2: case 3: case 4: case 5:
        case 6: case 7: case 8: case 9: case 10:
        case 15:
            return true;
        default:
            return false;
    }
}

bool PlayerInventory::Equip(PlayerState& p, const ItemDatabase& items, int slot, bool autoUnequipConflict) {
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

void PlayerInventory::UnequipSlot(PlayerState& p, const ItemDatabase& items, int slot) {
    if (!IsEquipped(p, items, slot)) return;
    if (slot < 0 || slot > 23) return;

    int8_t itemId = static_cast<int8_t>(std::abs(static_cast<int>(p.inventoryItemIds[slot])));
    p.inventoryItemIds[slot] = itemId;

    for (int i = 0; i < 7; i++) {
        if (p.equippedItems[i] == itemId) {
            p.equippedItems[i] = 0;
            break;
        }
    }
}

bool PlayerInventory::RemoveSlot(PlayerState& p, const ItemDatabase& items, int slot) {
    if (slot >= p.inventoryCount) return false;

    UnequipSlot(p, items, slot);
    p.inventoryItemIds[slot] = 0;

    for (int i = slot; i < p.inventoryCount - 1; i++) {
        p.inventoryItemIds[i] = p.inventoryItemIds[i + 1];
        p.inventoryItemData[i] = p.inventoryItemData[i + 1];
    }

    p.inventoryCount--;
    return true;
}

bool PlayerInventory::EquipLastPickedUpItem(PlayerState& p, const ItemDatabase& items, bool autoUnequipConflict) {
    int slot = p.inventoryCount - 1;
    return Equip(p, items, slot, autoUnequipConflict);
}

bool PlayerInventory::CanUseItem(const PlayerState& p, const ItemDatabase& items, int slot) {
    int itemId = std::abs(static_cast<int>(p.inventoryItemIds[slot]));
    return items.category[static_cast<size_t>(itemId - 1)] == 13;
}

std::string PlayerInventory::ItemTooltip(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items,
                                          const SpellDatabase& spells, int slot) {
    int itemId = std::abs(static_cast<int>(p.inventoryItemIds[slot]));
    int8_t category = items.category[static_cast<size_t>(itemId - 1)];
    std::string text;
    switch (category) {
        case 1: case 2: case 3: case 4: {
            text = items.name[static_cast<size_t>(itemId - 1)] + "\n" +
                   items.categoryNames[static_cast<size_t>(category - 1)];
            int weaponValue = items.questFlags[static_cast<size_t>(itemId - 1)] + (p.inventoryItemData[slot] & 0xFF);
            text += "\nWeapon value: " + std::to_string(weaponValue);
            break;
        }
        case 5: case 6: case 7: case 8: case 9: case 10: {
            text = items.name[static_cast<size_t>(itemId - 1)] + "\n" +
                   items.categoryNames[static_cast<size_t>(category - 1)];
            int armorValue = items.questFlags[static_cast<size_t>(itemId - 1)] + (p.inventoryItemData[slot] & 0xFF);
            text += "\nArmor value: " + std::to_string(armorValue);
            break;
        }
        case 11:
            text = items.name[static_cast<size_t>(itemId - 1)] + "\n" +
                   items.categoryNames[static_cast<size_t>(category - 1)];
            break;
        case 12: {
            text = items.name[static_cast<size_t>(itemId - 1)] + "\nSpell: ";
            int spellId = p.inventoryItemData[slot] & 0xFF;
            text += spells.all[static_cast<size_t>(spellId - 1)].name;
            if ((p.knownSpellsMask & (1u << (spellId - 1))) != 0) text += " (known)";
            break;
        }
        case 13: {
            int giftIdx = itemId - 87;
            text = items.name[static_cast<size_t>(itemId - 1)] + "\n" +
                   items.categoryNames[static_cast<size_t>(category - 1)] + "\n" +
                   kSpecialEffectText[static_cast<size_t>(giftIdx)];
            break;
        }
        case 15: {
            text = items.name[static_cast<size_t>(itemId - 1)] + "\n" +
                   items.categoryNames[static_cast<size_t>(category - 1)];
            int bonus = PlayerCombatStats::SkillValue(p, charData, 3, false);
            text += "\nWeapon value: " + std::to_string(20 + bonus);
            break;
        }
        case 14:
        default:
            text = items.name[static_cast<size_t>(itemId - 1)] + "\n" +
                   items.categoryNames[static_cast<size_t>(category - 1)];
            break;
    }
    return text;
}

void PlayerInventory::DropInventoryItem(PlayerState& p, const ItemDatabase& items, std::vector<GeneratedLevel>& levels,
                                         WorldRegistry& world, int slot) {
    int itemId = std::abs(static_cast<int>(p.inventoryItemIds[slot]));
    if (itemId != 101) {
        int32_t data = p.inventoryItemData[slot];
        std::array<uint8_t, 7> rec{};
        rec[0] = static_cast<uint8_t>(p.tileX);
        rec[1] = static_cast<uint8_t>(p.tileY);
        rec[2] = static_cast<uint8_t>(itemId);
        int packed = (data >> 16) & 0xFFFF;
        rec[3] = static_cast<uint8_t>((packed >> 8) & 0xFF);
        rec[4] = static_cast<uint8_t>(packed & 0xFF);
        rec[5] = static_cast<uint8_t>(data & 0xFF);
        rec[6] = 3;
        GeneratedLevel& level = levels[static_cast<size_t>(p.currentLevel - 1)];
        DungeonRuntime::AddDroppedItem(level, world, rec);
    }
    RemoveSlot(p, items, slot);
}

}  // namespace dawnstar
