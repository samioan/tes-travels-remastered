#pragma once
#include <array>
#include <cstdint>
#include <string>

#include "graphics/backbuffer.h"

namespace stormhold {

// GameCanvas's message-popup state (messageLines/unconfirmed_ad
// ("messageVisible")/messageShownAt/messagePriority) -- GameCanvas
// *static* fields in the original, folded in here rather than added to
// PlayerState (same "own small UI-state struct, not PlayerState" call
// M29's HudState already made, and dawnstar's own identical
// MessagePopupState, M30 there too).
struct MessagePopupState {
    std::array<std::string, 2> lines{"", ""};
    bool visible = false;
    int64_t shownAtMs = 0;
    int priority = 0;
};

// Renamed-source counterpart of GameCanvas's message-popup subsystem
// (showMessage()/paintMessagePopup(), plus run()'s own per-tick auto-
// hide timeout) -- M30, built on this milestone's own new
// graphics/bitmap_font.h text renderer and Backbuffer::FillRoundRect.
//
// NOT ported here: GameCanvas.wordWrap()/wrapToTwoLines() (UIScreen.java
// has its own, unrelated, wordWrap() for a different screen entirely --
// out of scope). Every one of GameCanvas's own real showMessage() call
// sites passes a pre-baked MSG_*/npcNameLines String[2] constant
// directly (../../../src/GameCanvas.java's own message-string table,
// confirmed by reading every real call site -- none ever wraps dynamic
// text through this popup, unlike dawnstar's own shop-greeting-name
// popup which needed WordWrap for exactly that reason). So there is
// nothing here to wrap.
//
// Also not ported: paintHud() (needs the still-missing hotbarIcons
// asset bundle and digit glyphs, a separate deferred milestone -- see
// docs/PORT_ROADMAP.md) and every showMessage() call site still
// unreachable because its own action (attack/spellcast/rest/respawn) or
// the surrounding tick-loop helper it lives in isn't wired into this
// port's live tick loop yet.
class MessagePopup {
public:
    // GameCanvas.showMessage(lines, priority), folding in the
    // "if (showMessage(...)) { messageShownAt = now; unconfirmed_ad =
    // true; }" pattern every real call site in GameCanvas.java repeats
    // identically right after calling it -- a SIMPLIFIED (but exactly
    // behavior-preserving) consolidation, same as dawnstar's own
    // identical fold.
    static bool Show(MessagePopupState& state, const std::array<std::string, 2>& lines, int priority, int64_t nowMs);

    // GameCanvas.run()'s own unconditional per-tick check: "if (frameStart
    // - messageShownAt > 3000L) { unconfirmed_ad = false; messagePriority
    // = 0; }" -- ported exactly, including running regardless of whether
    // a message is even currently visible. NOT modeled here: the
    // separate, unrelated `unconfirmed_ad = false; messagePriority = 0;`
    // reset embedded in run()'s own facing==2->3 camp-state transition
    // (a still-untranscribed tick-loop helper's own side effect, not
    // part of showMessage()/paintMessagePopup() itself).
    static void Tick(MessagePopupState& state, int64_t nowMs);

    // GameCanvas.paintMessagePopup(): the rounded-rect background plus
    // up to 2 lines of text, only while `state.visible`. The original
    // gates its second line on `messageLines.length > 1`, which is
    // ALWAYS true in practice (every real MSG_*/npcNameLines constant is
    // exactly 2 elements) -- MessagePopupState::lines is fixed-size for
    // the same reason, so this just always draws both lines (an empty
    // second line draws nothing anyway).
    static void Paint(Backbuffer& bb, const MessagePopupState& state);
};

}  // namespace stormhold
