#pragma once
#include <string>

#include "platform/win32/window.h"
#include "ui/display_control.h"

namespace dawnstar {

// The widescreen canvas: Off keeps the native 176 columns; Auto follows the
// window's shape; the others fix the ratio.
enum class Aspect { Off, Auto, R4_3, R16_10, R16_9, R21_9, Count };

struct DisplaySettings {
    Aspect aspect = Aspect::Off;
    bool fullscreen = false;
    Scaling scaling = Scaling::Fit;
    int scale = 3;  // windowed size: a whole multiple of the native 176x208
};

// The Settings screen's window: applies changes to the Window and remembers
// them in display.cfg. The launcher passes its own Fullscreen / window-size
// choice at every start, which takes precedence for those two; the scaling
// mode is remembered here.
class Display : public DisplayControl {
public:
    static constexpr int kMinScale = 2, kMaxScale = 6;

    // `cfgPath` may be empty (nothing is persisted).
    explicit Display(std::string cfgPath) : cfgPath_(std::move(cfgPath)) { Load(); }

    DisplaySettings& settings() { return s_; }

    // The canvas width to draw at, for a client area of clientW x clientH
    // (0x0 = unknown). 176 when widescreen is off.
    static int WidthFor(Aspect aspect, int clientW, int clientH);
    int LogicalWidth() const;
    // Attaches the window and applies the settings (fullscreen, size).
    void Attach(Window* window);

    void Load();
    void Save() const;

    // ---- DisplayControl ----
    std::string ResolutionName() const override { return std::to_string(s_.scale) + "x"; }
    void CycleResolution(int dir) override;
    std::string WidescreenName() const override;
    void CycleWidescreen(int dir) override;
    bool Fullscreen() const override { return s_.fullscreen; }
    void ToggleFullscreen() override;
    std::string ScalingName() const override { return s_.scaling == Scaling::Fit ? "Fit" : "Integer"; }
    void CycleScaling() override;

private:
    // The client size the window should have for the current size choice.
    void ApplySize();

    DisplaySettings s_;
    std::string cfgPath_;
    Window* window_ = nullptr;
};

}  // namespace dawnstar
