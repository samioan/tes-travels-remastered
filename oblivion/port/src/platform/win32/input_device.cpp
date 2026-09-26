#include "platform/win32/input_device.h"

#include <windows.h>

#include <cmath>

namespace oblivion {

namespace {

// The bits of XINPUT_STATE we use (avoids needing xinput.h / xinput.lib).
struct XGamepad {
    WORD wButtons;
    BYTE bLeftTrigger, bRightTrigger;
    SHORT sThumbLX, sThumbLY, sThumbRX, sThumbRY;
};
struct XState {
    DWORD dwPacketNumber;
    XGamepad Gamepad;
};
using XInputGetStateFn = DWORD(WINAPI*)(DWORD, XState*);

bool Down(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }
float Axis(SHORT v) { return v < 0 ? static_cast<float>(v) / 32768.0f : static_cast<float>(v) / 32767.0f; }

}  // namespace

InputDevice::InputDevice() {
    for (const wchar_t* dll : {L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll"}) {
        if (HMODULE m = LoadLibraryW(dll)) {
            getState_ = reinterpret_cast<void*>(GetProcAddress(m, "XInputGetState"));
            if (getState_) break;
        }
    }
}

RawInput InputDevice::Poll(const Window& window, int dtMs) {
    RawInput in;
    const bool focus = window.HasFocus();
    if (focus) {
        in.moveUp = Down('W') || Down(VK_UP);
        in.moveDown = Down('S') || Down(VK_DOWN);
        in.moveLeft = Down('A') || Down(VK_LEFT);
        in.moveRight = Down('D') || Down(VK_RIGHT);
        in.attack = Down(VK_SPACE) || Down('J');
        in.interact = Down('E') || Down(VK_RETURN);
        in.potionHealth = Down('1');
        in.potionMagicka = Down('2');
        in.toggleSpell = Down(VK_TAB) || Down('R');
        in.inventory = Down('I');
        in.menu = Down(VK_ESCAPE);
        in.cancel = Down(VK_BACK);
        int mx, my;
        if (window.CursorPos(&mx, &my)) {
            in.mouseInWindow = true;
            in.mouseX = mx;
            in.mouseY = my;
            in.mouseLeft = Down(VK_LBUTTON);
            in.mouseRight = Down(VK_RBUTTON);
        }
    }

    // Gamepad: look for one now and then (polling an empty slot is slow).
    if (getState_) {
        auto get = reinterpret_cast<XInputGetStateFn>(getState_);
        XState st{};
        bool ok = false;
        if (padIndex_ >= 0) {
            ok = get(static_cast<DWORD>(padIndex_), &st) == ERROR_SUCCESS;
            if (!ok) padIndex_ = -1;
        } else {
            rescanMs_ += dtMs;
            if (rescanMs_ >= 1000) {
                rescanMs_ = 0;
                for (int i = 0; i < 4 && !ok; i++) {
                    if (get(static_cast<DWORD>(i), &st) == ERROR_SUCCESS) {
                        ok = true;
                        padIndex_ = i;
                    }
                }
            }
        }
        if (ok && focus) {
            in.padConnected = true;
            in.padButtons = st.Gamepad.wButtons;
            in.padLX = Axis(st.Gamepad.sThumbLX);
            in.padLY = Axis(st.Gamepad.sThumbLY);
            in.padRX = Axis(st.Gamepad.sThumbRX);
            in.padRY = Axis(st.Gamepad.sThumbRY);
            in.padLT = st.Gamepad.bLeftTrigger / 255.0f;
            in.padRT = st.Gamepad.bRightTrigger / 255.0f;
        }
    }
    return in;
}

}  // namespace oblivion
