#include "player/player_creation.h"

#include "player/player_combat_stats.h"
#include "player/player_inventory.h"

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

// Player.java's grantStartingItems(): grants classIndex's starting item
// pair and auto-equips each one. addInventoryItem()/equipItem() (which
// GrantStartingItems used to carry small private copies of here) now
// live in player/player_inventory.h, shared with player/
// player_spellcasting.h's castOnSelf.
void GrantStartingItems(PlayerState& p, const ItemDatabase& items) {
    // Item.nextSpawnId() is a pure counter with no bearing on generation
    // correctness -- same simplification DungeonGenerator's chest
    // placement made (see world/dungeon_generator.cpp): a fixed
    // placeholder rather than the real cross-character-creation running
    // total, since nothing reads it yet.
    int spawnId = 1;
    const int(&startingItems)[2] = kStartingItems[p.classIndex];

    for (int itemId : startingItems) {
        PlayerInventory::AddItem(p, itemId, spawnId, 0);
        int slot = p.inventoryCount - 1;
        PlayerInventory::Equip(p, items, slot, true);
    }
}

}  // namespace

// Player.java's private computeStartingSpellMask(): for 5 specific
// skill slots (mapped to bit positions 0/5/10/15/20, i.e. spell ids
// 1/6/11/16/21), a nonzero class-template threshold grants that tier's
// first spell. Also sets p.selectedSpellId to the first one granted --
// a real side effect on `p` itself, not a pure query. Exposed as a
// public method (rather than kept file-private, like GrantStartingItems
// above) because player/player_save.h's ToBytesSummary calls this
// against a LIVE character being saved, exactly as Player.java's
// toBytes(false) does via this.computeStartingSpellMask() -- see that
// method's doc comment for the surprising consequence.
uint32_t PlayerCreation::ComputeStartingSpellMask(PlayerState& p, const CharacterData& charData) {
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

    p.knownSpellsMask = PlayerCreation::ComputeStartingSpellMask(p, charData);

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

// Player.java's buildCreationSummary(), transcribed line-for-line --
// shorter than buildCharacterSheet() (M39's own ui/options_menu.cpp):
// race+class, level/HP/Magicka/Fatigue (via statLabels[0/2/4/6], NOT
// hardcoded "Level"/"Health"/... literals like buildCharacterSheet
// uses -- a real, deliberate difference between the two methods in the
// original, ported exactly), all 8 attributes, then only the skills
// with a nonzero rank (a fresh character's own class-template starting
// ranks, since this is only ever called before any skill exp has been
// gained).
std::string PlayerCreation::BuildCreationSummary(const PlayerState& p, const CharacterData& charData) {
    std::string out = charData.raceNames[static_cast<size_t>(p.raceIndex)] + " " +
                       charData.classNames[static_cast<size_t>(p.classIndex)] + "\n";
    out += charData.statLabels[0] + ": " + std::to_string(p.coreStats[0]) + "\n";
    out += charData.statLabels[2] + ": " + std::to_string(PlayerCombatStats::EffectiveStat(p, charData, 2)) + "\n";
    out += charData.statLabels[4] + ": " + std::to_string(PlayerCombatStats::EffectiveStat(p, charData, 4)) + "\n";
    out += charData.statLabels[6] + ": " + std::to_string(PlayerCombatStats::EffectiveStat(p, charData, 6)) + "\n";
    for (int i = 0; i < 8; i++) {
        int slot = 2 * i;
        out += charData.attributeNames[static_cast<size_t>(slot)] + ": " +
               std::to_string(p.attributes[static_cast<size_t>(slot)]) + "\n";
    }
    for (int i = 0; i < 14; i++) {
        if (p.skills[static_cast<size_t>(i)][0] > 0) {
            out += charData.skillNames[static_cast<size_t>(i)] + ": " +
                   std::to_string(p.skills[static_cast<size_t>(i)][0]) + "\n";
        }
    }
    return out;
}

}  // namespace dawnstar
