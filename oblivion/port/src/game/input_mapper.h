#pragma once
#include <cstdint>
#include <vector>

#include "game/analog_input.h"
#include "game/game_app.h"

namespace oblivion {

// Gamepad buttons (the XInput layout; the platform layer fills the mask).
enum PadButton : uint16_t {
    kPadDpadUp = 0x0001, kPadDpadDown = 0x0002, kPadDpadLeft = 0x0004, kPadDpadRight = 0x0008,
    kPadStart = 0x0010, kPadBack = 0x0020, kPadLeftShoulder = 0x0100, kPadRightShoulder = 0x0200,
    kPadA = 0x1000, kPadB = 0x2000, kPadX = 0x4000, kPadY = 0x8000,
};

// One frame of raw device state. Keyboard fields are logical controls (the
// platform layer maps WASD + arrows to move*, Space/J to attack, E/Enter to
// interact, 1/2 to the potions, Tab/R to toggle, I to inventory, Esc to menu).
struct RawInput {
    bool moveUp = false, moveDown = false, moveLeft = false, moveRight = false;
    bool attack = false, interact = false;
    bool potionHealth = false, potionMagicka = false, toggleSpell = false;
    bool inventory = false, menu = false, cancel = false;

    bool padConnected = false;
    float padLX = 0, padLY = 0, padRX = 0, padRY = 0;  // -1..1, +y up (XInput)
    float padLT = 0, padRT = 0;                        // 0..1
    uint16_t padButtons = 0;

    bool mouseInWindow = false;
    int mouseX = 0, mouseY = 0;  // virtual screen coordinates
    bool mouseLeft = false, mouseRight = false;
};

struct MapperContext {
    int state = 0;            // Game state (0 playing, 1 shop, ...)
    bool analogMode = false;  // free movement: playing, no dialogue, input enabled
    bool playerOnScreen = false;
    int playerX = 0, playerY = 0;  // where the mouse aims from
};

struct InputEvent {
    enum class Type { KeyDown, Quick } type = Type::KeyDown;
    Key key = Key::None;  // KeyDown
    int quick = 0;        // Quick: 0 health potion, 1 magicka potion, 2 toggle weapon/spell
};

struct MappedInput {
    AnalogInput analog;
    Key held = Key::None;  // held direction in menu mode (text screens scroll while it is down)
    std::vector<InputEvent> events;
};

// Turns raw keyboard / mouse / gamepad state into what the game consumes:
// analogue walking + aiming while playing; d-pad style key events with
// auto-repeat in menus, dialogue and cutscenes.
class InputMapper {
public:
    MappedInput Update(const RawInput& in, const MapperContext& ctx, int dtMs);

    static constexpr float kStickDeadzone = 0.25f;

private:
    struct Repeat {
        bool down = false;
        int timerMs = 0;
    };
    bool Rising(int slot, bool now);

    bool prev_[24] = {};
    Repeat dir_[4];
    int lastMouseX_ = -1, lastMouseY_ = -1;
    int mouseIdleMs_ = 100000;
};

}  // namespace oblivion
