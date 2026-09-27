#pragma once
#include <cstdint>

namespace stormhold {

// One frame of the first connected XInput gamepad's digital buttons.
// Digital-only, unlike Oblivion's own InputDevice/RawInput (../../../oblivion/
// port/src/platform/win32/input_device.h): every action this port binds a pad
// button to is itself a discrete, tick-polled key (see main.cpp's own
// KeyPressed), not an analogue value, so there is no stick vector to carry
// here -- the left stick is folded into up/down/left/right through a plain
// deadzone instead, the same four directions the d-pad already offers.
struct GamepadButtons {
    bool up = false, down = false, left = false, right = false;
    bool a = false, b = false, x = false, y = false;
    bool lb = false, rb = false, back = false, start = false;
    bool l3 = false, r3 = false;  // left/right stick click
    bool connected = false;
};

// Polls the first connected XInput gamepad (rescans for one once a second
// while none is found -- polling an empty slot is comparatively slow).
// See main.cpp's own KeyPressed doc comment for the full button-to-key map:
// d-pad/stick = Up/Down/Left/Right, LB/RB = strafe left/right, A = attack/
// confirm, B = cancel, X = interact, Y = cast, Back = cycle spell, Start =
// options menu, L3 = camp/rest, R3 = minimap zoom.
class Gamepad {
public:
    Gamepad();
    GamepadButtons Poll();

private:
    int padIndex_ = -1;
    std::int64_t lastRescanMs_ = 0;
    // XInputGetState, loaded dynamically so the exe runs without the DLL
    // (same approach as oblivion's own InputDevice).
    void* getState_ = nullptr;
};

}  // namespace stormhold
