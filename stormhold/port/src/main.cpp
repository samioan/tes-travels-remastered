// M1: real tick loop + backbuffer + present. GameClock runs the actual
// 250ms/4Hz cadence (GameCanvas.run(), see ../docs/PORT_ROADMAP.md), a
// 176x208 Backbuffer is presented via GDI each frame. No game logic yet --
// this proves the loop/presentation architecture only, same scope as
// dawnstar's own M1.
#include <windows.h>

#include "engine/game_clock.h"
#include "graphics/backbuffer.h"
#include "platform/win32/window.h"

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    stormhold::Window window(stormhold::Backbuffer::kWidth * 2, stormhold::Backbuffer::kHeight * 2,
                              L"Stormhold Port");
    stormhold::GameClock clock;
    stormhold::Backbuffer backbuffer;

    window.RunMessageLoop([&]() {
        if (clock.ConsumeTick()) {
            backbuffer.Fill(stormhold::PackRGB565(20, 20, 40));
        }
        window.Present(backbuffer);
    });

    return 0;
}
