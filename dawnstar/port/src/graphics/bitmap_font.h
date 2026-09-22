#pragma once
#include <string>

#include "graphics/backbuffer.h"

namespace dawnstar {

// Renamed-source counterpart of Screen.java/GameCanvas.java's several
// distinct MIDP system fonts (SMALL_FONT/DEFAULT_TEXT_FONT/
// LARGE_TEXT_FONT/SOFT_KEY_FONT/TITLE_FONT/BIG_MESSAGE_FONT/
// COMPASS_FONT_ZOOMED -- every one of them a `Font.getFont(...)` MIDP
// built-in with no recoverable glyph data), collapsed onto ONE real
// font here.
//
// M30 through M55 drew every one of these with a hand-authored,
// INVENTED 4x7 monospace, uppercase-only pixel glyph table -- a
// deliberate stand-in, chosen because there was no way to see what a
// real device actually rendered. M58 changed that: a real KEmulator (a
// genuine MIDP emulator) screenshot of the actual original MIDlet
// shows a bold, wide, mixed-case sans-serif look nothing like those
// invented glyphs. Rather than hand-author a SECOND invented pixel
// font chasing that screenshot by eye, this renders real text through
// Win32 GDI (a bold system font) instead -- mixed case, every ASCII
// character, and a real typeface come for free.
//
// That's a real architectural swap, not just new glyph art: this is
// the one place in the whole port's rendering pipeline that does real
// ALPHA-BLENDED compositing rather than Backbuffer::Blit()/FillRect's
// binary on/off convention. Deliberate -- GDI's own anti-aliased
// rendering is what makes a real font legible at this port's small
// 176x208 scale, and unlike Blit's binary transparency (a real MIDP
// hardware constraint worth preserving), there is no MIDP hardware
// precedent to preserve here: this whole subsystem is already an
// invention, hand-drawn or GDI-rendered.
//
// Kept the same public surface (`DrawString`/`StringWidth`) so almost
// every existing call site (Screen, MessagePopup, HotbarRenderer,
// BootSplash, LoadingScreen, NameEntry, MinimapRenderer) needed no
// changes -- except the two spots that genuinely depended on the OLD
// monospace assumption (render/message_popup.cpp's WordWrap inner
// per-character loop, now using the new CharWidth; ui/name_entry.cpp's
// cursor position, now using a real cumulative StringWidth). Text is
// no longer case-folded to uppercase -- every real string this port
// displays (item/monster names, Shop.NAMES, etc.) now renders in its
// own real case, matching the KEmulator screenshot's own Title Case
// menu labels.
namespace BitmapFont {

// A representative "typical" glyph cell, still used by the handful of
// call sites that need a single scalar for coarse layout (vertical
// centering math, the name-entry cursor block's own size) rather than
// a real per-string measurement -- NOT the true per-character width
// anymore now that the font is genuinely proportional; see CharWidth/
// StringWidth for that.
constexpr int kGlyphWidth = 8;
constexpr int kGlyphHeight = 10;
constexpr int kAdvance = 8;

// The real (GDI-measured) width of a single character in this font --
// GameCanvas.wordWrap()'s own `font.charWidth(...)` call, honored
// properly now that there's a real, non-monospace font to measure
// instead of the old flat kAdvance stand-in. render/message_popup.cpp's
// WordWrap is this function's one real caller.
int CharWidth(char c);

// Font.stringWidth(text)'s counterpart -- a real GDI text-extent
// measurement (not text.length()*kAdvance anymore).
int StringWidth(const std::string& text);

// Draws `text` at (x, y) as its own TOP-LEFT corner (the only MIDP
// anchor any real call site in this port ever passes to drawChar/
// drawString), alpha-blended against whatever `bb` already holds --
// see this namespace's own doc comment for why.
void DrawString(Backbuffer& bb, int x, int y, const std::string& text, uint16_t rgb565);

}  // namespace BitmapFont

}  // namespace dawnstar
