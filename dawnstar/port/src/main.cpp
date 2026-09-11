// M1: real tick loop + backbuffer + present. See ../docs/PORT_ROADMAP.md.
// Still no game logic -- just proves the loop/presentation architecture,
// same scope as shadowkey-decomp's own M0/M1.
#include <windows.h>

#include "engine/game_clock.h"
#include "graphics/backbuffer.h"
#include "platform/win32/window.h"

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    dawnstar::Window window(dawnstar::Backbuffer::kWidth * 2, dawnstar::Backbuffer::kHeight * 2,
                             L"Dawnstar Port");
    dawnstar::Backbuffer backbuffer;
    dawnstar::GameClock clock;

    // Placeholder fill so the window visibly shows a live, presented
    // backbuffer rather than whatever GDI leaves behind by default -- real
    // rendering starts once GameCanvas's corridor renderer is ported.
    backbuffer.Fill(dawnstar::PackRGB565(20, 20, 30));

    window.RunMessageLoop([&] {
        if (clock.ConsumeTick()) {
            // Game tick goes here once there's game state to tick.
        }
        window.Present(backbuffer);
    });

    return 0;
}
