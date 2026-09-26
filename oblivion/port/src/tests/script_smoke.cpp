// M5 check: run every level's script headless. Each level is loaded like
// Game.loadLevel does, then ticked at 16 ms with a fire key pressed every
// 1.1 s (dismisses dialogue, WAIT_KEY, text screens). A level passes when its
// script reaches the playing state (or parks on the menu/shop screen that
// M6 will implement) with a player on the map, and never throws.
// Also prints the ops that were skipped because their subsystem is not ported.
#include <cstdio>
#include <filesystem>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "assets/asset_root.h"
#include "assets/image.h"
#include "world/game_world.h"

using namespace oblivion;
namespace fs = std::filesystem;

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "../../extracted";
    AssetRoot assets(dir);
    ImageCache images(assets);

    std::vector<std::string> levels;
    for (const auto& e : fs::directory_iterator(dir))
        if (e.path().extension() == ".scr") levels.push_back("/" + e.path().filename().string());

    int failures = 0;
    std::set<std::string> allUnimpl;
    for (const std::string& lvl : levels) {
        if (lvl == "/startup.scr" || lvl == "/startup2.scr") continue;
        World world(assets, images);
        try {
            // Boot like the game does: splash screens, then the main menu (state 3).
            world.Boot();
            for (int ms = 0; world.state() != 3; ms += 16) {
                if (ms > 60000) throw std::runtime_error("boot never reached the menu");
                if (ms % 1100 < 16) world.KeyPressed(7);
                world.Tick(16);
            }
            world.LoadLevel(lvl);
            bool sawMenu = false, sawPlaying = false;
            int ms = 0;
            const int kDt = 16, kLimitMs = 60000;
            for (; ms < kLimitMs; ms += kDt) {
                if (ms % 1100 < kDt) world.KeyPressed(7);
                world.Tick(kDt);
                sawPlaying |= world.state() == 0;
                if (world.state() == 3) sawMenu = true;
                // Stop once the script settled: playing, or parked on a menu/shop screen.
                if (sawMenu || (sawPlaying && ms > 3000 && !world.script().Running())) break;
                if (sawPlaying && ms > 20000) break;
            }
            // A level script loads a map. Patch scripts (l01_1r, l06_a, ...) have none:
            // they edit a level that is already running.
            const bool hasMap = world.view().loaded();
            const char* status = !hasMap ? "patch" : sawPlaying ? "playing" : sawMenu ? "menu" : "STUCK";
            const bool ok = std::string(status) != "STUCK" && (!hasMap || world.player() != nullptr);
            std::printf("%-16s %-8s %6d ms  state %2d", lvl.c_str(), status, ms, world.state());
            if (world.player()) std::printf("  player @ cell %d,%d", world.player()->cell[0], world.player()->cell[1]);
            std::printf("\n");
            for (const auto& u : world.unimplemented()) allUnimpl.insert(u.first);
            if (!ok) failures++;
        } catch (const std::exception& e) {
            std::printf("%-16s FAIL  exception: %s\n", lvl.c_str(), e.what());
            failures++;
        }
    }
    std::printf("ops skipped (subsystem not ported yet):");
    for (const auto& u : allUnimpl) std::printf(" %s", u.c_str());
    std::printf("\n%s\n", failures ? "FAILED" : "OK");
    return failures ? 1 : 0;
}
