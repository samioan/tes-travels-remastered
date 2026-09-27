// Display settings check: the PC-only Settings screen in the main menu and in
// the in-game Options menu, driven against a fake DisplayControl. Needs no game
// data. Also checks the window-size table's bounds.
#include <cstdio>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/help_text.h"
#include "assets/item_database.h"
#include "assets/shop_dialogue.h"
#include "assets/spell_database.h"
#include "dungeon/dungeon_runtime.h"
#include "graphics/backbuffer.h"
#include "platform/win32/display.h"
#include "player/player_state.h"
#include "ui/menu_flow.h"
#include "ui/options_menu.h"
#include "ui/settings_menu.h"
#include "world/dungeon_generator.h"

using namespace dawnstar;

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

bool Painted(MenuFlow& m) {
    Backbuffer bb;
    bb.Fill(0);
    m.Render(bb);
    for (int y = 0; y < Backbuffer::kHeight; y++)
        for (int x = 0; x < Backbuffer::kWidth; x++)
            if (bb.Data()[y * Backbuffer::kWidth + x] != 0) return true;
    return false;
}
}  // namespace

int main() {
    HelpText help;
    help.titles = {"Topic"};
    help.bodies = {"Body"};

    std::printf("main menu\n");
    {
        FakeDisplay fake;
        MenuFlow menu(help);
        // Without EnableSettings the original five items are untouched: index 4 is Exit.
        for (int i = 0; i < 4; i++) menu.OnDown();
        Check(menu.OnSelect() == MenuFlowAction::None, "original menu: item 5 (Exit) opens the quit confirmation");

        MenuFlow m2(help);
        m2.EnableSettings(&fake);
        for (int i = 0; i < 4; i++) m2.OnDown();  // New, Continue, Help, Credits, [Settings]
        Check(m2.OnSelect() == MenuFlowAction::None, "Settings item opens the Settings screen");
        Check(Painted(m2), "the Settings screen paints");
        m2.OnSelect();  // row 0: Resolution
        Check(fake.cycles == 1 && fake.scale == 4, "Resolution row cycles the window size");
        m2.OnDown();
        m2.OnSelect();
        Check(fake.wides == 1, "Widescreen row cycles the canvas shape");
        m2.OnDown();
        m2.OnSelect();
        Check(fake.toggles == 1 && fake.fullscreen, "Display row toggles fullscreen");
        m2.OnDown();
        m2.OnSelect();
        Check(fake.scalings == 1, "Scaling row cycles the scaling");
        m2.OnDown();
        m2.OnSelect();  // Back
        for (int i = 0; i < 4; i++) m2.OnUp();  // back at the top? (wraps) -- then New Game
        // Back returned to the main menu: Exit is now the 6th item.
        MenuFlow m3(help);
        m3.EnableSettings(&fake);
        for (int i = 0; i < 5; i++) m3.OnDown();
        m3.OnSelect();  // Exit -> confirmation (not Settings)
        Check(m3.OnSelect() == MenuFlowAction::Exit, "Exit moved to the 6th item and still quits");

        MenuFlow m4(help);
        m4.EnableSettings(&fake);
        for (int i = 0; i < 4; i++) m4.OnDown();
        m4.OnSelect();  // Settings
        m4.OnCancel();  // Cancel goes back
        m4.OnDown();
        Check(m4.OnSelect() == MenuFlowAction::None && Painted(m4), "Cancel leaves Settings for the main menu");
        m4.RefreshSettings();
    }

    std::printf("in-game options menu\n");
    {
        FakeDisplay fake;
        ShopDialogue shop;
        OptionsMenu menu(help, shop);
        menu.EnableSettings(&fake);
        PlayerState player;
        CharacterData charData;
        ItemDatabase items;
        SpellDatabase spells;
        std::vector<GeneratedLevel> levels;
        WorldRegistry world(0);
        int16_t nextId = 0;
        for (int i = 0; i < 9; i++) menu.OnDown();  // Settings sits before Quit Game
        menu.OnSelect(player, charData, items, spells, levels, world, nextId);
        menu.OnSelect(player, charData, items, spells, levels, world, nextId);  // Resolution
        Check(fake.cycles == 1, "Options > Settings > Resolution works in game");
        menu.OnDown();
        menu.OnSelect(player, charData, items, spells, levels, world, nextId);
        Check(fake.wides == 1, "Options > Settings > Widescreen works in game");
        menu.OnDown();
        menu.OnSelect(player, charData, items, spells, levels, world, nextId);
        Check(fake.toggles == 1, "Options > Settings > Display toggles fullscreen in game");
        menu.OnCancel();
        Backbuffer bb;
        menu.Render(bb);  // back on the Options list without crashing
        Check(menu.OnCancel() == OptionsMenuAction::ReturnToGame, "Cancel from Settings returns to Options; Cancel again returns to the game");
    }

    std::printf("widescreen canvas\n");
    Check(Display::WidthFor(Aspect::Off, 1920, 1080) == 176, "Off keeps the native 176 columns");
    Check(Display::WidthFor(Aspect::R16_9, 0, 0) == 370, "16:9 is 370 columns");
    Check(Display::WidthFor(Aspect::R4_3, 0, 0) == 278, "4:3 is 278 columns (277 rounded to even)");
    Check(Display::WidthFor(Aspect::R21_9, 0, 0) == 486, "21:9 is 486 columns (485 rounded to even)");
    Check(Display::WidthFor(Aspect::Auto, 1920, 1080) == 370, "Auto follows the window shape");
    Check(Display::WidthFor(Aspect::Auto, 300, 600) == 176, "... but never narrower than native");
    Check(Display::WidthFor(Aspect::Auto, 5000, 1000) == Backbuffer::kMaxWidth, "... nor wider than the buffer allows");
    for (int cw = 700; cw < 720; cw++)
        if (Display::WidthFor(Aspect::Auto, cw, 500) % 2 != 0) Check(false, "every canvas width is even");
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
