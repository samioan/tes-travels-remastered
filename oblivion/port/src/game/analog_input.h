#pragma once

namespace oblivion {

// The continuous part of the modern controls, sampled once per frame by the
// platform layer (see game/input_mapper.h) and consumed by GameApp::Tick.
struct AnalogInput {
    // Walking: screen space (x right, y down), length 0..1 (a half-pushed stick walks slower).
    float moveX = 0, moveY = 0;
    // Aiming (mouse cursor / right stick), screen space; `aim` false = face where you walk.
    bool aim = false;
    float aimX = 0, aimY = 0;
    bool attack = false;    // held
    bool interact = false;  // true for the one frame the button went down
};

}  // namespace oblivion
