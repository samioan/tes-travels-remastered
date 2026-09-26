#include "game/input_mapper.h"

#include <algorithm>
#include <cmath>

namespace oblivion {

namespace {

enum Slot { S_ATTACK, S_INTERACT, S_HP, S_MP, S_TOGGLE, S_INV, S_MENU, S_CANCEL, S_LB, S_RB, S_CONFIRM, S_COUNT };

// Radial dead zone, rescaled so the stick starts at 0 just outside it.
void Stick(float x, float y, float* ox, float* oy) {
    const float m = std::sqrt(x * x + y * y);
    if (m < InputMapper::kStickDeadzone) {
        *ox = *oy = 0;
        return;
    }
    const float scaled = std::min(1.0f, (m - InputMapper::kStickDeadzone) / (1.0f - InputMapper::kStickDeadzone));
    *ox = x / m * scaled;
    *oy = y / m * scaled;
}

bool IsPromptState(int st) { return st == 11 || st == 14 || st == 16 || st == 19; }

}  // namespace

bool InputMapper::Rising(int slot, bool now) {
    const bool r = now && !prev_[slot];
    prev_[slot] = now;
    return r;
}

MappedInput InputMapper::Update(const RawInput& in, const MapperContext& ctx, int dt) {
    MappedInput out;
    const bool pad = in.padConnected;
    auto padHeld = [&](uint16_t b) { return pad && (in.padButtons & b) != 0; };

    // Sticks (screen y is down, XInput y is up).
    float lx = 0, ly = 0, rx = 0, ry = 0;
    if (pad) {
        Stick(in.padLX, -in.padLY, &lx, &ly);
        Stick(in.padRX, -in.padRY, &rx, &ry);
    }

    // Mouse activity: the cursor only steers attacks while it is in use.
    if (in.mouseX != lastMouseX_ || in.mouseY != lastMouseY_ || in.mouseLeft || in.mouseRight) mouseIdleMs_ = 0;
    else mouseIdleMs_ += dt;
    lastMouseX_ = in.mouseX;
    lastMouseY_ = in.mouseY;
    const bool mouseUsable = in.mouseInWindow && mouseIdleMs_ < 2500;

    const bool attackNow = in.attack || in.mouseLeft || padHeld(kPadA) || (pad && in.padRT > 0.4f);
    const bool interactNow = in.interact || in.mouseRight || padHeld(kPadX);
    const bool hpNow = in.potionHealth || padHeld(kPadLeftShoulder);
    const bool mpNow = in.potionMagicka || padHeld(kPadRightShoulder);
    const bool toggleNow = in.toggleSpell || padHeld(kPadY);
    const bool invNow = in.inventory || padHeld(kPadBack);
    const bool menuNow = in.menu || padHeld(kPadStart);
    const bool cancelNow = in.cancel || padHeld(kPadB);

    if (ctx.analogMode) {
        // ---- playing: free movement ----
        float kx = (in.moveRight ? 1.f : 0.f) - (in.moveLeft ? 1.f : 0.f);
        float ky = (in.moveDown ? 1.f : 0.f) - (in.moveUp ? 1.f : 0.f);
        if (padHeld(kPadDpadRight)) kx += 1;
        if (padHeld(kPadDpadLeft)) kx -= 1;
        if (padHeld(kPadDpadDown)) ky += 1;
        if (padHeld(kPadDpadUp)) ky -= 1;
        kx = std::max(-1.f, std::min(1.f, kx));
        ky = std::max(-1.f, std::min(1.f, ky));
        float mx = kx + lx, my = ky + ly;
        const float m = std::sqrt(mx * mx + my * my);
        if (m > 1.0f) {
            mx /= m;
            my /= m;
        }
        out.analog.moveX = mx;
        out.analog.moveY = my;

        // Aim: right stick when pushed, else the cursor while attacking with it.
        if (std::sqrt(rx * rx + ry * ry) > 0.3f) {
            out.analog.aim = true;
            out.analog.aimX = rx;
            out.analog.aimY = ry;
        } else if (mouseUsable && ctx.playerOnScreen && (attackNow || in.mouseLeft)) {
            const float dx = static_cast<float>(in.mouseX - ctx.playerX), dy = static_cast<float>(in.mouseY - ctx.playerY);
            if (dx * dx + dy * dy > 9.0f) {
                out.analog.aim = true;
                out.analog.aimX = dx;
                out.analog.aimY = dy;
            }
        }
        out.analog.attack = attackNow;
        const bool interactEdge = Rising(S_INTERACT, interactNow);
        out.analog.interact = interactEdge;
        // Script key hooks, WAIT_KEY and dialogue see the fire key on every press.
        const bool attackEdge = Rising(S_ATTACK, attackNow);
        if (attackEdge || interactEdge) out.events.push_back({InputEvent::Type::KeyDown, Key::Fire, 0});
        if (Rising(S_HP, hpNow)) out.events.push_back({InputEvent::Type::Quick, Key::None, 0});
        if (Rising(S_MP, mpNow)) out.events.push_back({InputEvent::Type::Quick, Key::None, 1});
        if (Rising(S_TOGGLE, toggleNow)) out.events.push_back({InputEvent::Type::Quick, Key::None, 2});
        if (Rising(S_INV, invNow)) out.events.push_back({InputEvent::Type::KeyDown, Key::SoftRight, 0});
        const bool menuEdge = Rising(S_MENU, menuNow);
        const bool cancelEdge = Rising(S_CANCEL, cancelNow);
        if (menuEdge || cancelEdge) out.events.push_back({InputEvent::Type::KeyDown, Key::SoftLeft, 0});
        // Keep the menu-mode edge trackers in step so nothing fires when the mode flips.
        for (auto& d : dir_) d = Repeat{};
        prev_[S_CONFIRM] = attackNow || interactNow;
        prev_[S_LB] = padHeld(kPadLeftShoulder);
        prev_[S_RB] = padHeld(kPadRightShoulder);
        return out;
    }

    // ---- menus, dialogue, cutscenes: key events ----
    // One direction at a time: the dominant of keys / d-pad / left stick.
    bool want[4] = {in.moveUp || padHeld(kPadDpadUp), in.moveDown || padHeld(kPadDpadDown),
                    in.moveLeft || padHeld(kPadDpadLeft), in.moveRight || padHeld(kPadDpadRight)};
    if (!(want[0] || want[1] || want[2] || want[3]) && std::sqrt(lx * lx + ly * ly) > 0.5f) {
        if (std::abs(lx) > std::abs(ly)) want[lx > 0 ? 3 : 2] = true;
        else want[ly > 0 ? 1 : 0] = true;
    }
    if (want[0] && want[1]) want[0] = want[1] = false;
    if (want[2] && want[3]) want[2] = want[3] = false;
    static const Key kDirKey[4] = {Key::Up, Key::Down, Key::Left, Key::Right};
    for (int i = 0; i < 4; i++) {
        Repeat& r = dir_[i];
        if (want[i]) {
            if (!r.down) {
                r.down = true;
                r.timerMs = 0;
                out.events.push_back({InputEvent::Type::KeyDown, kDirKey[i], 0});
            } else {
                r.timerMs += dt;
                // 400 ms before repeating, then every 130 ms.
                while (r.timerMs >= 400) {
                    r.timerMs -= 130;
                    out.events.push_back({InputEvent::Type::KeyDown, kDirKey[i], 0});
                }
            }
            out.held = kDirKey[i];
        } else {
            r = Repeat{};
        }
    }
    if (Rising(S_LB, padHeld(kPadLeftShoulder))) out.events.push_back({InputEvent::Type::KeyDown, Key::Left, 0});
    if (Rising(S_RB, padHeld(kPadRightShoulder))) out.events.push_back({InputEvent::Type::KeyDown, Key::Right, 0});

    if (Rising(S_CONFIRM, attackNow || interactNow)) {
        out.events.push_back({InputEvent::Type::KeyDown, Key::Fire, 0});
        if (IsPromptState(ctx.state)) out.events.push_back({InputEvent::Type::KeyDown, Key::SoftRight, 0});
    }
    // Back: Esc / Start, B / Backspace; the inventory button closes the inventory.
    // Each button keeps its own edge tracker across the playing <-> menu switch, so
    // a button that opened the screen (and is still held) cannot also close it.
    const bool menuEdge = Rising(S_MENU, menuNow);
    const bool cancelEdge = Rising(S_CANCEL, cancelNow);
    const bool invEdge = Rising(S_INV, invNow);
    if (menuEdge || cancelEdge || (ctx.state == 2 && invEdge))
        out.events.push_back({InputEvent::Type::KeyDown, Key::SoftLeft, 0});
    prev_[S_ATTACK] = attackNow;
    prev_[S_INTERACT] = interactNow;
    prev_[S_HP] = hpNow;
    prev_[S_MP] = mpNow;
    prev_[S_TOGGLE] = toggleNow;
    return out;
}

}  // namespace oblivion
