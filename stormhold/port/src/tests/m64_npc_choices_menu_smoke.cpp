// M64 smoke test: ui/npc_choices_menu.h's NpcChoicesMenu -- the
// interactive Train/Give/Befriend/Threaten/Kill follow-up menu for
// shops 0-3 (ESGame.java's npcChoicesUI[shopId]/dispatchNpcChoice(),
// case 0/1/2/3), built on M56's already-ported
// ShopInteraction::QuestShopDialogue/IsValidShopAction/ShopActionCode.
#include <cstdint>
#include <cstdio>
#include <string>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/shop_dialogue.h"
#include "dungeon/dungeon_runtime.h"
#include "player/player_creation.h"
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

void TestOpenAndCancelFromChoices() {
    std::printf("-- NpcChoicesMenu::Open/Cancel from Choices --\n");
    NpcChoicesMenuState state;
    Expect(!state.active, "a fresh NpcChoicesMenuState should start inactive");

    NpcChoicesMenu::Open(state, 2);
    Expect(state.active, "Open should set active");
    Expect(state.shopId == 2, "Open should store shopId");
    Expect(state.screen == NpcChoicesScreen::Choices, "Open should land on the Choices screen");
    Expect(state.selectedIndex == 0, "Open should reset the cursor to 0");

    // Matches the real npcChoicesUI[shopId].nextScreen = gameCanvas.
    NpcChoicesMenu::Cancel(state);
    Expect(!state.active, "Cancel from Choices should close the whole menu, back to gameplay");
}

void TestMoveSelectionOnChoices(const PlayerState& p) {
    std::printf("-- NpcChoicesMenu::MoveSelection on Choices --\n");
    NpcChoicesMenuState state;
    NpcChoicesMenu::Open(state, 0);

    NpcChoicesMenu::MoveSelection(state, -1, p);
    Expect(state.selectedIndex == 0, "MoveSelection should clamp at 0, not wrap");

    NpcChoicesMenu::MoveSelection(state, 1000, p);
    Expect(state.selectedIndex == 4, "MoveSelection should clamp at 4 (5 choices), not overflow");

    NpcChoicesMenuState inactive;
    NpcChoicesMenu::MoveSelection(inactive, 5, p);
    Expect(inactive.selectedIndex == 0, "MoveSelection on an inactive state should do nothing");
}

void TestTrainFlow(const CharacterData& charData, const ItemDatabase& items, const ShopDialogue& text) {
    std::printf("-- NpcChoicesMenu::Confirm: Train --\n");
    for (int shopId = 0; shopId < 4; shopId++) {
        int validCount = 0;
        for (int skillIndex = 0; skillIndex < 14; skillIndex++) {
            if (ShopInteraction::IsValidShopAction(shopId, skillIndex)) validCount++;
        }
        Expect(validCount == 3, "each of shops 0-3 should allow exactly 3 skills (ShopActionCode's own kCodes table)");

        PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
        ShopState shop;
        shop.rewardsGiven[static_cast<size_t>(shopId)] = 1;  // Avoid the "no rewards" early-exit line.
        GeneratedLevel hub = MakeSyntheticHub();
        JavaRandom rng(1);
        int16_t spawnIdCounter = 10000;

        NpcChoicesMenuState state;
        NpcChoicesMenu::Open(state, shopId);
        state.selectedIndex = 0;  // "Train"
        NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
        Expect(state.screen == NpcChoicesScreen::TrainWhat, "selecting Train should move to the TrainWhat screen");

        NpcChoicesMenu::MoveSelection(state, 1000, p);
        Expect(state.selectedIndex == validCount - 1, "TrainWhat's own list should be exactly validCount long");

        state.selectedIndex = 0;
        NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
        Expect(state.screen == NpcChoicesScreen::Result, "confirming a skill should move to the Result screen");
        Expect(!state.resultBody.empty(), "Train should produce a real, non-empty rumor line");
        Expect(state.resultTitle == "NPC name here",
               "the result title should be the real game's own dead-write placeholder, not the shop's real name");

        // Deliberate port-only mapping (see class comment): Result's own
        // "Ok" returns to Choices, not closing the whole menu.
        NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
        Expect(state.active && state.screen == NpcChoicesScreen::Choices,
               "confirming Result should return to Choices without closing the menu");
    }
}

void TestGiveFlowWithItems(const CharacterData& charData, const ItemDatabase& items, const ShopDialogue& text) {
    std::printf("-- NpcChoicesMenu::Confirm: Give (non-empty inventory) --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    Expect(p.inventoryCount > 0, "a fresh character should start with at least one item to give");
    ShopState shop;
    GeneratedLevel hub = MakeSyntheticHub();
    JavaRandom rng(1);
    int16_t spawnIdCounter = 10000;

    NpcChoicesMenuState state;
    NpcChoicesMenu::Open(state, 1);
    state.selectedIndex = 1;  // "Give"
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::GiveWhat, "selecting Give with items should move to the GiveWhat screen");

    NpcChoicesMenu::MoveSelection(state, 1000, p);
    Expect(state.selectedIndex == p.inventoryCount - 1, "GiveWhat's own list should be exactly inventoryCount long");

    state.selectedIndex = 0;
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::Result, "confirming an item should move to the Result screen");
    Expect(!state.resultBody.empty(), "Give should produce a real, non-empty response line");
}

void TestGiveFlowEmptyInventory(const CharacterData& charData, const ItemDatabase& items, const ShopDialogue& text) {
    std::printf("-- NpcChoicesMenu::Confirm: Give (empty inventory) --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    p.inventoryCount = 0;
    ShopState shop;
    GeneratedLevel hub = MakeSyntheticHub();
    JavaRandom rng(1);
    int16_t spawnIdCounter = 10000;

    NpcChoicesMenuState state;
    NpcChoicesMenu::Open(state, 3);
    state.selectedIndex = 1;  // "Give"
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::Result, "Give with an empty inventory should go straight to Result");
    Expect(state.resultBody == "You have nothing to give me!",
           "the empty-inventory message should match the real literal string exactly");
    Expect(state.resultTitle == "Oracle",
           "the empty-inventory title should be npcResponseUI's own leftover placeholder, not the shop's real name");
}

void TestBefriendThreatenKill(const CharacterData& charData, const ItemDatabase& items, const ShopDialogue& text) {
    std::printf("-- NpcChoicesMenu::Confirm: Befriend/Threaten/Kill --\n");
    for (int choice = 2; choice <= 4; choice++) {
        PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
        ShopState shop;
        GeneratedLevel hub = MakeSyntheticHub();
        JavaRandom rng(1);
        int16_t spawnIdCounter = 10000;

        NpcChoicesMenuState state;
        NpcChoicesMenu::Open(state, 0);
        state.selectedIndex = choice;
        NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
        Expect(state.screen == NpcChoicesScreen::Result, "Befriend/Threaten/Kill should go straight to Result");
        Expect(!state.resultBody.empty(), "Befriend/Threaten/Kill should each produce a real, non-empty line");
    }

    // Kill (choice 4, action 6) also clears the hub's own shop-tile bit 32.
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    ShopState shop;
    GeneratedLevel hub = MakeSyntheticHub();
    int x = Shop::kShopX[0];
    int y = Shop::kShopY[0];
    hub.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] = 32;
    JavaRandom rng(1);
    int16_t spawnIdCounter = 10000;
    NpcChoicesMenuState state;
    NpcChoicesMenu::Open(state, 0);
    state.selectedIndex = 4;  // Kill
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect((hub.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] & 32) == 0,
           "Kill should clear the hub's own shop-tile bit 32 via QuestShopDialogue's action 6");
}

void TestCancelSemantics(const CharacterData& charData, const ItemDatabase& items, const ShopDialogue& text) {
    std::printf("-- NpcChoicesMenu::Cancel from every screen --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    ShopState shop;
    GeneratedLevel hub = MakeSyntheticHub();
    JavaRandom rng(1);
    int16_t spawnIdCounter = 10000;

    // TrainWhat: real nextScreen = gameCanvas, so Cancel closes the whole
    // menu (not just backing up to Choices).
    NpcChoicesMenuState state;
    NpcChoicesMenu::Open(state, 0);
    state.selectedIndex = 0;
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::TrainWhat, "should be on TrainWhat");
    NpcChoicesMenu::Cancel(state);
    Expect(!state.active, "Cancel from TrainWhat should close the whole menu, matching nextScreen=gameCanvas");

    // GiveWhat: same real target.
    NpcChoicesMenu::Open(state, 0);
    state.selectedIndex = 1;
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::GiveWhat, "should be on GiveWhat");
    NpcChoicesMenu::Cancel(state);
    Expect(!state.active, "Cancel from GiveWhat should close the whole menu, matching nextScreen=gameCanvas");

    // Result: deliberate port-only mapping, back to Choices.
    NpcChoicesMenu::Open(state, 0);
    state.selectedIndex = 2;  // Befriend
    NpcChoicesMenu::Confirm(state, p, shop, text, charData, items, hub, rng, spawnIdCounter);
    Expect(state.screen == NpcChoicesScreen::Result, "should be on Result");
    NpcChoicesMenu::Cancel(state);
    Expect(state.active && state.screen == NpcChoicesScreen::Choices,
           "Cancel from Result should return to Choices, not close the menu");
}

void TestRenderDoesNotCrash(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items) {
    std::printf("-- NpcChoicesMenu::Render (every screen) --\n");
    Backbuffer bb;
    ShopState shop;
    shop.rewardsGiven[0] = 2;

    for (NpcChoicesScreen screen :
         {NpcChoicesScreen::Choices, NpcChoicesScreen::TrainWhat, NpcChoicesScreen::GiveWhat, NpcChoicesScreen::Result}) {
        NpcChoicesMenuState state;
        NpcChoicesMenu::Open(state, 0);
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

        TestOpenAndCancelFromChoices();
        TestMoveSelectionOnChoices(freshPlayer);
        TestTrainFlow(charData, items, text);
        TestGiveFlowWithItems(charData, items, text);
        TestGiveFlowEmptyInventory(charData, items, text);
        TestBefriendThreatenKill(charData, items, text);
        TestCancelSemantics(charData, items, text);
        TestRenderDoesNotCrash(freshPlayer, charData, items);

        if (!g_ok) {
            std::fprintf(stderr, "m64_npc_choices_menu_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m64_npc_choices_menu_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
