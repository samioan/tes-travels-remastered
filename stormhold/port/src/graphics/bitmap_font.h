#pragma once
#include <string>

#include "graphics/backbuffer.h"

namespace stormhold {

// A hand-authored monospace 4x7 pixel font -- NOT decompiled/recovered
// data. GameCanvas.smallFont (`Font.getFont(64, 0, 8)`: FACE_MONOSPACE,
// STYLE_PLAIN, SIZE_SMALL) is a MIDP built-in system font whose exact
// glyph bitmaps and per-character pixel metrics were always platform/
// device-dependent -- unlike every other visual in this project, there
// is no real original glyph shape to recover here, so this is a
// deliberate invention rather than a simplification of something
// recoverable.
//
// Same shapes/metrics as dawnstar's own identical invention
// (dawnstar/port/src/graphics/bitmap_font.h, its own M30) -- both ports
// share the same "ngame" engine and the same class of unrecoverable
// MIDP system font, so reusing one hand-authored alphabet keeps the two
// ports visually consistent rather than inventing a second, arbitrarily
// different one for no real reason.
//
// SIMPLIFIED character set: only space, ' - ! and A-Z (30 glyphs) are
// defined -- confirmed sufficient for every real string this milestone's
// only real consumer (render/message_popup.h's MessagePopup::Paint,
// drawing GameCanvas's own MSG_*/npcNameLines String constants) actually
// displays: every character in every MSG_* constant and npcNameLines
// entry (../../../src/GameCanvas.java) is one of space/'/A-Z (no '-' or
// digit is actually used yet, but '-' is kept for parity with dawnstar's
// identical font rather than trimming it over one unused glyph).
// Lowercase is folded to uppercase before drawing rather than separately
// hand-authoring a second full glyph set purely for cosmetic case-
// fidelity on an already-invented font -- a real, visible (`"REST"`
// instead of `"Rest"`), but harmless, simplification. The per-character
// advance (`kAdvance` = 5px) is itself invented (no real smallFont
// metric survives).
//
// Digits (hotbarKeyGlyphs -- '1'/'3'/'5'/'7'/'9'/'0', paintHud()'s own
// hotbar key prompts) are NOT yet defined here -- paintHud()'s actual
// pixel drawing (its hotbarIcons image row plus these digit glyphs) is
// deferred to a future milestone, same as this file's own header
// comment already flags for FillRoundRect's second real call site. See
// docs/PORT_ROADMAP.md's "what's next" for the exact scope split.
namespace BitmapFont {

constexpr int kGlyphWidth = 4;
constexpr int kGlyphHeight = 7;
// The advance from one character's left edge to the next's (4px glyph
// + 1px gap) -- invented (see this namespace's own doc comment), not a
// recovered smallFont metric.
constexpr int kAdvance = 5;

// Draws `text` (case-folded to uppercase) at (x, y) as its own TOP-LEFT
// corner -- the only MIDP anchor (TOP|LEFT = 20) any real call site in
// this port ever passes to drawChar/drawString. A character outside the
// supported set (see this namespace's own class comment -- none appear
// in any real string this port displays) draws nothing but still
// advances by kAdvance, matching a real Font's per-character advance
// semantics.
void DrawString(Backbuffer& bb, int x, int y, const std::string& text, uint16_t rgb565);

// Font.stringWidth(text)'s counterpart: text.length() * kAdvance,
// exact for a monospace font.
int StringWidth(const std::string& text);

}  // namespace BitmapFont

}  // namespace stormhold
