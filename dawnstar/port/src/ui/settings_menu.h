#pragma once
#include "graphics/backbuffer.h"
#include "ui/display_control.h"
#include "ui/screen.h"

namespace dawnstar {

// The PC-only Settings screen reachable from the main menu and the in-game
// Options menu (not part of the original game): one list row per display
// option -- Resolution, Display (fullscreen / windowed), Scaling -- and Back.
// A highlighted-list Screen like every other menu here, so it looks and
// navigates like the rest; Select changes the highlighted row in place.
class SettingsMenu {
public:
    explicit SettingsMenu(DisplayControl* display);

    void OnUp() { screen_.MoveSelectionUp(); }
    void OnDown() { screen_.MoveSelectionDown(); }
    // Applies the highlighted row; true when it was Back (or nothing to edit),
    // i.e. the host should return to the menu it came from.
    bool OnSelect();
    // Re-reads the option values (a hotkey may have changed one) keeping the highlight.
    void Refresh();
    void Render(Backbuffer& bb) const { screen_.Paint(bb); }
    const Screen& screen() const { return screen_; }
    Screen& screen() { return screen_; }

private:
    DisplayControl* display_;
    Screen screen_;
};

}  // namespace dawnstar
