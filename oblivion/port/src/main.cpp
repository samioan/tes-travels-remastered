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
#include "world/game_world.h"

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

// Game.mapKey: keyboard -> game action (3 up, 4 down, 5 left, 6 right, 7 fire).
int ActionForKey(unsigned vk) {
    switch (vk) {
        case VK_UP: return 3;
        case VK_DOWN: return 4;
        case VK_LEFT: return 5;
        case VK_RIGHT: return 6;
        case VK_RETURN:
        case VK_SPACE: return 7;
        default: return 0;
    }
}

int HeldAction() {
    static const unsigned keys[] = {VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_RETURN, VK_SPACE};
    for (unsigned k : keys)
        if (GetAsyncKeyState(static_cast<int>(k)) & 0x8000) return ActionForKey(k);
    return 0;
}

std::wstring Widen(const std::string& s) {
    std::wstring w;
    for (unsigned char c : s) w += static_cast<wchar_t>(c);
    return w;
}

// Boots like the game: the startup scripts run until the main menu opens.
void BootToMenu(oblivion::World& world) {
    world.Boot();
    for (int ms = 0; world.state() != 3 && ms < 60000; ms += 16) {
        if (ms % 1100 < 16) world.KeyPressed(7);
        world.Tick(16);
    }
}

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    std::string assetDir = "../../extracted";  // from oblivion/port/build/
    std::string level = kLevels[0];
    std::string dump;
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
        else if (a == "--level") level = next();
        else if (a == "--dump") dump = next();
        else if (a == "--run-ms") runMs = std::atoi(next().c_str());
    }

    try {
        oblivion::AssetRoot assets(assetDir);
        oblivion::ImageCache images(assets);
        oblivion::World world(assets, images);
        oblivion::Backbuffer bb;

        BootToMenu(world);
        world.LoadLevel(level);

        if (!dump.empty()) {
            for (int ms = 0; ms < runMs; ms += 16) {
                if (ms % 1100 < 16) world.KeyPressed(7);
                world.Tick(16);
            }
            world.Draw(bb);
            for (const auto& u : world.unimplemented())
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
                case VK_PRIOR: {
                    levelIndex = (levelIndex + (vk == VK_NEXT ? 1 : kLevelCount - 1)) % kLevelCount;
                    try {
                        world.LoadLevel(kLevels[levelIndex]);
                    } catch (const std::exception& e) {
                        MessageBoxA(nullptr, e.what(), "Oblivion Port", MB_OK);
                    }
                    break;
                }
                case VK_ESCAPE: window.RequestClose(); return;
                default:
                    if (int action = ActionForKey(vk)) world.KeyPressed(action);
                    return;
            }
        });

        ULONGLONG last = GetTickCount64();
        std::wstring lastTitle;
        window.RunMessageLoop([&]() {
            ULONGLONG now = GetTickCount64();
            int dt = static_cast<int>(now - last);
            if (dt < 16) return;
            last = now;
            if (dt > 200) dt = 200;  // a stall (window drag) must not fast-forward the game
            world.HeldAction(HeldAction(), dt);
            world.Tick(dt);
            world.Draw(bb);
            window.Present(bb);
            std::wstring title = L"Oblivion Port";
            const std::string caption = world.Caption();
            if (!caption.empty()) title += L" - " + Widen(caption);
            if (title != lastTitle) {
                window.SetTitle(title);
                lastTitle = title;
            }
        });
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        if (dump.empty()) MessageBoxA(nullptr, e.what(), "Oblivion Port", MB_OK | MB_ICONERROR);
        return 1;
    }
    return 0;
}
