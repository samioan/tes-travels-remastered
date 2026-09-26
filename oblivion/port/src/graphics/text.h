#pragma once
#include <string>

#include "graphics/backbuffer.h"

namespace oblivion {

// Text in the Nokia 3650's own ROM bitmap fonts when the player has provided
// them (assets/gdr_font.h), else a GDI stand-in. The phone's MIDP runtime
// ignores Font.getFont's face argument and maps size x style onto fixed ROM
// typefaces; Oblivion only asks for three:
//
//   Game.fontSmall      = getFont(0, PLAIN, SMALL)   -> LatinPlain12  (Ceurope.gdr)
//   Game.fontMedium     = getFont(0, PLAIN, MEDIUM)  -> Alp13         (Browsereur.gdr)
//   Game.fontSmallBold  = getFont(0, BOLD,  SMALL)   -> LatinBold12   (Ceurope.gdr)
//   Game.fontLargeBold  = getFont(0, BOLD,  LARGE)   -> LatinBold17   (Ceurope.gdr)
//
// Without Browsereur.gdr, medium falls back to LatinPlain12. The fonts are
// Nokia's, so they are user-provisioned (see .gitignore, docs/PORT_ROADMAP.md).
namespace Text {

enum class Face { SmallPlain, MediumPlain, SmallBold, LargeBold };

// Loads Ceurope.gdr (+ Browsereur.gdr next to it, any capitalisation) from
// `dir`. True when the Latin faces loaded; non-fatal otherwise.
bool LoadDeviceFonts(const std::string& dir);
bool IsDeviceFace(Face face);

int StringWidth(const std::string& text, Face face);
// Font.substringWidth(text, offset, length).
int SubstringWidth(const std::string& text, size_t offset, size_t length, Face face);
// Font.getHeight().
int LineHeight(Face face);

// (x, y) is the top-left of the text cell (Graphics.TOP | LEFT).
void DrawString(Backbuffer& bb, int x, int y, const std::string& text, uint32_t rgb, Face face);

}  // namespace Text
}  // namespace oblivion
