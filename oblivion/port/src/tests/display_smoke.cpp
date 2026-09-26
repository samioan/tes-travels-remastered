// Display settings check: the aspect -> canvas width table, the backbuffer's
// centred views, the camera on a widescreen canvas, screens staying in their
// native 176-column layout, and the Settings menu driving a DisplayControl.
#include <cstdio>
#include <string>

#include "assets/asset_root.h"
#include "assets/image.h"
#include "game/game_app.h"
#include "platform/win32/display.h"

using namespace oblivion;

namespace {
int failures = 0;
void Check(bool ok, const std::string& what) {
    std::printf("  %-64s %s\n", what.c_str(), ok ? "ok" : "FAILED");
    failures += !ok;
}

// A DisplayControl that records what the Settings menu did.
struct FakeDisplay : DisplayControl {
    int aspect = 0, cycles = 0, toggles = 0, scalings = 0;
    bool fullscreen = false;
    std::string ResolutionName() const override { return "R" + std::to_string(aspect); }
    void CycleResolution(int dir) override {
        aspect += dir;
        cycles++;
    }
    bool Fullscreen() const override { return fullscreen; }
    void ToggleFullscreen() override {
        fullscreen = !fullscreen;
        toggles++;
    }
    std::string ScalingName() const override { return "Fit"; }
    void CycleScaling() override { scalings++; }
};

bool ColumnBlank(const Backbuffer& bb, int x0, int x1) {
    for (int y = 0; y < Backbuffer::kHeight; y++)
        for (int x = x0; x < x1; x++)
            if (bb.Data()[y * bb.RealWidth() + x] != 0) return false;
    return true;
}
}  // namespace

int main(int argc, char** argv) {
    std::printf("aspect ratios\n");
    Check(Display::WidthFor(Aspect::Original, 1920, 1080) == 176, "Original stays 176 columns");
    Check(Display::WidthFor(Aspect::R4_3, 0, 0) == 277, "4:3 is 277 columns at 208 rows");
    Check(Display::WidthFor(Aspect::R16_10, 0, 0) == 333, "16:10 is 333 columns");
    Check(Display::WidthFor(Aspect::R16_9, 0, 0) == 370, "16:9 is 370 columns");
    Check(Display::WidthFor(Aspect::R21_9, 0, 0) == 485, "21:9 is 485 columns");
    Check(Display::WidthFor(Aspect::Auto, 1920, 1080) == 370, "Auto follows a 1920x1080 window");
    Check(Display::WidthFor(Aspect::Auto, 528, 624) == 176, "Auto never goes narrower than the original");
    Check(Display::WidthFor(Aspect::Auto, 5000, 1000) == Backbuffer::kMaxWidth, "Auto is capped (24:9)");

    std::printf("backbuffer views\n");
    {
        Backbuffer bb(370);
        Check(bb.RealWidth() == 370 && bb.Width() == 370, "a widescreen buffer is 370 wide");
        bb.CenterView(176);
        Check(bb.Width() == 176 && bb.ViewX() == 97, "a centred 176-column view starts at column 97");
        bb.FillRect(0, 0, 10, 10, 0xFFFFFF);
        bb.SetPixel(-1, 5, 0xFFFFFF);   // outside the view: dropped, not spilled into the bars
        bb.SetPixel(176, 5, 0xFFFFFF);
        bb.FillRect(170, 20, 30, 5, 0xFFFFFF);  // overhangs the right edge of the view
        bb.ResetView();
        const uint32_t* p = bb.Data();
        Check(p[0 * 370 + 97] == 0xFFFFFF && p[9 * 370 + 106] == 0xFFFFFF, "drawing is offset into the view");
        Check(p[5 * 370 + 96] == 0 && p[5 * 370 + 273] == 0, "pixels outside the view are dropped");
        Check(p[20 * 370 + 272] == 0xFFFFFF && p[20 * 370 + 273] == 0, "rectangles are clipped to the view");
        bb.Resize(100);
        Check(bb.RealWidth() == 176, "a buffer is never narrower than the original canvas");
    }

    std::printf("widescreen game\n");
    const std::string dir = argc > 1 ? argv[1] : "../../extracted";
    AssetRoot assets(dir);
    ImageCache images(assets);
    FakeDisplay fake;
    GameApp app(assets, images);
    app.SetDisplayControl(&fake);
    World& world = app.world();
    Backbuffer wide(370), narrow;
    auto step = [&](Backbuffer& bb, int ms) {
        const int st = world.state();
        app.SetHeldKey(st == 10 || st == 9 || st == 4 ? Key::Down : Key::None);
        app.SetScreenWidth(bb.RealWidth());
        app.Tick(ms);
        app.Draw(bb);
    };
    app.SetScreenWidth(370);
    app.Start();
    for (int ms = 0; world.state() != 3 && ms < 60000; ms += 16) {
        if (ms % 1100 < 16) app.OnKeyDown(Key::Fire);
        step(wide, 16);
    }

    // The main menu is a 176-column screen: centred, the bars beside it black.
    step(wide, 16);
    Check(ColumnBlank(wide, 0, 97) && ColumnBlank(wide, 273, 370), "menu: the bars beside the 176-column screen are black");
    Check(!ColumnBlank(wide, 97, 273), "menu: the screen itself is drawn");

    // Settings menu: New Game, Settings, Help, About, Exit (no save file in this run).
    app.OnKeyDown(Key::Right);
    app.OnKeyDown(Key::Fire);  // Settings
    app.OnKeyDown(Key::Fire);  // row 0: Resolution
    Check(fake.cycles == 1 && fake.aspect == 1, "Settings > Resolution cycles the aspect");
    app.OnKeyDown(Key::Right);
    app.OnKeyDown(Key::Fire);
    Check(fake.toggles == 1 && fake.fullscreen, "Settings > Display toggles fullscreen");
    app.OnKeyDown(Key::Right);
    app.OnKeyDown(Key::Fire);
    Check(fake.scalings == 1, "Settings > Scaling cycles the scaling");
    app.OnKeyDown(Key::SoftLeft);  // back to the main menu
    step(wide, 16);
    app.OnKeyDown(Key::Fire);  // the item under the cursor is Settings again (its selection is remembered)
    step(wide, 16);
    const int cyclesBefore = fake.cycles;
    app.OnKeyDown(Key::Fire);
    Check(fake.cycles == cyclesBefore + 1, "Settings can be re-entered after Back");

    // In a level: the camera centres on the *wide* screen and the HUD hugs its corners.
    world.LoadLevel("/l04_4b.scr");
    for (int ms = 0; ms < 30000; ms += 16) {
        if (ms % 1100 < 16) app.OnKeyDown(Key::Fire);
        step(wide, 16);
        if (world.state() == 0 && ms > 3000) break;
    }
    Actor* p = world.player();
    Check(p && world.state() == 0, "playing at 370 columns");
    if (p) {
        int px = 0, py = 0;
        world.PlayerScreenPos(&px, &py);
        Check(px > 150 && px < 220, "the player is centred on the 370-column screen (x=" + std::to_string(px) + ")");
        step(wide, 16);
        Check(!ColumnBlank(wide, 0, 40) && !ColumnBlank(wide, 330, 370), "the world fills the sides too");
        // Same level at 176: the camera recentres on the narrow screen.
        step(narrow, 16);
        step(narrow, 16);
        world.PlayerScreenPos(&px, &py);
        Check(px > 60 && px < 115, "and on the 176-column screen after switching back (x=" + std::to_string(px) + ")");
    }
    std::printf("%s\n", failures ? "FAILED" : "OK");
    return failures ? 1 : 0;
}
