// M66 smoke test: ui/npc_choices_menu.h's NpcChoicesMenu, extended for
// Helga (shopId 5) -- ESGame.java's dispatchNpcChoice()'s own `case 5`
// (Rumors/Give Crystal/Enchant/Bless/Cure/Warp/Recovery), built on M58's
// already-ported ShopInteraction::HelgaDialogue. See M64/M65's own smoke
// tests for the shared Choices/GiveWhat/Cancel machinery this milestone
// reuses -- this file covers what's NEW: Helga's own 7-item Choices list,
// her Enchant screen, the Rumors popup, and the corrected `resultCloses`
// distinction (this milestone's own big finding: Kill's and Warp's result
// screens genuinely, confirmedly work in the original and close the whole
// menu, unlike every other result here, including Helga's own Bless/
// Cure/Recovery -- M64's own Kill dispatch is fixed here too).
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/shop_dialogue.h"
#include "dungeon/dungeon_runtime.h"
#include "player/player_creation.h"
#include "player/player_inventory.h"
#include "ui/npc_choices_menu.h"
#include "world/shop_state.h"

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

GeneratedLevel MakeSyntheticHub() {
    GeneratedLevel level;
    level.number = 1;
    level.width = 35;
    level.height = 35;
    level.tiles.assign(35, std::vector<uint8_t>(35, 0));
    level.populated = true;
    return level;
}

void TestChoicesListIsHelgaSpecific(const PlayerState& p) {
    std::printf("-- NpcChoicesMenu: Helga's own 7-item Choices list --\n");
    NpcChoicesMenuState state;
    NpcChoicesMenu::Open(state, 5);
    Expect(state.shopId == 5, "Open should store shopId 5");

    NpcChoicesMenu::MoveSelection(state, 1000, p);
    Expect(state.selectedIndex == 6,
           "Helga's own Choices list should be exactly 7 items (Rumors/Give Crystal/Enchant/Bless/Cure/Warp/"
           "Recovery)");
}

void TestKillResultClosesTheWholeMenu(const CharacterData& charData, const ItemDatabase& items,
                                      const ShopDialogue& text) {
    std::printf("-- NpcChoicesMenu: M64 correction -- Kill's result closes the whole menu --\n");
    // Corrected finding: screenGroup 26 (Kill's own result) has a real,
    // WORKING, unconditional showScreen(gameCanvas) handler in the
    // original -- M64 originally (incorrectly) treated it the same as
    // every other (confirmed-dead) result, returning to Choices. Verify
    // the fix directly against a quest shop (shopId 0).
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    ShopState shop;
    GeneratedLevel hub = MakeSyntheticHub();
    JavaRandom rng(1);
    int16_t spawnIdCounter = 10000;

    NpcChoicesMenuState state;
    NpcChoicesMenu::Open(state, 0);
    state.selectedIndex = 4;  // Kill
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::Result, "Kill should move to the Result screen");
    Expect(state.resultCloses, "Kill's own Result should be flagged resultCloses (screenGroup 26 really works)");

    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(!state.active, "confirming Kill's Result should close the whole menu, matching screenGroup 26 exactly");
}

void TestBefriendStillReturnsToChoices(const CharacterData& charData, const ItemDatabase& items,
                                       const ShopDialogue& text) {
    std::printf("-- NpcChoicesMenu: Befriend's result is still a confirmed dead end --\n");
    // Contrast case: Befriend (screenGroup 24) is genuinely still dead in
    // the original -- unaffected by the Kill correction.
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    ShopState shop;
    GeneratedLevel hub = MakeSyntheticHub();
    JavaRandom rng(1);
    int16_t spawnIdCounter = 10000;

    NpcChoicesMenuState state;
    NpcChoicesMenu::Open(state, 0);
    state.selectedIndex = 2;  // Befriend
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(!state.resultCloses, "Befriend's own Result should NOT be flagged resultCloses (screenGroup 24 is dead)");
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.active && state.screen == NpcChoicesScreen::Choices,
           "confirming Befriend's Result should still return to Choices, not close the menu");
}

void TestRumors(const CharacterData& charData, const ItemDatabase& items, const ShopDialogue& text) {
    std::printf("-- NpcChoicesMenu::Confirm: Helga's own Rumors --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    ShopState shop;
    GeneratedLevel hub = MakeSyntheticHub();
    JavaRandom rng(1);
    int16_t spawnIdCounter = 10000;

    NpcChoicesMenuState state;
    NpcChoicesMenu::Open(state, 5);
    state.selectedIndex = 0;  // Rumors
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::Result, "Rumors should go straight to a Result screen");
    Expect(state.resultTitle == "Rumors",
           "Rumors' own title should be rumorsUI's own leftover placeholder \"Rumors\", not \"NPC name here\"");
    Expect(!state.resultBody.empty(), "Rumors should produce a real, non-empty line");
    Expect(!state.resultCloses,
           "Rumors' own Result should NOT close the menu -- rumorsUI has the identical confirmed softlock npcHelloUI "
           "does (see class comment), not reproduced, so this is the same port-only 'return to Choices' mapping");

    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.active && state.screen == NpcChoicesScreen::Choices, "confirming Rumors should return to Choices");
}

void TestGiveCrystalUsesHelgaDialogue(const CharacterData& charData, const ItemDatabase& items,
                                      const ShopDialogue& text) {
    std::printf("-- NpcChoicesMenu::Confirm: Helga's own Give Crystal --\n");
    // Category 13 so HelgaDialogue's action 4 takes its "accepted" branch.
    int itemId = -1;
    for (int id = 1; id <= items.ItemCount(); id++) {
        if (items.category[static_cast<size_t>(id - 1)] == 13) {
            itemId = id;
            break;
        }
    }
    if (itemId < 0) {
        std::printf("  (no real category-13 item found in itemsin.dat -- skipping)\n");
        return;
    }

    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    p.inventoryCount = 0;
    for (auto& id : p.inventoryItemIds) id = 0;
    PlayerInventory::AddInventoryItemRaw(p, itemId, 0, 0);
    int slot = p.inventoryCount - 1;

    ShopState shop;
    int16_t beforePoints = shop.helgaPoints;
    GeneratedLevel hub = MakeSyntheticHub();
    JavaRandom rng(1);
    int16_t spawnIdCounter = 10000;

    NpcChoicesMenuState state;
    NpcChoicesMenu::Open(state, 5);
    state.selectedIndex = 1;  // "Give Crystal"
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::GiveWhat, "selecting Give Crystal should move to the GiveWhat screen");

    state.selectedIndex = slot;
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::Result, "confirming an item should move to the Result screen");
    Expect(shop.helgaPoints > beforePoints,
           "giving an acceptable item to Helga should increment helgaPoints (HelgaDialogue's own action 4)");
    Expect(p.inventoryCount == 0, "the given item should be removed from inventory");
}

void TestEnchant(const CharacterData& charData, const ItemDatabase& items, const ShopDialogue& text) {
    std::printf("-- NpcChoicesMenu::Confirm: Helga's own Enchant --\n");
    // A real equipment-category item (1-10), uncharged, with enough
    // helgaPoints (>= 7) -- HelgaDialogue's own action 8 "accepted" path.
    int itemId = -1;
    for (int id = 1; id <= items.ItemCount(); id++) {
        if (items.IsEquipmentCategory(id)) {
            itemId = id;
            break;
        }
    }
    if (itemId < 0) {
        std::printf("  (no real equipment-category item found in itemsin.dat -- skipping)\n");
        return;
    }

    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    p.inventoryCount = 0;
    for (auto& id : p.inventoryItemIds) id = 0;
    PlayerInventory::AddInventoryItemRaw(p, itemId, 0, 0);
    int slot = p.inventoryCount - 1;
    Expect(!PlayerInventory::IsItemCharged(p, slot), "a freshly-added item should start uncharged");

    ShopState shop;
    shop.helgaPoints = 10;
    GeneratedLevel hub = MakeSyntheticHub();
    JavaRandom rng(1);
    int16_t spawnIdCounter = 10000;

    NpcChoicesMenuState state;
    NpcChoicesMenu::Open(state, 5);
    state.selectedIndex = 2;  // "Enchant"
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::EnchantWhat, "selecting Enchant should move to the EnchantWhat screen");

    NpcChoicesMenu::MoveSelection(state, 1000, p);
    Expect(state.selectedIndex == p.inventoryCount - 1, "EnchantWhat's own list should be exactly inventoryCount long");

    state.selectedIndex = slot;
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::Result, "confirming an item should move to the Result screen");
    Expect(shop.helgaPoints == 3, "a successful Enchant should spend exactly 7 helgaPoints");
    Expect(PlayerInventory::IsItemCharged(p, slot), "a successful Enchant should actually charge the item");
    Expect(!state.resultCloses, "Enchant's own Result should NOT close the menu (screenGroup 351 is a dead end)");

    // Cancel from EnchantWhat itself (not yet confirmed): real
    // nextScreen=gameCanvas, closes the whole menu -- same as
    // GiveWhat/TrainWhat, NOT TakeWhat's own exception.
    NpcChoicesMenuState state2;
    NpcChoicesMenu::Open(state2, 5);
    state2.selectedIndex = 2;
    NpcChoicesMenu::Confirm(state2, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state2.screen == NpcChoicesScreen::EnchantWhat, "should be on EnchantWhat");
    NpcChoicesMenu::Cancel(state2);
    Expect(!state2.active, "Cancel from EnchantWhat should close the whole menu, matching nextScreen=gameCanvas");
}

void TestBlessCureRecovery(const CharacterData& charData, const ItemDatabase& items, const ShopDialogue& text) {
    std::printf("-- NpcChoicesMenu::Confirm: Bless/Cure/Recovery --\n");
    for (int choice = 3; choice <= 4; choice++) {  // Bless(3), Cure(4).
        PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
        ShopState shop;
        shop.helgaPoints = 10;
        GeneratedLevel hub = MakeSyntheticHub();
        JavaRandom rng(1);
        int16_t spawnIdCounter = 10000;

        NpcChoicesMenuState state;
        NpcChoicesMenu::Open(state, 5);
        state.selectedIndex = choice;
        NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
        Expect(state.screen == NpcChoicesScreen::Result, "Bless/Cure should go straight to Result");
        Expect(!state.resultBody.empty(), "Bless/Cure should each produce a real, non-empty line");
        Expect(!state.resultCloses, "Bless/Cure's own Results should NOT close the menu (352/353 are dead ends)");
    }

    // Recovery (choice 6, action 12): the ONE Helga action with NO points
    // gate at all -- confirmed by reading Shop.java's own action==12
    // branch directly (no `if (helgaPoints < n)` check, unlike every
    // other Helga action). Verify it succeeds even at 0 points.
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    p.coreStats[2] = 1;  // Damaged HP...
    p.coreStats[3] = 50;  // ...below max.
    p.coreStats[4] = 1;
    p.coreStats[5] = 50;
    ShopState shop;
    shop.helgaPoints = 0;
    GeneratedLevel hub = MakeSyntheticHub();
    JavaRandom rng(1);
    int16_t spawnIdCounter = 10000;

    NpcChoicesMenuState state;
    NpcChoicesMenu::Open(state, 5);
    state.selectedIndex = 6;  // Recovery
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::Result, "Recovery should go straight to Result");
    Expect(p.coreStats[2] == p.coreStats[3], "Recovery should fully heal HP even at 0 helgaPoints (no gate)");
    Expect(p.coreStats[4] == p.coreStats[5], "Recovery should fully restore Magicka even at 0 helgaPoints (no gate)");
    Expect(!state.resultCloses, "Recovery's own Result should NOT close the menu (screenGroup 355 is a dead end)");
}

void TestWarpClosesTheWholeMenuAndClearsCampMark(const CharacterData& charData, const ItemDatabase& items,
                                                 const ShopDialogue& text) {
    std::printf("-- NpcChoicesMenu::Confirm: Warp closes the whole menu, and clears justMarkedCamp --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    PlayerInventory::MarkCampAndReturnToTown(p);
    Expect(PlayerInventory::HasCampMark(p), "the player should have a real camp mark to warp to");
    p.justMarkedCamp = true;  // MarkCampAndReturnToTown's own real side effect.

    ShopState shop;
    shop.helgaPoints = 5;
    GeneratedLevel hub = MakeSyntheticHub();
    JavaRandom rng(1);
    int16_t spawnIdCounter = 10000;

    NpcChoicesMenuState state;
    NpcChoicesMenu::Open(state, 5);
    state.selectedIndex = 5;  // Warp
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::Result, "Warp should move to the Result screen");
    Expect(!p.justMarkedCamp,
           "Warp's own dispatch should clear justMarkedCamp, matching screenGroup 41's real handler exactly");
    Expect(shop.helgaPoints == 4, "a successful Warp should spend exactly 1 helgaPoint");
    Expect(state.resultCloses, "Warp's own Result should be flagged resultCloses (screenGroup 41 really works)");

    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(!state.active, "confirming Warp's Result should close the whole menu, matching screenGroup 41 exactly");
}

void TestTitleAndRenderDoNotCrash(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items) {
    std::printf("-- NpcChoicesMenu::Render: Helga's own screens --\n");
    Backbuffer bb;
    ShopState shop;
    shop.helgaPoints = 4;

    for (NpcChoicesScreen screen : {NpcChoicesScreen::Choices, NpcChoicesScreen::GiveWhat,
                                     NpcChoicesScreen::EnchantWhat, NpcChoicesScreen::Result}) {
        NpcChoicesMenuState state;
        NpcChoicesMenu::Open(state, 5);
        state.screen = screen;
        state.resultTitle = "Rumors";
        state.resultBody = "Some result text.";
        NpcChoicesMenu::Render(bb, state, p, charData, items, shop);
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assets(root);
        CharacterData charData = CharacterData::Load(assets);
        ItemDatabase items = ItemDatabase::Load(assets);
        ShopDialogue text = ShopDialogue::Load(assets);

        PlayerState freshPlayer = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);

        TestChoicesListIsHelgaSpecific(freshPlayer);
        TestKillResultClosesTheWholeMenu(charData, items, text);
        TestBefriendStillReturnsToChoices(charData, items, text);
        TestRumors(charData, items, text);
        TestGiveCrystalUsesHelgaDialogue(charData, items, text);
        TestEnchant(charData, items, text);
        TestBlessCureRecovery(charData, items, text);
        TestWarpClosesTheWholeMenuAndClearsCampMark(charData, items, text);
        TestTitleAndRenderDoNotCrash(freshPlayer, charData, items);

        if (!g_ok) {
            std::fprintf(stderr, "m66_helga_choices_menu_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m66_helga_choices_menu_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
