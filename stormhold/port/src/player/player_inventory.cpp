#include "player/player_inventory.h"

#include <cmath>
#include <stdexcept>

#include "player/player_combat_stats.h"
#include "player/player_movement.h"

namespace stormhold {

bool PlayerInventory::AddInventoryItemRaw(PlayerState& p, int itemId, int packedValue, int charge) {
    if (p.inventoryCount >= 24) return false;
    int slot = p.inventoryCount;
    p.inventoryItemIds[static_cast<size_t>(slot)] = static_cast<int8_t>(itemId);
    int32_t packed = (static_cast<int32_t>(packedValue) << 16) + static_cast<int8_t>(charge);
    p.inventoryItemData[static_cast<size_t>(slot)] = packed;
    p.inventoryCount++;
    return true;
}

bool PlayerInventory::IsEquippedSlot(const PlayerState& p, int slot, const ItemDatabase& items) {
    int8_t id = p.inventoryItemIds[static_cast<size_t>(slot)];
    if (!items.IsEquipmentCategory(static_cast<int>(std::abs(id)))) return false;
    return id < 0;
}

void PlayerInventory::UnequipSlot(PlayerState& p, int slot, const ItemDatabase& items) {
    if (!IsEquippedSlot(p, slot, items)) return;

    int8_t id = static_cast<int8_t>(std::abs(p.inventoryItemIds[static_cast<size_t>(slot)]));
    p.inventoryItemIds[static_cast<size_t>(slot)] = id;
    for (int i = 0; i < 7; i++) {
        if (p.equippedItems[static_cast<size_t>(i)] == id) {
            p.equippedItems[static_cast<size_t>(i)] = 0;
            break;
        }
    }
}

namespace {
// Player.unequipMatchingCategory(equipSlot) -- private helper, only
// EquipItem's swap path calls this.
void UnequipMatchingEquipSlot(PlayerState& p, int equipSlotWanted, const ItemDatabase& items) {
    for (int i = 0; i < p.inventoryCount; i++) {
        int id = std::abs(p.inventoryItemIds[static_cast<size_t>(i)]);
        if (items.EquipSlotOf(id) == equipSlotWanted) {
            PlayerInventory::UnequipSlot(p, i, items);
        }
    }
}
}  // namespace

bool PlayerInventory::EquipItem(PlayerState& p, int slot, bool allowSwap, const ItemDatabase& items) {
    int8_t id = p.inventoryItemIds[static_cast<size_t>(slot)];
    if (id < 0) return false;
    if (!items.IsEquipmentCategory(id)) return false;

    int equipSlot = items.EquipSlotOf(id);
    if (p.equippedItems[static_cast<size_t>(equipSlot)] != 0) {
        if (!allowSwap) return false;
        UnequipMatchingEquipSlot(p, equipSlot, items);
    }

    p.equippedItems[static_cast<size_t>(equipSlot)] = id;
    p.inventoryItemIds[static_cast<size_t>(slot)] = static_cast<int8_t>(-std::abs(id));
    return true;
}

bool PlayerInventory::EquipLastPickedUpItem(PlayerState& p, bool allowSwap, const ItemDatabase& items) {
    int slot = p.inventoryCount - 1;
    return EquipItem(p, slot, allowSwap, items);
}

bool PlayerInventory::RemoveInventorySlot(PlayerState& p, int slot, const ItemDatabase& items) {
    if (slot < 0) {
        throw std::runtime_error("PlayerInventory::RemoveInventorySlot: negative slot -- the real "
                                  "removeInventorySlot() has no guard here either, so the original would "
                                  "throw ArrayIndexOutOfBoundsException reading this.H[-1]");
    }
    if (slot >= p.inventoryCount) return false;

    UnequipSlot(p, slot, items);
    p.inventoryItemIds[static_cast<size_t>(slot)] = 0;

    for (int i = slot; i < p.inventoryCount - 1; i++) {
        p.inventoryItemIds[static_cast<size_t>(i)] = p.inventoryItemIds[static_cast<size_t>(i + 1)];
        p.inventoryItemData[static_cast<size_t>(i)] = p.inventoryItemData[static_cast<size_t>(i + 1)];
    }

    p.inventoryCount--;
    return true;
}

int PlayerInventory::FindEquippedSlotForItem(const PlayerState& p, int itemId) {
    int8_t target = static_cast<int8_t>(-std::abs(itemId));
    for (int i = 0; i < p.inventoryCount; i++) {
        if (target == p.inventoryItemIds[static_cast<size_t>(i)]) return i;
    }
    return -1;
}

bool PlayerInventory::InitializeItemCharge(PlayerState& p, int slot, const ItemDatabase& items) {
    int id = std::abs(p.inventoryItemIds[static_cast<size_t>(slot)]);
    if (!items.IsEquipmentCategory(id)) return false;

    int32_t charge = 3;
    p.inventoryItemData[static_cast<size_t>(slot)] =
        (p.inventoryItemData[static_cast<size_t>(slot)] & ~static_cast<int32_t>(0xFF)) | charge;
    return true;
}

bool PlayerInventory::IsItemCharged(const PlayerState& p, int slot) {
    int8_t charge = static_cast<int8_t>(p.inventoryItemData[static_cast<size_t>(slot)] & 0xFF);
    return charge == 3;
}

bool PlayerInventory::TryPickUpItem(PlayerState& p, const std::array<int8_t, 7>& record) {
    int8_t itemId = record[2];
    // Sign-extending shift, matching Java's `(record[3] << 8) +
    // record[4]` on signed bytes exactly -- see this method's own
    // declaration comment / DropInventoryItem's header comment.
    int32_t packedValue = (static_cast<int32_t>(record[3]) << 8) + record[4];
    int8_t charge = record[5];
    return AddInventoryItemRaw(p, itemId, packedValue, charge);
}

std::optional<std::array<int8_t, 7>> PlayerInventory::DropInventoryItem(PlayerState& p, int slot,
                                                                         const ItemDatabase& items,
                                                                         GeneratedLevel& level, WorldRegistry& world) {
    int itemId = std::abs(p.inventoryItemIds[static_cast<size_t>(slot)]);
    if (itemId == 109) {
        RemoveInventorySlot(p, slot, items);
        return std::nullopt;
    }

    std::array<int8_t, 7> record{};
    record[0] = p.tileX;
    record[1] = p.tileY;
    record[2] = static_cast<int8_t>(itemId);
    record[5] = static_cast<int8_t>(p.inventoryItemData[static_cast<size_t>(slot)] & 0xFF);

    uint32_t extended = static_cast<uint32_t>(p.inventoryItemData[static_cast<size_t>(slot)]) >> 16 & 0xFFFFu;
    record[3] = static_cast<int8_t>(extended >> 8 & 0xFF);
    record[4] = static_cast<int8_t>(extended & 0xFF);
    record[6] = 3;

    // Player.dropInventoryItem()'s own `this.currentDungeon().
    // addDroppedItem(record)` call -- see this method's own declaration
    // comment.
    DungeonRuntime::AddDroppedItem(level, world, record);

    RemoveInventorySlot(p, slot, items);
    return record;
}

int PlayerInventory::CollectChestItem(PlayerState& p, std::array<int8_t, 8> record, const ItemDatabase& items,
                                       GeneratedLevel& level, WorldRegistry& world,
                                       const GameAdvancement::LevelLookup& levels) {
    // record[2] = 2 -- confirmed dead, not reproduced; see this method's
    // own declaration comment.
    if (p.inventoryCount < 24) {
        int8_t itemId = record[4];
        int32_t packedValue = (static_cast<int32_t>(record[5]) << 8) + record[6];
        int8_t charge = record[7];
        AddInventoryItemRaw(p, itemId, packedValue, charge);
        DungeonRuntime::RemoveChest(level, world, record);

        int itemIndex = itemId - 1;
        if (items.category[static_cast<size_t>(itemIndex)] == 11) {
            p.giftPointsFound =
                static_cast<int16_t>(p.giftPointsFound + items.subtype[static_cast<size_t>(itemIndex)]);
            // M52: see this method's own declaration comment.
            GameAdvancement::OpenZone(GameAdvancement::Level(p.giftPointsFound), levels);
        }

        return 1;
    }

    std::array<int8_t, 7> dropped{record[0], record[1], record[4], record[5], record[6], record[7], 1};
    DungeonRuntime::AddDroppedItem(level, world, dropped);
    DungeonRuntime::RemoveChest(level, world, record);
    return 0;
}

bool PlayerInventory::HasCampMark(const PlayerState& p) { return p.campLevel > 0; }

void PlayerInventory::MarkCampAndReturnToTown(PlayerState& p, const GameAdvancement::LevelLookup& levels) {
    p.campLevel = p.currentLevel;
    p.campX = p.tileX;
    p.campY = p.tileY;
    p.campFacing = p.facing;

    // setHubSpawnPosition(true) -- the death/respawn hub point, (12, 14),
    // DISTINCT from character creation's (9, 10). See this method's own
    // header comment.
    p.currentLevel = p.pendingLevel = 1;
    p.tileX = p.pendingTileX = 12;
    p.tileY = p.pendingTileY = 14;
    p.facing = p.pendingFacing = 1;

    // Player.java line 2509 -- see this method's own header comment.
    PlayerMovement::RefreshCorridorView(p, levels(p.currentLevel), levels);
    p.justMarkedCamp = true;
}

void PlayerInventory::WarpToCampMark(PlayerState& p, const GameAdvancement::LevelLookup& levels) {
    p.currentLevel = p.pendingLevel = p.campLevel;
    p.tileX = p.pendingTileX = p.campX;
    p.tileY = p.pendingTileY = p.campY;
    // Player.java line 2517 -- see this method's own header comment.
    PlayerMovement::RefreshCorridorView(p, levels(p.currentLevel), levels);
    p.justMarkedCamp = true;
}

bool PlayerInventory::IsSlotEquipped(const PlayerState& p, int slot, const ItemDatabase& items) {
    int8_t itemId = p.inventoryItemIds[static_cast<size_t>(slot)];
    if (!items.IsEquippable(static_cast<int>(std::abs(itemId)))) return false;
    return itemId < 0;
}

bool PlayerInventory::CanEquipOrUnequip(const PlayerState& p, int slot, const ItemDatabase& items) {
    int id = std::abs(p.inventoryItemIds[static_cast<size_t>(slot)]);
    int8_t cat = items.category[static_cast<size_t>(id - 1)];
    return (cat >= 1 && cat <= 10) || cat == 17;
}

bool PlayerInventory::IsScrollCategory(const PlayerState& p, int slot, const ItemDatabase& items) {
    int id = std::abs(p.inventoryItemIds[static_cast<size_t>(slot)]);
    int8_t cat = items.category[static_cast<size_t>(id - 1)];
    return cat == 13 || cat == 15;
}

bool PlayerInventory::CanLearnSpellFromScroll(const PlayerState& p, int slot, const ItemDatabase& items,
                                               const SpellDatabase& spells) {
    int id = std::abs(p.inventoryItemIds[static_cast<size_t>(slot)]);
    int8_t cat = items.category[static_cast<size_t>(id - 1)];
    if (cat != 12) return false;

    int spellId = p.inventoryItemData[static_cast<size_t>(slot)] & 0xFF;
    int8_t skillIdx = spells.ById(spellId).skillRequired;
    return p.skills[static_cast<size_t>(skillIdx)][0] > 0;
}

bool PlayerInventory::LearnSpellFromScroll(PlayerState& p, int slot, const ItemDatabase& items) {
    int packedValue = p.inventoryItemData[static_cast<size_t>(slot)] & 0xFF;
    int bit = packedValue - 1;
    p.knownSpellsMask |= (1u << bit);
    RemoveInventorySlot(p, slot, items);
    return true;
}

bool PlayerInventory::CanUseItem(const PlayerState& p, int slot, const ItemDatabase& items) {
    int id = std::abs(p.inventoryItemIds[static_cast<size_t>(slot)]);
    int8_t cat = items.category[static_cast<size_t>(id - 1)];
    return cat == 13 || cat == 15;
}

void PlayerInventory::UseItem(PlayerState& p, int slot, MonsterState* target, const ItemDatabase& items,
                               const MonsterDatabase& monsters, WorldRegistry& world, JavaRandom& rng,
                               const GameAdvancement::LevelLookup& levels) {
    int id = std::abs(p.inventoryItemIds[static_cast<size_t>(slot)]);
    int8_t cat = items.category[static_cast<size_t>(id - 1)];
    if (cat != 13 && cat != 15) return;

    bool consume = true;
    switch (id) {
        case 87:
            if (p.currentLevel == 1 && HasCampMark(p)) {
                WarpToCampMark(p, levels);
                break;
            }
            MarkCampAndReturnToTown(p, levels);
            break;
        case 88:
            PlayerCombatStats::CureRandomAilment(p, rng);
            break;
        case 89:
            p.coreStats[2] = p.coreStats[3];
            break;
        case 90:
            p.coreStats[4] = p.coreStats[5];
            break;
        // A real, confirmed original quirk, preserved rather than
        // "corrected": adds 3*coreStats[5] (max MAGICKA, the same index
        // case 90 restores current Magicka to) to coreStats[6] (current
        // FATIGUE), uncapped -- NOT 3*coreStats[7] (max Fatigue), despite
        // this id sitting right after the Magicka restorative in the 87-99
        // sequence. Confirmed by reading useItem()'s own case 91 line
        // directly. Also the one id in this whole switch whose own
        // kSpecialEffectText entry (giftIdx 4, below) is blank -- possibly
        // unused/debug content in the original, not a live player-facing
        // item; ported faithfully either way.
        case 91:
            p.coreStats[6] = static_cast<int16_t>(p.coreStats[6] + 3 * p.coreStats[5]);
            break;
        case 92:
            p.coreStats[1]++;
            break;
        case 93:
            p.coreStats[2] = p.coreStats[3];
            p.coreStats[4] = p.coreStats[5];
            break;
        case 94:
            p.increaseHarmBuff = true;
            break;
        case 95:
            p.increaseArmorBuff = true;
            break;
        case 96:
            p.safeCampingBuff = true;
            consume = false;
            break;
        case 97:
            if (target != nullptr) {
                int a = MonsterRuntime::Stat(*target, monsters, 4);
                int b = MonsterRuntime::Stat(*target, monsters, 10);
                if (a <= 13 && b <= 13) {
                    target->currentHp = 0;
                    DungeonRuntime::StoreMonster(world, *target);
                }
            }
            break;
        case 98:
            if (target != nullptr) {
                int a = MonsterRuntime::Stat(*target, monsters, 4);
                int b = MonsterRuntime::Stat(*target, monsters, 10);
                if (a <= 22 && b <= 22) {
                    target->currentHp = 0;
                    DungeonRuntime::StoreMonster(world, *target);
                }
            }
            break;
        case 99:
            if (target != nullptr) {
                int a = MonsterRuntime::Stat(*target, monsters, 4);
                int b = MonsterRuntime::Stat(*target, monsters, 10);
                if (a <= 29 && b <= 29) {
                    target->currentHp = 0;
                    DungeonRuntime::StoreMonster(world, *target);
                }
            }
            break;
        default:
            break;
    }

    if (consume) {
        RemoveInventorySlot(p, slot, items);
    }
}

namespace {
// Item.specialEffectText: fixed flavor text for the 13 "gift"/special
// consumable item ids 87-99, in order, each pre-wrapped into up to 2
// display lines -- see ItemTooltip's own declaration comment for why this
// lives here as a literal table rather than a loaded asset.
const char* const kSpecialEffectText[13][2] = {
    {"Warp to camp", ""},        {"Cures ailment", ""},   {"Restores Health", ""},
    {"Restores Magicka", ""},    {"", ""},                {"Grants level", "experience"},
    {"Health & Magicka", ""},    {"Increase harm", ""},   {"Increase armor", ""},
    {"Safe camping", ""},        {"Kill monster", ""},    {"Kill monster", ""},
    {"Kill monster", ""},
};
}  // namespace

std::string PlayerInventory::ItemTooltip(const PlayerState& p, int slot, const ItemDatabase& items,
                                          const SpellDatabase& spells, const CharacterData& charData) {
    int id = std::abs(p.inventoryItemIds[static_cast<size_t>(slot)]);
    int8_t cat = items.category[static_cast<size_t>(id - 1)];
    const std::string& name = items.name[static_cast<size_t>(id - 1)];
    const std::string& catName = items.categoryNames[static_cast<size_t>(cat - 1)];

    switch (cat) {
        case 1:
        case 2:
        case 3:
        case 4: {
            // Item.column(3, id) -- the questFlags column doubling as a
            // weapon-value magnitude; see player_combat_stats.cpp's own
            // WeaponDamage for the same real quirk already documented.
            int wv = items.questFlags[static_cast<size_t>(id - 1)] +
                     (p.inventoryItemData[static_cast<size_t>(slot)] & 0xFF);
            return name + "\n" + catName + "\nWeapon value: " + std::to_string(wv);
        }
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10: {
            int av = items.questFlags[static_cast<size_t>(id - 1)] +
                     (p.inventoryItemData[static_cast<size_t>(slot)] & 0xFF);
            return name + "\n" + catName + "\nArmor value: " + std::to_string(av);
        }
        case 11:
            return name + "\n" + catName;
        case 12: {
            int spellId = p.inventoryItemData[static_cast<size_t>(slot)] & 0xFF;
            return name + "\nSpell: " + spells.ById(spellId).name;
        }
        case 13: {
            int giftIdx = id - 87;
            const char* const* lines = kSpecialEffectText[giftIdx];
            std::string out = name + "\n" + catName + "\n" + lines[0];
            if (lines[1][0] != '\0') out += std::string("\n") + lines[1];
            return out;
        }
        case 17: {
            int bonus = PlayerCombatStats::SkillValue(p, charData, /*skillIndex=*/3, false);
            int val17 = 20 + bonus;
            return name + "\n" + catName + "\nWeapon value: " + std::to_string(val17);
        }
        default:
            return name + "\n" + catName;
    }
}

}  // namespace stormhold
