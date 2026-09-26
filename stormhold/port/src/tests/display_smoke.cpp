// Display settings check: the PC-only Settings screen in the main menu and in
// the in-game pause (Options) menu, driven against a fake DisplayControl.
// Needs no game data. Also checks the window-size table.
#include <cstdio>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/shop_dialogue.h"
#include "assets/spell_database.h"
#include "dungeon/dungeon_runtime.h"
#include "graphics/backbuffer.h"
#include "platform/win32/display.h"
#include "player/player_state.h"
#include "ui/inventory_ui.h"
#include "ui/menu_flow.h"
#include "ui/pause_menu.h"
#include "world/shop_state.h"
#include "world/warden.h"

using namespace stormhold;

namespace {
int failures = 0;
void Check(bool ok, const std::string& what) {
    std::printf("  %-66s %s\n", what.c_str(), ok ? "ok" : "FAILED");
    failures += !ok;
}

struct FakeDisplay : DisplayControl {
    int scale = 3, cycles = 0, toggles = 0, scalings = 0, wides = 0;
    bool fullscreen = false;
    std::string ResolutionName() const override { return std::to_string(scale) + "x"; }
    void CycleResolution(int dir) override {
        scale += dir;
        cycles++;
    }
    std::string WidescreenName() const override { return "Auto"; }
    void CycleWidescreen(int) override { wides++; }
    bool Fullscreen() const override { return fullscreen; }
    void ToggleFullscreen() override {
        fullscreen = !fullscreen;
        toggles++;
    }
    std::string ScalingName() const override { return "Fit"; }
    void CycleScaling() override { scalings++; }
};

void Down(MenuFlowState& s, int n, const CharacterData& cd) {
    for (int i = 0; i < n; i++) MenuFlow::MoveSelection(s, 1, cd);
}
}  // namespace

int main() {
    CharacterData charData;
    ItemDatabase items;

    std::printf("main menu\n");
    {
        MenuFlowState original;  // no display: the original five items
        Down(original, 4, charData);
        MenuFlow::Confirm(original, charData, items);
        Check(original.exitRequested, "original menu: item 5 is still Exit");

        FakeDisplay fake;
        MenuFlowState s;
        s.display = &fake;
        Down(s, 4, charData);
        MenuFlow::Confirm(s, charData, items);
        Check(s.screen == MenuScreen::Settings, "Settings item opens the Settings screen");
        Backbuffer bb;
        ShopDialogue dialogue;
        MenuFlow::Render(bb, s, charData, dialogue);
        MenuFlow::Confirm(s, charData, items);  // Resolution
        Check(fake.cycles == 1 && fake.scale == 4, "Resolution row cycles the window size");
        MenuFlow::MoveSelection(s, 1, charData);
        MenuFlow::Confirm(s, charData, items);
        Check(fake.wides == 1, "Widescreen row cycles the canvas shape");
        MenuFlow::MoveSelection(s, 1, charData);
        MenuFlow::Confirm(s, charData, items);
        Check(fake.toggles == 1 && fake.fullscreen, "Display row toggles fullscreen");
        MenuFlow::MoveSelection(s, 1, charData);
        MenuFlow::Confirm(s, charData, items);
        Check(fake.scalings == 1, "Scaling row cycles the scaling");
        MenuFlow::MoveSelection(s, 1, charData);
        MenuFlow::Confirm(s, charData, items);  // Back
        Check(s.screen == MenuScreen::MainMenu && s.selectedIndex == 4, "Back returns to the main menu on the Settings item");

        MenuFlowState e;
        e.display = &fake;
        Down(e, 5, charData);
        MenuFlow::Confirm(e, charData, items);
        Check(e.exitRequested, "Exit moved to the 6th item and still quits");

        MenuFlowState c;
        c.display = &fake;
        Down(c, 4, charData);
        MenuFlow::Confirm(c, charData, items);
        MenuFlow::Cancel(c);
        Check(c.screen == MenuScreen::MainMenu, "Cancel leaves Settings");
    }

    std::printf("in-game pause menu\n");
    {
        FakeDisplay fake;
        PlayerState p;
        SpellDatabase spells;
        InventoryUiState inventoryUi;
        WorldRegistry world(0);
        ShopState shop;
        WardenState warden;
        PauseMenuState st;
        st.display = &fake;
        PauseMenu::Open(st);
        for (int i = 0; i < 7; i++) PauseMenu::MoveSelection(st, 1, p, spells);  // Settings sits before Quit Game
        PauseMenu::Confirm(st, p, spells, inventoryUi, "", 1, world, shop, warden);
        Check(st.screen == PauseScreen::Settings, "Options > Settings opens the Settings screen");
        PauseMenu::Confirm(st, p, spells, inventoryUi, "", 1, world, shop, warden);
        Check(fake.cycles == 1, "Options > Settings > Resolution works in game");
        PauseMenu::MoveSelection(st, 1, p, spells);
        PauseMenu::Confirm(st, p, spells, inventoryUi, "", 1, world, shop, warden);
        Check(fake.wides == 1, "Options > Settings > Widescreen works in game");
        PauseMenu::MoveSelection(st, 1, p, spells);
        PauseMenu::Confirm(st, p, spells, inventoryUi, "", 1, world, shop, warden);
        Check(fake.toggles == 1, "Options > Settings > Display toggles fullscreen in game");
        Backbuffer bb;
        ShopDialogue dialogue;
        PauseMenu::Render(bb, st, p, charData, spells, dialogue);
        PauseMenu::Cancel(st);
        Check(st.active && st.screen == PauseScreen::Options && st.selectedIndex == 7, "Cancel returns to Options on the Settings item");
        PauseMenu::MoveSelection(st, 1, p, spells);  // Quit Game is now item 8 (still shows the credits, as in the original)
        PauseMenu::Confirm(st, p, spells, inventoryUi, "", 1, world, shop, warden);
        Check(st.screen == PauseScreen::Credits, "Quit Game moved to the 9th item and behaves as before");

        PauseMenuState plain;  // without a display the original eight items stay
        PauseMenu::Open(plain);
        for (int i = 0; i < 7; i++) PauseMenu::MoveSelection(plain, 1, p, spells);
        PauseMenu::Confirm(plain, p, spells, inventoryUi, "", 1, world, shop, warden);
        Check(plain.screen == PauseScreen::Credits, "original pause menu: item 8 is Quit Game");
    }

    std::printf("widescreen canvas\n");
    Check(Display::WidthFor(Aspect::Off, 1920, 1080) == 176, "Off keeps the native 176 columns");
    Check(Display::WidthFor(Aspect::R16_9, 0, 0) == 370, "16:9 is 370 columns");
    Check(Display::WidthFor(Aspect::R4_3, 0, 0) == 278, "4:3 is 278 columns (277 rounded to even)");
    Check(Display::WidthFor(Aspect::R21_9, 0, 0) == 486, "21:9 is 486 columns (485 rounded to even)");
    Check(Display::WidthFor(Aspect::Auto, 1920, 1080) == 370, "Auto follows the window shape");
    for (int cw = 700; cw < 720; cw++)
        if (Display::WidthFor(Aspect::Auto, cw, 500) % 2 != 0) Check(false, "every canvas width is even");
    Check(Display::WidthFor(Aspect::Auto, 300, 600) == 176, "... but never narrower than native");
    Check(Display::WidthFor(Aspect::Auto, 5000, 1000) == Backbuffer::kMaxWidth, "... nor wider than the buffer allows");
    {
        Display d("");
        Check(d.WidescreenName() == "Off", "widescreen defaults to Off (the authentic view)");
        d.CycleWidescreen(1);
        Check(d.WidescreenName() == "Auto", "cycles to Auto");
        for (int i = 0; i < 5; i++) d.CycleWidescreen(1);
        Check(d.WidescreenName() == "Off", "and wraps around");
    }

    std::printf("window sizes\n");
    Check(Display::kMinScale == 2 && Display::kMaxScale == 6, "window sizes run 2x..6x");
    {
        Display d("");
        d.settings().scale = 6;
        d.CycleResolution(1);
        Check(d.ResolutionName() == "2x", "cycling past 6x wraps to 2x");
        d.CycleResolution(-1);
        Check(d.ResolutionName() == "6x", "and back");
        d.ToggleFullscreen();
        Check(d.Fullscreen(), "fullscreen flag toggles without a window");
    }

    std::printf("%s\n", failures ? "FAILED" : "OK");
    return failures ? 1 : 0;
}
