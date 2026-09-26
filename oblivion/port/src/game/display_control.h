#pragma once
#include <string>

namespace oblivion {

// The PC-only display options the Settings menu edits. The platform layer
// (platform/win32/display.h) implements it; GameApp only sees this interface,
// so headless runs and tests simply have no Settings item.
class DisplayControl {
public:
    virtual ~DisplayControl() = default;
    // Resolution = the canvas aspect ratio: Original 176x208, Auto (follow the window), 4:3 ... 21:9.
    virtual std::string ResolutionName() const = 0;
    virtual void CycleResolution(int dir) = 0;
    virtual bool Fullscreen() const = 0;  // borderless fullscreen
    virtual void ToggleFullscreen() = 0;
    virtual std::string ScalingName() const = 0;  // Fit / Integer
    virtual void CycleScaling() = 0;
};

}  // namespace oblivion
