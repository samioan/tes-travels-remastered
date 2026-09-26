#pragma once
#include "game/input_mapper.h"
#include "platform/win32/window.h"

namespace oblivion {

// Polls the keyboard, mouse and the first connected XInput gamepad into a
// RawInput. Keys only count while the window has focus.
//
// Keyboard: WASD / arrows walk (screen-relative), Space / J attack, E / Enter
// interact, 1 / 2 potions, Tab / R toggle weapon-spell, I inventory,
// Esc menu / back, Backspace back.
// Mouse: left button attacks towards the cursor, right button interacts.
// Gamepad: left stick / d-pad walk, right stick aims, A / RT attack, X interact,
// LB / RB potions, Y toggle, Back inventory, Start menu, B back.
class InputDevice {
public:
    InputDevice();
    RawInput Poll(const Window& window, int dtMs);
    bool padConnected() const { return padIndex_ >= 0; }

private:
    int padIndex_ = -1;
    int rescanMs_ = 0;
    // XInputGetState, loaded dynamically so the exe runs without the DLL.
    void* getState_ = nullptr;
};

}  // namespace oblivion
