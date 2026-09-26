// Oblivion port entry point. Boots the original startup scripts, loads a level
// script and runs it: the script VM spawns the actors, sets the triggers and
// plays the cutscenes, and the player walks the level with the arrow keys.
// There are no menus, HUD or fonts yet (M6), so dialogue, messages and text
// screens show in the window title; Enter/Space is the fire key.
//
//   oblivion_port.exe [--assets DIR] [--level /l01_1.scr] [--dump out.ppm [--run-ms N]]
//
// --dump runs the world headless for --run-ms (default 6000) simulated
// milliseconds -- pressing fire every 1.1 s to get past dialogue -- then writes
// one frame to a binary PPM and exits.
#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <vector>

#include "assets/asset_root.h"
#include "assets/image.h"
#include "graphics/backbuffer.h"
#include "platform/win32/display.h"
#include "platform/win32/input_device.h"
#include "platform/win32/window.h"
#include "game/game_app.h"
#include "graphics/text.h"
#include "world/combat.h"

namespace {

// The 12 story levels in Game.buildMenus order.
const char* const kLevels[] = {"/l01_1.scr",  "/l02_2_1.scr", "/l03_3.scr",   "/l04_4.scr",
                               "/l05_5.scr",  "/l06_6_cr.scr", "/l07_7_cr.scr", "/l08_8_cr.scr",
                               "/l09_9_cr.scr", "/l10_10_cr.scr", "/l11_11_cr.scr", "/l12_12.scr"};
constexpr int kLevelCount = static_cast<int>(sizeof(kLevels) / sizeof(kLevels[0]));

bool WritePpm(const char* path, const oblivion::Backbuffer& bb) {
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    std::fprintf(f, "P6\n%d %d\n255\n", bb.RealWidth(), oblivion::Backbuffer::kHeight);
    const uint32_t* p = bb.Data();
    for (int i = 0; i < bb.RealWidth() * oblivion::Backbuffer::kHeight; i++) {
        unsigned char rgb[3] = {static_cast<unsigned char>(p[i] >> 16), static_cast<unsigned char>(p[i] >> 8),
                                static_cast<unsigned char>(p[i])};
        std::fwrite(rgb, 1, 3, f);
    }
    std::fclose(f);
    return true;
}

// Desktop keys -> the phone keypad as Game.mapKey sees it.
// Keypad character for a digit / '*' / '#' key (the phone keys the game can bind), or 0.
int KeypadCode(unsigned vk) {
    if (vk >= '0' && vk <= '9') return static_cast<int>(vk);
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) return static_cast<int>('0' + (vk - VK_NUMPAD0));
    if (vk == VK_MULTIPLY) return '*';
    if (vk == VK_DIVIDE) return '#';
    return 0;
}

oblivion::Key KeyForVk(unsigned vk) {
    using oblivion::Key;
    if (KeypadCode(vk)) return Key::Char;
    switch (vk) {
        case VK_UP: return Key::Up;
        case VK_DOWN: return Key::Down;
        case VK_LEFT: return Key::Left;
        case VK_RIGHT: return Key::Right;
        case VK_RETURN:
        case VK_SPACE: return Key::Fire;
        case 'Z':
        case VK_F1: return Key::SoftLeft;
        case 'X':
        case VK_F2: return Key::SoftRight;
        default: return Key::Other;
    }
}


}  // namespace

// OBLIVION_USER_DIR (set by the launcher to <install>/user) is where the save
// and the log live; OBLIVION_SCALE (the launcher's window-size picker) is the
// integer window scale over the native 176x208. Unset (a plain dev build), the
// game behaves exactly as before: save in %APPDATA%, no log, 3x window.
std::string EnvString(const char* name) {
    char* value = nullptr;
    size_t size = 0;
    if (_dupenv_s(&value, &size, name) != 0 || !value) return std::string();
    std::string result(value);
    std::free(value);
    return result;
}

int ResolveScale() {
    const std::string v = EnvString("OBLIVION_SCALE");
    if (v.empty()) return 3;
    return std::max(1, std::min(8, std::atoi(v.c_str())));
}

// stdout/stderr into <userDir>/oblivion_port.log: this is a WINAPI-subsystem
// app with no console, so a bug report needs somewhere to look.
void OpenLogFile(const std::string& userDir) {
    if (userDir.empty()) return;
    std::error_code error;
    std::filesystem::create_directories(userDir, error);
    const std::string path = (std::filesystem::path(userDir) / "oblivion_port.log").string();
    FILE* unused = nullptr;
    freopen_s(&unused, path.c_str(), "a", stdout);
    freopen_s(&unused, path.c_str(), "a", stderr);
    std::printf("--- oblivion_port starting ---\n");
    std::fflush(stdout);
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    // Defaults: a packaged copy keeps `extracted/` and `fonts/` next to the exe; a
    // dev build finds them from oblivion/port/build/. Both are user-provided
    // (the game's data and the Nokia ROM fonts are not redistributed).
    auto firstExisting = [](std::initializer_list<const char*> dirs, const char* probe) {
        for (const char* d : dirs)
            if (std::filesystem::exists(std::filesystem::path(d) / probe)) return std::string(d);
        return std::string(*dirs.begin());
    };
    std::string assetDir = firstExisting({"extracted", "../../extracted"}, "startup.scr");
    std::string fontDir = firstExisting({"fonts", "../assets/fonts"}, ".");
    std::string level, then, dump, keys, savePath;
    int dumpWidth = 0;  // --width N: render headless at a widescreen canvas of N columns
    int fullscreenArg = -1;  // --fullscreen / --windowed override the saved display setting
    int runMs = 6000, runScript = -1;
    for (int i = 1; i < __argc; i++) {
        auto narrow = [](const wchar_t* w) {
            std::string out;
            for (; *w; w++) out += static_cast<char>(*w);  // ASCII paths/flags only
            return out;
        };
        std::string a = narrow(__wargv[i]);
        auto next = [&]() { return i + 1 < __argc ? narrow(__wargv[++i]) : std::string(); };
        if (a == "--assets") assetDir = next();
        else if (a == "--fonts") fontDir = next();
        else if (a == "--level") level = next();
        else if (a == "--save") savePath = next();  // save record file (default: %APPDATA%/OblivionPort/oblivion.eso)
        else if (a == "--width") dumpWidth = std::atoi(next().c_str());
        else if (a == "--fullscreen") fullscreenArg = 1;
        else if (a == "--windowed") fullscreenArg = 0;
        else if (a == "--then") then = next();  // after --level has played, load this level too
        else if (a == "--script") runScript = std::atoi(next().c_str());  // with --then: run this script id too
        else if (a == "--dump") dump = next();
        else if (a == "--keys") keys = next();
        else if (a == "--run-ms") runMs = std::atoi(next().c_str());
    }

    const std::string userDir = EnvString("OBLIVION_USER_DIR");
    OpenLogFile(userDir);
    try {
        oblivion::Text::LoadDeviceFonts(fontDir);
        oblivion::AssetRoot assets(assetDir);
        oblivion::ImageCache images(assets);
        oblivion::GameApp app(assets, images);
        oblivion::Backbuffer bb;
        if (dumpWidth > 0) {
            bb.Resize(dumpWidth);
            app.SetScreenWidth(bb.RealWidth());
        }
        // Headless runs (--dump) only touch a save when told to.
        if (savePath.empty() && dump.empty()) {
            const char* appdata = std::getenv("APPDATA");
            savePath = !userDir.empty() ? (std::filesystem::path(userDir) / "oblivion.eso").string()
                                        : std::string(appdata ? appdata : ".") + "/OblivionPort/oblivion.eso";
        }
        app.SetSavePath(savePath);
        // Display settings (resolution / aspect, fullscreen, scaling) live next to the save.
        const std::string displayCfg =
            !userDir.empty() ? (std::filesystem::path(userDir) / "display.cfg").string()
                             : (std::filesystem::path(std::getenv("APPDATA") ? std::getenv("APPDATA") : ".") /
                                "OblivionPort" / "display.cfg").string();
        oblivion::Display display(dump.empty() ? displayCfg : std::string());  // headless runs never touch the config
        if (fullscreenArg >= 0) display.settings().fullscreen = fullscreenArg == 1;
        app.SetDisplayControl(&display);

        if (!level.empty()) app.StartLevel(level);
        else if (!keys.empty()) app.StartMenu();
        else app.Start();

        if (!dump.empty() && !then.empty()) {
            auto play = [&](int ms) {
                for (int t = 0; t < ms; t += 16) {
                    if (t % 1100 < 16) app.OnKeyDown(oblivion::Key::Fire);
                    const int st = app.world().state();
                    app.SetHeldKey(st == 10 || st == 9 || st == 4 ? oblivion::Key::Down : oblivion::Key::None);
                    app.Tick(16);
                    app.Draw(bb);
                }
            };
            play(runMs);
            app.world().LoadLevel(then);
            play(runMs);
            if (runScript >= 0) {
                app.world().script().RunScript(runScript);
                play(2000);
            }
            for (const auto& u : app.world().unimplemented())
                std::fprintf(stderr, "not ported yet: %s (x%d)\n", u.first.c_str(), u.second);
            return WritePpm(dump.c_str(), bb) ? 0 : 1;
        }
        if (!dump.empty() && !keys.empty()) {
            // With --level the script plays first (fire pressed for dialogue, text scrolled along).
            for (int ms = 0; !level.empty() && ms < runMs; ms += 16) {
                if (ms % 1100 < 16) app.OnKeyDown(oblivion::Key::Fire);
                const int st = app.world().state();
                app.SetHeldKey(st == 10 || st == 9 || st == 4 ? oblivion::Key::Down : oblivion::Key::None);
                app.Tick(16);
                app.Draw(bb);
            }
            app.SetHeldKey(oblivion::Key::None);
            // --keys up,down,left,right,fire,softl,softr,<digit>: typed 300 ms apart.
            size_t pos = 0;
            while (pos <= keys.size()) {
                size_t comma = keys.find(',', pos);
                const std::string tok = keys.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
                pos = comma == std::string::npos ? keys.size() + 1 : comma + 1;
                using oblivion::Key;
                if (tok == "up") app.OnKeyDown(Key::Up);
                else if (tok == "down") app.OnKeyDown(Key::Down);
                else if (tok == "left") app.OnKeyDown(Key::Left);
                else if (tok == "right") app.OnKeyDown(Key::Right);
                else if (tok == "fire") app.OnKeyDown(Key::Fire);
                else if (tok == "softl") app.OnKeyDown(Key::SoftLeft);
                else if (tok == "softr") app.OnKeyDown(Key::SoftRight);
                else if (tok == "foe") {  // debug: stand next to the nearest enemy
                    auto& w = app.world();
                    if (oblivion::Actor* p = w.player()) {
                        int best = 1 << 30;
                        for (int i = 1; i < oblivion::World::kMaxActors; i++) {
                            oblivion::Actor* o = w.ActorAt(i);
                            if (!o || o->dead || o->team == p->team) continue;
                            const int d = oblivion::Combat::Distance(p->pos, o->pos);
                            if (d >= best) continue;
                            best = d;
                            oblivion::ActorSystem::SetPosition(*p, o->pos[0] - 60, o->pos[1]);
                            p->facing = 3;
                        }
                    }
                } else if (tok == "hold") {  // debug: hold fire for one second
                    app.SetHeldKey(Key::Fire);
                    for (int ms = 0; ms < 1000; ms += 16) {
                        app.Tick(16);
                        app.Draw(bb);
                    }
                    app.SetHeldKey(Key::None);
                }
                else if (tok.size() == 1) app.OnKeyDown(Key::Char, tok[0]);
                for (int ms = 0; ms < 300; ms += 16) {
                    app.Tick(16);
                    app.Draw(bb);
                }
            }
            app.Draw(bb);
            return WritePpm(dump.c_str(), bb) ? 0 : 1;
        }
        if (!dump.empty()) {
            for (int ms = 0; ms < runMs; ms += 16) {
                if (ms % 1100 < 16) app.OnKeyDown(oblivion::Key::Fire);
                const int st = app.world().state();  // scroll text screens along
                app.SetHeldKey(st == 10 || st == 9 || st == 4 ? oblivion::Key::Down : oblivion::Key::None);
                app.Tick(16);
                app.Draw(bb);
            }
            app.Draw(bb);
            for (const auto& u : app.world().unimplemented())
                std::fprintf(stderr, "not ported yet: %s (x%d)\n", u.first.c_str(), u.second);
            return WritePpm(dump.c_str(), bb) ? 0 : 1;
        }

        int levelIndex = 0;
        for (int i = 0; i < kLevelCount; i++)
            if (level == kLevels[i]) levelIndex = i;

        const int scale = ResolveScale();
        int clientW, clientH;
        display.InitialClientSize(scale, &clientW, &clientH);
        oblivion::Window window(clientW, clientH, L"Oblivion Port");
        display.Attach(&window);
        window.SetKeyCallback([&](unsigned vk) {
            switch (vk) {
                case VK_NEXT:
                case VK_PRIOR: {  // developer shortcut: jump between story levels
                    levelIndex = (levelIndex + (vk == VK_NEXT ? 1 : kLevelCount - 1)) % kLevelCount;
                    try {
                        app.world().LoadLevel(kLevels[levelIndex]);
                    } catch (const std::exception& e) {
                        MessageBoxA(nullptr, e.what(), "Oblivion Port", MB_OK);
                    }
                    break;
                }
                case VK_F11: display.ToggleFullscreen(); return;
                case VK_F8: display.CycleResolution(GetKeyState(VK_SHIFT) & 0x8000 ? -1 : 1); return;
                // Arrows / WASD / Space / Enter / Esc / the mouse and the gamepad go through
                // the input mapper; what is left is the phone keypad (digits for the
                // rebindable quick keys, * and #) and the soft-key letters.
                default:
                    if (KeypadCode(vk) || vk == 'Z' || vk == 'X' || vk == VK_F1 || vk == VK_F2)
                        app.OnKeyDown(KeyForVk(vk), KeypadCode(vk));
                    return;
            }
        });

        oblivion::InputDevice device;
        oblivion::InputMapper mapper;
        ULONGLONG last = GetTickCount64();
        window.RunMessageLoop([&]() {
            ULONGLONG now = GetTickCount64();
            int dt = static_cast<int>(now - last);
            if (dt < 16) return;
            last = now;
            if (dt > 200) dt = 200;  // a stall (window drag) must not fast-forward the game
            oblivion::MapperContext ctx;
            ctx.state = app.world().state();
            ctx.analogMode = app.AnalogMode();
            ctx.playerOnScreen = app.world().PlayerScreenPos(&ctx.playerX, &ctx.playerY);
            const oblivion::MappedInput mapped = mapper.Update(device.Poll(window, dt), ctx, dt);
            for (const oblivion::InputEvent& ev : mapped.events) {
                if (ev.type == oblivion::InputEvent::Type::Quick) app.OnQuick(ev.quick);
                else app.OnKeyDown(ev.key);
            }
            app.SetHeldKey(mapped.held);
            app.SetAnalogInput(mapped.analog);
            // The canvas width follows the resolution setting (and the window's shape for Auto).
            const int width = display.LogicalWidth();
            if (bb.RealWidth() != width) bb.Resize(width);
            app.SetScreenWidth(width);
            app.Tick(dt);
            app.Draw(bb);
            window.Present(bb, display.settings().scaling);
            if (app.quit()) window.RequestClose();
        });
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        if (dump.empty()) MessageBoxA(nullptr, e.what(), "Oblivion Port", MB_OK | MB_ICONERROR);
        return 1;
    }
    return 0;
}
