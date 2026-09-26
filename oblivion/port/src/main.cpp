// Oblivion port entry point. Currently: a level viewer -- loads a level .scr
// from the extracted jar assets and draws its isometric tile map. Arrow keys
// pan, PageUp/PageDown switch level, Esc quits.
//
//   oblivion_port.exe [--assets DIR] [--level /l01_1.scr] [--dump out.ppm]
//
// --dump renders one frame to a binary PPM and exits (no window), used to
// check rendering without a display.
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

        auto render = [&]() {
            bb.Fill(0);
            view.Draw(bb);
        };

        if (!dump.empty()) {
            render();
            return WritePpm(dump.c_str(), bb) ? 0 : 1;
        }

        int levelIndex = 0;
        for (int i = 0; i < kLevelCount; i++)
            if (level == kLevels[i]) levelIndex = i;

        oblivion::Window window(oblivion::Backbuffer::kWidth * 3, oblivion::Backbuffer::kHeight * 3,
                                L"Oblivion Port");
        bool dirty = true;
        window.SetKeyCallback([&](unsigned vk) {
            const int step = 16;
            switch (vk) {
                case VK_LEFT: view.Pan(step, 0); break;
                case VK_RIGHT: view.Pan(-step, 0); break;
                case VK_UP: view.Pan(0, step); break;
                case VK_DOWN: view.Pan(0, -step); break;
                case VK_NEXT:
                case VK_PRIOR: {
                    levelIndex = (levelIndex + (vk == VK_NEXT ? 1 : kLevelCount - 1)) % kLevelCount;
                    try {
                        view.LoadScr(kLevels[levelIndex]);
                    } catch (const std::exception& e) {
                        MessageBoxA(nullptr, e.what(), "Oblivion Port", MB_OK);
                    }
                    break;
                }
                case VK_ESCAPE: window.RequestClose(); return;
                default: return;
            }
            dirty = true;
        });
        window.RunMessageLoop([&]() {
            if (dirty) {
                render();
                window.Present(bb);
                dirty = false;
            }
        });
    } catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "Oblivion Port", MB_OK | MB_ICONERROR);
        return 1;
    }
    return 0;
}
