// M7 check: the random dungeon generator. Every spawn group defined by the
// patch scripts is generated with several seeds; each dungeon must connect its
// entry stairs to its exit stairs through walkable cells and stay in bounds.
// Prints one map as ASCII for eyeballing.
#include <cstdio>
#include <queue>
#include <random>
#include <string>
#include <vector>

#include "assets/asset_root.h"
#include "assets/image.h"
#include "assets/scr.h"
#include "game/game_app.h"
#include "world/dungeon.h"

using namespace oblivion;

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "../../extracted";
    AssetRoot assets(dir);
    int failures = 0, generated = 0;
    bool printed = false;
    for (const char* file : {"/l01_1r.scr", "/l04_4r.scr", "/l06_a.scr", "/l06_b.scr"}) {
        Scr scr = ParseScr(assets.Read(file));
        for (int g = 0; g < 10; g++) {
            if (!scr.tables.has[9][g]) continue;
            const int* row = scr.tables.spawnGroups[g];
            for (int seed = 1; seed <= 8; seed++) {
                std::mt19937 rng(static_cast<unsigned>(seed * 7919));
                Dungeon d = GenerateDungeon(row, 5, 9, [&]() { return static_cast<int>(rng()); });
                generated++;
                const int w = d.map.width, h = d.map.height;
                const std::vector<uint8_t>& col = d.map.collision();
                // BFS over walkable cells from the entry to the exit's neighbourhood.
                std::vector<char> seen(col.size(), 0);
                std::queue<int> q;
                q.push(d.start[0] * h + d.start[1]);
                seen[static_cast<size_t>(q.front())] = 1;
                bool reached = false;
                int walkable = 0;
                for (uint8_t c : col) walkable += c == 0;
                while (!q.empty()) {
                    const int i = q.front();
                    q.pop();
                    const int x = i / h, y = i % h;
                    if (std::abs(x - d.end[0]) <= 1 && std::abs(y - d.end[1]) <= 1) reached = true;
                    const int dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
                    for (int k = 0; k < 4; k++) {
                        const int nx = x + dx[k], ny = y + dy[k];
                        if (nx < 0 || ny < 0 || nx >= w || ny >= h) continue;
                        const int n = nx * h + ny;
                        if (col[static_cast<size_t>(n)] != 0 || seen[static_cast<size_t>(n)]) continue;
                        seen[static_cast<size_t>(n)] = 1;
                        q.push(n);
                    }
                }
                const bool ok = reached && walkable > w + h;
                if (!ok) {
                    std::printf("%s group %d seed %d: %dx%d %s (%d walkable)\n", file, g, seed, w, h,
                                reached ? "too empty" : "exit unreachable", walkable);
                    failures++;
                }
                if (!printed && ok) {
                    printed = true;
                    std::printf("%s group %d seed %d: %dx%d, %d walkable, %zu branch points\n", file, g, seed, w, h,
                                walkable, d.branchPoints.size() / 2);
                    for (int y = 0; y < h; y++) {
                        for (int x = 0; x < w; x++) {
                            const size_t i = static_cast<size_t>(x * h + y);
                            char ch = col[i] ? '#' : '.';
                            if (x == d.start[0] && y == d.start[1]) ch = 'S';
                            if (x == d.end[0] && y == d.end[1]) ch = 'E';
                            std::putchar(ch);
                        }
                        std::putchar('\n');
                    }
                }
            }
        }
    }
    // In the world: run l06_a's dungeon script on top of a loaded level.
    {
        ImageCache images(assets);
        GameApp app(assets, images);
        Backbuffer bb;
        World& world = app.world();
        auto step = [&](int ms) {
            const int st = world.state();
            app.SetHeldKey(st == 10 || st == 9 || st == 4 ? Key::Down : Key::None);
            app.Tick(ms);
            app.Draw(bb);
        };
        app.Start();
        for (int ms = 0; world.state() != 3 && ms < 60000; ms += 16) {
            if (ms % 1100 < 16) app.OnKeyDown(Key::Fire);
            step(16);
        }
        world.LoadLevel("/l06_6.scr");
        for (int ms = 0; ms < 8000; ms += 16) {
            if (ms % 1100 < 16) app.OnKeyDown(Key::Fire);
            step(16);
        }
        world.LoadLevel("/l06_a.scr");
        for (int ms = 0; ms < 4000; ms += 16) step(16);
        world.script().RunScript(3);  // GENERATE_DUNGEON 1 5 9
        for (int ms = 0; ms < 2000; ms += 16) step(16);
        int monsters = 0;
        for (int i = 1; i < World::kMaxActors; i++) monsters += world.ActorAt(i) != nullptr;
        const bool ok = world.view().map().layers.size() == 4 && world.pickupCount() >= 0 && monsters > 0 &&
                        world.player() && world.unimplemented().count("GENERATE_DUNGEON") == 0;
        std::printf("in-world dungeon: %zu layers, %d monsters, %d pickups, player %s -> %s\n",
                    world.view().map().layers.size(), monsters, world.pickupCount(),
                    world.player() ? "present" : "missing", ok ? "ok" : "FAILED");
        failures += !ok;
    }
    std::printf("%d dungeons generated, %d failures\n%s\n", generated, failures, failures || !generated ? "FAILED" : "OK");
    return failures || !generated ? 1 : 0;
}
