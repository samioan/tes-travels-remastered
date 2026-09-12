#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "graphics/backbuffer.h"

namespace dawnstar {

// GameCanvas's message-popup state (messageLines/messageVisible/
// messageShownAt/messagePriority) -- GameCanvas *static* fields in the
// original, folded in here (see player/player_state.h's own
// npcInSight/minimapDirty/minimapZoomedOut doc comments, M28/M29, for
// the established reasoning). Deliberately kept SEPARATE from
// PlayerState rather than added there as a 4th instance of that
// pattern: 2 strings plus 3 more fields made "just keep growing
// PlayerState with transient UI state" start to feel like the wrong
// home for something that isn't part of the character/save data at
// all. A future cleanup might fold ALL of GameCanvas's leftover static
// fields (this struct AND PlayerState's own trio) into one dedicated
// UI-state struct -- not done here, to keep this milestone's footprint
// to what it actually needs.
struct MessagePopupState {
    std::array<std::string, 2> lines{"", ""};
    bool visible = false;
    int64_t shownAtMs = 0;
    int priority = 0;
};

// Renamed-source counterpart of GameCanvas's message-popup subsystem
// (showMessage()/wordWrap()/wrapToTwoLines()/paintMessagePopup(), plus
// run()'s own per-tick auto-hide timeout) -- M30, built on M30's own
// new graphics/bitmap_font.h text renderer and Backbuffer::
// FillRoundRect. NOT ported here: paintHotbar() (a separate,
// independently-deferred milestone -- see docs/PORT_ROADMAP.md) and
// every showMessage() call site still unreachable because its own
// action (attack/spell-cast/camp/menu) isn't wired into the live tick
// loop yet.
class MessagePopup {
public:
    // GameCanvas.showMessage(lines, priority), folding in the
    // "if (showMessage(...)) { messageShownAt = now; messageVisible =
    // true; }" pattern every real call site in GameCanvas.java repeats
    // identically right after calling it -- a SIMPLIFIED (but exactly
    // behavior-preserving) consolidation: no real call site ever calls
    // showMessage() without that exact follow-up, so folding it in here
    // removes boilerplate without changing what happens.
    static bool Show(MessagePopupState& state, const std::array<std::string, 2>& lines, int priority, int64_t nowMs);

    // GameCanvas.run()'s own unconditional per-tick check: "if (now -
    // messageShownAt > 3000) { messageVisible = false; messagePriority
    // = 0; }" -- ported exactly, including running regardless of
    // whether a message is even currently visible.
    static void Tick(MessagePopupState& state, int64_t nowMs);

    // GameCanvas.wordWrap(text, maxWidth, font): ported using
    // BitmapFont::kAdvance in place of the original's real (device-
    // dependent, unrecoverable) SMALL_FONT metrics -- see
    // graphics/bitmap_font.h's own doc comment. `\n` is honored as a
    // hard break exactly like the original's own recursive structure.
    // SIMPLIFIED: the hard-break inner loop additionally bounds-checks
    // against the string's own length (the original has no equivalent
    // guard and would throw if it were ever exercised) -- same
    // "C++ has no equivalent safety net" reasoning as player/
    // player_movement.h's own out-of-bounds neighbor guard; the real
    // invariant that keeps the original's own loop in-bounds (a
    // substring already known to reach `maxWidth` before this loop even
    // starts) still holds here too, so this guard is defensive, not a
    // behavior change.
    static std::vector<std::string> WordWrap(const std::string& text, int maxWidthPx);

    // GameCanvas.wrapToTwoLines(text): WordWrap at the fixed 69px popup
    // width, padded/truncated to exactly 2 lines. SIMPLIFIED: the
    // original's `System.arraycopy(wrapped, 0, result, 0, 2)` assumes
    // `wrapped` has at least 2 elements whenever it doesn't have
    // exactly 1 (true for every real string this port displays); this
    // port instead copies however many of the first 2 elements
    // actually exist, defensively -- the same real-invariant-holds-in-
    // practice reasoning as WordWrap's own guard above.
    static std::array<std::string, 2> WrapToTwoLines(const std::string& text);

    // GameCanvas.paintMessagePopup(): the rounded-rect background plus
    // up to 2 lines of text, only while `state.visible`. The original
    // gates its second line on `messageLines.length > 1`, which is
    // ALWAYS true in practice (every real call site's lines array,
    // whether an MSG_* constant or WrapToTwoLines' own output, is
    // exactly 2 elements) -- MessagePopupState::lines is fixed-size
    // for the same reason, so this just always draws both lines (an
    // empty second line draws nothing anyway).
    static void Paint(Backbuffer& bb, const MessagePopupState& state);
};

}  // namespace dawnstar
