// M50 smoke test: GameSave::Exists/Save/Load (ESGame.saveGameState()/
// loadGameState(), the plain-file framing around PlayerSave/WorldSave M49's
// own "what's next" note flagged as ready to build), plus
// MenuFlow::Confirm's own "Continue Game" wiring now that GameSave::Exists
// gives it a real answer instead of always taking NoSavedGame.
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
    Expect(!GameSave::Save("", p, world), "Save fails outright on an empty path");
    Expect(!GameSave::Exists(""), "Exists is false for an empty path");

    PlayerState outP;
    WorldRegistry outWorld(37);
    Expect(!GameSave::Load("", 37, outP, outWorld), "Load fails outright on an empty path");
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

    Expect(!GameSave::Exists(path), "Exists is false before the first Save");
    Expect(GameSave::Save(path, player, world), "Save succeeds against a writable temp path");
    Expect(GameSave::Exists(path), "Exists is true right after Save");

    PlayerState loadedPlayer;
    WorldRegistry loadedWorld(37);
    Expect(GameSave::Load(path, 37, loadedPlayer, loadedWorld), "Load succeeds reading the file Save just wrote");

    Expect(loadedPlayer.name == "HERO", "the loaded player's name round-trips");
    Expect(loadedPlayer.currentLevel == 5 && loadedPlayer.tileX == 11 && loadedPlayer.tileY == 4,
           "the loaded player's position round-trips");
    Expect(loadedPlayer.coreStats == player.coreStats, "the loaded player's coreStats round-trip exactly");

    Expect(loadedWorld.monsters[2].count(201) == 1, "the loaded world's monster round-trips under its own level");
    MonsterState loadedM = MonsterRuntime::FromBytes(loadedWorld.monsters[2][201]);
    Expect(loadedM.tileX == 9 && loadedM.tileY == 9 && loadedM.currentHp == 7,
           "the loaded monster's own fields round-trip exactly");
    Expect(loadedWorld.chests[6].count(PackTileKey(3, 3)) == 1, "the loaded world's chest round-trips");

    std::filesystem::remove(path);
}

void TestLoadFailsOnMissingFile() {
    std::printf("-- GameSave::Load fails cleanly when the file isn't there --\n");

    std::string path = (std::filesystem::temp_directory_path() / "m50_gamesave_missing.dat").string();
    std::filesystem::remove(path);

    PlayerState outP;
    WorldRegistry outWorld(37);
    Expect(!GameSave::Load(path, 37, outP, outWorld), "Load returns false, doesn't throw, for a missing file");
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
        Expect(GameSave::Save(path, player, world), "setup: Save succeeds so the next check has a file to find");

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
