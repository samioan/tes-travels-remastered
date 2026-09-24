#pragma once
#include <string>

#include "graphics/backbuffer.h"

namespace stormhold {

// Text rendering, in the Nokia 3650's own ROM bitmap fonts when the player
// has provided them, else a GDI stand-in.
//
// M78: what the original really drew. The MIDlet's `Font.getFont(face,
// style, size)` calls ran on the Nokia 3650 (S60 1st Edition), whose MIDP
// runtime (kmidrun.dll, `CMIDFont.cpp`) IGNORES the face argument and maps
// size x style through a fixed table onto ROM typefaces -- read straight
// out of the runtime's machine code (see docs/PORT_ROADMAP.md, M78):
//
//               PLAIN          BOLD           ITALIC
//     SMALL     LatinPlain12   LatinBold12    Alpi12
//     MEDIUM    Alp13          LatinBold13    Alpi13
//     LARGE     alp17          LatinBold17    alpi17
//
// (bold-italic: Albi12/Albi13/albi17b). Bold and italic are separate hand-
// drawn typefaces, never synthesised. The game only ever asks for six of
// them -- the Face enum below. The Latin* faces live in Ceurope.gdr, the
// Al* ones in Browsereur.gdr, both Nokia firmware the launcher asks the
// player for (launcher/install.h); nothing of Nokia's is in this repo.
//
// These are 1-bit fonts: every pixel is ink or not, no anti-aliasing,
// exactly as on the device. The earlier M74 GDI "Arial Black" renderer
// (anti-aliased, one 10px face for everything) is kept as the fallback for
// whenever a face isn't loaded -- a plain dev build, a smoke test, or a
// player who skipped the font step.
namespace BitmapFont {

enum class Face {
    SmallBold,    // Font.getFont(0, BOLD, SMALL): UIScreen itemFont/commandFont -- list items,
                  // body text, command bar
    MediumBold,   // Font.getFont(0, BOLD, MEDIUM): titleFont/progressFont
    SmallPlain,   // Font.getFont(64, PLAIN, SMALL): GameCanvas.smallFont -- HUD, message popup,
                  // zoomed-out compass
    LargeBold,    // Font.getFont(64, BOLD, LARGE): minimapFont -- the normal-zoom compass
    MediumPlain,  // Font.getDefaultFont() = getFont(SYSTEM, PLAIN, MEDIUM) (MIDP spec):
                  // Alp13 -- the splash credits, which set no font
    LargeItalic,  // Font.getFont(64, ITALIC, LARGE): deadScreenFont/campScreenFont
};

// Layout scalars the screens were built around (kLineHeight =
// kGlyphHeight + 2 = 12 everywhere, which is exactly LatinBold12's own
// cell height -- the original's `lineHeight = itemFont.getHeight()`).
constexpr int kGlyphWidth = 8;
constexpr int kGlyphHeight = 10;
constexpr int kAdvance = 8;

// Loads every face from the device font store `ceuropePath` (Ceurope.gdr)
// and, for the italic face, its sibling Browsereur.gdr in the same folder.
// Returns true when Ceurope.gdr provided the four Latin faces; the italic
// face alone is optional (LargeItalic falls back to LargeBold without
// it). Non-fatal either way -- unloaded faces use the GDI stand-in.
bool LoadDeviceFonts(const std::string& ceuropePath);

// True when LoadDeviceFonts has put real ROM glyphs behind `face`.
bool IsDeviceFace(Face face);

// Real measured width of `text` in whichever font DrawString would use.
int StringWidth(const std::string& text, Face face = Face::SmallBold);

// Cell height of `face` -- `Font.getHeight()` on the device (12/13/17 for
// small/medium/large), kGlyphHeight for the stand-in.
int LineHeight(Face face = Face::SmallBold);

// Draws `text` with (x, y) as the top-left of its cell -- the only MIDP
// anchor (TOP|LEFT) any call site passes after its own centring.
void DrawString(Backbuffer& bb, int x, int y, const std::string& text, uint16_t rgb565,
                Face face = Face::SmallBold);

}  // namespace BitmapFont

}  // namespace stormhold
