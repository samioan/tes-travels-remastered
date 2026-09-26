#pragma once
#include <string>
#include <vector>

namespace stormhold {

// The PC-only display options the Settings screen edits. The platform layer
// (platform/win32/display.h) implements it; the menus only see this
// interface, so smoke tests can drive them with a fake.
//
// The game is a fixed 176x208 picture (a first-person view drawn from
// pre-rendered sprites), so "resolution" here is the size of the game window
// -- a whole multiple of the native picture -- and widescreen displays get
// black bars rather than a stretched image. Fullscreen is borderless.
class DisplayControl {
public:
    virtual ~DisplayControl() = default;
    virtual std::string ResolutionName() const = 0;  // "3x"
    virtual void CycleResolution(int dir) = 0;
    virtual bool Fullscreen() const = 0;
    virtual void ToggleFullscreen() = 0;
    virtual std::string ScalingName() const = 0;  // Fit / Integer
    virtual void CycleScaling() = 0;
};

// The rows of the Settings screen ("Name: value", then Back) and what Select does
// on one. Shared by every menu that hosts the screen.
inline std::vector<std::string> SettingsRows(const DisplayControl* d) {
    std::vector<std::string> rows;
    if (d) {
        rows.push_back("Resolution: " + d->ResolutionName());
        rows.push_back(std::string("Display: ") + (d->Fullscreen() ? "Fullscreen" : "Windowed"));
        rows.push_back("Scaling: " + d->ScalingName());
    }
    rows.push_back("Back");
    return rows;
}

// Applies row `row`; true when it was Back (or there is nothing to edit).
inline bool ApplySettingsRow(DisplayControl* d, int row) {
    if (!d) return true;
    if (row == 0) d->CycleResolution(1);
    else if (row == 1) d->ToggleFullscreen();
    else if (row == 2) d->CycleScaling();
    else return true;
    return false;
}

}  // namespace stormhold
