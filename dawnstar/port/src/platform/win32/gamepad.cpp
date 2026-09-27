#include "platform/win32/gamepad.h"

#include <windows.h>

#include <initializer_list>

namespace dawnstar {

namespace {

// The bits of XINPUT_STATE we use (avoids needing xinput.h / xinput.lib),
// same trimmed-down struct oblivion's own input_device.cpp already uses.
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

enum : WORD {
    kDpadUp = 0x0001,
    kDpadDown = 0x0002,
    kDpadLeft = 0x0004,
    kDpadRight = 0x0008,
    kStart = 0x0010,
    kBack = 0x0020,
    kL3 = 0x0040,
    kR3 = 0x0080,
    kLShoulder = 0x0100,
    kRShoulder = 0x0200,
    kA = 0x1000,
    kB = 0x2000,
    kX = 0x4000,
    kY = 0x8000,
};

// Comfortably past thumbstick drift/wear -- about a quarter of the way to
// the stick's own maximum travel.
constexpr SHORT kStickDeadzone = 8000;

}  // namespace

Gamepad::Gamepad() {
    for (const wchar_t* dll : {L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll"}) {
        if (HMODULE m = LoadLibraryW(dll)) {
            getState_ = reinterpret_cast<void*>(GetProcAddress(m, "XInputGetState"));
            if (getState_) break;
        }
    }
}

GamepadButtons Gamepad::Poll() {
    GamepadButtons out;
    if (!getState_) return out;
    auto get = reinterpret_cast<XInputGetStateFn>(getState_);
    XState st{};
    bool ok = false;
    if (padIndex_ >= 0) {
        ok = get(static_cast<DWORD>(padIndex_), &st) == ERROR_SUCCESS;
        if (!ok) padIndex_ = -1;
    } else {
        const std::int64_t now = static_cast<std::int64_t>(GetTickCount64());
        if (now - lastRescanMs_ >= 1000) {
            lastRescanMs_ = now;
            for (int i = 0; i < 4 && !ok; i++) {
                if (get(static_cast<DWORD>(i), &st) == ERROR_SUCCESS) {
                    ok = true;
                    padIndex_ = i;
                }
            }
        }
    }
    if (!ok) return out;

    out.connected = true;
    const WORD b = st.Gamepad.wButtons;
    out.up = (b & kDpadUp) != 0 || st.Gamepad.sThumbLY > kStickDeadzone;
    out.down = (b & kDpadDown) != 0 || st.Gamepad.sThumbLY < -kStickDeadzone;
    out.left = (b & kDpadLeft) != 0 || st.Gamepad.sThumbLX < -kStickDeadzone;
    out.right = (b & kDpadRight) != 0 || st.Gamepad.sThumbLX > kStickDeadzone;
    out.a = (b & kA) != 0;
    out.b = (b & kB) != 0;
    out.x = (b & kX) != 0;
    out.y = (b & kY) != 0;
    out.lb = (b & kLShoulder) != 0;
    out.rb = (b & kRShoulder) != 0;
    out.back = (b & kBack) != 0;
    out.start = (b & kStart) != 0;
    out.l3 = (b & kL3) != 0;
    out.r3 = (b & kR3) != 0;
    return out;
}

}  // namespace dawnstar
