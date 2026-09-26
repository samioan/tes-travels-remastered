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
#include <cstring>
#include <string>
#include <vector>

#include "assets/asset_root.h"
#include "assets/image.h"
#include "graphics/backbuffer.h"
#include "platform/win32/window.h"
#include "game/game_app.h"
#include "graphics/text.h"

namespace {

// The 12 story levels in Game.buildMenus order.
const char* const kLevels[] = {"/l01_1.scr",  "/l02_2_1.scr", "/l03_3.scr",   "/l04_4.scr",
                               "/l05_5.scr",  "/l06_6_cr.scr", "/l07_7_cr.scr", "/l08_8_cr.scr",
                               "/l09_9_cr.scr", "/l10_10_cr.scr", "/l11_11_cr.scr", "/l12_12.scr"};
constexpr int kLevelCount = static_cast<int>(sizeof(kLevels) / sizeof(kLevels[0]));

bool WritePpm(const char* path, const oblivion::Backbuffer& bb) {
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    std::fprintf(f, "P6\n%d %d\n255\n", oblivion::Backbuffer::kWidth, oblivion::Backbuffer::kHeight);
    const uint32_t* p = bb.Data();
    for (int i = 0; i < oblivion::Backbuffer::kWidth * oblivion::Backbuffer::kHeight; i++) {
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

oblivion::Key HeldKey() {
    static const unsigned keys[] = {VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_RETURN, VK_SPACE};
    for (unsigned k : keys)
        if (GetAsyncKeyState(static_cast<int>(k)) & 0x8000) return KeyForVk(k);
    return oblivion::Key::None;
}

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    std::string assetDir = "../../extracted";  // from oblivion/port/build/
    std::string fontDir = "../assets/fonts";    // Nokia ROM fonts, user-provided (see .gitignore)
    std::string level, dump, keys;
    int runMs = 6000;
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
        else if (a == "--dump") dump = next();
        else if (a == "--keys") keys = next();
        else if (a == "--run-ms") runMs = std::atoi(next().c_str());
    }

    try {
        oblivion::Text::LoadDeviceFonts(fontDir);
        oblivion::AssetRoot assets(assetDir);
        oblivion::ImageCache images(assets);
        oblivion::GameApp app(assets, images);
        oblivion::Backbuffer bb;

        if (!level.empty()) app.StartLevel(level);
        else if (!keys.empty()) app.StartMenu();
        else app.Start();

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

        oblivion::Window window(oblivion::Backbuffer::kWidth * 3, oblivion::Backbuffer::kHeight * 3,
                                L"Oblivion Port");
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
                case VK_ESCAPE: window.RequestClose(); return;
                default: app.OnKeyDown(KeyForVk(vk), KeypadCode(vk)); return;
            }
        });

        ULONGLONG last = GetTickCount64();
        window.RunMessageLoop([&]() {
            ULONGLONG now = GetTickCount64();
            int dt = static_cast<int>(now - last);
            if (dt < 16) return;
            last = now;
            if (dt > 200) dt = 200;  // a stall (window drag) must not fast-forward the game
            app.SetHeldKey(HeldKey());
            app.Tick(dt);
            app.Draw(bb);
            window.Present(bb);
            if (app.quit()) window.RequestClose();
        });
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        if (dump.empty()) MessageBoxA(nullptr, e.what(), "Oblivion Port", MB_OK | MB_ICONERROR);
        return 1;
    }
    return 0;
}
