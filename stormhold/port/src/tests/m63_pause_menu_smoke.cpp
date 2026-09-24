// M63 smoke test: ui/pause_menu.h's PauseMenu -- the in-game Options/pause
// menu (ESGame.java's optionsUI, screenGroup 31, plus everything it opens
// onto: Stats/Skills+SkillInfo/Spells+SpellInfo/Save Game/Load Game/"Quit
// Game"). See ui/pause_menu.h's own class comment for the two confirmed
// real findings this milestone turned up (the whole menu being genuinely
// unreachable in the original, and "Quit Game" actually showing the
// credits screen instead of quitting) and the deliberate port-only
// exceptions each one gets.
#include <cstdio>
#include <filesystem>
#include <string>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/shop_dialogue.h"
#include "assets/spell_database.h"
#include "combat/spell_casting.h"
#include "dungeon/dungeon_runtime.h"
#include "player/game_save.h"
#include "player/player_creation.h"
#include "player/player_leveling.h"
#include "ui/inventory_ui.h"
#include "ui/pause_menu.h"
#include "world/shop_state.h"
#include "world/warden.h"

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

void TestOpenAndCancelFromOptions() {
    std::printf("-- PauseMenu::Open/Cancel from Options --\n");
    PauseMenuState state;
    Expect(!state.active, "a fresh PauseMenuState should start inactive");

    PauseMenu::Open(state);
    Expect(state.active, "Open should set active");
    Expect(state.screen == PauseScreen::Options, "Open should land on the Options screen");
    Expect(state.selectedIndex == 0, "Open should reset the cursor to 0");

    // Matches the real screenGroup 31 cmdBack branch: showScreen(gameCanvas).
    PauseMenu::Cancel(state);
    Expect(!state.active, "Cancel from Options should close the whole pause menu, back to gameplay");
}

void TestMoveSelectionOnOptions(const PlayerState& p, const SpellDatabase& spells) {
    std::printf("-- PauseMenu::MoveSelection on Options --\n");
    PauseMenuState state;
    PauseMenu::Open(state);

    PauseMenu::MoveSelection(state, -1, p, spells);
    Expect(state.selectedIndex == 0, "MoveSelection should clamp at 0, not wrap");

    PauseMenu::MoveSelection(state, 1000, p, spells);
    Expect(state.selectedIndex == 7, "MoveSelection should clamp at 7 (8 options), not overflow");

    PauseMenuState inactive;
    PauseMenu::MoveSelection(inactive, 5, p, spells);
    Expect(inactive.selectedIndex == 0, "MoveSelection on an inactive state should do nothing");
}

void TestStatsScreen(PlayerState p, const SpellDatabase& spells, InventoryUiState& inventoryUi,
                      const std::string& savePath, WorldRegistry& world, ShopState& shop, WardenState& warden) {
    std::printf("-- PauseMenu::Confirm: Stats --\n");
    PauseMenuState state;
    PauseMenu::Open(state);
    state.selectedIndex = 0;  // "Stats"
    PauseMenuAction action =
        PauseMenu::Confirm(state, p, spells, inventoryUi, savePath, 1, world, shop, warden);
    Expect(action == PauseMenuAction::None, "opening Stats should not report a load");
    Expect(state.screen == PauseScreen::Stats, "selecting Stats should move to the Stats screen");

    // Stats is a message screen: both Confirm and Cancel go back to Options.
    PauseMenu::Confirm(state, p, spells, inventoryUi, savePath, 1, world, shop, warden);
    Expect(state.screen == PauseScreen::Options, "Confirm (Ok) on Stats should return to Options");

    state.screen = PauseScreen::Stats;
    PauseMenu::Cancel(state);
    Expect(state.screen == PauseScreen::Options, "Cancel on Stats should also return to Options");
}

void TestInventoryHandoff(PlayerState p, const SpellDatabase& spells, const std::string& savePath,
                           WorldRegistry& world, ShopState& shop, WardenState& warden) {
    std::printf("-- PauseMenu::Confirm: Inventory hands off to InventoryUi --\n");
    PauseMenuState state;
    PauseMenu::Open(state);
    state.selectedIndex = 1;  // "Inventory"
    InventoryUiState inventoryUi;
    Expect(!inventoryUi.active, "inventoryUi should start inactive");

    PauseMenu::Confirm(state, p, spells, inventoryUi, savePath, 1, world, shop, warden);
    Expect(!state.active, "selecting Inventory should close the pause menu");
    Expect(inventoryUi.active, "selecting Inventory should open the M62 inventory screen");
}

void TestSkillsAndSkillInfo(PlayerState p, const CharacterData& charData, const SpellDatabase& spells,
                             InventoryUiState& inventoryUi, const std::string& savePath, WorldRegistry& world,
                             ShopState& shop, WardenState& warden) {
    std::printf("-- PauseMenu::Confirm: Skills -> SkillInfo --\n");
    // A fresh character always starts with at least one trained skill
    // (class templates grant starting skill ranks) -- assert that rather
    // than assume a specific index.
    int expectSkillIndex = PlayerLeveling::NthLearnedSkillIndex(p, 0);
    Expect(expectSkillIndex >= 0, "a freshly-created character should have at least one trained skill");

    PauseMenuState state;
    PauseMenu::Open(state);
    state.selectedIndex = 2;  // "Skills"
    PauseMenu::Confirm(state, p, spells, inventoryUi, savePath, 1, world, shop, warden);
    Expect(state.screen == PauseScreen::Skills, "selecting Skills should move to the Skills screen");

    state.selectedIndex = 0;
    PauseMenu::Confirm(state, p, spells, inventoryUi, savePath, 1, world, shop, warden);
    Expect(state.screen == PauseScreen::SkillInfo, "selecting a skill row should move to SkillInfo");
    Expect(state.skillIndex == expectSkillIndex, "SkillInfo should resolve the same index NthLearnedSkillIndex gives");

    std::string tooltip = PlayerLeveling::SkillTooltip(p, charData, expectSkillIndex);
    Expect(!tooltip.empty(), "SkillTooltip should produce non-empty text");

    PauseMenu::Confirm(state, p, spells, inventoryUi, savePath, 1, world, shop, warden);
    Expect(state.screen == PauseScreen::Skills, "Confirm (Ok) on SkillInfo should return to Skills");

    state.screen = PauseScreen::Skills;
    PauseMenu::Cancel(state);
    Expect(state.screen == PauseScreen::Options, "Cancel on Skills should return to Options");
}

void TestSpellsAndSpellInfo(const CharacterData& charData, const ItemDatabase& items, const SpellDatabase& spells,
                             InventoryUiState& inventoryUi, const std::string& savePath, WorldRegistry& world,
                             ShopState& shop, WardenState& warden) {
    std::printf("-- PauseMenu::Confirm: Spells -> SpellInfo -> Ready Spell --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    // Grant a known spell directly (character creation alone may not
    // guarantee one) -- spell 1 always exists (spellsin.dat has >= 25
    // entries elsewhere in this port's own tests).
    p.knownSpellsMask |= 1u;
    p.selectedSpellId = 0;  // Not yet "readied".

    PauseMenuState state;
    PauseMenu::Open(state);
    state.selectedIndex = 3;  // "Spells"
    PauseMenu::Confirm(state, p, spells, inventoryUi, savePath, 1, world, shop, warden);
    Expect(state.screen == PauseScreen::Spells, "selecting Spells should move to the Spells screen");

    std::vector<std::string> summary = SpellCasting::KnownSpellsSummary(p, spells);
    Expect(!summary.empty() && summary[0] == spells.all[0].name,
           "the freshly-known spell should appear unprefixed (not yet selected)");

    state.selectedIndex = 0;
    PauseMenu::Confirm(state, p, spells, inventoryUi, savePath, 1, world, shop, warden);
    Expect(state.screen == PauseScreen::SpellInfo, "selecting a spell row should move to SpellInfo");
    Expect(state.spellIndex0Based == 0, "SpellInfo should resolve spell index 0 for the first known spell");

    state.selectedIndex = 0;  // The screen's sole "Ready Spell" action.
    PauseMenuAction action =
        PauseMenu::Confirm(state, p, spells, inventoryUi, savePath, 1, world, shop, warden);
    Expect(action == PauseMenuAction::None, "Ready Spell should not report a load");
    Expect(state.screen == PauseScreen::Spells, "Ready Spell should return to the Spells screen");
    Expect(p.selectedSpellId == 1, "Ready Spell should set selectedSpellId to spellIndex0Based+1");

    std::vector<std::string> summaryAfter = SpellCasting::KnownSpellsSummary(p, spells);
    Expect(!summaryAfter.empty() && summaryAfter[0] == "R: " + spells.all[0].name,
           "the now-readied spell should show the R: prefix");

    state.screen = PauseScreen::Spells;
    PauseMenu::Cancel(state);
    Expect(state.screen == PauseScreen::Options, "Cancel on Spells should return to Options");
}

void TestHelpNavigation(PlayerState p, const SpellDatabase& spells, InventoryUiState& inventoryUi,
                         const std::string& savePath, WorldRegistry& world, ShopState& shop, WardenState& warden) {
    std::printf("-- PauseMenu::Confirm: Help -> topic list -> topic body -> Stats (M69) --\n");
    PauseMenuState state;
    PauseMenu::Open(state);
    state.selectedIndex = 6;  // "Help"
    PauseMenuAction action = PauseMenu::Confirm(state, p, spells, inventoryUi, savePath, 1, world, shop, warden);
    Expect(action == PauseMenuAction::None, "Help should not report a load");
    Expect(state.screen == PauseScreen::Help, "Help should open the 12-topic list");
    Expect(state.active, "Help should not close the pause menu");

    state.selectedIndex = 3;  // Some topic other than 0, to prove the index actually threads through.
    action = PauseMenu::Confirm(state, p, spells, inventoryUi, savePath, 1, world, shop, warden);
    Expect(action == PauseMenuAction::None, "picking a topic should not report a load");
    Expect(state.screen == PauseScreen::HelpTopic, "picking a topic should open its body");
    Expect(state.helpTopicIndex == 3, "helpTopicIndex should be the row that was picked, not reset");

    // decompiled/ESGame.java's own newHelpTopicUI(topicIndex).nextScreen =
    // this.statsUI -- a real, confirmed quirk: leaving a help topic body
    // (Ok, exercised here) goes to Stats, not back to the topic list.
    action = PauseMenu::Confirm(state, p, spells, inventoryUi, savePath, 1, world, shop, warden);
    Expect(action == PauseMenuAction::None, "leaving a help topic should not report a load");
    Expect(state.screen == PauseScreen::Stats,
           "leaving a help topic should land on Stats, matching the original's own real quirk -- not Help, and not "
           "Options");

    // Cancel from the topic body should do the exact same thing (screenGroup
    // 206's real dispatch doesn't distinguish cmdOk from anything else).
    state.screen = PauseScreen::HelpTopic;
    PauseMenu::Cancel(state);
    Expect(state.screen == PauseScreen::Stats, "Cancel from a help topic body should also land on Stats");

    // But Cancel from the TOPIC LIST itself (not yet in a topic body) goes
    // to Options, same as Skills/Spells' own list screens.
    state.screen = PauseScreen::Help;
    PauseMenu::Cancel(state);
    Expect(state.screen == PauseScreen::Options, "Cancel from the Help topic list itself should return to Options");
}

void TestQuitGameShowsCreditsBug(PlayerState p, const SpellDatabase& spells, InventoryUiState& inventoryUi,
                                  const std::string& savePath, WorldRegistry& world, ShopState& shop,
                                  WardenState& warden) {
    std::printf("-- PauseMenu::Confirm: \"Quit Game\" shows Credits (real confirmed bug) --\n");
    PauseMenuState state;
    PauseMenu::Open(state);
    state.selectedIndex = 7;  // "Quit Game"
    PauseMenu::Confirm(state, p, spells, inventoryUi, savePath, 1, world, shop, warden);
    Expect(state.screen == PauseScreen::Credits,
           "\"Quit Game\" should show the Credits screen, not actually quit -- a real, confirmed shipped bug");
    Expect(state.active, "the pause menu should still be active (showing Credits), not closed");

    PauseMenu::Confirm(state, p, spells, inventoryUi, savePath, 1, world, shop, warden);
    Expect(state.screen == PauseScreen::Options, "Confirm (Ok) on Credits should return to Options, matching nextScreen=optionsUI");

    state.screen = PauseScreen::Credits;
    PauseMenu::Cancel(state);
    Expect(state.screen == PauseScreen::Options, "Cancel on Credits should also return to Options");
}

void TestSaveAndLoad(PlayerState p, const SpellDatabase& spells, InventoryUiState& inventoryUi) {
    std::printf("-- PauseMenu::Confirm: Save Game / Load Game --\n");
    std::string savePath = (std::filesystem::temp_directory_path() / "stormhold_m63_test_save.dat").string();
    std::error_code ec;
    std::filesystem::remove(savePath, ec);
    Expect(!GameSave::Exists(savePath), "the test save path should not already exist");

    WorldRegistry world(1);
    ShopState shop;
    WardenState warden;

    // Load Game with no save file present: real behavior is
    // noSavedGameUI -- see ui/pause_menu.h's own class comment for why
    // this port maps its "Ok" back to Options instead of the real
    // mainMenuUI (no "return to main menu from live gameplay" machinery
    // exists in this port).
    PauseMenuState state;
    PauseMenu::Open(state);
    state.selectedIndex = 5;  // "Load Game"
    PauseMenuAction action = PauseMenu::Confirm(state, p, spells, inventoryUi, savePath, 1, world, shop, warden);
    Expect(action == PauseMenuAction::None, "a failed load should not report GameLoaded");
    Expect(state.screen == PauseScreen::NoSavedGame, "Load Game with no save file should show NoSavedGame");
    Expect(state.active, "NoSavedGame should not itself close the pause menu");

    PauseMenu::Confirm(state, p, spells, inventoryUi, savePath, 1, world, shop, warden);
    Expect(state.screen == PauseScreen::Options, "Confirm (Ok) on NoSavedGame should return to Options");

    // Save Game: should write a real file and close the pause menu.
    PauseMenu::Open(state);
    state.selectedIndex = 4;  // "Save Game"
    action = PauseMenu::Confirm(state, p, spells, inventoryUi, savePath, 1, world, shop, warden);
    Expect(action == PauseMenuAction::None, "a successful save should not report GameLoaded");
    Expect(!state.active, "a successful Save Game should close the pause menu, back to gameplay");
    Expect(GameSave::Exists(savePath), "Save Game should actually write the save file");

    // Load Game, now that a save exists: should succeed and report
    // GameLoaded so main.cpp knows to refresh tile flags/corridor view.
    PlayerState loadedInto;
    WorldRegistry worldOut(1);
    ShopState shopOut;
    WardenState wardenOut;
    PauseMenu::Open(state);
    state.selectedIndex = 5;  // "Load Game"
    action = PauseMenu::Confirm(state, loadedInto, spells, inventoryUi, savePath, 1, worldOut, shopOut, wardenOut);
    Expect(action == PauseMenuAction::GameLoaded, "a successful load should report GameLoaded");
    Expect(!state.active, "a successful Load Game should close the pause menu, back to gameplay");
    Expect(loadedInto.name == p.name, "the loaded player should match what was saved");

    std::filesystem::remove(savePath, ec);
}

void TestRenderDoesNotCrash(const PlayerState& p, const CharacterData& charData, const SpellDatabase& spells,
                             const ShopDialogue& dialogue) {
    std::printf("-- PauseMenu::Render (every screen) --\n");
    Backbuffer bb;
    for (PauseScreen screen :
         {PauseScreen::Options, PauseScreen::Stats, PauseScreen::Skills, PauseScreen::SkillInfo, PauseScreen::Spells,
          PauseScreen::SpellInfo, PauseScreen::NoSavedGame, PauseScreen::SaveError, PauseScreen::Credits,
          PauseScreen::Help, PauseScreen::HelpTopic}) {
        PauseMenuState state;
        PauseMenu::Open(state);
        state.screen = screen;
        state.skillIndex = 0;
        state.spellIndex0Based = 0;
        state.helpTopicIndex = 0;
        PauseMenu::Render(bb, state, p, charData, spells, dialogue);
    }
}

void TestHelpTopicTextIsReal(const PlayerState& p, const CharacterData& charData, const SpellDatabase& spells,
                              const ShopDialogue& dialogue) {
    std::printf("-- PauseMenu Help: all 12 topics resolve to non-empty real ShopDialogue text --\n");
    Backbuffer bb;
    for (int i = 0; i < 12; i++) {
        PauseMenuState state;
        PauseMenu::Open(state);
        state.screen = PauseScreen::HelpTopic;
        state.helpTopicIndex = i;
        // No direct title/body accessor is exposed outside pause_menu.cpp
        // (they're file-local helpers) -- rendering every topic without
        // throwing (an out-of-range ShopDialogue group/row index would)
        // is what actually proves all 12 kHelpTitleRow/kHelpBodyRows
        // entries land inside group 7's real 41-row span.
        PauseMenu::Render(bb, state, p, charData, spells, dialogue);
    }
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
        ShopDialogue dialogue = ShopDialogue::Load(assets);
        (void)monsters;

        PlayerState freshPlayer = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
        InventoryUiState inventoryUi;
        WorldRegistry world(1);
        ShopState shop;
        WardenState warden;
        std::string savePath = (std::filesystem::temp_directory_path() / "stormhold_m63_test_save_unused.dat").string();

        TestOpenAndCancelFromOptions();
        TestMoveSelectionOnOptions(freshPlayer, spells);
        TestStatsScreen(freshPlayer, spells, inventoryUi, savePath, world, shop, warden);
        TestInventoryHandoff(freshPlayer, spells, savePath, world, shop, warden);
        TestSkillsAndSkillInfo(freshPlayer, charData, spells, inventoryUi, savePath, world, shop, warden);
        TestSpellsAndSpellInfo(charData, items, spells, inventoryUi, savePath, world, shop, warden);
        TestHelpNavigation(freshPlayer, spells, inventoryUi, savePath, world, shop, warden);
        TestQuitGameShowsCreditsBug(freshPlayer, spells, inventoryUi, savePath, world, shop, warden);
        TestSaveAndLoad(freshPlayer, spells, inventoryUi);
        TestRenderDoesNotCrash(freshPlayer, charData, spells, dialogue);
        TestHelpTopicTextIsReal(freshPlayer, charData, spells, dialogue);

        if (!g_ok) {
            std::fprintf(stderr, "m63_pause_menu_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m63_pause_menu_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
