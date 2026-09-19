// M30 smoke test: the message popup (render/message_popup.h) and its
// two new foundations, graphics/bitmap_font.h (a hand-authored pixel
// font -- there is no real font asset to recover, see its own doc
// comment) and Backbuffer::FillRoundRect -- GameCanvas.showMessage()/
// paintMessagePopup() plus run()'s own per-tick auto-hide timeout. No
// JVM ground truth available (same reason as every prior milestone) --
// verified via:
//  - Pure BitmapFont/FillRoundRect primitive checks (no game data
//    needed).
//  - Show/Tick's exact priority-gate and auto-hide-timeout arithmetic,
//    independently re-derived from decompiled/e.java's a(String[],int)
//    (now transcribed into ../../../src/GameCanvas.java's own
//    showMessage()), not read back from message_popup.cpp.
//  - Paint's visible-vs-hidden gating and pixel placement, against
//    GameCanvas.paintMessagePopup()'s own real constants.
#include <cstdint>
#include <cstdio>

#include "graphics/backbuffer.h"
#include "graphics/bitmap_font.h"
#include "render/message_popup.h"

namespace {

using stormhold::Backbuffer;
namespace BitmapFont = stormhold::BitmapFont;
using stormhold::MessagePopup;
using stormhold::MessagePopupState;
using stormhold::PackRGB565;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

uint16_t PixelAt(const Backbuffer& bb, int x, int y) { return bb.Data()[static_cast<size_t>(y) * Backbuffer::kWidth + x]; }

}  // namespace

int main() {
    // --- A: BitmapFont primitives ---
    {
        Check(BitmapFont::StringWidth("AB") == 2 * BitmapFont::kAdvance, "StringWidth is length*kAdvance");

        // 'A' == {0110,1001,1001,1111,1001,1001,1001} (bit3=leftmost
        // col0 .. bit0=col3) -- independently re-derived from
        // bitmap_font.cpp's own doc comment shape, not read back from
        // its glyph table.
        Backbuffer bb;
        bb.Fill(0);
        constexpr uint16_t kWhite = PackRGB565(255, 255, 255);
        BitmapFont::DrawString(bb, 10, 10, "A", kWhite);
        // row0 "0110": cols 1,2 lit, 0 and 3 dark.
        Check(PixelAt(bb, 10 + 0, 10 + 0) == 0, "'A' row0 col0 should be dark");
        Check(PixelAt(bb, 10 + 1, 10 + 0) == kWhite, "'A' row0 col1 should be lit");
        Check(PixelAt(bb, 10 + 2, 10 + 0) == kWhite, "'A' row0 col2 should be lit");
        Check(PixelAt(bb, 10 + 3, 10 + 0) == 0, "'A' row0 col3 should be dark");
        // row3 "1111": all 4 columns lit (the crossbar).
        Check(PixelAt(bb, 10 + 0, 10 + 3) == kWhite, "'A' row3 (crossbar) col0 should be lit");
        Check(PixelAt(bb, 10 + 3, 10 + 3) == kWhite, "'A' row3 (crossbar) col3 should be lit");

        // Case-folding: lowercase 'a' should draw identically to 'A'.
        Backbuffer bbLower;
        bbLower.Fill(0);
        BitmapFont::DrawString(bbLower, 10, 10, "a", kWhite);
        bool identical = true;
        for (int y = 0; y < BitmapFont::kGlyphHeight && identical; y++) {
            for (int x = 0; x < BitmapFont::kGlyphWidth; x++) {
                if (PixelAt(bb, 10 + x, 10 + y) != PixelAt(bbLower, 10 + x, 10 + y)) identical = false;
            }
        }
        Check(identical, "lowercase 'a' should draw identically to uppercase 'A' (case-folded font)");

        // An unsupported character (a digit -- not yet defined, see
        // bitmap_font.h's own doc comment) draws nothing but still
        // advances -- check the SECOND character lands exactly
        // kAdvance further, regardless.
        Backbuffer bbUnsupported;
        bbUnsupported.Fill(0);
        BitmapFont::DrawString(bbUnsupported, 0, 0, "1A", kWhite);
        Check(PixelAt(bbUnsupported, 0, 0) == 0 && PixelAt(bbUnsupported, 1, 0) == 0,
              "an unsupported character (digit) should draw nothing");
        Check(PixelAt(bbUnsupported, BitmapFont::kAdvance + 1, 0) == kWhite,
              "the character after an unsupported one should still land kAdvance further along");
    }

    // --- B: Backbuffer::FillRoundRect ---
    {
        Backbuffer bb;
        bb.Fill(0);
        constexpr uint16_t kWhite = PackRGB565(255, 255, 255);
        bb.FillRoundRect(0, 0, 10, 10, 4, 4, kWhite);
        // Corner (0,0): dx=dy=(0+2)-0-0.5=1.5; (1.5/2)^2*2=1.125>1 -- excluded.
        Check(PixelAt(bb, 0, 0) == 0, "FillRoundRect should cut away its own rounded corner");
        // Center (5,5): outside every corner's own box (rx=ry=2) -- filled unconditionally.
        Check(PixelAt(bb, 5, 5) == kWhite, "FillRoundRect should fill its own non-corner interior");

        Backbuffer bbSquare;
        bbSquare.Fill(0);
        bbSquare.FillRoundRect(0, 0, 10, 10, 0, 0, kWhite);
        Check(PixelAt(bbSquare, 0, 0) == kWhite, "a zero arc width/height should fill the full square, corners included");

        Backbuffer bbNoop;
        bbNoop.Fill(0);
        bbNoop.FillRoundRect(0, 0, -5, -5, 4, 4, kWhite);
        Check(PixelAt(bbNoop, 0, 0) == 0, "a non-positive w/h should be a no-op");
    }

    // --- C: MessagePopup::Show / Tick priority-gate + timeout ---
    // Independently re-derived from decompiled/e.java's a(String[],int)
    // and run()'s own "frameStart - messageShownAt > 3000L" check.
    {
        MessagePopupState state;
        Check(MessagePopup::Show(state, {"A", "B"}, 1, 1000), "priority 1 over the initial priority 0 should succeed");
        Check(state.visible && state.priority == 1 && state.shownAtMs == 1000, "state after the first Show");

        Check(!MessagePopup::Show(state, {"C", "D"}, 1, 2000),
              "an equal priority should NOT override the active message");
        Check(state.lines[0] == "A" && state.shownAtMs == 1000, "a failed Show should leave state untouched");

        Check(MessagePopup::Show(state, {"E", "F"}, 2, 3000), "a strictly higher priority should override");
        Check(state.lines[0] == "E" && state.priority == 2 && state.shownAtMs == 3000, "state after the override");

        Check(MessagePopup::Show(state, {"G", "H"}, -1, 4000),
              "a negative priority (\"always show, max priority\") should always succeed");
        Check(state.lines[0] == "G" && state.priority == 10 && state.shownAtMs == 4000,
              "a negative priority should set priority to 10, not -1 itself");

        MessagePopup::Tick(state, 4000 + 3000);  // exactly at the threshold -- NOT past it
        Check(state.visible, "exactly 3000ms since shownAt should NOT yet time out (strict >)");

        MessagePopup::Tick(state, 4000 + 3001);
        Check(!state.visible && state.priority == 0, "3001ms since shownAt should time out and reset priority");
    }

    // --- D: MessagePopup::Paint ---
    // Against GameCanvas.paintMessagePopup()'s own real constants
    // (../../../src/GameCanvas.java): fillRoundRect(96,118,75,35,5,5),
    // color 13080935 (0xC79967) background, drawString at (100,122)/
    // (100,134), color 0 (black) text.
    {
        Backbuffer bb;
        bb.Fill(0);
        MessagePopupState hidden;
        MessagePopup::Paint(bb, hidden);
        Check(PixelAt(bb, 100, 120) == 0, "Paint should draw nothing while not visible");

        // Real content: MSG_REST_COMPLETE (../../../src/GameCanvas.java),
        // mixed-case with a trailing '!' -- exercises case-folding and
        // punctuation together on an actual message string, not just a
        // synthetic all-caps one.
        MessagePopupState visible;
        visible.visible = true;
        visible.lines = {"Rest", "complete!"};
        MessagePopup::Paint(bb, visible);
        // Popup background: fillRoundRect(96,118,75,35,5,5) -- (100,120)
        // is well inside the rect and outside any rounded corner.
        constexpr uint16_t kPopupBg = PackRGB565(0xC7, 0x99, 0x67);
        Check(PixelAt(bb, 100, 120) == kPopupBg, "Paint should draw the popup background while visible");
        // Line 0 ("Rest") drawn at (100,122): case-folded 'R's row0 "1110" -> cols 0-2 lit.
        constexpr uint16_t kBlack = PackRGB565(0, 0, 0);
        Check(PixelAt(bb, 100 + 0, 122 + 0) == kBlack, "Paint should draw line 0's text (case-folded) at (100,122)");
        // Line 1 ("complete!") drawn at (100,134): case-folded 'C's row0 "0111" -> cols 1-3 lit.
        Check(PixelAt(bb, 100 + 1, 134 + 0) == kBlack, "Paint should draw line 1's text (case-folded) at (100,134)");
    }

    if (g_ok) {
        std::printf("all message-popup checks passed\n");
        return 0;
    } else {
        std::printf("SOME CHECKS FAILED\n");
        return 1;
    }
}
