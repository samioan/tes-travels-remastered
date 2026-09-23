// M65 smoke test: ui/npc_choices_menu.h's NpcChoicesMenu, extended for
// Beneca (shopId 4) -- ESGame.java's dispatchNpcChoice()'s own `case 4`
// (Give Item/Take Crystal), built on M57's already-ported
// ShopInteraction::BenecaDialogue. See M64's own smoke test
// (m64_npc_choices_menu_smoke.cpp) for the shared Choices/GiveWhat/Result
// machinery this milestone reuses rather than re-testing from scratch --
// this file only covers what's NEW: Beneca's own 2-item Choices list, her
// own "Take Crystal" (TakeWhat) screen, and the confirmed real Cancel
// asymmetry between TakeWhat and every other sub-screen.
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

void TestChoicesListIsBenecaSpecific(const PlayerState& p) {
    std::printf("-- NpcChoicesMenu: Beneca's own 2-item Choices list --\n");
    NpcChoicesMenuState state;
    NpcChoicesMenu::Open(state, 4);
    Expect(state.shopId == 4, "Open should store shopId 4");

    NpcChoicesMenu::MoveSelection(state, 1000, p);
    Expect(state.selectedIndex == 1, "Beneca's own Choices list should be exactly 2 items (Give Item/Take Crystal)");

    NpcChoicesMenu::MoveSelection(state, -1000, p);
    Expect(state.selectedIndex == 0, "MoveSelection should still clamp at 0");
}

void TestGiveItemUsesBenecaDialogue(const CharacterData& charData, const ItemDatabase& items,
                                    const ShopDialogue& text) {
    std::printf("-- NpcChoicesMenu::Confirm: Beneca's own Give Item --\n");
    // A category not in {13, 15, 17} so BenecaDialogue's action 4 takes
    // its "accepted" branch (lines[2]) rather than the "declined" one
    // (lines[1]) -- category 1 (a plain weapon) is safe for this.
    int itemId = -1;
    for (int id = 1; id <= items.ItemCount(); id++) {
        if (items.category[static_cast<size_t>(id - 1)] == 1) {
            itemId = id;
            break;
        }
    }
    if (itemId < 0) {
        std::printf("  (no real category-1 item found in itemsin.dat -- skipping)\n");
        return;
    }

    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    p.inventoryCount = 0;
    for (auto& id : p.inventoryItemIds) id = 0;
    PlayerInventory::AddInventoryItemRaw(p, itemId, 0, 0);
    int slot = p.inventoryCount - 1;

    ShopState shop;
    int16_t beforePoints = shop.benecaPoints;
    GeneratedLevel hub = MakeSyntheticHub();
    JavaRandom rng(1);
    int16_t spawnIdCounter = 10000;

    NpcChoicesMenuState state;
    NpcChoicesMenu::Open(state, 4);
    state.selectedIndex = 0;  // "Give Item"
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::GiveWhat, "selecting Give Item should move to the GiveWhat screen");

    state.selectedIndex = slot;
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::Result, "confirming an item should move to the Result screen");
    Expect(shop.benecaPoints == beforePoints + 1,
           "giving an acceptable item to Beneca should increment benecaPoints (BenecaDialogue's own action 4)");
    Expect(p.inventoryCount == 0, "the given item should be removed from inventory");
}

void TestGiveItemEmptyInventory(const CharacterData& charData, const ItemDatabase& items, const ShopDialogue& text) {
    std::printf("-- NpcChoicesMenu::Confirm: Beneca's own Give Item (empty inventory) --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    p.inventoryCount = 0;
    ShopState shop;
    GeneratedLevel hub = MakeSyntheticHub();
    JavaRandom rng(1);
    int16_t spawnIdCounter = 10000;

    NpcChoicesMenuState state;
    NpcChoicesMenu::Open(state, 4);
    state.selectedIndex = 0;  // "Give Item"
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::Result, "Give Item with an empty inventory should go straight to Result");
    Expect(state.resultBody == "You have nothing to give me!",
           "the empty-inventory message should match the real literal string exactly, same as shops 0-3");
}

void TestTakeCrystalFlow(const CharacterData& charData, const ItemDatabase& items, const ShopDialogue& text) {
    std::printf("-- NpcChoicesMenu::Confirm: Beneca's own Take Crystal --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    p.inventoryCount = 0;
    for (auto& id : p.inventoryItemIds) id = 0;
    ShopState shop;
    shop.benecaPoints = 5;  // >= 3, so BenecaDialogue's action 7 succeeds.
    GeneratedLevel hub = MakeSyntheticHub();
    JavaRandom rng(1);
    int16_t spawnIdCounter = 10000;

    NpcChoicesMenuState state;
    NpcChoicesMenu::Open(state, 4);
    state.selectedIndex = 1;  // "Take Crystal"
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::TakeWhat, "selecting Take Crystal should move to the TakeWhat screen");

    NpcChoicesMenu::MoveSelection(state, 1000, p);
    Expect(state.selectedIndex == 12, "TakeWhat's own list should be exactly the 13 real gift items (ids 87-99)");

    state.selectedIndex = 0;  // Item id 87.
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::Result, "confirming a crystal choice should move to the Result screen");
    Expect(shop.benecaPoints == 2, "a successful Take Crystal should spend exactly 3 benecaPoints");
    Expect(p.inventoryCount == 1, "a successful Take Crystal should actually grant the item");
    Expect(std::abs(static_cast<int>(p.inventoryItemIds[0])) == 87,
           "the granted item's id should be selectedIndex+87 (a real item id, NOT a slot -- see BenecaDialogue's "
           "own naming-trap doc comment)");
}

void TestTakeCrystalNotEnoughPoints(const CharacterData& charData, const ItemDatabase& items,
                                    const ShopDialogue& text) {
    std::printf("-- NpcChoicesMenu::Confirm: Beneca's own Take Crystal (not enough points) --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    ShopState shop;
    shop.benecaPoints = 2;  // < 3.
    GeneratedLevel hub = MakeSyntheticHub();
    JavaRandom rng(1);
    int16_t spawnIdCounter = 10000;

    NpcChoicesMenuState state;
    NpcChoicesMenu::Open(state, 4);
    state.selectedIndex = 1;
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    state.selectedIndex = 0;
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(!state.resultBody.empty(), "an insufficient-points result should still be a real, non-empty line");
    Expect(shop.benecaPoints == 2, "insufficient points should leave benecaPoints unchanged");
}

void TestCancelAsymmetry(const CharacterData& charData, const ItemDatabase& items, const ShopDialogue& text) {
    std::printf("-- NpcChoicesMenu::Cancel: TakeWhat returns to Choices, unlike GiveWhat --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    ShopState shop;
    GeneratedLevel hub = MakeSyntheticHub();
    JavaRandom rng(1);
    int16_t spawnIdCounter = 10000;

    // TakeWhat: takeWhatMenu's own nextScreen=null forces the real
    // screenGroup-27 cmdBack handler, which shows npcChoicesUI[shopId] --
    // a genuine, confirmed EXCEPTION to GiveWhat/TrainWhat's own
    // close-the-whole-menu behavior (see class comment's "fifth finding").
    NpcChoicesMenuState state;
    NpcChoicesMenu::Open(state, 4);
    state.selectedIndex = 1;  // "Take Crystal"
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::TakeWhat, "should be on TakeWhat");
    NpcChoicesMenu::Cancel(state);
    Expect(state.active && state.screen == NpcChoicesScreen::Choices,
           "Cancel from TakeWhat should return to Choices, NOT close the whole menu");

    // GiveWhat (Beneca's own, shares the exact same screen/dispatch as
    // shops 0-3's own): real nextScreen=gameCanvas, closes the whole menu.
    NpcChoicesMenu::Open(state, 4);
    state.selectedIndex = 0;  // "Give Item"
    p.inventoryCount = 1;  // Force the GiveWhat branch, not the empty-inventory Result.
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::GiveWhat, "should be on GiveWhat");
    NpcChoicesMenu::Cancel(state);
    Expect(!state.active, "Cancel from GiveWhat should close the whole menu, matching nextScreen=gameCanvas");

    // Choices itself: closes the whole menu, same as every shop group.
    NpcChoicesMenu::Open(state, 4);
    NpcChoicesMenu::Cancel(state);
    Expect(!state.active, "Cancel from Beneca's own Choices screen should close the whole menu");
}

void TestGreetingReturnsNulloptAfterFirstVisit(const CharacterData& charData, const ItemDatabase& items,
                                               const ShopDialogue& text) {
    std::printf("-- ShopInteraction::BenecaDialogue: greeting is null after the first visit --\n");
    // This is the real premise main.cpp's own M65 fallback (opening
    // NpcChoicesMenu directly for shop 4 when the greeting comes back
    // empty) depends on -- verified here against the real primitive;
    // the actual main.cpp wiring is exercised by this milestone's own
    // full clean rebuild + manual exe launch instead (same as every other
    // main.cpp-only change in this port).
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    ShopState shop;
    int16_t spawnIdCounter = 10000;

    std::optional<std::string> first = ShopInteraction::BenecaDialogue(p, shop, text, items, spawnIdCounter, 1, 0);
    Expect(first.has_value(), "the first visit should have a real greeting line");

    std::optional<std::string> second = ShopInteraction::BenecaDialogue(p, shop, text, items, spawnIdCounter, 1, 0);
    Expect(!second.has_value(), "every visit after the first should return nullopt (the real 'nothing more to say' case)");
}

void TestTitleAndRenderDoNotCrash(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items) {
    std::printf("-- NpcChoicesMenu::Render: Beneca's own screens --\n");
    Backbuffer bb;
    ShopState shop;
    shop.benecaPoints = 4;

    for (NpcChoicesScreen screen :
         {NpcChoicesScreen::Choices, NpcChoicesScreen::GiveWhat, NpcChoicesScreen::TakeWhat, NpcChoicesScreen::Result}) {
        NpcChoicesMenuState state;
        NpcChoicesMenu::Open(state, 4);
        state.screen = screen;
        state.resultTitle = "NPC name here";
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

        TestChoicesListIsBenecaSpecific(freshPlayer);
        TestGiveItemUsesBenecaDialogue(charData, items, text);
        TestGiveItemEmptyInventory(charData, items, text);
        TestTakeCrystalFlow(charData, items, text);
        TestTakeCrystalNotEnoughPoints(charData, items, text);
        TestCancelAsymmetry(charData, items, text);
        TestGreetingReturnsNulloptAfterFirstVisit(charData, items, text);
        TestTitleAndRenderDoNotCrash(freshPlayer, charData, items);

        if (!g_ok) {
            std::fprintf(stderr, "m65_beneca_choices_menu_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m65_beneca_choices_menu_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
