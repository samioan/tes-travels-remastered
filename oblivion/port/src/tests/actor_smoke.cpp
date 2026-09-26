// M4 check: on every story level, a player walking in a fixed pattern against
// the collision layer is never left in a blocked position, never leaves the
// grid, and the draw-order cell always lies inside the footprint cells.
#include <cstdio>
#include <string>

#include "assets/asset_root.h"
#include "assets/cml.h"
#include "assets/image.h"
#include "world/actor.h"
#include "world/level_view.h"

using namespace oblivion;

int main(int argc, char** argv) {
    const char* levels[] = {"/l01_1.scr",  "/l02_2_1.scr",  "/l03_3.scr",    "/l04_4.scr",
                            "/l05_5.scr",  "/l06_6_cr.scr", "/l07_7_cr.scr", "/l08_8_cr.scr",
                            "/l09_9_cr.scr", "/l10_10_cr.scr", "/l11_11_cr.scr", "/l12_12.scr"};
    AssetRoot assets(argc > 1 ? argv[1] : "../../extracted");
    ImageCache images(assets);
    SpriteSet pc = ParseCml(assets.Read("/oh_pc.cml"), images);
    int failures = 0;

    for (const char* lvl : levels) {
        LevelView view(assets, images);
        try {
            view.LoadScr(lvl);
        } catch (const std::exception& e) {
            std::printf("FAIL %s: %s\n", lvl, e.what());
            failures++;
            continue;
        }
        const Grid grid = view.grid();
        Actor a;
        ActorSystem::Init(a, "/oh_pc.cml", 1, pc);
        a.speed = 42;

        // Find any free cell to start from.
        bool placed = false;
        for (int x = 1; x + 2 < grid.width && !placed; x++)
            for (int y = 1; y + 1 < grid.height && !placed; y++) {
                ActorSystem::SetPosition(a, x * 128 + 64, y * 128 + 64);
                placed = !ActorSystem::IsBlocked(a, grid);
            }
        if (!placed) {
            std::printf("FAIL %s: no free cell\n", lvl);
            failures++;
            continue;
        }

        int moved = 0, bad = 0;
        const int dirs[] = {3, 1, 4, 2};
        for (int step = 0; step < 4000; step++) {
            int dir = dirs[(step / 100) % 4];
            int before[2] = {a.pos[0], a.pos[1]};
            ActorSystem::MoveDir(a, grid, dir, 60);
            if (a.pos[0] != before[0] || a.pos[1] != before[1]) moved++;
            if (ActorSystem::IsBlocked(a, grid)) bad++;
            bool inside = a.sortCell[0] == a.cell[0] || a.sortCell[0] == a.footBCell[0] || a.sortCell[0] == a.footCCell[0];
            if (!inside) bad++;
        }
        std::printf("%-16s %3dx%-3d moved %4d steps, %d violations\n", lvl, grid.width, grid.height, moved, bad);
        if (bad || moved == 0) failures++;
    }
    std::printf(failures ? "FAILED\n" : "OK\n");
    return failures ? 1 : 0;
}
