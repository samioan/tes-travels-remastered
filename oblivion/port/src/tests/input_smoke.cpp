// Modern controls check: the input mapper (keyboard / mouse / gamepad -> what
// the game consumes) and free isometric movement in a real level.
#include <cmath>
#include <cstdio>
#include <string>

#include "assets/asset_root.h"
#include "assets/image.h"
#include "game/game_app.h"
#include "game/input_mapper.h"
#include "world/combat.h"

using namespace oblivion;

namespace {
int failures = 0;
void Check(bool ok, const char* what) {
    std::printf("  %-58s %s\n", what, ok ? "ok" : "FAILED");
    failures += !ok;
}
bool Near(float a, float b, float eps = 0.02f) { return std::fabs(a - b) <= eps; }
int CountKey(const MappedInput& m, Key k) {
    int n = 0;
    for (const auto& e : m.events) n += e.type == InputEvent::Type::KeyDown && e.key == k;
    return n;
}
}  // namespace

int main(int argc, char** argv) {
    // ---- mapper ----
    {
        std::printf("input mapper\n");
        InputMapper m;
        MapperContext play;
        play.analogMode = true;
        RawInput in;
        in.moveUp = true;
        MappedInput o = m.Update(in, play, 16);
        Check(Near(o.analog.moveX, 0) && Near(o.analog.moveY, -1), "W walks straight up the screen");
        in.moveRight = true;
        o = m.Update(in, play, 16);
        Check(Near(o.analog.moveX, 0.7071f) && Near(o.analog.moveY, -0.7071f), "W+D is a normalised diagonal");
        in = RawInput{};
        in.padConnected = true;
        in.padLX = 0.1f;
        in.padLY = 0.1f;
        o = m.Update(in, play, 16);
        Check(o.analog.moveX == 0 && o.analog.moveY == 0, "stick inside the dead zone does not move");
        in.padLX = 1.0f;
        in.padLY = 0.0f;
        o = m.Update(in, play, 16);
        Check(Near(o.analog.moveX, 1.0f), "full stick right walks right at full speed");
        in.padLX = 0.625f;
        o = m.Update(in, play, 16);
        Check(o.analog.moveX > 0.4f && o.analog.moveX < 0.6f, "half stick walks slower");
        in = RawInput{};
        in.padConnected = true;
        in.padLY = 1.0f;  // XInput +y is up
        o = m.Update(in, play, 16);
        Check(Near(o.analog.moveY, -1.0f), "stick up moves up the screen");
        in = RawInput{};
        in.padConnected = true;
        in.padRX = 1.0f;
        o = m.Update(in, play, 16);
        Check(o.analog.aim && o.analog.aimX > 0.9f, "right stick aims");
        in = RawInput{};
        in.mouseInWindow = true;
        in.mouseX = 100;
        in.mouseY = 50;
        in.mouseLeft = true;
        play.playerOnScreen = true;
        play.playerX = 88;
        play.playerY = 100;
        o = m.Update(in, play, 16);
        Check(o.analog.attack && o.analog.aim && o.analog.aimX > 0 && o.analog.aimY < 0, "left click attacks towards the cursor");
        Check(CountKey(o, Key::Fire) == 1, "the press also reaches dialogue/script as Fire");
        o = m.Update(in, play, 16);
        Check(o.analog.attack && CountKey(o, Key::Fire) == 0, "holding attack repeats the attack, not the Fire event");
        in = RawInput{};
        in.potionHealth = true;
        o = m.Update(in, play, 16);
        Check(o.events.size() == 1 && o.events[0].type == InputEvent::Type::Quick && o.events[0].quick == 0, "1 quaffs a health potion");
        in = RawInput{};
        in.padConnected = true;
        in.padButtons = kPadRightShoulder | kPadY;
        o = m.Update(in, play, 16);
        Check(o.events.size() == 2, "RB = magicka potion, Y = toggle spell");
        in = RawInput{};
        in.menu = true;
        o = m.Update(in, play, 16);
        Check(CountKey(o, Key::SoftLeft) == 1, "Esc opens the menu");
        in = RawInput{};
        in.inventory = true;
        o = m.Update(in, play, 16);
        Check(CountKey(o, Key::SoftRight) == 1, "I opens the inventory");
        in = RawInput{};
        in.interact = true;
        o = m.Update(in, play, 16);
        Check(o.analog.interact, "E is a one-frame interact");
        o = m.Update(in, play, 16);
        Check(!o.analog.interact, "holding E does not repeat interact");

        // menus
        InputMapper mm;
        MapperContext menu;
        menu.state = 3;
        in = RawInput{};
        in.moveDown = true;
        o = mm.Update(in, menu, 16);
        Check(CountKey(o, Key::Down) == 1 && o.held == Key::Down, "menu: down key press");
        int repeats = 0;
        for (int t = 0; t < 1000; t += 16) repeats += CountKey(mm.Update(in, menu, 16), Key::Down);
        Check(repeats >= 4 && repeats <= 8, "menu: auto-repeat after a delay");
        mm.Update(RawInput{}, menu, 16);  // release everything first
        in = RawInput{};
        in.padConnected = true;
        in.padLY = -1.0f;
        o = mm.Update(in, menu, 16);
        Check(CountKey(o, Key::Down) == 1, "menu: left stick down");
        in = RawInput{};
        in.padConnected = true;
        in.padButtons = kPadA;
        o = mm.Update(in, menu, 16);
        Check(CountKey(o, Key::Fire) == 1 && CountKey(o, Key::SoftRight) == 0, "menu: A confirms");
        in = RawInput{};
        o = mm.Update(in, menu, 16);
        MapperContext prompt;
        prompt.state = 19;
        in.padConnected = true;
        in.padButtons = kPadA;
        o = mm.Update(in, prompt, 16);
        Check(CountKey(o, Key::SoftRight) == 1, "quit prompt: A answers yes");
        in = RawInput{};
        o = mm.Update(in, prompt, 16);
        in.padConnected = true;
        in.padButtons = kPadB;
        o = mm.Update(in, prompt, 16);
        Check(CountKey(o, Key::SoftLeft) == 1, "prompt: B answers no / back");
    }

    // Regression: a button that opens a screen and is still held must not also
    // close it (Select opened the inventory and it closed again at once).
    {
        struct Case { const char* name; RawInput in; Key opens; int state; };
        Case cases[3];
        cases[0] = {"Select (Back) -> inventory", RawInput{}, Key::SoftRight, 2};
        cases[0].in.padConnected = true;
        cases[0].in.padButtons = kPadBack;
        cases[1] = {"I -> inventory", RawInput{}, Key::SoftRight, 2};
        cases[1].in.inventory = true;
        cases[2] = {"Start -> menu", RawInput{}, Key::SoftLeft, 3};
        cases[2].in.padConnected = true;
        cases[2].in.padButtons = kPadStart;
        for (const Case& c : cases) {
            InputMapper m;
            MapperContext play;
            play.analogMode = true;
            MappedInput o = m.Update(c.in, play, 16);
            Check(CountKey(o, c.opens) == 1, (std::string(c.name) + " opens it").c_str());
            MapperContext screen;
            screen.state = c.state;
            int closes = 0;
            for (int t = 0; t < 600; t += 16) closes += CountKey(m.Update(c.in, screen, 16), Key::SoftLeft);
            Check(closes == 0, (std::string(c.name) + ": still held, it stays open").c_str());
            RawInput released;
            released.padConnected = c.in.padConnected;
            m.Update(released, screen, 16);
            o = m.Update(c.in, screen, 16);
            if (c.state == 2) Check(CountKey(o, Key::SoftLeft) == 1, (std::string(c.name) + ": a fresh press closes it").c_str());
        }
        // B / Backspace opens the menu in play; it must stay open while held, too.
        InputMapper m;
        MapperContext play;
        play.analogMode = true;
        RawInput b;
        b.cancel = true;
        MappedInput o = m.Update(b, play, 16);
        Check(CountKey(o, Key::SoftLeft) == 1, "Backspace opens the menu");
        MapperContext menu;
        menu.state = 3;
        int closes = 0;
        for (int t = 0; t < 600; t += 16) closes += CountKey(m.Update(b, menu, 16), Key::SoftLeft);
        Check(closes == 0, "Backspace held: the menu stays open");
    }

    // ---- free movement in a level ----
    const std::string dir = argc > 1 ? argv[1] : "../../extracted";
    AssetRoot assets(dir);
    ImageCache images(assets);
    Backbuffer bb;
    GameApp app(assets, images);
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
    world.LoadLevel("/l04_4b.scr");
    for (int ms = 0; ms < 30000; ms += 16) {
        if (ms % 1100 < 16) app.OnKeyDown(Key::Fire);
        step(16);
        if (world.state() == 0 && ms > 3000) break;
    }
    Actor* p = world.player();
    std::printf("free movement (level 4b)\n");
    Check(p && world.state() == 0 && app.AnalogMode(), "playing in analogue mode");
    if (!p) return 1;

    // Find open ground: a cell whose surrounding 9x9 is walkable.
    const JtmMap& map = world.view().map();
    int fx = -1, fy = -1;
    for (int x = 5; x < map.width - 5 && fx < 0; x++)
        for (int y = 5; y < map.height - 5 && fx < 0; y++) {
            bool free = true;
            for (int dx = -4; dx <= 4 && free; dx++)
                for (int dy = -4; dy <= 4 && free; dy++)
                    free = map.collision()[static_cast<size_t>((x + dx) * map.height + y + dy)] == 0;
            if (free) fx = x, fy = y;
        }
    Check(fx >= 0, "found open ground");
    if (fx < 0) return 1;
    p->invulnerable = 1;
    p->speed = 100;
    struct Dir { const char* name; float sx, sy; };
    const Dir dirs[8] = {{"up", 0, -1}, {"down", 0, 1}, {"left", -1, 0}, {"right", 1, 0},
                         {"up-left", -1, -1}, {"up-right", 1, -1}, {"down-left", -1, 1}, {"down-right", 1, 1}};
    double speeds[8];
    for (int i = 0; i < 8; i++) {
        ActorSystem::SetPosition(*p, fx * 128 + 64, fy * 128 + 64);
        const int x0 = p->pos[0], y0 = p->pos[1];
        AnalogInput a;
        a.moveX = dirs[i].sx;
        a.moveY = dirs[i].sy;
        for (int t = 0; t < 500; t += 16) {
            app.SetAnalogInput(a);
            world.Tick(0);
            app.Tick(16);
        }
        const double dx = p->pos[0] - x0, dy = p->pos[1] - y0;
        speeds[i] = std::sqrt(dx * dx + dy * dy) / 0.512;  // units per second (32 frames of 16 ms)
        // Expected ground direction: screen up = world (-x,-y), right = (+x,-y).
        const double ex = (dirs[i].sx + dirs[i].sy), ey = (dirs[i].sy - dirs[i].sx);
        const bool dirOk = (ex == 0 || (dx > 0) == (ex > 0)) && (ey == 0 || (dy > 0) == (ey < 0 ? false : true)) ;
        (void)dirOk;
        const bool signX = ex == 0 ? std::fabs(dx) < 2 : (dx > 0) == (ex > 0);
        const bool signY = ey == 0 ? std::fabs(dy) < 2 : (dy > 0) == (ey > 0);
        char msg[96];
        std::snprintf(msg, sizeof(msg), "%-10s moves world (%+.0f,%+.0f) at %.0f u/s", dirs[i].name, dx, dy, speeds[i]);
        Check(signX && signY && speeds[i] > 60, msg);
    }
    double lo = speeds[0], hi = speeds[0];
    for (double s : speeds) lo = std::min(lo, s), hi = std::max(hi, s);
    Check(hi - lo < hi * 0.08, "all eight directions walk at the same speed");

    // Facing follows the walk direction; the aim overrides it.
    {
        AnalogInput a;
        a.moveX = 1;
        a.moveY = 1;  // world +x
        app.SetAnalogInput(a);
        app.Tick(16);
        Check(p->facing == 3, "walking down-right faces +x");
        a.aim = true;
        a.aimX = -1;
        a.aimY = 1;  // screen down-left = world +y
        app.SetAnalogInput(a);
        app.Tick(16);
        Check(p->facing == 1, "aiming overrides the walking facing");
    }

    // Sliding: walk diagonally into a wall and keep moving along it.
    {
        // A wall cell to the east of open ground: find x where cell x+1 is solid, same free row.
        int wx = -1, wy = -1;
        for (int x = 2; x < map.width - 3 && wx < 0; x++)
            for (int y = 3; y < map.height - 3 && wx < 0; y++) {
                auto solid = [&](int cx, int cy) { return map.collision()[static_cast<size_t>(cx * map.height + cy)] != 0; };
                if (!solid(x, y) && !solid(x, y - 1) && !solid(x, y - 2) && !solid(x, y + 1) && solid(x + 1, y) &&
                    solid(x + 1, y - 1) && solid(x + 1, y + 1) && solid(x + 1, y - 2))
                    wx = x, wy = y;
            }
        if (wx >= 0) {
            ActorSystem::SetPosition(*p, wx * 128 + 40, wy * 128 + 64);
            const int y0 = p->pos[1];
            AnalogInput a;
            a.moveX = 1;
            a.moveY = 0.0f;  // world (+x, -y): into the wall and along it
            for (int t = 0; t < 400; t += 16) {
                app.SetAnalogInput(a);
                app.Tick(16);
            }
            Check(p->pos[1] < y0 - 5, "walking into a wall slides along it");
        } else {
            std::printf("  (no suitable wall in this level, slide check skipped)\n");
        }
    }

    // Attack through the analogue path: aim at the nearest enemy and hold attack.
    {
        Actor* foe = nullptr;
        int best = 1 << 30;
        for (int i = 1; i < World::kMaxActors; i++) {
            Actor* o = world.ActorAt(i);
            if (!o || o->dead || o->team == p->team) continue;
            const int d = Combat::Distance(p->pos, o->pos);
            if (d < best) best = d, foe = o;
        }
        Check(foe != nullptr, "an enemy is on the level");
        if (foe) {
            ActorSystem::SetPosition(*p, foe->pos[0] - 60, foe->pos[1]);
            p->killTimer = 0;
            const int hp0 = foe->hp;
            std::shared_ptr<Actor> keep = foe->shared_from_this();
            AnalogInput a;
            a.attack = true;
            a.aim = true;
            a.aimX = 1;
            a.aimY = 1;
            for (int t = 0; t < 8000 && !keep->dead; t += 16) {
                app.SetAnalogInput(a);
                app.Tick(16);
            }
            Check(keep->hp < hp0 || keep->dead, "holding attack hurts the enemy");
        }
    }
    std::printf("%s\n", failures ? "FAILED" : "OK");
    return failures ? 1 : 0;
}
