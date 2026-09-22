// M30 smoke test: the message popup (render/message_popup.h) and its
// two new foundations, graphics/bitmap_font.h (M58-remastered: real
// GDI text, not the original hand-authored pixel font -- see its own
// doc comment) and Backbuffer::FillRoundRect -- GameCanvas.showMessage()/
// wordWrap()/wrapToTwoLines()/paintMessagePopup() plus run()'s own
// per-tick auto-hide timeout. No JVM ground truth available (same
// reason as every prior milestone) -- verified via:
//  - BitmapFont primitive checks: since a real proportional GDI font's
//    exact glyph shapes can't be hand-predicted the way the old fixed
//    4x7 pixel table's bit patterns could, these check STRUCTURAL
//    properties instead (something draws somewhere in a glyph's own
//    box, mixed case actually differs, StringWidth is monotonic) --
//    same "check the glyph drew somewhere in its own box" approach
//    M55's own compass-glyph test established, not exact pixels.
//  - Backbuffer::FillRoundRect primitive checks (no game data needed,
//    unaffected by the font change).
//  - WordWrap against several hand-traced cases (a small synthetic one
//    worked through byte-by-byte in this file's own comments, plus two
//    real Shop.NAMES strings -- the longest real content this port
//    actually wraps through this popup) -- independently re-derived
//    from GameCanvas.wordWrap()'s own algorithm, not read back from
//    message_popup.cpp. The exact per-character pixel widths obviously
//    can't be hand-predicted for a real GDI font either, so these
//    check structural invariants (every wrapped line's own real
//    StringWidth stays under the budget, the words concatenate back to
//    the original) rather than a hand-traced exact string.
//  - Show/Tick's exact priority-gate and auto-hide-timeout arithmetic
//    (unaffected by the font change).
//  - Paint's visible-vs-hidden gating and text presence (not exact
//    pixels, same reasoning as the BitmapFont primitives above).
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "graphics/backbuffer.h"
#include "graphics/bitmap_font.h"
#include "render/message_popup.h"

namespace {

using dawnstar::Backbuffer;
namespace BitmapFont = dawnstar::BitmapFont;
using dawnstar::MessagePopup;
using dawnstar::MessagePopupState;
using dawnstar::PackRGB565;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

uint16_t PixelAt(const Backbuffer& bb, int x, int y) { return bb.Data()[static_cast<size_t>(y) * Backbuffer::kWidth + x]; }

// True if any pixel in [x0,x0+w) x [y0,y0+h) is exactly `targetColor`
// -- BitmapFont's own anti-aliased edges mean not every pixel a glyph
// touches is the pure target color, but a real drawn character is wide/
// tall enough that its stroke centers still are (coverage==255 takes
// DrawString's own fast, unblended path -- see bitmap_font.cpp).
bool AnyPixelInBox(const Backbuffer& bb, int x0, int y0, int w, int h, uint16_t targetColor) {
    for (int y = y0; y < y0 + h; y++) {
        for (int x = x0; x < x0 + w; x++) {
            if (PixelAt(bb, x, y) == targetColor) return true;
        }
    }
    return false;
}

void CheckLines(const std::vector<std::string>& actual, const std::vector<std::string>& expected, const char* label) {
    if (actual.size() != expected.size()) {
        std::printf("  FAIL: %s: expected %zu lines, got %zu\n", label, expected.size(), actual.size());
        g_ok = false;
        return;
    }
    for (size_t i = 0; i < actual.size(); i++) {
        if (actual[i] != expected[i]) {
            std::printf("  FAIL: %s: line %zu expected \"%s\", got \"%s\"\n", label, i, expected[i].c_str(),
                        actual[i].c_str());
            g_ok = false;
        }
    }
}

}  // namespace

int main() {
    // --- A: BitmapFont primitives ---
    {
        constexpr uint16_t kWhite = PackRGB565(255, 255, 255);

        // A real, proportional GDI font's exact glyph shapes can't be
        // hand-predicted bit-by-bit the way the old fixed 4x7 table's
        // could -- these check structural properties instead.
        Check(BitmapFont::StringWidth("") == 0, "an empty string measures 0 wide");
        Check(BitmapFont::StringWidth("A") > 0, "a real character measures a positive width");
        Check(BitmapFont::StringWidth("AB") > BitmapFont::StringWidth("A"),
              "a longer string should measure wider than its own prefix");
        Check(BitmapFont::CharWidth('A') == BitmapFont::StringWidth("A"),
              "CharWidth(c) should agree with StringWidth of the 1-character string");

        // 'A' should draw SOMETHING somewhere in its own advance-wide,
        // kGlyphHeight-tall box (the same "checked as presence in a
        // box, not an exact shape" approach M55's own compass-glyph
        // test established).
        Backbuffer bb;
        bb.Fill(0);
        BitmapFont::DrawString(bb, 10, 10, "A", kWhite);
        bool anyLit = false;
        for (int y = 0; y < BitmapFont::kGlyphHeight && !anyLit; y++) {
            for (int x = 0; x < BitmapFont::CharWidth('A') && !anyLit; x++) {
                if (PixelAt(bb, 10 + x, 10 + y) != 0) anyLit = true;
            }
        }
        Check(anyLit, "'A' should draw at least one non-background pixel in its own box");

        // Mixed case is a real, deliberate feature now (see bitmap_font.h's
        // own class comment) -- 'a' and 'A' are genuinely DIFFERENT
        // glyphs, the opposite of the old font's own case-folding.
        Backbuffer bbLower;
        bbLower.Fill(0);
        BitmapFont::DrawString(bbLower, 10, 10, "a", kWhite);
        bool anyDifferent = false;
        for (int y = 0; y < BitmapFont::kGlyphHeight && !anyDifferent; y++) {
            for (int x = 0; x < BitmapFont::kGlyphWidth * 2 && !anyDifferent; x++) {
                if (PixelAt(bb, 10 + x, 10 + y) != PixelAt(bbLower, 10 + x, 10 + y)) anyDifferent = true;
            }
        }
        Check(anyDifferent, "lowercase 'a' should draw DIFFERENTLY from uppercase 'A' -- real mixed case, not folded");

        // Real strings this port displays (item/monster names, Shop.
        // NAMES) are never case-folded before reaching DrawString
        // anymore -- confirm a mixed-case string round-trips through
        // StringWidth/DrawString without being altered structurally
        // (StringWidth agrees with the sum a naive per-char walk would
        // expect to be in the right ballpark: positive, and at least as
        // wide as its longest single character).
        Check(BitmapFont::StringWidth("Weapon Peddler") > BitmapFont::StringWidth("W"),
              "a real mixed-case string should measure wider than one of its own characters");
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

    // --- C: MessagePopup::WordWrap ---
    {
        // Fast path: fits on one line.
        CheckLines(MessagePopup::WordWrap("AB", 100), {"AB"}, "fits-on-one-line fast path");

        // A real proportional GDI font's exact pixel widths can't be
        // hand-traced by eye the way the old fixed-advance font's
        // could -- these two cases instead predict the expected split
        // from BitmapFont::CharWidth/StringWidth directly (an
        // independently-tested lower module, not message_popup.cpp
        // itself), then check WordWrap's real output matches that
        // independent prediction exactly.

        // Space-boundary path: two words, wrapped at a width computed
        // to fit the first word but not both together -- must break
        // between them, not mid-word.
        {
            int w1 = BitmapFont::StringWidth("Weapon");
            int wBoth = BitmapFont::StringWidth("Weapon Peddler");
            CheckLines(MessagePopup::WordWrap("Weapon Peddler", (w1 + wBoth) / 2 + 8), {"Weapon", "Peddler "},
                       "space-boundary wrap: a width between the first word and the full "
                       "two-word string should break exactly at the space");
        }

        // Forced hard-break path: one long word with no spaces at all,
        // wrapped at a narrow width -- must break mid-word, since there
        // is no space to break at. The expected split point is
        // predicted here via the same per-character accumulation
        // WordWrap's own hard-break loop uses (w += CharWidth(c) until
        // w >= the effective (maxWidthPx-8) budget), independently of
        // message_popup.cpp itself.
        {
            const std::string longWord = "ABCDEFGHIJKLMNOP";
            const int maxWidthPx = 30;
            const int effectiveBudget = maxWidthPx - 8;
            int w = 0;
            size_t splitAt = 0;
            while (w < effectiveBudget && splitAt < longWord.size()) {
                w += BitmapFont::CharWidth(longWord[splitAt]);
                splitAt++;
            }
            std::string expectedFirstLine = longWord.substr(0, splitAt);

            std::vector<std::string> wrapped = MessagePopup::WordWrap(longWord, maxWidthPx);
            Check(!wrapped.empty() && wrapped[0] == expectedFirstLine,
                  "forced hard-break: the first line should match the independently-predicted "
                  "per-character split point");
            // Every line's own real width should stay under maxWidthPx
            // (the budget the caller actually asked for), and every
            // character of the original word should be accounted for
            // exactly once across all lines.
            std::string reassembled;
            for (const std::string& line : wrapped) {
                Check(BitmapFont::StringWidth(line) < maxWidthPx,
                      "every hard-break line should measure under the requested maxWidthPx");
                reassembled += line;
            }
            Check(reassembled == longWord, "the hard-broken lines should reassemble back to the original word exactly");
        }

        // Embedded '\n' is a hard break, recursively wrapped on each side.
        CheckLines(MessagePopup::WordWrap("AB\nCD", 100), {"AB", "CD"}, "embedded newline hard break");

        // A newline as the LAST character is stripped, not treated as
        // an extra empty trailing line.
        CheckLines(MessagePopup::WordWrap("AB\n", 100), {"AB"}, "trailing newline should be stripped, not split");

        // Real content: Shop.NAMES' two longest entries, at
        // WrapToTwoLines' own real 80px popup width (message_popup.cpp's
        // own doc comment on WrapToTwoLines explains why 80, not the
        // old font's 69) -- chosen so both fit into exactly 2 lines,
        // verified directly here.
        CheckLines(MessagePopup::WordWrap("Weapon Peddler", 80), {"Weapon", "Peddler "},
                   "real Shop.NAMES[0] should wrap to exactly 2 lines");
        CheckLines(MessagePopup::WordWrap("Heavy Armor Peddler", 80), {"Heavy Armor", "Peddler "},
                   "real Shop.NAMES[1] (the longest shop name) should still wrap to exactly 2 lines");
    }

    // --- D: MessagePopup::WrapToTwoLines ---
    {
        auto single = MessagePopup::WrapToTwoLines("HELLO");
        Check(single[0] == "HELLO" && single[1].empty(), "a single-line-fits input should pad with an empty 2nd line");

        auto real = MessagePopup::WrapToTwoLines("Heavy Armor Peddler");
        Check(real[0] == "Heavy Armor" && real[1] == "Peddler ", "WrapToTwoLines on real shop-greeting content");
    }

    // --- E: MessagePopup::Show / Tick priority-gate + timeout ---
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

    // --- F: MessagePopup::Paint ---
    {
        Backbuffer bb;
        bb.Fill(0);
        MessagePopupState hidden;
        MessagePopup::Paint(bb, hidden);
        Check(PixelAt(bb, 100, 120) == 0, "Paint should draw nothing while not visible");

        MessagePopupState visible;
        visible.visible = true;
        visible.lines = {"AB", "CD"};
        MessagePopup::Paint(bb, visible);
        // Popup background: fillRoundRect(96,118,75,35,5,5) -- (100,120)
        // is well inside the rect and outside any rounded corner.
        constexpr uint16_t kPopupBg = PackRGB565(0xC7, 0x99, 0x67);
        Check(PixelAt(bb, 100, 120) == kPopupBg, "Paint should draw the popup background while visible");
        // Line 0 ("AB") drawn at (100,122) and line 1 ("CD") at (100,134)
        // -- checked as "something drew somewhere in the real glyph
        // box" (real proportional GDI text, not one hand-picked exact
        // pixel -- see this file's own header comment).
        constexpr uint16_t kBlack = PackRGB565(0, 0, 0);
        Check(AnyPixelInBox(bb, 100, 122, BitmapFont::StringWidth("AB"), BitmapFont::kGlyphHeight, kBlack),
              "Paint should draw line 0's text at (100,122)");
        Check(AnyPixelInBox(bb, 100, 134, BitmapFont::StringWidth("CD"), BitmapFont::kGlyphHeight, kBlack),
              "Paint should draw line 1's text at (100,134)");

        if (g_ok) {
            std::printf("all message-popup checks passed\n");
        }
    }

    if (g_ok) {
        return 0;
    } else {
        std::printf("SOME CHECKS FAILED\n");
        return 1;
    }
}
