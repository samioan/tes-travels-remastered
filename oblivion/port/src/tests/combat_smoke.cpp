// M7 check: combat. Each playable level is loaded, the player is dropped next
// to its nearest enemy and holds the fire key while the world ticks. Reports
// what happened (damage dealt/taken, kills, xp, level-ups, loot lying around,
// projectiles) and fails if nothing in the whole run ever resolved a fight.
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "assets/asset_root.h"
#include "assets/image.h"
#include "game/game_app.h"
#include "world/combat.h"

using namespace oblivion;
namespace fs = std::filesystem;

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "../../extracted";
    AssetRoot assets(dir);
    ImageCache images(assets);
    std::vector<std::string> levels;
    for (const auto& e : fs::directory_iterator(dir))
        if (e.path().extension() == ".scr") levels.push_back("/" + e.path().filename().string());

    int failures = 0, fights = 0, kills = 0, tookDamage = 0, levelups = 0, loot = 0;
    for (const std::string& lvl : levels) {
        if (lvl == "/startup.scr" || lvl == "/startup2.scr") continue;
        GameApp app(assets, images);
        Backbuffer bb;
        World& world = app.world();
        auto step = [&](int ms) {
            const int st = world.state();
            app.SetHeldKey(st == 10 || st == 9 || st == 4 ? Key::Down : Key::None);
            app.Tick(ms);
            app.Draw(bb);
        };
        try {
            app.Start();
            for (int ms = 0; world.state() != 3; ms += 16) {
                if (ms > 60000) throw std::runtime_error("boot never reached the menu");
                if (ms % 1100 < 16) app.OnKeyDown(Key::Fire);
                step(16);
            }
            world.LoadLevel(lvl);
            bool playing = false;
            for (int ms = 0; ms < 30000 && !(playing && ms > 3000); ms += 16) {
                if (ms % 1100 < 16) app.OnKeyDown(Key::Fire);
                step(16);
                playing |= world.state() == 0;
                if (world.state() == 3) break;
            }
            Actor* p = world.player();
            if (!playing || !p || world.state() != 0) continue;

            // Nearest living enemy.
            Actor* foe = nullptr;
            int best = 1 << 30;
            for (int i = 1; i < World::kMaxActors; i++) {
                Actor* o = world.ActorAt(i);
                if (!o || o->dead || o->team == p->team) continue;
                const int d = Combat::Distance(p->pos, o->pos);
                if (d < best) best = d, foe = o;
            }
            if (!foe) {
                std::printf("%-16s no enemies\n", lvl.c_str());
                continue;
            }
            const std::string foeName = foe->name.empty() ? foe->cmlPath : foe->name;
            const int foeMax = foe->maxHp, foeLevel = foe->level;
            if (p->level == 1) p->xp = 95;  // 100 xp is level 2: any kill should level the player up
            const int xp0 = p->xp, lvl0 = p->level, hp0 = p->hp;
            world.script().tables();  // (keeps the interpreter linked in)
            ActorSystem::SetPosition(*p, foe->pos[0] - 60, foe->pos[1]);
            p->facing = 3;
            p->killTimer = 0;
            int minHp = p->hp, weaponSwings = 0;
            std::shared_ptr<Actor> foeKeep = foe->shared_from_this();
            for (int ms = 0; ms < 12000 && world.state() == 0; ms += 16) {
                world.HeldAction(7, 16);
                world.Tick(16);
                weaponSwings++;
                minHp = std::min(minHp, p->hp);
                if (foeKeep->dead) break;
            }
            const bool killed = foeKeep->dead != 0;
            fights++;
            kills += killed;
            tookDamage += minHp < hp0;
            levelups += p->level > lvl0;
            loot += world.pickupCount() > 0;
            std::printf("%-16s %-22s lvl %2d hp %3d -> %3d  %s  player hp %3d -> %3d  xp +%d%s  pickups %d\n",
                        lvl.c_str(), foeName.c_str(), foeLevel, foeMax, foeKeep->hp, killed ? "KILLED " : "alive  ",
                        hp0, minHp, p->xp - xp0, p->level > lvl0 ? "  LEVEL UP" : "", world.pickupCount());
        } catch (const std::exception& e) {
            std::printf("%-16s FAIL  exception: %s\n", lvl.c_str(), e.what());
            failures++;
        }
    }
    std::printf("fights %d, kills %d, player hurt in %d, level-ups %d, levels with loot %d\n", fights, kills, tookDamage,
                levelups, loot);
    if (fights == 0 || kills == 0) {
        std::printf("no fight was ever won\n");
        failures++;
    }
    std::printf("%s\n", failures ? "FAILED" : "OK");
    return failures ? 1 : 0;
}
