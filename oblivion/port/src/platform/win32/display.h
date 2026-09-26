#pragma once
#include <string>

#include "game/display_control.h"
#include "platform/win32/window.h"

namespace oblivion {

// Resolution = the canvas aspect ratio. The canvas keeps the original 208-row
// height and widens to the chosen ratio, so sprites and HUD keep their native
// pixel size and the world simply shows more at the sides. Auto follows the
// window's own shape (fullscreen on a 16:9 monitor becomes 16:9).
enum class Aspect { Original, Auto, R4_3, R16_10, R16_9, R21_9, Count };

struct DisplaySettings {
    bool fullscreen = false;
    Scaling scaling = Scaling::Fit;
    Aspect aspect = Aspect::Auto;
};

// The Settings menu's window: loads/saves display.cfg and applies changes to the
// Window. Modelled on the Going Mobile / Clone Home ports' display module.
class Display : public DisplayControl {
public:
    // `cfgPath` may be empty (nothing is persisted).
    explicit Display(std::string cfgPath) : cfgPath_(std::move(cfgPath)) { Load(); }

    DisplaySettings& settings() { return s_; }
    // Attaches the window and enters fullscreen if the settings say so.
    void Attach(Window* window);
    // The canvas width for the current window shape (176 .. 554).
    int LogicalWidth() const;
    // A first client size for a windowed window: the chosen shape at `scale`x.
    void InitialClientSize(int scale, int* w, int* h) const;

    void Load();
    void Save() const;

    // ---- DisplayControl ----
    std::string ResolutionName() const override;
    void CycleResolution(int dir) override;
    bool Fullscreen() const override { return s_.fullscreen; }
    void ToggleFullscreen() override;
    std::string ScalingName() const override { return s_.scaling == Scaling::Fit ? "Fit" : "Integer"; }
    void CycleScaling() override;

    // The canvas width for an aspect at a window shape (pure: for tests).
    static int WidthFor(Aspect aspect, int clientW, int clientH);

private:
    DisplaySettings s_;
    std::string cfgPath_;
    Window* window_ = nullptr;
};

}  // namespace oblivion
