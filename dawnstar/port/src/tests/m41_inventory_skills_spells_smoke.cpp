// M41 smoke test: OptionsMenu's newly-real "Inventory"/"Skills"/"Spells"
// actions (ui/options_menu.h) -- ESGame.java's own secondaryParam
// 33/34/35/36/37/38 dispatch, M39 had deferred as silent no-ops.
//
// No JVM ground truth is available (same reason as every prior
// milestone). Every expected value is independently re-derived from
// ../../../src/Player.java's own itemTooltip()/knownSkillsSummary()/
// nthKnownSkillIndex()/skillTooltip()/knownSpellsSummary()/spellTooltip()
// -- reimplemented AGAIN here, not read back from player_inventory.cpp/
// player_combat_stats.cpp/player_spellcasting.cpp/options_menu.cpp, so
// this test can't just be checking those implementations' own arithmetic
// against itself (same standard M38/M39/M40's own tests already hold
// to). PlayerCombatStats::SkillValue/EffectiveStat (M14) and
// PlayerInventory::CanEquipOrUnequip/IsEquipped (M16-era, long since
// independently verified) are reused as already-trusted building blocks,
// same precedent M39's own ExpectedCharacterSheet already set for
// PlayerCombatStats::EffectiveStat/HasAilment.
#include <cstdio>
#include <cstdlib>
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
#include "combat/combat_resolution.h"
#include "dungeon/dungeon_runtime.h"
#include "graphics/backbuffer.h"
#include "graphics/bitmap_font.h"
#include "player/player_combat_stats.h"
#include "player/player_creation.h"
#include "player/player_inventory.h"
#include "player/player_state.h"
#include "render/message_popup.h"
#include "ui/options_menu.h"
#include "ui/screen.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::Backbuffer;
namespace BitmapFont = dawnstar::BitmapFont;
using dawnstar::CharacterData;
using dawnstar::CombatResolution;
using dawnstar::GeneratedLevel;
using dawnstar::ItemDatabase;
using dawnstar::MessagePopup;
using dawnstar::OptionsMenu;
using dawnstar::OptionsMenuAction;
using dawnstar::PlayerCombatStats;
using dawnstar::PlayerInventory;
using dawnstar::PlayerState;
using dawnstar::Screen;
using dawnstar::SpellDatabase;
using dawnstar::WorldRegistry;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

uint16_t PixelAt(const Backbuffer& bb, int x, int y) { return bb.Data()[static_cast<size_t>(y) * Backbuffer::kWidth + x]; }

// Same oracle technique m37/m38/m39's own tests established.
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

constexpr uint16_t kTitleTextColor = dawnstar::PackRGB565(255, 255, 255);
constexpr uint16_t kItemTextColor = dawnstar::PackRGB565(255, 255, 0);

int CenteredTitleX(const std::string& title) { return Screen::width() / 2 - BitmapFont::StringWidth(title) / 2; }

bool TitleShownIs(const Backbuffer& bb, const std::string& title) {
    return TextRenderedAt(bb, CenteredTitleX(title), 0, title, kTitleTextColor);
}

// A PromptList screen's own first item row -- Screen::RenderPromptList's
// `20 + promptLines.size()*12 + 5` formula (see screen.cpp), for a prompt
// that word-wraps to `promptLineCount` lines, marginX_=10.
int FirstItemY(int promptLineCount) { return 20 + promptLineCount * 12 + 5; }

int WrappedLineCount(const std::string& text) {
    return static_cast<int>(MessagePopup::WordWrap(text, Screen::width() - 10 - 10).size());
}

// --- Independent re-derivations of Player.java's own methods (NOT
// reused from player_inventory.cpp/player_combat_stats.cpp/
// player_spellcasting.cpp) ---

std::vector<std::string> ExpectedInventoryNames(const PlayerState& p, const ItemDatabase& items) {
    std::vector<std::string> out;
    for (int i = 0; i < p.inventoryCount; i++) {
        int id = static_cast<int>(p.inventoryItemIds[static_cast<size_t>(i)]);
        std::string name = items.name[static_cast<size_t>(std::abs(id) - 1)];
        out.push_back(id < 0 ? "E: " + name : name);
    }
    return out;
}

// Item.java's own specialEffectText, transcribed AGAIN here (independent
// of player_inventory.cpp's own kSpecialEffectText).
const char* const kExpectedGiftText[13] = {
    "Warp to camp",       "Cures ailment",          "Restores Health",
    "Restores Magicka",   " ",                      "Grants level experience",
    "Health & Magicka",   "Increase harm",          "Increase armor",
    "Safe camping",       "Kills weak monster",     "Kills normal monster",
    "Kills strong monster",
};

std::string ExpectedItemTooltip(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items,
                                 const SpellDatabase& spells, int slot) {
    int itemId = std::abs(static_cast<int>(p.inventoryItemIds[slot]));
    int8_t category = items.category[static_cast<size_t>(itemId - 1)];
    std::string base = items.name[static_cast<size_t>(itemId - 1)] + "\n";
    switch (category) {
        case 1: case 2: case 3: case 4: {
            int weaponValue = items.questFlags[static_cast<size_t>(itemId - 1)] + (p.inventoryItemData[slot] & 0xFF);
            return base + items.categoryNames[static_cast<size_t>(category - 1)] + "\nWeapon value: " +
                   std::to_string(weaponValue);
        }
        case 5: case 6: case 7: case 8: case 9: case 10: {
            int armorValue = items.questFlags[static_cast<size_t>(itemId - 1)] + (p.inventoryItemData[slot] & 0xFF);
            return base + items.categoryNames[static_cast<size_t>(category - 1)] + "\nArmor value: " +
                   std::to_string(armorValue);
        }
        case 12: {
            int spellId = p.inventoryItemData[slot] & 0xFF;
            std::string text = items.name[static_cast<size_t>(itemId - 1)] + "\nSpell: " +
                                spells.all[static_cast<size_t>(spellId - 1)].name;
            if ((p.knownSpellsMask & (1u << (spellId - 1))) != 0) text += " (known)";
            return text;
        }
        case 13:
            return base + items.categoryNames[static_cast<size_t>(category - 1)] + "\n" +
                   kExpectedGiftText[itemId - 87];
        case 15: {
            int bonus = PlayerCombatStats::SkillValue(p, charData, 3, false);
            return base + items.categoryNames[static_cast<size_t>(category - 1)] + "\nWeapon value: " +
                   std::to_string(20 + bonus);
        }
        default:
            return base + items.categoryNames[static_cast<size_t>(category - 1)];
    }
}

std::vector<std::string> ExpectedSkillsSummary(const PlayerState& p, const CharacterData& charData) {
    std::vector<std::string> out;
    for (int i = 0; i < 14; i++) {
        if (p.skills[static_cast<size_t>(i)][0] > 0) {
            out.push_back(charData.skillNames[static_cast<size_t>(i)] + ": " +
                          std::to_string(p.skills[static_cast<size_t>(i)][0]));
        }
    }
    return out;
}

int ExpectedNthKnownSkillIndex(const PlayerState& p, int index) {
    int seen = 0;
    for (int i = 0; i < 14; i++) {
        if (p.skills[static_cast<size_t>(i)][0] > 0) {
            if (seen == index) return i;
            seen++;
        }
    }
    return -1;
}

std::string ExpectedSkillTooltip(const PlayerState& p, const CharacterData& charData, int skillIndex) {
    return charData.skillNames[static_cast<size_t>(skillIndex)] + "\nRank: " +
           std::to_string(p.skills[static_cast<size_t>(skillIndex)][0]) + "\nExp: " +
           std::to_string(p.skills[static_cast<size_t>(skillIndex)][2]) + "/10";
}

std::vector<std::string> ExpectedSpellsSummary(const PlayerState& p, const SpellDatabase& spells) {
    std::vector<std::string> out;
    for (int i = 0; i < spells.Count(); i++) {
        if ((p.knownSpellsMask & (1u << i)) != 0) {
            int spellId = i + 1;
            std::string line = spells.all[static_cast<size_t>(i)].name;
            if (spellId == p.selectedSpellId) line = "R: " + line;
            out.push_back(line);
        }
    }
    return out;
}

int ExpectedNthKnownSpellId(const PlayerState& p, const SpellDatabase& spells, int index) {
    int seen = 0;
    for (int i = 0; i < spells.Count(); i++) {
        if ((p.knownSpellsMask & (1u << i)) != 0) {
            if (seen == index) return i;
            seen++;
        }
    }
    return -1;
}

std::string ExpectedSpellTooltip(const CharacterData& charData, const SpellDatabase& spells, int index0) {
    const dawnstar::Spell& s = spells.all[static_cast<size_t>(index0)];
    return s.name + "\n" + charData.skillNames[static_cast<size_t>(s.skillRequired)] + "\nCost: " +
           std::to_string(s.magickaCost) + "\n" + s.description;
}

// Navigates a fresh/currently-Options OptionsMenu down to `idx` (0-based)
// from a forced-known index 0, same precedent M38/M39's own tests use.
void SelectOptionsIndex(OptionsMenu& menu, int idx) {
    for (int i = 0; i < 10; i++) menu.OnUp();
    for (int i = 0; i < idx; i++) menu.OnDown();
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        ItemDatabase items = ItemDatabase::Load(archive);
        dawnstar::MonsterDatabase monsterDb = dawnstar::MonsterDatabase::Load(archive);
        SpellDatabase spells = SpellDatabase::Load(archive);
        CharacterData charData = CharacterData::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);
        dawnstar::HelpText helpText = dawnstar::HelpText::Load(archive);
        dawnstar::ShopDialogue shopDialogue = dawnstar::ShopDialogue::Load(root + "/npcstrings.dat");

        std::vector<GeneratedLevel> levels;
        WorldRegistry world(geometry.rows.size());
        for (size_t i = 0; i < geometry.rows.size(); i++) {
            int levelNumber = static_cast<int>(i) + 1;
            levels.push_back(levelNumber == 1
                                  ? dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[0])
                                  : dawnstar::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[i], items,
                                                                              monsterDb));
            dawnstar::DungeonRuntime::RegisterGeneratedSpawns(levels.back(), world);
        }

        // M11's own finding: a spellcasting class (Sorcerer/High Elf)
        // starts with known spells, needed to exercise the real Spells
        // screen meaningfully -- found by NAME rather than a hardcoded
        // index, same "M4's swapped classNames/raceNames" lesson every
        // milestone since has applied.
        int sorcererIndex = -1;
        for (size_t i = 0; i < charData.classNames.size(); i++) {
            if (charData.classNames[i] == "Sorcerer") sorcererIndex = static_cast<int>(i);
        }
        Check(sorcererIndex >= 0, "the real charin.dat should have a Sorcerer class");

        dawnstar::JavaRandom rng(20260913);
        PlayerState player = dawnstar::PlayerCreation::CreateCharacter(sorcererIndex, "Caster", charData, items, rng);
        Check(player.inventoryCount > 0, "a fresh Sorcerer should start with at least one inventory item");

        // M43: OnSelect's grown `nextItemSpawnId` parameter -- a plain
        // stand-in for main.cpp's own nextDropSpawnId, started at the same
        // 1 this test doesn't stress (m43_reveal_traitor_smoke.cpp does).
        int16_t nextItemSpawnId = 1;
        OptionsMenu menu(helpText, shopDialogue);
        Backbuffer bb;

        // --- A: Options -> "Inventory" (index 1) -> the real item list. ---
        SelectOptionsIndex(menu, 1);
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::None,
              "selecting Inventory should not itself be a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Inventory"), "selecting Inventory should show the real Inventory list");
        std::vector<std::string> expectedNames = ExpectedInventoryNames(player, items);
        std::string expectedPrompt = "Your gold: " + std::to_string(player.gold);
        Check(TextRenderedAt(bb, 10, FirstItemY(WrappedLineCount(expectedPrompt)), expectedNames[0], kItemTextColor),
              "the Inventory list's own first item should match the real, independently-derived item name");

        // --- B: select the first item (index 0) -> the real Item screen
        // (tooltip + Drop/[Equip-or-Unequip]/[Learn]/[Use] actions). ---
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::None,
              "selecting an inventory item should not itself be a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Item"), "selecting an inventory item should show the real Item screen");
        std::string expectedTooltip0 = ExpectedItemTooltip(player, charData, items, spells, 0);
        std::vector<std::string> tooltip0Lines = MessagePopup::WordWrap(expectedTooltip0, Screen::width() - 10 - 10);
        Check(TextRenderedAt(bb, 10, 20, tooltip0Lines[0], kItemTextColor),
              "the Item screen's own tooltip should match the real, independently-derived itemTooltip() text");
        Check(TextRenderedAt(bb, 10, FirstItemY(static_cast<int>(tooltip0Lines.size())), "Drop", kItemTextColor),
              "the Item screen's own action list should always start with \"Drop\"");

        // --- C: Cancel chain: Item -> Inventory -> Options. ---
        Check(menu.OnCancel() == OptionsMenuAction::None, "Cancel on the Item screen is not a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Inventory"), "Cancel on the Item screen should return to Inventory, not Options");
        Check(menu.OnCancel() == OptionsMenuAction::None, "Cancel on the Inventory list is not a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Options"), "Cancel on the Inventory list should return to Options");

        // --- D: Equip/Unequip round trip on whichever of the Sorcerer's
        // own real starting slots is actually equip/unequip-gated
        // (found by scanning, not assumed to be slot 0 -- starting
        // items aren't granted in a documented order). ---
        int equipSlot = -1;
        for (int i = 0; i < player.inventoryCount; i++) {
            if (PlayerInventory::CanEquipOrUnequip(player, items, i)) {
                equipSlot = i;
                break;
            }
        }
        Check(equipSlot >= 0, "a Sorcerer's own real starting gear should include at least one equip/unequip-gated item");
        bool wasEquipped = PlayerInventory::IsEquipped(player, items, equipSlot);
        SelectOptionsIndex(menu, 1);
        menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId);  // Options -> Inventory
        for (int i = 0; i < 10; i++) menu.OnUp();
        for (int i = 0; i < equipSlot; i++) menu.OnDown();
        menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId);  // Inventory -> Item(equipSlot)
        // Drop is always index 0; Equip/Unequip is always index 1 when
        // present (RebuildInventoryItem's own conditional order).
        menu.OnDown();
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::None,
              "toggling Equip/Unequip should not itself be a main.cpp-level action");
        Check(PlayerInventory::IsEquipped(player, items, equipSlot) != wasEquipped,
              "selecting Equip/Unequip should really flip the item's equipped state");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Inventory"), "finishing Equip/Unequip should return straight to Inventory");

        // --- E: Drop -- a fresh, isolated item added just for this
        // scenario (category-11 "misc" item, found by scanning the real
        // item table rather than guessing an id), dropped at a
        // controlled position, and confirmed to land in the live
        // WorldRegistry's own dropped-item list at that exact tile. ---
        int miscItemId = -1;
        for (size_t i = 0; i < items.category.size(); i++) {
            if (items.category[i] == 11) {
                miscItemId = static_cast<int>(i) + 1;
                break;
            }
        }
        Check(miscItemId > 0, "the real item table should have at least one category-11 item");
        player.currentLevel = 3;
        player.tileX = 6;
        player.tileY = 9;
        PlayerInventory::AddItem(player, miscItemId, 0, 0);
        int dropSlot = player.inventoryCount - 1;
        int countBeforeDrop = player.inventoryCount;
        menu.OnCancel();  // Inventory -> Options (scenario D's own finish already left this on Inventory)
        SelectOptionsIndex(menu, 1);
        menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId);  // Options -> Inventory (fresh rebuild)
        for (int i = 0; i < 10; i++) menu.OnUp();
        for (int i = 0; i < dropSlot; i++) menu.OnDown();
        menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId);  // Inventory -> Item(dropSlot)
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::None,
              "Drop (index 0) is not itself a main.cpp-level action");
        Check(player.inventoryCount == countBeforeDrop - 1, "Drop should remove the item from the inventory");
        auto& dropped = world.droppedItems[static_cast<size_t>(player.currentLevel - 1)];
        bool foundDrop = false;
        for (const auto& rec : dropped) {
            if (rec[0] == 6 && rec[1] == 9 && rec[2] == static_cast<uint8_t>(miscItemId)) foundDrop = true;
        }
        Check(foundDrop, "Drop should register the item in the live WorldRegistry at the player's own tile");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Inventory"), "finishing Drop should return to a freshly-rebuilt Inventory list");

        // A real, faithfully-preserved quirk: StarFrost (id 101) is
        // dropped from the inventory but NEVER actually placed on the
        // ground (see Player.java's own dropInventoryItem() doc
        // comment).
        PlayerInventory::AddItem(player, 101, 0, 0);
        int starFrostSlot = player.inventoryCount - 1;
        int droppedCountBefore = static_cast<int>(dropped.size());
        menu.OnCancel();  // Options
        SelectOptionsIndex(menu, 1);
        menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId);
        for (int i = 0; i < 10; i++) menu.OnUp();
        for (int i = 0; i < starFrostSlot; i++) menu.OnDown();
        menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId);
        menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId);  // Drop
        Check(static_cast<int>(dropped.size()) == droppedCountBefore,
              "dropping StarFrost (id 101) should NOT add a dropped-item record -- a real, preserved quirk");

        // --- F: Learn a spell scroll -- a fresh category-12 item,
        // engineered (via AddItem's own charge param) to encode a known,
        // valid spell id, with its required skill bumped to rank>0 so
        // CanLearnSpell actually gates true. ---
        int scrollItemId = -1;
        for (size_t i = 0; i < items.category.size(); i++) {
            if (items.category[i] == 12) {
                scrollItemId = static_cast<int>(i) + 1;
                break;
            }
        }
        Check(scrollItemId > 0, "the real item table should have at least one category-12 scroll item");
        int learnSpellId = 1;
        int requiredSkill = spells.all[static_cast<size_t>(learnSpellId - 1)].skillRequired;
        player.knownSpellsMask &= ~(1u << (learnSpellId - 1));  // ensure not already known
        player.skills[static_cast<size_t>(requiredSkill)][0] = 5;
        PlayerInventory::AddItem(player, scrollItemId, 0, learnSpellId);
        int scrollSlot = player.inventoryCount - 1;
        menu.OnCancel();  // Options (from scenario E's Inventory)
        SelectOptionsIndex(menu, 1);
        menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId);
        for (int i = 0; i < 10; i++) menu.OnUp();
        for (int i = 0; i < scrollSlot; i++) menu.OnDown();
        menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId);  // Inventory -> Item(scrollSlot)
        int countBeforeLearn = player.inventoryCount;
        // Drop(0) always present; Equip/Unequip absent for a category-12
        // scroll (CanEquipOrUnequip is category 1-10/15 only); Learn is
        // therefore index 1.
        menu.OnDown();
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::None,
              "Learn is not itself a main.cpp-level action");
        Check((player.knownSpellsMask & (1u << (learnSpellId - 1))) != 0,
              "Learn should really set the corresponding knownSpellsMask bit");
        Check(player.inventoryCount == countBeforeLearn - 1, "Learn should consume the scroll");

        // --- G: Use -- item 87 ("Warp to Camp") should round-trip
        // through OptionsMenuAction::UseInventoryItem/FinishUseItem
        // (simulating main.cpp's own real CombatResolution::UseItem
        // call in between), and its real suppressStrafeAdjust side
        // effect should route straight back to the game view. ---
        PlayerInventory::AddItem(player, 87, 0, 0);
        int warpSlot = player.inventoryCount - 1;
        menu.OnCancel();  // Options
        SelectOptionsIndex(menu, 1);
        menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId);
        for (int i = 0; i < 10; i++) menu.OnUp();
        for (int i = 0; i < warpSlot; i++) menu.OnDown();
        menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId);  // Inventory -> Item(warpSlot)
        // Drop(0); no Equip/Unequip (category 13); no Learn; Use is
        // index 1.
        menu.OnDown();
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::UseInventoryItem,
              "selecting Use on a real gift item should ask main.cpp to perform the actual UseItem call");
        Check(menu.PendingUseItemSlot() == warpSlot, "PendingUseItemSlot should be the slot Use was chosen for");
        CombatResolution::UseItem(player, menu.PendingUseItemSlot(), nullptr, items, monsterDb, levels, world, rng);
        Check(player.suppressStrafeAdjust, "item 87 should really set suppressStrafeAdjust");
        Check(menu.FinishUseItem(player, items) == OptionsMenuAction::ReturnToGame,
              "finishing item 87's Use should route straight back to the game view");
        Check(!player.suppressStrafeAdjust, "FinishUseItem should clear suppressStrafeAdjust, same as the original");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Options"), "after item 87's Use, the menu's own internal state should be back at Options");

        // --- H: item 96 ("Safe Camping") is NOT consumed on use (a real
        // quirk M18 already established) and does not set
        // suppressStrafeAdjust, so Use should return to the Inventory
        // list instead. ---
        PlayerInventory::AddItem(player, 96, 0, 0);
        int campItemSlot = player.inventoryCount - 1;
        SelectOptionsIndex(menu, 1);
        menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId);  // Options -> Inventory
        for (int i = 0; i < 10; i++) menu.OnUp();
        for (int i = 0; i < campItemSlot; i++) menu.OnDown();
        menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId);  // Inventory -> Item(campItemSlot)
        menu.OnDown();  // Use (index 1: Drop(0), no Equip/Learn, Use(1))
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::UseInventoryItem,
              "selecting Use on item 96 should also ask main.cpp to perform the real UseItem call");
        int countBeforeCampUse = player.inventoryCount;
        CombatResolution::UseItem(player, menu.PendingUseItemSlot(), nullptr, items, monsterDb, levels, world, rng);
        Check(menu.FinishUseItem(player, items) == OptionsMenuAction::None,
              "finishing item 96's Use should return to Inventory, not the game view");
        Check(player.inventoryCount == countBeforeCampUse,
              "item 96 should NOT be consumed on use -- a real, preserved quirk (M18)");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Inventory"), "finishing item 96's Use should return to a freshly-rebuilt Inventory list");

        // --- I: Skills. ---
        menu.OnCancel();  // Inventory -> Options
        SelectOptionsIndex(menu, 3);
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::None,
              "selecting Skills should not itself be a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Skills"), "selecting Skills should show the real Skills list");
        std::vector<std::string> expectedSkills = ExpectedSkillsSummary(player, charData);
        Check(!expectedSkills.empty(), "a Sorcerer should have at least one rank>0 skill from character creation");
        Check(TextRenderedAt(bb, 10, FirstItemY(WrappedLineCount("Your Skills:")), expectedSkills[0], kItemTextColor),
              "the Skills list's own first item should match the real, independently-derived summary");

        int firstSkillIndex = ExpectedNthKnownSkillIndex(player, 0);
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::None,
              "selecting a skill should not itself be a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Skill Info"), "selecting a skill should show the real Skill Info screen");
        std::string expectedSkillTooltip = ExpectedSkillTooltip(player, charData, firstSkillIndex);
        std::vector<std::string> skillTooltipLines = MessagePopup::WordWrap(expectedSkillTooltip, Screen::width() - 5 - 5);
        Check(TextRenderedAt(bb, 5, 20, skillTooltipLines[0], kItemTextColor),
              "the Skill Info screen should match the real, independently-derived skillTooltip() text");
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::None,
              "Ok on Skill Info is not a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Skills"), "Ok on Skill Info should return to Skills, not Options");
        Check(menu.OnCancel() == OptionsMenuAction::None, "Cancel on Skills is not a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Options"), "Cancel on Skills should return to Options");

        // --- J: Spells. ---
        SelectOptionsIndex(menu, 4);
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::None,
              "selecting Spells should not itself be a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Spells"), "selecting Spells should show the real Spells list");
        std::vector<std::string> expectedSpells = ExpectedSpellsSummary(player, spells);
        Check(!expectedSpells.empty(), "a Sorcerer should start with at least one known spell (M11)");
        Check(TextRenderedAt(bb, 10, FirstItemY(WrappedLineCount("Your Spells:")), expectedSpells[0], kItemTextColor),
              "the Spells list's own first item should match the real, independently-derived summary");

        int firstSpellIndex0 = ExpectedNthKnownSpellId(player, spells, 0);
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::None,
              "selecting a spell should not itself be a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Spell Info"), "selecting a spell should show the real Spell Info screen");
        std::string expectedSpellTooltip0 = ExpectedSpellTooltip(charData, spells, firstSpellIndex0);
        std::vector<std::string> spellTooltipLines = MessagePopup::WordWrap(expectedSpellTooltip0, Screen::width() - 10 - 10);
        Check(TextRenderedAt(bb, 10, 20, spellTooltipLines[0], kItemTextColor),
              "the Spell Info screen should match the real, independently-derived spellTooltip() text");
        Check(TextRenderedAt(bb, 10, FirstItemY(static_cast<int>(spellTooltipLines.size())), "Ready Spell", kItemTextColor),
              "the Spell Info screen's own action should be \"Ready Spell\"");

        // "Ready Spell" should set selectedSpellId and return to a
        // freshly-rebuilt Spells list with the same slot still selected.
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::None,
              "Ready Spell is not itself a main.cpp-level action");
        Check(player.selectedSpellId == static_cast<int8_t>(firstSpellIndex0 + 1),
              "Ready Spell should set selectedSpellId to the real 1-based spell id");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Spells"), "Ready Spell should return to Spells, not Options");
        std::vector<std::string> expectedSpellsAfterReady = ExpectedSpellsSummary(player, spells);
        Check(TextRenderedAt(bb, 10, FirstItemY(WrappedLineCount("Your Spells:")), expectedSpellsAfterReady[0],
                             kItemTextColor),
              "the freshly-rebuilt Spells list should show the real \"R: \"-prefixed selection");

        // Cancel from Spell Info returns to Spells (not Options); Cancel
        // from Spells returns to Options.
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::None,
              "re-selecting a known spell should not itself be a main.cpp-level action");
        Check(menu.OnCancel() == OptionsMenuAction::None, "Cancel on Spell Info is not a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Spells"), "Cancel on Spell Info should return to Spells, not Options");
        Check(menu.OnCancel() == OptionsMenuAction::None, "Cancel on Spells is not a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Options"), "Cancel on Spells should return to Options");

        if (g_ok) {
            std::printf("all inventory/skills/spells checks passed\n");
            return 0;
        } else {
            std::printf("SOME CHECKS FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("EXCEPTION: %s\n", e.what());
        return 1;
    }
}
