#pragma once
#include <string>

#include "graphics/backbuffer.h"

namespace dawnstar {

// A hand-authored monospace 5x7 pixel font -- NOT decompiled/recovered
// data. GameCanvas.SMALL_FONT (`Font.getFont(64, 0, 8)`: FACE_MONOSPACE,
// STYLE_PLAIN, SIZE_SMALL) is a MIDP built-in system font whose exact
// glyph bitmaps and per-character pixel metrics were always platform/
// device-dependent -- unlike every other visual in this project, there
// is no real original glyph shape to recover here, so this is a
// deliberate invention rather than a simplification of something
// recoverable. Monospace is kept (matching SMALL_FONT's own
// FACE_MONOSPACE) since render/message_popup.h's WordWrap is
// transcribed straight from GameCanvas.wordWrap()'s own arithmetic,
// which only behaves the way it does (every `font.charWidth()` call
// interchangeable) for a monospace font.
//
// SIMPLIFIED character set: only space, ' - ! and A-Z (30 glyphs) are
// defined. Every real string this port actually displays through this
// font (Shop.NAMES, Item/Monster names, the message-popup MSG_*
// constants) is case-folded to uppercase before being drawn, rather
// than separately hand-authoring a full lowercase glyph set purely for
// cosmetic case-fidelity on a font that's already invented. This is a
// real, visible (but harmless) simplification -- displayed text reads
// in caps ("WEAPON PEDDLER" instead of "Weapon Peddler") -- not a
// functional gap: every character actually present in the real
// extracted item/monster names and Shop.NAMES (confirmed by a
// temporary diagnostic dump, not assumed) is one of space/'/-/! or a
// letter.
//
// M31 adds 0-9 (10 more glyphs, 40 total) for GameCanvas.
// HOTBAR_DIGIT_CHARS, the hotbar panel's own numeric prompts -- same
// hand-authored, invented-shape status as every other glyph here.
namespace BitmapFont {

constexpr int kGlyphWidth = 4;
constexpr int kGlyphHeight = 7;
// The advance from one character's left edge to the next's (4px glyph
// + 1px gap) -- invented (see this namespace's own doc comment), not a
// recovered SMALL_FONT metric. Deliberately chosen narrow enough that
// render/message_popup.h's WrapToTwoLines still fits real shop-greeting
// content (e.g. Shop.NAMES' own "Heavy Armor Peddler", the longest real
// string wrapped through this port's fixed 69px popup width) into
// exactly 2 lines rather than needlessly overflowing a 3rd (silently
// discarded by WrapToTwoLines) that a real device's own unrecoverable
// font metric probably wouldn't have needed either -- verified in
// m30_message_popup_smoke.cpp, not just assumed.
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

}  // namespace dawnstar
