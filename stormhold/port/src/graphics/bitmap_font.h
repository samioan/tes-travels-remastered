#pragma once
#include <string>

#include "graphics/backbuffer.h"

namespace stormhold {

// Renamed-source counterpart of GameCanvas.smallFont (`Font.getFont(64, 0,
// 8)`: FACE_MONOSPACE, STYLE_PLAIN, SIZE_SMALL) -- a MIDP built-in system
// font whose exact glyph bitmaps and per-character pixel metrics were
// always platform/device-dependent, so there was never a real original
// glyph shape to recover here.
//
// M30/M31/M40 drew it with a hand-authored, INVENTED 4x7 monospace,
// uppercase-only pixel glyph table -- a deliberate stand-in, shared
// (same shapes/metrics) with dawnstar's own identical invention for the
// same class of unrecoverable MIDP font. Dawnstar's own M58 later found a
// real KEmulator screenshot of the actual original MIDlet showing a bold,
// wide, mixed-case sans-serif look nothing like those invented glyphs, and
// swapped dawnstar's font for one rendered through real Win32 GDI instead
// of chasing that screenshot with a second hand-drawn pixel font. M74
// makes the same swap here: no Stormhold-specific screenshot was needed to
// justify it -- both games share the same "ngame" engine and the same
// class of invented placeholder font, so the same real-GDI-font
// replacement applies for the same reason, keeping the two ports visually
// consistent the way the old invented font already tried to.
//
// That's a real architectural swap, not just new glyph art: this is the
// one place in the whole port's rendering pipeline that does real
// ALPHA-BLENDED compositing rather than Backbuffer::Blit()/FillRect's
// binary on/off convention. Deliberate -- GDI's own anti-aliased
// rendering is what makes a real font legible at this port's small
// 176x208 scale, and unlike Blit's binary transparency (a real MIDP
// hardware constraint worth preserving), there is no MIDP hardware
// precedent to preserve here: this whole subsystem is already an
// invention, hand-drawn or GDI-rendered.
//
// Kept the same public surface (`DrawString`/`StringWidth`) so every
// existing call site (render/game_renderer.h, render/message_popup.h,
// ui/boot_splash.h, ui/inventory_ui.h, ui/menu_flow.h,
// ui/npc_choices_menu.h, ui/npc_dialogue.h, ui/pause_menu.h) needed no
// changes -- every one of them already measures word-wrap candidates and
// title/label centering through whole-string `StringWidth` calls rather
// than a flat `kAdvance`-per-character assumption, so there was no
// per-character-loop call site to update the way dawnstar's own
// message_popup.cpp/name_entry.cpp needed (see dawnstar's own
// bitmap_font.h class comment) -- `CharWidth` isn't ported here, since
// nothing in this port would ever call it. Text is no longer case-folded
// to uppercase -- every real string this port displays now renders in its
// own real case, matching the KEmulator screenshot's own Title Case menu
// labels dawnstar's own M58 confirmed.
namespace BitmapFont {

// A representative "typical" glyph cell, still used by the handful of
// call sites that need a single scalar for coarse layout (every screen's
// own `kLineHeight = BitmapFont::kGlyphHeight + 2`) rather than a real
// per-string measurement -- NOT the true per-character width anymore now
// that the font is genuinely proportional; see StringWidth for that.
constexpr int kGlyphWidth = 8;
constexpr int kGlyphHeight = 10;
constexpr int kAdvance = 8;

// Font.stringWidth(text)'s counterpart -- a real GDI text-extent
// measurement (not text.length()*kAdvance anymore).
int StringWidth(const std::string& text);

// Draws `text` at (x, y) as its own TOP-LEFT corner (the only MIDP anchor
// any real call site in this port ever passes to drawChar/drawString),
// alpha-blended against whatever `bb` already holds -- see this
// namespace's own doc comment for why.
void DrawString(Backbuffer& bb, int x, int y, const std::string& text, uint16_t rgb565);

}  // namespace BitmapFont

}  // namespace stormhold
