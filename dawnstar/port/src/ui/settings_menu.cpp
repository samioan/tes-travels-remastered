#include "ui/settings_menu.h"

#include <string>
#include <vector>

namespace dawnstar {

SettingsMenu::SettingsMenu(DisplayControl* display) : display_(display), screen_(ScreenMode::HighlightedList) {
    Refresh();
}

void SettingsMenu::Refresh() {
    const int keep = screen_.SelectedIndexOrMinusOne();
    std::vector<std::string> rows;
    if (display_) {
        rows = {"Resolution: " + display_->ResolutionName(), "Widescreen: " + display_->WidescreenName(),
                std::string("Display: ") + (display_->Fullscreen() ? "Fullscreen" : "Windowed"),
                "Scaling: " + display_->ScalingName()};
    }
    rows.push_back("Back");
    screen_ = Screen(ScreenMode::HighlightedList);
    screen_.SetupList("Settings", rows, true);  // Cancel also goes back
    if (keep >= 0 && keep < static_cast<int>(rows.size())) screen_.SetSelectedIndex(keep);
}

bool SettingsMenu::OnSelect() {
    const int row = screen_.SelectedIndexOrMinusOne();
    if (!display_) return true;
    if (row == 0) display_->CycleResolution(1);
    else if (row == 1) display_->CycleWidescreen(1);
    else if (row == 2) display_->ToggleFullscreen();
    else if (row == 3) display_->CycleScaling();
    else return true;  // Back
    Refresh();
    return false;
}

}  // namespace dawnstar
