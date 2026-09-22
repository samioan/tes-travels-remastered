// M38 smoke test: MenuFlow (ui/menu_flow.h) -- the real main-menu/help/
// credits/quit-confirm navigation flow, built on M37's own Screen. Also
// exercises (indirectly, via MenuFlow's own real behavior) the M38
// decompiler-bug fix in ../../../src/ESGame.java: commandAction1() used
// to dispatch on `uic.mode` where the real bytecode must have meant
// `uic.secondaryParam` -- see MenuFlow's own class comment for the full
// writeup, and ESGame.java's own doc comment on commandAction1 itself.
//
// No JVM ground truth is available (same reason as every prior
// milestone) -- every real target (which screen Select/Cancel lands on,
// and which text it shows) is independently re-derived from
// ../../../src/ESGame.java's own commandAction1 branches (now correctly
// read as secondaryParam, not mode), not read back from menu_flow.cpp.
// Text placement is cross-checked against BitmapFont::DrawString used as
// an oracle, the same technique M37's own test established.
#include <cstdio>
#include <string>

#include "assets/dat_archive.h"
#include "assets/help_text.h"
#include "graphics/backbuffer.h"
#include "graphics/bitmap_font.h"
#include "render/message_popup.h"
#include "ui/menu_flow.h"
#include "ui/screen.h"

namespace {

using dawnstar::Backbuffer;
namespace BitmapFont = dawnstar::BitmapFont;
using dawnstar::MenuFlow;
using dawnstar::MenuFlowAction;
using dawnstar::MessagePopup;
using dawnstar::Screen;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

uint16_t PixelAt(const Backbuffer& bb, int x, int y) {
    return bb.Data()[static_cast<size_t>(y) * Backbuffer::kWidth + x];
}

// Same oracle technique m37_screen_smoke.cpp established: render `text`
// via the already-verified BitmapFont::DrawString onto a blank scratch
// buffer, then confirm every "on" pixel it produces appears at the same
// offset from (x0,y0) in `bb`, in `color`.
bool TextRenderedAt(const Backbuffer& bb, int x0, int y0, const std::string& text, uint16_t color) {
    Backbuffer scratch;
    scratch.Fill(0);
    BitmapFont::DrawString(scratch, 0, 0, text, 0xFFFF);
    bool sawOnPixel = false;
    int spanW = static_cast<int>(text.size()) * BitmapFont::kAdvance;
    for (int dy = 0; dy < BitmapFont::kGlyphHeight; dy++) {
        for (int dx = 0; dx < spanW; dx++) {
            if (PixelAt(scratch, dx, dy) == 0) continue;
            sawOnPixel = true;
            int px = x0 + dx;
            int py = y0 + dy;
            if (px < 0 || px >= Backbuffer::kWidth || py < 0 || py >= Backbuffer::kHeight) return false;
            if (PixelAt(bb, px, py) != color) return false;
        }
    }
    return sawOnPixel;
}

constexpr uint16_t kTitleTextColor = dawnstar::PackRGB565(255, 255, 255);
constexpr uint16_t kItemTextColor = dawnstar::PackRGB565(255, 255, 0);

int CenteredTitleX(const std::string& title) { return Screen::width() / 2 - BitmapFont::StringWidth(title) / 2; }

bool TitleShownIs(const Backbuffer& bb, const std::string& title) {
    return TextRenderedAt(bb, CenteredTitleX(title), 0, title, kTitleTextColor);
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::HelpText helpText = dawnstar::HelpText::Load(archive);
        Check(helpText.titles.size() == 12, "the real helptext.dat should group into 12 topics (M8)");

        MenuFlow menu(helpText);
        Backbuffer bb;

        // --- A: initial state is the real main menu. ---
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Main Menu"), "MenuFlow should start on the real main menu");
        Check(TextRenderedAt(bb, 15, 20, "New Game", kItemTextColor),
              "the main menu's own first item should render (\"New Game\")");

        // --- B: "Continue Game" (index 1) -- M52: OnSelect() itself just
        // returns the action; main.cpp does the real GameSave load/resume
        // work (see main_smoke's own... no such test exists for main.cpp
        // itself, so this is checked as "launches and stays up" only, same
        // as every other main.cpp wiring since M28). What IS unit-testable
        // here is MenuFlow's own half: the action returned, and
        // ShowNoSavedGame()'s screen (main.cpp's own real call site on a
        // failed load). ---
        menu.OnDown();
        Check(menu.OnSelect() == MenuFlowAction::ContinueGame,
              "Continue Game should return MenuFlowAction::ContinueGame (M52)");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Main Menu"),
              "MenuFlow's own screen state doesn't change on ContinueGame -- only main.cpp acts on it");

        // --- B2: ShowNoSavedGame() -- main.cpp's own call site for a
        // failed load, reusing the shared info screen (like Credits/a Help
        // topic body do) with backTarget left at MainMenu (unlike ui/
        // options_menu.h's own separate copy of this same message, whose
        // backTarget is OptionsUI instead). ---
        menu.ShowNoSavedGame();
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Unavailable"), "ShowNoSavedGame should show the real \"Unavailable\" title");
        Check(menu.OnSelect() == MenuFlowAction::None, "Ok on the no-saved-game screen is not a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Main Menu"), "Ok on the no-saved-game screen should return to the main menu");

        // --- C: "Help" (index 2) -> the real topic list, built from the
        // real M8 HelpText titles. ---
        menu.OnDown();
        Check(menu.OnSelect() == MenuFlowAction::None, "selecting Help should not itself be a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Help"), "selecting Help should show the real Help topic list");
        Check(TextRenderedAt(bb, 15, 20, helpText.titles[0], kItemTextColor),
              "the Help topic list's own first item should be the real first HelpText title");

        // --- D: selecting a topic (index 1, after one Down) shows the
        // real GenericInfoUI-equivalent info screen with that topic's
        // real title/body (secondaryParam==206's own real target). ---
        menu.OnDown();
        Check(menu.OnSelect() == MenuFlowAction::None, "selecting a Help topic should not itself be a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, helpText.titles[1]), "the info screen should show the SELECTED topic's own real title");
        std::vector<std::string> expectedBody = MessagePopup::WordWrap(helpText.bodies[1], Screen::width() - 5 - 5);
        Check(TextRenderedAt(bb, 5, 20, expectedBody[0], kItemTextColor),
              "the info screen should show the selected topic's own real (word-wrapped) body");

        // --- E: Ok on a help topic's own info screen returns to the
        // Help topic list (secondaryParam==206 always returns to helpUI,
        // NOT the main menu) -- confirmed via the now-fixed
        // secondaryParam dispatch, not the (broken) `.mode` reading. ---
        Check(menu.OnSelect() == MenuFlowAction::None, "Ok on a help topic's info screen is not a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Help"), "Ok on a help topic's body should return to the Help topic list, not Main Menu");

        // --- F: Cancel on the Help topic list returns to the main menu.
        // A real, faithfully-preserved quirk confirmed here: the real
        // mainMenuUI Screen instance is REUSED, not recreated, so its
        // own selectedIndex (last left on "Help", index 2, by step C)
        // persists across this round trip rather than resetting to 0 --
        // matching real MIDP Screen/Displayable semantics (this is
        // exactly why the next 2 sections force the selection back to
        // index 0 first via repeated OnUp() calls, rather than assuming
        // a fresh index 0). ---
        menu.OnCancel();
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Main Menu"), "Cancel on the Help topic list should return to Main Menu");

        // --- G: "Credits" (index 3) -> the real hardcoded credits text,
        // via the SAME info screen Help topics use -- Ok always returns
        // to Main Menu (secondaryParam==204's own real, hardcoded
        // target), confirming the shared info screen's own backTarget
        // tracking distinguishes the two real cases correctly. ---
        for (int i = 0; i < 10; i++) menu.OnUp();  // force selectedIndex back to 0 first
        menu.OnDown();
        menu.OnDown();
        menu.OnDown();  // index 0 -> 1 -> 2 -> 3 ("Credits")
        Check(menu.OnSelect() == MenuFlowAction::None, "selecting Credits should not itself be a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Credits"), "selecting Credits should show the real credits info screen");
        Check(menu.OnSelect() == MenuFlowAction::None, "Ok on the credits screen is not a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Main Menu"), "Ok on the credits screen should return to Main Menu (not Help)");

        // --- H: "Exit" (index 4) -> the real "Are you sure?" quit
        // confirmation, NOT an immediate exit. ---
        for (int i = 0; i < 10; i++) menu.OnUp();  // force selectedIndex back to 0 first
        menu.OnDown();
        menu.OnDown();
        menu.OnDown();
        menu.OnDown();  // index 0 -> ... -> 4 ("Exit")
        Check(menu.OnSelect() == MenuFlowAction::None, "selecting Exit should show a confirmation, not exit immediately");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Quit?"), "selecting Exit should show the real quit-confirmation prompt");

        // --- I: Cancel on the quit confirmation is a real no-op -- its
        // own Cancel command was explicitly removed in the original, so
        // there's no way to back out of it at all. ---
        menu.OnCancel();
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Quit?"), "Cancel on the quit confirmation should be a real no-op (stays showing)");

        // --- J: the real, preserved bug -- EITHER "Yes" or "No" quits.
        // Checked with "No" actually selected (index 1), not just the
        // default "Yes" (index 0), to prove this isn't merely an
        // untested default -- the original's own commandAction1 branch
        // never reads selectedIndex at all here. ---
        menu.OnDown();  // select "No"
        Check(menu.OnSelect() == MenuFlowAction::Exit,
              "selecting \"No\" on the quit confirmation should STILL exit -- the real, preserved bug");

        // --- K: fresh MenuFlow, "New Game" (index 0) starts the game. ---
        MenuFlow menu2(helpText);
        Check(menu2.OnSelect() == MenuFlowAction::StartNewGame, "selecting New Game should request starting the game");

        if (g_ok) {
            std::printf("all menu-flow checks passed\n");
            return 0;
        } else {
            std::printf("SOME CHECKS FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("EXCEPTION: %s\n", e.what());
        return 1;
    }
}
