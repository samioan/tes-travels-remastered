// Oblivion port entry point. Currently: a level viewer with a test player --
// loads a level .scr from the extracted jar assets, draws its isometric tile
// map and walks a player actor (oh_pc.cml) around it with collision. Arrow
// keys move, PageUp/PageDown switch level, Esc quits.
//
//   oblivion_port.exe [--assets DIR] [--level /l01_1.scr] [--dump out.ppm]
//
// --dump renders one frame to a binary PPM and exits (no window), used to
// check rendering without a display.
#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "assets/asset_root.h"
#include "assets/image.h"
#include "graphics/backbuffer.h"
#include "platform/win32/window.h"
#include "world/actor.h"
#include "world/level_view.h"

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

// Puts the player on the free cell nearest the map centre (the real spawn
// point comes from the level script, M5).
void SpawnPlayer(oblivion::Actor& player, const oblivion::LevelView& view) {
    using namespace oblivion;
    const Grid grid = view.grid();
    int bestD = std::numeric_limits<int>::max(), bx = 0, by = 0;
    for (int x = 1; x + 2 < grid.width; x++) {
        for (int y = 1; y + 1 < grid.height; y++) {
            int dx = x - grid.width / 2, dy = y - grid.height / 2;
            int d = dx * dx + dy * dy;
            if (d >= bestD) continue;
            ActorSystem::SetPosition(player, x * 128 + 64, y * 128 + 64);
            if (!ActorSystem::IsBlocked(player, grid)) {
                bestD = d;
                bx = x;
                by = y;
            }
        }
    }
    ActorSystem::SetPosition(player, bx * 128 + 64, by * 128 + 64);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    std::string assetDir = "../../extracted";  // from oblivion/port/build/
    std::string level = kLevels[0];
    std::string dump;
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
    }

    try {
        oblivion::AssetRoot assets(assetDir);
        oblivion::ImageCache images(assets);
        oblivion::LevelView view(assets, images);
        view.LoadScr(level);
        oblivion::Backbuffer bb;

        oblivion::Actor player;
        oblivion::ActorSystem::Init(player, "/oh_pc.cml", 1,
                                    oblivion::ParseCml(assets.Read("/oh_pc.cml"), images));
        player.speed = 42;  // the character sheet shows a fixed 42
        auto placePlayer = [&]() {
            SpawnPlayer(player, view);
            view.CenterOnScreen(player.screenPos[0], player.screenPos[1]);
        };
        placePlayer();

        auto render = [&]() {
            bb.Fill(0);
            view.Draw(bb, {&player});
        };

        int levelIndex = 0;
        for (int i = 0; i < kLevelCount; i++)
            if (level == kLevels[i]) levelIndex = i;

        if (!dump.empty()) {
            render();
            return WritePpm(dump.c_str(), bb) ? 0 : 1;
        }

        oblivion::Window window(oblivion::Backbuffer::kWidth * 3, oblivion::Backbuffer::kHeight * 3,
                                L"Oblivion Port");
        window.SetKeyCallback([&](unsigned vk) {
            switch (vk) {
                case VK_NEXT:
                case VK_PRIOR: {
                    levelIndex = (levelIndex + (vk == VK_NEXT ? 1 : kLevelCount - 1)) % kLevelCount;
                    try {
                        view.LoadScr(kLevels[levelIndex]);
                        placePlayer();
                    } catch (const std::exception& e) {
                        MessageBoxA(nullptr, e.what(), "Oblivion Port", MB_OK);
                    }
                    break;
                }
                case VK_ESCAPE: window.RequestClose(); return;
                default: return;
            }
        });
        // ActorSystem.handleAction: up/down/left/right walk along -y/+y/-x/+x.
        ULONGLONG last = GetTickCount64();
        window.RunMessageLoop([&]() {
            ULONGLONG now = GetTickCount64();
            int dt = static_cast<int>(now - last);
            if (dt < 16) return;
            last = now;
            int dir = 0;
            if (GetAsyncKeyState(VK_UP) & 0x8000) dir = 2;
            else if (GetAsyncKeyState(VK_DOWN) & 0x8000) dir = 1;
            else if (GetAsyncKeyState(VK_LEFT) & 0x8000) dir = 4;
            else if (GetAsyncKeyState(VK_RIGHT) & 0x8000) dir = 3;
            if (dir) {
                oblivion::ActorSystem::SetAnimState(player, 1);
                oblivion::ActorSystem::MoveDir(player, view.grid(), dir, dt);
            }
            oblivion::ActorSystem::Update(player, dt);
            view.CenterOnScreen(player.screenPos[0], player.screenPos[1]);
            render();
            window.Present(bb);
        });
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        if (dump.empty()) MessageBoxA(nullptr, e.what(), "Oblivion Port", MB_OK | MB_ICONERROR);
        return 1;
    }
    return 0;
}
