// M8 check: the save record. Plays level 1 as a class-1 player, changes the
// character, saves, then a fresh GameApp must find the save, restore the
// player and settings at start-up, and re-enter the saved level on load.
#include <cstdio>
#include <filesystem>
#include <string>

#include "assets/asset_root.h"
#include "assets/image.h"
#include "game/game_app.h"
#include "world/combat.h"
#include "world/items.h"

using namespace oblivion;
namespace fs = std::filesystem;

namespace {
int failures = 0;
void Check(bool ok, const char* what) {
    std::printf("  %-44s %s\n", what, ok ? "ok" : "FAILED");
    failures += !ok;
}
void Run(GameApp& app, Backbuffer& bb, int ms, bool untilPlaying = false) {
    World& w = app.world();
    for (int t = 0; t < ms; t += 16) {
        if (t % 1100 < 16) app.OnKeyDown(Key::Fire);
        const int st = w.state();
        app.SetHeldKey(st == 10 || st == 9 || st == 4 ? Key::Down : Key::None);
        app.Tick(16);
        app.Draw(bb);
        if (untilPlaying && st == 0 && t > 3000) break;
    }
}
}  // namespace

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "../../extracted";
    const fs::path save = fs::temp_directory_path() / "oblivion_save_smoke" / "test.eso";
    fs::remove_all(save.parent_path());
    AssetRoot assets(dir);
    ImageCache images(assets);
    Backbuffer bb;

    int xp = 0, level = 0, gold = 0, strength = 0, weapon = 0;
    int items = 0, classId = 0;
    {
        GameApp app(assets, images);
        app.SetSavePath(save.string());
        app.Start();
        Check(!app.HasSavedGame(), "no save yet");
        Run(app, bb, 60000, false);
        app.world().SetPlayerClass(3);
        app.world().LoadLevel("/l01_1.scr");
        Run(app, bb, 30000, true);
        Actor* p = app.world().player();
        Check(p != nullptr && app.world().state() == 0, "level 1 playing");
        if (!p) return 1;
        p->xp = 123;
        Combat::LevelUpTo(*p, 3, app.world().script());
        app.world().gold = 777;
        if (const int* row = app.world().script().GetRow(2, 1)) Items::AddItem(*p, 2, row, app.world().script());
        xp = p->xp; level = p->level; gold = app.world().gold; strength = p->strength; weapon = p->weapon; classId = p->classId;
        for (int i = 0; i < 255 && p->inventory[i]; i++) items++;
        app.SaveGame();
        Check(app.HasSavedGame(), "save written");
    }
    {
        GameApp app(assets, images);
        app.SetSavePath(save.string());
        app.Start();
        Check(app.HasSavedGame(), "fresh app sees the save");
        Actor* p = app.world().player();
        Check(p != nullptr, "player restored at start-up");
        if (p) {
            Check(p->xp == xp && p->level == level && p->strength == strength, "xp / level / strength");
            Check(app.world().gold == gold, "gold");
            std::printf("    weapon %d (was %d), class %d\n", p->weapon, weapon, p->classId);
            Check(p->weapon == weapon && p->classId == classId, "weapon and class");
            int n = 0;
            for (int i = 0; i < 255 && p->inventory[i]; i++) n++;
            Check(n == items, "inventory size");
        }
        Run(app, bb, 60000, false);
        app.LoadGame(true);
        Run(app, bb, 30000, true);
        Check(app.world().currentLevel() == "/l01_1.scr" && app.world().state() == 0, "load re-enters level 1");
        Check(app.world().player() && app.world().player()->level == level, "player in the loaded level");
    }
    fs::remove_all(save.parent_path());
    std::printf("%s\n", failures ? "FAILED" : "OK");
    return failures ? 1 : 0;
}
