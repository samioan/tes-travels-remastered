// M37 smoke test: Screen (ui/screen.h) -- Screen.java's generic
// list/message/prompt-list UI primitive: data-model construction
// (SetupList/SetupMessage/SetupPromptList), pixel rendering (Paint),
// up/down list navigation (MoveSelectionUp/Down), and soft-key command
// resolution (LeftSoftKeyCommand/RightSoftKeyCommand). See ui/screen.h's
// own class comment for what's deliberately deferred (the ESGame/
// GameCanvas navigation graph that constructs real Screens and dispatches
// their commands into game actions -- a future, much larger milestone).
//
// No JVM ground truth is available (same reason as every prior
// milestone) -- every expectation below is either independently
// hand-derived from ../../../src/Screen.java's own arithmetic, or
// (for text placement) cross-checked against M30/M31's own
// already-verified `BitmapFont::DrawString` used as an oracle: rendering
// the same string onto a blank scratch Backbuffer and comparing its own
// "on" pixels against Screen::Paint's output at the expected offset,
// rather than hand-transcribing bitmap_font.cpp's own glyph bit table a
// second time.
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/help_text.h"
#include "assets/item_database.h"
#include "graphics/backbuffer.h"
#include "graphics/bitmap_font.h"
#include "render/message_popup.h"
#include "ui/screen.h"

namespace {

using dawnstar::Backbuffer;
namespace BitmapFont = dawnstar::BitmapFont;
using dawnstar::CommandId;
using dawnstar::MessagePopup;
using dawnstar::PackRGB565;
using dawnstar::Screen;
using dawnstar::ScreenMode;

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

// Screen.java's own g.setColor(...) literals -- independently re-derived
// here from the same real integers ../../../src/Screen.java uses, not
// read back from ui/screen.cpp's own (separately-derived) copy.
constexpr uint16_t RgbColor(int rgb) {
    return PackRGB565(static_cast<uint8_t>((rgb >> 16) & 0xFF), static_cast<uint8_t>((rgb >> 8) & 0xFF),
                       static_cast<uint8_t>(rgb & 0xFF));
}
constexpr uint16_t kBackgroundColor = RgbColor(2510210);
constexpr uint16_t kTitleBarColor = RgbColor(0);
constexpr uint16_t kTitleTextColor = RgbColor(16777215);
constexpr uint16_t kItemTextColor = RgbColor(16776960);
constexpr uint16_t kHighlightBoxColor = RgbColor(6710886);
constexpr uint16_t kSoftKeyBarColor = RgbColor(16777215);
constexpr uint16_t kSoftKeyTextColor = RgbColor(0);

// Renders `text` via BitmapFont::DrawString (M30/M31's own already-
// verified primitive) onto a blank scratch Backbuffer, then checks that
// every "on" pixel it produces appears at the same offset from (x0,y0)
// in `bb`, in `color` -- and that at least one "on" pixel exists at all,
// so this can't pass vacuously on an empty string.
//
// M58: BitmapFont is now a real, proportional, ALPHA-BLENDED GDI font
// (see its own class comment), not the old fixed-advance binary on/off
// one -- two real consequences here: (1) `spanW`, the real rendered
// width, is BitmapFont::StringWidth(text) now, not text.size()*kAdvance
// (no longer meaningful for a proportional font); (2) only a pixel the
// scratch render is FULLY confident is part of a glyph stroke (exact
// white -- coverage==255 takes DrawString's own unblended fast path) is
// compared 1:1 against `bb`. A partially-covered (anti-aliased) EDGE
// pixel's exact blended color depends on whatever background it was
// composited against, which differs between this scratch (always
// black) and `bb` (whatever the real screen background is) -- so
// unlike a real MIDP binary on/off font, an edge pixel can't be
// compared this way; only checking full-coverage pixels sidesteps that
// without weakening what's actually being verified (the glyph's own
// solid interior still has to land in the right place, in the right
// color).
bool TextRenderedAt(const Backbuffer& bb, int x0, int y0, const std::string& text, uint16_t color) {
    Backbuffer scratch;
    scratch.Fill(0);
    BitmapFont::DrawString(scratch, 0, 0, text, 0xFFFF);
    bool sawOnPixel = false;
    int spanW = BitmapFont::StringWidth(text);
    for (int dy = 0; dy < BitmapFont::kGlyphHeight; dy++) {
        for (int dx = 0; dx < spanW; dx++) {
            if (PixelAt(scratch, dx, dy) != 0xFFFF) continue;
            sawOnPixel = true;
            int px = x0 + dx;
            int py = y0 + dy;
            if (px < 0 || px >= Backbuffer::kWidth || py < 0 || py >= Backbuffer::kHeight) return false;
            if (PixelAt(bb, px, py) != color) return false;
        }
    }
    return sawOnPixel;
}

int CenteredTitleX(const std::string& title) { return Screen::width() / 2 - BitmapFont::StringWidth(title) / 2; }

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::CharacterData charData = dawnstar::CharacterData::Load(archive);
        dawnstar::HelpText helpText = dawnstar::HelpText::Load(archive);
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);

        // --- A: mode HighlightedList via SetupList -- ESGame.mainMenuUI's
        // own real shape (a 5-item, non-cancelable menu -> exactly 1
        // command, so it's ALWAYS the right softkey regardless of which
        // command it is). ---
        {
            Screen s(ScreenMode::HighlightedList);
            std::vector<std::string> menu{"New Game", "Continue Game", "Help", "Credits", "Exit"};
            s.SetupList("Main Menu", menu, false);

            Check(s.Title() == "Main Menu", "SetupList should set the title");
            Check(s.ItemCount() == 5, "SetupList's itemCount should equal the item count");
            Check(s.ScrollBottom() == 4, "5 items (<=10) should all be visible: scrollBottom == 4");
            Check(s.SelectedIndex() == 0, "SetupList should default selectedIndex to 0");

            CommandId cmd;
            Check(s.RightSoftKeyCommand(&cmd) && cmd == CommandId::Select,
                  "a single command should always resolve as the right softkey");
            Check(!s.LeftSoftKeyCommand(&cmd), "a single command should leave no left softkey");

            Backbuffer bb;
            bb.Fill(0);
            s.Paint(bb);

            Check(PixelAt(bb, 1, 1) == kTitleBarColor, "title bar background should be black at an untitled corner");
            Check(TextRenderedAt(bb, CenteredTitleX("Main Menu"), 0, "Main Menu", kTitleTextColor),
                  "title text should render centered in white");
            Check(PixelAt(bb, 90, 178) == kBackgroundColor,
                  "the screen backdrop below the item rows should be the real background color");
            Check(TextRenderedAt(bb, 15, 20, "New Game", kItemTextColor),
                  "the first (selected) item's own text should render in yellow at (marginX, 20)");
            Check(PixelAt(bb, 160, 20) == kHighlightBoxColor,
                  "the selected row's own highlight box should be drawn behind it");
            Check(PixelAt(bb, 160, 35) == kBackgroundColor,
                  "a non-selected row should have no highlight box");
            Check(PixelAt(bb, 5, 192) == kSoftKeyBarColor, "the softkey bar background should be white");
            int selectX = (Screen::width() - 10) - BitmapFont::StringWidth("Select");
            Check(TextRenderedAt(bb, selectX, 194, "Select", kSoftKeyTextColor),
                  "the lone command's label (\"Select\") should render right-aligned");
        }

        // --- B: mode HighlightedList via SetupList, ESGame.OptionsUI's
        // own real shape -- setupList(cancelable=false) then a Back
        // command added manually afterward -> 2 commands, right = Select
        // (matches ok/select), left = Back (matches back/cancel). ---
        {
            Screen s(ScreenMode::HighlightedList);
            std::vector<std::string> options{"Stats",  "Inventory", "Clue Log", "Skills",    "Spells",
                                              "Save Game", "Load Game", "Help",  "Reveal Traitor", "Quit Game"};
            s.SetupList("Options", options, false);
            s.AddCommand(CommandId::Back);

            Check(s.ItemCount() == 10, "the real Options list has exactly 10 entries");
            Check(s.ScrollBottom() == 9, "10 items (<=10) should all be visible: scrollBottom == 9");

            CommandId cmd;
            Check(s.RightSoftKeyCommand(&cmd) && cmd == CommandId::Select, "right softkey should resolve to Select");
            Check(s.LeftSoftKeyCommand(&cmd) && cmd == CommandId::Back, "left softkey should resolve to Back");

            Backbuffer bb;
            bb.Fill(0);
            s.Paint(bb);
            Check(TextRenderedAt(bb, 10, 194, "Back", kSoftKeyTextColor), "left softkey label \"Back\" should render");
            int selectX = (Screen::width() - 10) - BitmapFont::StringWidth("Select");
            Check(TextRenderedAt(bb, selectX, 194, "Select", kSoftKeyTextColor),
                  "right softkey label \"Select\" should still render alongside a left one");
        }

        // --- C: mode PlainList via SetupMessage -- ESGame.GenericInfoUI's
        // own real shape: the mode-4 constructor already added an Ok
        // command, so this needs no explicit AddCommand at all. ---
        {
            Screen s(ScreenMode::PlainList);
            const std::string& body = helpText.bodies[0];
            s.SetupMessage(helpText.titles[0], body);

            std::vector<std::string> expectedWrap = MessagePopup::WordWrap(body, Screen::width() - 5 - 5);
            Check(s.Items() == expectedWrap,
                  "SetupMessage's own word-wrap should match MessagePopup::WordWrap at the 5px-margin width");
            Check(s.ItemCount() == static_cast<int>(expectedWrap.size()), "itemCount should equal the wrapped line count");
            int expectedVisible = std::min(s.ItemCount(), 11);
            Check(s.ScrollBottom() == expectedVisible - 1, "at most 11 lines should be visible at once");

            CommandId cmd;
            Check(s.RightSoftKeyCommand(&cmd) && cmd == CommandId::Ok,
                  "the mode-4 constructor's own auto-added Ok command should be the right softkey");
            Check(!s.LeftSoftKeyCommand(&cmd), "a lone Ok command should leave no left softkey");

            Backbuffer bb;
            bb.Fill(0);
            s.Paint(bb);
            Check(TextRenderedAt(bb, 5, 20, s.Items()[0], kItemTextColor),
                  "the first wrapped line should render at the 5px margin, unselected (plain list, no highlight box)");
            Check(PixelAt(bb, 160, 20) == kBackgroundColor,
                  "mode PlainList should never draw a highlight box, even at the first row");
        }

        // --- D: mode PromptList via SetupPromptList -- ESGame.newGameUI's
        // own real shape (real Player class names via CharacterData). ---
        {
            Screen s(ScreenMode::PromptList);
            s.SetupPromptList("New Game", "Select a Class:", charData.classNames);

            std::vector<std::string> expectedPrompt = MessagePopup::WordWrap("Select a Class:", Screen::width() - 10 - 10);
            Check(s.ItemGroupStart().size() == charData.classNames.size(),
                  "no real class name should be long enough to force a split -- one group entry per class");
            for (size_t i = 0; i < charData.classNames.size(); i++) {
                Check(s.ItemGroupStart()[i] == static_cast<int>(i),
                      "an unsplit prompt list's own itemGroupStart should stay the identity mapping");
            }
            Check(s.ItemCount() == static_cast<int>(charData.classNames.size()),
                  "itemCount should equal the real class count when nothing split");

            CommandId cmd;
            Check(s.RightSoftKeyCommand(&cmd) && cmd == CommandId::Select, "prompt lists always add Select");
            Check(s.LeftSoftKeyCommand(&cmd) && cmd == CommandId::Cancel, "prompt lists always add Cancel");

            Backbuffer bb;
            bb.Fill(0);
            s.Paint(bb);
            Check(TextRenderedAt(bb, 10, 20, expectedPrompt[0], kItemTextColor),
                  "the prompt's own first wrapped line should render at (10, 20)");
        }

        // --- E: mode PromptListWithFooter -- ESGame.characterMainUI's own
        // real shape (a real footer string, "", exercised too). ---
        {
            Screen s(ScreenMode::PromptListWithFooter);
            std::vector<std::string> charItems{"See Class Info", "Create Character"};
            s.SetupPromptList("Character", "You selected:", "", charItems);
            Check(s.Items() == charItems, "the 4-arg overload should carry the same items as the 3-arg one");
            Check(!s.ItemGroupStart().empty(), "SetupPromptList should always populate itemGroupStart");

            Backbuffer bb;
            bb.Fill(0);
            s.Paint(bb);
            // Prompt "You selected:" (1 line) + footer "" (1 blank
            // wrapped line) + a 5px gap before the item rows begin.
            Check(TextRenderedAt(bb, 10, 20 + 12 + 12 + 5, "See Class Info", kItemTextColor),
                  "the item rows should start after both the prompt AND footer lines, plus the 5px gap");
        }

        // --- F: MoveSelectionUp/Down on a real >10-item HighlightedList
        // (no itemGroupStart -- the "plain selectedIndex" branch), using
        // the first 15 real item names so the list is real data, not
        // synthetic filler. ---
        {
            Check(items.ItemCount() >= 15, "the real item database should have at least 15 entries to sample");
            std::vector<std::string> names(items.name.begin(), items.name.begin() + 15);
            Screen s(ScreenMode::HighlightedList);
            s.SetupList("Items", names, false);
            Check(s.ScrollBottom() == 9, "15 items (>10) should cap scrollBottom at 9 initially");

            for (int i = 0; i < 9; i++) s.MoveSelectionDown();
            Check(s.SelectedIndex() == 9, "9 downs from index 0 should land on index 9 (still within the window)");
            Check(s.ScrollTop() == 0 && s.ScrollBottom() == 9, "the window shouldn't scroll until index 9 is exceeded");

            s.MoveSelectionDown();
            Check(s.SelectedIndex() == 10, "a 10th down should advance past the window");
            Check(s.ScrollTop() == 1 && s.ScrollBottom() == 10, "the window should scroll down by exactly 1");

            for (int i = 0; i < 10; i++) s.MoveSelectionDown();
            Check(s.SelectedIndex() == 14, "selectedIndex should clamp at itemCount-1 (14), not overshoot");
            Check(s.ScrollBottom() == 14, "the window should have scrolled all the way to the last item");

            for (int i = 0; i < 20; i++) s.MoveSelectionUp();
            Check(s.SelectedIndex() == 0, "selectedIndex should clamp at 0, not undershoot");
            Check(s.ScrollTop() == 0, "the window should have scrolled all the way back to the top");
        }

        // --- G: SetupPromptList's own per-item word-wrap-splitting
        // branch -- no real prompt-list item in this game is long enough
        // to force this (confirmed by section D above), so a synthetic
        // overlong item exercises it directly, the same "real data
        // doesn't hit this rare branch, so a synthetic case does"
        // precedent M16/M30 already established. ---
        {
            std::string longItem =
                "This Is A Very Long Item Name That Definitely Exceeds The Available Prompt List Width";
            std::vector<std::string> raw{"Short One", longItem, "Short Two"};
            Screen s(ScreenMode::PromptList);
            s.SetupPromptList("Test", "Prompt", raw);

            std::vector<std::string> expectedWrapped = MessagePopup::WordWrap(longItem, Screen::width() - 10 - 10);
            Check(expectedWrapped.size() > 1, "the synthetic long item should actually need multiple wrapped lines");

            int n = static_cast<int>(expectedWrapped.size());
            Check(s.ItemCount() == 2 + n, "itemCount should grow by exactly (wrapped lines - 1) net new rows");
            Check(s.ItemGroupStart().size() == 3, "itemGroupStart stays sized to the ORIGINAL (logical) item count");
            Check(s.ItemGroupStart()[0] == 0, "the first (unsplit) item's own group start is untouched");
            Check(s.ItemGroupStart()[1] == 1, "the split item's own group start stays at its original row");
            Check(s.ItemGroupStart()[2] == 1 + n,
                  "the item AFTER the split one should now start after all of its wrapped rows");

            Check(s.Items().size() == static_cast<size_t>(2 + n), "the merged items array should have the right size");
            Check(s.Items()[0] == "Short One", "the first item should be untouched");
            for (int i = 0; i < n; i++) {
                Check(s.Items()[static_cast<size_t>(1 + i)] == expectedWrapped[static_cast<size_t>(i)],
                      "each of the long item's own wrapped lines should appear in order");
            }
            Check(s.Items()[static_cast<size_t>(1 + n)] == "Short Two",
                  "the item after the split one should be preserved, just moved");

            // Selecting the split item (logical index 1) should highlight
            // a box exactly `n` rows tall -- confirmed visually, not just
            // via ItemGroupStart's own two endpoints, since RenderItemRows
            // computes the same difference internally.
            s.MoveSelectionDown();
            Check(s.SelectedIndex() == 1, "one down from index 0 should select the split item");
            Backbuffer bb;
            bb.Fill(0);
            s.Paint(bb);
            int boxTop = 20 + 12 + 5 + 1 * 13;  // prompt (1 line) + 5px gap + row 1's own cursorY
            // x=172: near the box's own right edge (marginX==10 here, so
            // the box spans the full 176px width) but well beyond where
            // any wrapped line's own text reaches (wrapping guarantees
            // each line's width stays under ~148px) -- avoids sampling a
            // pixel that could be either color depending on the exact
            // wrapped text, unlike x=160 which real wrapped content can
            // reach.
            Check(PixelAt(bb, 172, boxTop) == kHighlightBoxColor,
                  "the split item's own highlight box should start at its first wrapped row");
            Check(PixelAt(bb, 172, boxTop + (n - 1) * 13) == kHighlightBoxColor,
                  "the split item's own highlight box should still cover its LAST wrapped row");
        }

        // --- H: navigation + windowing on a real >9-logical-item prompt
        // list (the itemGroupStart-populated branch, distinct from
        // section F's plain-list branch) -- 12 real item names, none of
        // them split, so itemGroupStart stays the identity mapping and
        // this isolates the windowing logic itself. ---
        {
            Check(items.ItemCount() >= 12, "the real item database should have at least 12 entries to sample");
            std::vector<std::string> names(items.name.begin(), items.name.begin() + 12);
            Screen s(ScreenMode::PromptList);
            s.SetupPromptList("Test", "Pick one:", names);
            Check(s.ScrollBottom() == 8, "12 items (>9) should cap scrollBottom at 8 initially");

            // Unlike section F's plain-list branch (which scrolls once
            // `selectedIndex_` itself outgrows `scrollBottom_`), this
            // branch looks at the NEXT logical item's own group-start row
            // (`itemGroupStart_[selectedIndex_+1]`) -- with an unsplit,
            // identity groupStart, that's really just `selectedIndex_+1`,
            // so the window actually scrolls one step EARLIER than the
            // plain-list branch does: on the 8th down (once selectedIndex
            // reaches 8, since itemGroupStart_[9]==9 > scrollBottom==8
            // already), not a 9th/10th one. A real, confirmed difference
            // between the two navigation branches, not a copy-paste of
            // section F's own expectations.
            for (int i = 0; i < 20; i++) s.MoveSelectionDown();
            Check(s.SelectedIndex() == 11, "selectedIndex should clamp at itemCount-1 (11)");
            Check(s.ScrollBottom() == 11, "the window should have scrolled to show the last item");
            Check(s.ScrollTop() == 3, "the window should be exactly 8 rows tall (itemCount-windowSize-1 == 3)");

            for (int i = 0; i < 20; i++) s.MoveSelectionUp();
            Check(s.SelectedIndex() == 0, "navigating all the way back up should reach index 0");
            Check(s.ScrollTop() == 0 && s.ScrollBottom() == 8,
                  "the window should return to its EXACT original span, not just include index 0");
        }

        // --- I: the small runtime-reconfiguration accessors. ---
        {
            Screen s(ScreenMode::PlainList);
            s.SetupMessage("Old Title", "Hello there");
            int itemCountBefore = s.ItemCount();

            s.SetTitle("New Title");
            Check(s.Title() == "New Title", "SetTitle should replace the title");

            s.SetItems("Goodbye");
            Check(s.Items().size() == 1 && s.Items()[0] == "Goodbye",
                  "SetItems should replace a mode-PlainList screen's own items");
            // A real, preserved quirk: itemCount_/scrollBottom_ are NOT
            // refreshed by setItems() in the original -- see SetItems's
            // own doc comment.
            Check(s.ItemCount() == itemCountBefore,
                  "SetItems should leave itemCount stale, matching the original's own real (unrefreshed) behavior");

            Screen prompt(ScreenMode::PromptList);
            std::vector<std::string> two{"A", "B"};
            prompt.SetupPromptList("T", "P", two);
            Check(prompt.SelectedIndexOrMinusOne() == 0, "a selectable mode should report its real selectedIndex");
            Screen plain(ScreenMode::PlainList);
            plain.SetupMessage("T", "x");
            Check(plain.SelectedIndexOrMinusOne() == -1, "a non-selectable mode (PlainList) should report -1");

            prompt.SetSelectedIndex(1);
            Check(prompt.SelectedIndex() == 1, "SetSelectedIndex should move the selection");
            Check(prompt.SelectedItemText() == "B", "SelectedItemText should read back the newly selected item");
            prompt.SetSelectedIndex(50);
            Check(prompt.SelectedIndex() == 1, "SetSelectedIndex should clamp to the last real item, not overshoot");

            Check(prompt.FirstLine() == MessagePopup::WordWrap("P", Screen::width() - 10 - 10)[0],
                  "FirstLine on a prompt-list mode should return its own first prompt line");
            Screen highlighted(ScreenMode::HighlightedList);
            highlighted.SetupList("T", {"x"}, false);
            Check(highlighted.FirstLine().empty(), "FirstLine on mode HighlightedList returns empty (null in the original)");

            prompt.SetTextColumn(1, "Footer text");
            // column 1 on a mode-5 (PromptList, not PromptListWithFooter)
            // Screen should be a no-op -- only mode 6 accepts column 1.
            Check(prompt.Items().size() == 2, "SetTextColumn(1, ...) on mode PromptList should be a no-op");
        }

        if (g_ok) {
            std::printf("all screen checks passed\n");
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
