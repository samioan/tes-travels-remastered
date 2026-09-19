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
// SIMPLIFIED character set: space, ' - ! , A-Z, and 0-9 (40 glyphs) --
// confirmed sufficient for every real string this port's two real
// consumers actually display: render/message_popup.h's MessagePopup::
// Paint (GameCanvas's own MSG_*/npcNameLines String constants -- every
// character in every entry is one of space/'/A-Z, no '-' or digit is
// actually used there, but '-' is kept for parity with dawnstar's
// identical font rather than trimming it over one unused glyph) and
// render/game_renderer.h's GameRenderer::RenderHud (paintHud()'s own
// hotbarKeyGlyphs = '1'/'3'/'5'/'7'/'9'/'0', M31, added on top of M30's
// original 30-glyph letters-only set -- same split dawnstar's own
// identical font took, M30 letters then M31 digits). Lowercase is
// folded to uppercase before drawing rather than separately hand-
// authoring a second full glyph set purely for cosmetic case-fidelity
// on an already-invented font -- a real, visible (`"REST"` instead of
// `"Rest"`), but harmless, simplification. The per-character advance
// (`kAdvance` = 5px) is itself invented (no real smallFont metric
// survives). Digit shapes are identical to dawnstar's own (its M31) --
// same "reuse rather than invent a second arbitrary shape" reasoning as
// this file's own letters.
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
