// M50 smoke test: GameSave::Exists/Save/Load (ESGame.saveGameState()/
// loadGameState(), the plain-file framing around PlayerSave/WorldSave M49's
// own "what's next" note flagged as ready to build), plus
// MenuFlow::Confirm's own "Continue Game" wiring now that GameSave::Exists
// gives it a real answer instead of always taking NoSavedGame. M55 extends
// Save/Load to also cover ShopState/WardenState -- see player/game_save.h's
// own header comment for the file-layout change that needed.
#include <cstdio>
#include <filesystem>
#include <string>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "player/game_save.h"
#include "player/player_creation.h"
#include "ui/menu_flow.h"

namespace {

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

using namespace stormhold;

void TestExistsAndEmptyPath() {
    std::printf("-- GameSave::Exists / empty-path no-ops --\n");

    std::string missing = (std::filesystem::temp_directory_path() / "m50_gamesave_does_not_exist.dat").string();
    std::filesystem::remove(missing);
    Expect(!GameSave::Exists(missing), "Exists is false for a path nothing ever wrote");

    PlayerState p;
    WorldRegistry world(37);
    ShopState shop;
    WardenState warden;
    Expect(!GameSave::Save("", p, world, shop, warden), "Save fails outright on an empty path");
    Expect(!GameSave::Exists(""), "Exists is false for an empty path");

    PlayerState outP;
    WorldRegistry outWorld(37);
    ShopState outShop;
    WardenState outWarden;
    Expect(!GameSave::Load("", 37, outP, outWorld, outShop, outWarden), "Load fails outright on an empty path");
}

void TestRoundTrip(const CharacterData& charData, const ItemDatabase& items, const MonsterDatabase& monsterDb) {
    std::printf("-- GameSave::Save/Load round-trips a real player + world --\n");

    std::string path = (std::filesystem::temp_directory_path() / "m50_gamesave_roundtrip.dat").string();
    std::filesystem::remove(path);

    PlayerState player = PlayerCreation::CreateCharacter(0, "HERO", 1, charData, items);
    player.currentLevel = 5;
    player.tileX = 11;
    player.tileY = 4;

    WorldRegistry world(37);
    MonsterState m = MonsterRuntime::Spawn(201, 2, /*dungeonLevel=*/3, monsterDb);
    m.tileX = 9;
    m.tileY = 9;
    m.currentHp = 7;
    DungeonRuntime::StoreMonster(world, m);
    world.chests[6][PackTileKey(3, 3)] = std::array<int8_t, 8>{3, 3, 0, 2, 10, 0, 1, 0};

    // M55: a real, non-default ShopState/WardenState, so a round-trip
    // that silently dropped either would be caught rather than passing
    // by coincidence against all-zero defaults.
    ShopState shop;
    shop.questRewardClaimable[3] = false;
    shop.firstVisit[5] = false;
    shop.questState1 = {1, 2, 3, 4};
    shop.questState2 = {5, 6, 7, 8};
    shop.interactionCount = {10, 20, 30, 40};
    shop.rewardsGiven = {1, 1, 2, 3};
    shop.unconfirmedCooldownH = {100, 200, 300, 400};
    shop.benecaPoints = 42;
    shop.helgaPoints = 17;
    shop.showSpecialGreeting = true;

    WardenState warden;
    warden.visitCount = 2;
    warden.present = true;

    Expect(!GameSave::Exists(path), "Exists is false before the first Save");
    Expect(GameSave::Save(path, player, world, shop, warden), "Save succeeds against a writable temp path");
    Expect(GameSave::Exists(path), "Exists is true right after Save");

    PlayerState loadedPlayer;
    WorldRegistry loadedWorld(37);
    ShopState loadedShop;
    WardenState loadedWarden;
    Expect(GameSave::Load(path, 37, loadedPlayer, loadedWorld, loadedShop, loadedWarden),
           "Load succeeds reading the file Save just wrote");

    Expect(loadedPlayer.name == "HERO", "the loaded player's name round-trips");
    Expect(loadedPlayer.currentLevel == 5 && loadedPlayer.tileX == 11 && loadedPlayer.tileY == 4,
           "the loaded player's position round-trips");
    Expect(loadedPlayer.coreStats == player.coreStats, "the loaded player's coreStats round-trip exactly");

    Expect(loadedWorld.monsters[2].count(201) == 1, "the loaded world's monster round-trips under its own level");
    MonsterState loadedM = MonsterRuntime::FromBytes(loadedWorld.monsters[2][201]);
    Expect(loadedM.tileX == 9 && loadedM.tileY == 9 && loadedM.currentHp == 7,
           "the loaded monster's own fields round-trip exactly");
    Expect(loadedWorld.chests[6].count(PackTileKey(3, 3)) == 1, "the loaded world's chest round-trips");

    Expect(loadedShop.questRewardClaimable == shop.questRewardClaimable,
           "the loaded ShopState's questRewardClaimable round-trips (M55)");
    Expect(loadedShop.firstVisit == shop.firstVisit, "the loaded ShopState's firstVisit round-trips (M55)");
    Expect(loadedShop.questState1 == shop.questState1, "the loaded ShopState's questState1 round-trips (M55)");
    Expect(loadedShop.questState2 == shop.questState2, "the loaded ShopState's questState2 round-trips (M55)");
    Expect(loadedShop.interactionCount == shop.interactionCount,
           "the loaded ShopState's interactionCount round-trips (M55)");
    Expect(loadedShop.rewardsGiven == shop.rewardsGiven, "the loaded ShopState's rewardsGiven round-trips (M55)");
    Expect(loadedShop.unconfirmedCooldownH == shop.unconfirmedCooldownH,
           "the loaded ShopState's unconfirmedCooldownH round-trips (M55)");
    Expect(loadedShop.benecaPoints == shop.benecaPoints, "the loaded ShopState's benecaPoints round-trips (M55)");
    Expect(loadedShop.helgaPoints == shop.helgaPoints, "the loaded ShopState's helgaPoints round-trips (M55)");
    Expect(loadedShop.showSpecialGreeting == shop.showSpecialGreeting,
           "the loaded ShopState's showSpecialGreeting round-trips (M55)");
    Expect(loadedWarden.visitCount == warden.visitCount, "the loaded WardenState's visitCount round-trips (M55)");
    Expect(loadedWarden.present == warden.present, "the loaded WardenState's present round-trips (M55)");

    std::filesystem::remove(path);
}

void TestLoadFailsOnMissingFile() {
    std::printf("-- GameSave::Load fails cleanly when the file isn't there --\n");

    std::string path = (std::filesystem::temp_directory_path() / "m50_gamesave_missing.dat").string();
    std::filesystem::remove(path);

    PlayerState outP;
    WorldRegistry outWorld(37);
    ShopState outShop;
    WardenState outWarden;
    Expect(!GameSave::Load(path, 37, outP, outWorld, outShop, outWarden),
           "Load returns false, doesn't throw, for a missing file");
}

void TestMenuFlowContinueGame(const CharacterData& charData, const ItemDatabase& items) {
    std::printf("-- MenuFlow::Confirm's Continue Game item, now GameSave-backed --\n");

    std::string path = (std::filesystem::temp_directory_path() / "m50_gamesave_menuflow.dat").string();
    std::filesystem::remove(path);

    // No save file yet, and no savePath passed at all (every pre-M50 call
    // site's own default) -- both take the same NoSavedGame branch M40
    // already established.
    {
        MenuFlowState state;
        state.selectedIndex = 1;
        MenuFlow::Confirm(state, charData, items);
        Expect(state.screen == MenuScreen::NoSavedGame, "Confirm with no savePath argument still falls to NoSavedGame");
        Expect(!state.loadRequested, "loadRequested stays false when there's no save to find");
    }

    // A savePath is passed, but nothing was ever saved there.
    {
        MenuFlowState state;
        state.selectedIndex = 1;
        MenuFlow::Confirm(state, charData, items, path);
        Expect(state.screen == MenuScreen::NoSavedGame, "a real savePath with no file still falls to NoSavedGame");
        Expect(!state.loadRequested, "loadRequested stays false when GameSave::Exists is false");
    }

    // Now a real save file exists at that path.
    {
        PlayerState player;
        WorldRegistry world(37);
        ShopState shop;
        WardenState warden;
        Expect(GameSave::Save(path, player, world, shop, warden),
               "setup: Save succeeds so the next check has a file to find");

        MenuFlowState state;
        state.selectedIndex = 1;
        MenuFlow::Confirm(state, charData, items, path);
        Expect(state.screen == MenuScreen::Finished, "Continue Game jumps straight to Finished once a save exists");
        Expect(state.loadRequested, "loadRequested is set so main.cpp's own hand-off block does the real Load");
    }

    std::filesystem::remove(path);
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assets(root);
        CharacterData charData = CharacterData::Load(assets);
        ItemDatabase items = ItemDatabase::Load(assets);
        MonsterDatabase monsterDb = MonsterDatabase::Load(assets);

        TestExistsAndEmptyPath();
        TestRoundTrip(charData, items, monsterDb);
        TestLoadFailsOnMissingFile();
        TestMenuFlowContinueGame(charData, items);

        if (!g_ok) {
            std::fprintf(stderr, "m50_game_save_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m50_game_save_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
