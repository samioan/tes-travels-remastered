// M62 smoke test: ui/inventory_ui.h's InventoryUi -- the item-list +
// per-slot action-menu state machine (ESGame.java's newInventoryUI()/
// newInventoryItemUI(), GameCanvas.openInventory()'s own dispatch), the
// last branch of M41's dispatch web, built on M61's already-ported
// PlayerInventory action logic and this milestone's new IsEquippedSlot.
#include <cstdio>
#include <cstdlib>
#include <string>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/spell_database.h"
#include "dungeon/dungeon_runtime.h"
#include "player/player_creation.h"
#include "player/player_inventory.h"
#include "ui/inventory_ui.h"

namespace {

using namespace stormhold;

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

GeneratedLevel MakeSyntheticLevel(int number) {
    GeneratedLevel level;
    level.number = number;
    level.width = 35;
    level.height = 35;
    level.tiles.assign(35, std::vector<uint8_t>(35, 0));
    level.populated = true;
    return level;
}

void TestOpenAndCancel() {
    std::printf("-- InventoryUi::Open/Cancel --\n");
    InventoryUiState state;
    Expect(!state.active, "a fresh InventoryUiState should start inactive");

    InventoryUi::Open(state);
    Expect(state.active, "Open should set active");
    Expect(state.screen == InventoryScreen::List, "Open should land on the List screen");
    Expect(state.selectedIndex == 0, "Open should reset the cursor to 0");
    Expect(state.selectedSlot == -1, "Open should clear selectedSlot");

    // From the List screen, Cancel closes the whole thing back to live
    // gameplay -- the deliberate port-only mapping documented in
    // ui/inventory_ui.h's own Cancel comment (the real game's own
    // `nextScreen` there is the not-yet-built pause menu).
    InventoryUi::Cancel(state);
    Expect(!state.active, "Cancel from the List screen should close the whole inventory view");
}

void TestMoveSelectionClamps(const CharacterData& charData, const ItemDatabase& items) {
    std::printf("-- InventoryUi::MoveSelection clamping --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    Expect(p.inventoryCount > 1, "a fresh character should start with more than one inventory item");

    InventoryUiState state;
    InventoryUi::Open(state);
    InventoryUi::MoveSelection(state, -1, p);
    Expect(state.selectedIndex == 0, "MoveSelection should clamp at 0, not wrap");

    InventoryUi::MoveSelection(state, 1000, p);
    Expect(state.selectedIndex == p.inventoryCount - 1, "MoveSelection should clamp at inventoryCount-1, not overflow");

    // Inactive: a no-op.
    InventoryUiState inactive;
    InventoryUi::MoveSelection(inactive, 5, p);
    Expect(inactive.selectedIndex == 0, "MoveSelection on an inactive state should do nothing");
}

void TestSelectSlotBuildsActionMenu(const CharacterData& charData, const ItemDatabase& items,
                                    const SpellDatabase& spells, const MonsterDatabase& monsters) {
    std::printf("-- InventoryUi::Confirm (List -> ItemAction) --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    GeneratedLevel level = MakeSyntheticLevel(1);
    WorldRegistry world(1);
    JavaRandom rng(1);

    int slot = 0;
    bool wantEquip = PlayerInventory::CanEquipOrUnequip(p, slot, items);
    bool wantLearn = PlayerInventory::CanLearnSpellFromScroll(p, slot, items, spells);
    bool wantUse = PlayerInventory::CanUseItem(p, slot, items);

    InventoryUiState state;
    InventoryUi::Open(state);
    state.selectedIndex = slot;
    InventoryUi::Confirm(state, p, items, spells, monsters, level, world, rng);

    Expect(state.screen == InventoryScreen::ItemAction, "selecting a valid slot should move to the ItemAction screen");
    Expect(state.selectedSlot == slot, "selectedSlot should record which slot the action menu is about");
    Expect(state.selectedIndex == 0, "the action menu's own cursor should reset to 0");
    Expect(!state.actionLabels.empty() && state.actionLabels[0] == "Drop" && state.actionCodes[0] == 0,
           "Drop should always be the first action");

    bool haveEquip = false, haveLearn = false, haveUse = false;
    for (size_t i = 0; i < state.actionCodes.size(); i++) {
        if (state.actionCodes[i] == 1) haveEquip = true;
        if (state.actionCodes[i] == 2) haveLearn = true;
        if (state.actionCodes[i] == 3) haveUse = true;
    }
    Expect(haveEquip == wantEquip, "Equip/Unequip should appear iff CanEquipOrUnequip says so");
    Expect(haveLearn == wantLearn, "Learn should appear iff CanLearnSpellFromScroll says so");
    Expect(haveUse == wantUse, "Use should appear iff CanUseItem says so");

    // An out-of-range slot (e.g. the real UIScreen's own -1 for an empty
    // list) should be a no-op, not a crash.
    InventoryUiState state2;
    InventoryUi::Open(state2);
    state2.selectedIndex = -1;
    InventoryUi::Confirm(state2, p, items, spells, monsters, level, world, rng);
    Expect(state2.screen == InventoryScreen::List, "an out-of-range slot selection should stay on the List screen");
}

void TestDropAction(const CharacterData& charData, const ItemDatabase& items, const SpellDatabase& spells,
                     const MonsterDatabase& monsters) {
    std::printf("-- InventoryUi::Confirm: Drop --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    p.currentLevel = 1;
    p.tileX = 5;
    p.tileY = 5;
    GeneratedLevel level = MakeSyntheticLevel(1);
    WorldRegistry world(1);
    JavaRandom rng(1);

    int slot = 0;
    int beforeCount = p.inventoryCount;
    bool droppable = std::abs(p.inventoryItemIds[static_cast<size_t>(slot)]) != 109;

    InventoryUiState state;
    InventoryUi::Open(state);
    state.selectedIndex = slot;
    InventoryUi::Confirm(state, p, items, spells, monsters, level, world, rng);
    Expect(state.screen == InventoryScreen::ItemAction, "should be on the action menu after selecting slot 0");

    // "Drop" is always action code 0 -- find its cursor position.
    int dropCursor = -1;
    for (size_t i = 0; i < state.actionCodes.size(); i++) {
        if (state.actionCodes[i] == 0) dropCursor = static_cast<int>(i);
    }
    Expect(dropCursor >= 0, "Drop should always be present");
    state.selectedIndex = dropCursor;
    InventoryUi::Confirm(state, p, items, spells, monsters, level, world, rng);

    Expect(state.screen == InventoryScreen::List, "after Drop, control should return to the List screen");
    Expect(state.selectedSlot == -1, "after Drop, selectedSlot should be cleared");
    Expect(p.inventoryCount == beforeCount - 1, "Drop should consume the inventory slot");
    if (droppable) {
        Expect(!world.droppedItems[0].empty(), "Drop should register a dropped-item record for a droppable item");
    }
}

void TestEquipUnequipToggle(const CharacterData& charData, const ItemDatabase& items, const SpellDatabase& spells,
                             const MonsterDatabase& monsters) {
    std::printf("-- InventoryUi::Confirm: Equip/Unequip toggle --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    GeneratedLevel level = MakeSyntheticLevel(1);
    WorldRegistry world(1);
    JavaRandom rng(1);

    int slot = -1;
    for (int i = 0; i < p.inventoryCount; i++) {
        if (PlayerInventory::CanEquipOrUnequip(p, i, items) && !PlayerInventory::IsEquippedSlot(p, i, items)) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        std::printf("  (no unequipped equippable starting item found -- skipping)\n");
        return;
    }

    InventoryUiState state;
    InventoryUi::Open(state);
    state.selectedIndex = slot;
    InventoryUi::Confirm(state, p, items, spells, monsters, level, world, rng);
    int equipCursor = -1;
    for (size_t i = 0; i < state.actionCodes.size(); i++) {
        if (state.actionCodes[i] == 1) equipCursor = static_cast<int>(i);
    }
    Expect(equipCursor >= 0, "an equippable, unequipped item should offer an Equip/Unequip action");
    Expect(state.actionLabels[static_cast<size_t>(equipCursor)] == "Equip",
           "an unequipped item's label should read Equip");

    state.selectedIndex = equipCursor;
    InventoryUi::Confirm(state, p, items, spells, monsters, level, world, rng);
    Expect(PlayerInventory::IsEquippedSlot(p, slot, items), "confirming Equip should actually equip the slot");

    // Reselect the same underlying item (still at `slot`, item ids don't
    // move on equip -- only their sign does) and confirm the label now
    // reads Unequip.
    InventoryUi::Open(state);
    state.selectedIndex = slot;
    InventoryUi::Confirm(state, p, items, spells, monsters, level, world, rng);
    equipCursor = -1;
    for (size_t i = 0; i < state.actionCodes.size(); i++) {
        if (state.actionCodes[i] == 1) equipCursor = static_cast<int>(i);
    }
    Expect(equipCursor >= 0 && state.actionLabels[static_cast<size_t>(equipCursor)] == "Unequip",
           "once equipped, the same slot's label should read Unequip");

    state.selectedIndex = equipCursor;
    InventoryUi::Confirm(state, p, items, spells, monsters, level, world, rng);
    Expect(!PlayerInventory::IsEquippedSlot(p, slot, items), "confirming Unequip should actually unequip the slot");
}

void TestLearnAction(const CharacterData& charData, const ItemDatabase& items, const SpellDatabase& spells,
                      const MonsterDatabase& monsters) {
    std::printf("-- InventoryUi::Confirm: Learn --\n");
    int scrollId = -1;
    for (int id = 1; id <= items.ItemCount(); id++) {
        if (items.category[static_cast<size_t>(id - 1)] == 12) {
            scrollId = id;
            break;
        }
    }
    if (scrollId < 0) {
        std::printf("  (no real category-12 spell scroll found in itemsin.dat -- skipping)\n");
        return;
    }

    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    p.inventoryCount = 0;
    for (auto& id : p.inventoryItemIds) id = 0;
    PlayerInventory::AddInventoryItemRaw(p, scrollId, 0, 0);
    int slot = p.inventoryCount - 1;
    p.inventoryItemData[static_cast<size_t>(slot)] = 1;  // Spell id 1 -- see player_inventory_smoke.cpp's own note.
    int spellId = 1;
    int8_t skillIdx = spells.ById(spellId).skillRequired;
    p.skills[static_cast<size_t>(skillIdx)][0] = 5;  // Meets CanLearnSpellFromScroll's own rank-prerequisite gate.

    GeneratedLevel level = MakeSyntheticLevel(1);
    WorldRegistry world(1);
    JavaRandom rng(1);

    InventoryUiState state;
    InventoryUi::Open(state);
    state.selectedIndex = slot;
    InventoryUi::Confirm(state, p, items, spells, monsters, level, world, rng);
    int learnCursor = -1;
    for (size_t i = 0; i < state.actionCodes.size(); i++) {
        if (state.actionCodes[i] == 2) learnCursor = static_cast<int>(i);
    }
    Expect(learnCursor >= 0, "a learnable scroll should offer a Learn action");

    state.selectedIndex = learnCursor;
    InventoryUi::Confirm(state, p, items, spells, monsters, level, world, rng);
    Expect((p.knownSpellsMask & (1u << (spellId - 1))) != 0, "confirming Learn should set the matching knownSpellsMask bit");
    Expect(p.inventoryCount == 0, "confirming Learn should consume the scroll slot");
    Expect(state.screen == InventoryScreen::List, "after Learn, control should return to the List screen");
}

void TestUseAction(const CharacterData& charData, const ItemDatabase& items, const SpellDatabase& spells,
                    const MonsterDatabase& monsters) {
    std::printf("-- InventoryUi::Confirm: Use --\n");
    if (items.ItemCount() < 92) {
        std::printf("  (itemsin.dat has fewer than 92 items -- skipping)\n");
        return;
    }
    // id 92: "Grants level experience" -- Player.useItem()'s case 92,
    // coreStats[1]++, the simplest of the 87-99 switch to assert on
    // without needing a monster target (M61's own TestUseItemGiftIds
    // already covers the other 12 ids' own effects in isolation -- this
    // test's job is proving Confirm's dispatch actually reaches UseItem
    // with target=nullptr, not re-deriving each case's own behavior).
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    p.inventoryCount = 0;
    for (auto& id : p.inventoryItemIds) id = 0;
    PlayerInventory::AddInventoryItemRaw(p, 92, 0, 0);
    int slot = p.inventoryCount - 1;
    int16_t beforeLevel = p.coreStats[1];

    GeneratedLevel level = MakeSyntheticLevel(1);
    WorldRegistry world(1);
    MonsterDatabase emptyMonsters;
    JavaRandom rng(1);

    InventoryUiState state;
    InventoryUi::Open(state);
    state.selectedIndex = slot;
    InventoryUi::Confirm(state, p, items, spells, monsters, level, world, rng);
    int useCursor = -1;
    for (size_t i = 0; i < state.actionCodes.size(); i++) {
        if (state.actionCodes[i] == 3) useCursor = static_cast<int>(i);
    }
    Expect(useCursor >= 0, "a category-13 gift item should offer a Use action");

    state.selectedIndex = useCursor;
    InventoryUi::Confirm(state, p, items, spells, emptyMonsters, level, world, rng);
    Expect(p.coreStats[1] == beforeLevel + 1, "confirming Use on id 92 should increment coreStats[1]");
    Expect(p.inventoryCount == 0, "confirming Use should consume the slot");
}

void TestCancelFromItemActionGoesBackToList(const CharacterData& charData, const ItemDatabase& items,
                                             const SpellDatabase& spells, const MonsterDatabase& monsters) {
    std::printf("-- InventoryUi::Cancel from the ItemAction screen --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    GeneratedLevel level = MakeSyntheticLevel(1);
    WorldRegistry world(1);
    JavaRandom rng(1);

    InventoryUiState state;
    InventoryUi::Open(state);
    state.selectedIndex = 0;
    InventoryUi::Confirm(state, p, items, spells, monsters, level, world, rng);
    Expect(state.screen == InventoryScreen::ItemAction, "should be on the action menu");

    InventoryUi::Cancel(state);
    Expect(state.active, "Cancel from ItemAction should NOT close the whole inventory view");
    Expect(state.screen == InventoryScreen::List, "Cancel from ItemAction should return to the List screen");
    Expect(state.selectedSlot == -1, "Cancel from ItemAction should clear selectedSlot");
}

void TestRenderDoesNotCrash(const CharacterData& charData, const ItemDatabase& items, const SpellDatabase& spells,
                             const MonsterDatabase& monsters) {
    std::printf("-- InventoryUi::Render (both screens) --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    GeneratedLevel level = MakeSyntheticLevel(1);
    WorldRegistry world(1);
    JavaRandom rng(1);
    Backbuffer bb;

    InventoryUiState state;
    InventoryUi::Open(state);
    InventoryUi::Render(bb, state, p, items, spells, charData);

    state.selectedIndex = 0;
    InventoryUi::Confirm(state, p, items, spells, monsters, level, world, rng);
    InventoryUi::Render(bb, state, p, items, spells, charData);
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assets(root);
        CharacterData charData = CharacterData::Load(assets);
        ItemDatabase items = ItemDatabase::Load(assets);
        SpellDatabase spells = SpellDatabase::Load(assets);
        MonsterDatabase monsters = MonsterDatabase::Load(assets);

        TestOpenAndCancel();
        TestMoveSelectionClamps(charData, items);
        TestSelectSlotBuildsActionMenu(charData, items, spells, monsters);
        TestDropAction(charData, items, spells, monsters);
        TestEquipUnequipToggle(charData, items, spells, monsters);
        TestLearnAction(charData, items, spells, monsters);
        TestUseAction(charData, items, spells, monsters);
        TestCancelFromItemActionGoesBackToList(charData, items, spells, monsters);
        TestRenderDoesNotCrash(charData, items, spells, monsters);

        if (!g_ok) {
            std::fprintf(stderr, "m62_inventory_ui_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m62_inventory_ui_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
