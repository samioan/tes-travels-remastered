#pragma once

// Turning "the .jar the user picked" into a working install.
//
// Unlike shadowkey-decomp's launcher (which asks for a folder and searches
// it, because the N-Gage dump has no fixed depth), Dawnstar's game data is
// a single file: `roms/TEST-Dawnstar.jar` is a plain zip, and
// `tools/extract_jar.py`'s own loader is nothing more than
// `zipfile.extractall()` -- there is no structure to search for, only a
// file to unzip. The launcher's whole job on the data side is: unzip the
// picked `.jar` into the install's own `data/`, and check the result looks
// like the game before trusting it.
//
// M78: and, separately, the Nokia 3650's own ROM fonts -- Ceurope.gdr (the
// Latin faces) and, if present beside it, Browsereur.gdr (the italic one).
// Nokia device firmware, so this project never ships them either; same
// "optional, stand-in letters without it" deal as shadowkey-decomp's own
// Ceurope.gdr step. See graphics/bitmap_font.h for which faces are used.

#include <string>

namespace dawnstar {
namespace launcher {

// The three top-level files a real extracted Dawnstar jar always has
// (confirmed against dawnstar/extracted/ -- see docs/ASSET_FORMATS.md),
// checked together so a folder that merely contains one of them by
// coincidence is not mistaken for the game.
bool IsGameDataRoot(const std::string& directory);

// Unzips `jarPath` into `destDir` (creating it, overwriting what's already
// there) and checks the result via IsGameDataRoot. On failure returns
// false and puts something a user can act on in `error` (not a real jar,
// a permission problem, a full disk, a source that vanished mid-copy).
bool InstallGameJar(const std::string& jarPath, const std::string& destDir, std::string& error);

// A file is a usable font iff the port's own parser can load LatinBold12
// out of it -- the same call the game makes, so "the launcher accepted it"
// and "the game can use it" cannot disagree.
bool IsUsableFont(const std::string& path);

// Copies `ceuropePath` into `destDir` as Ceurope.gdr, and Browsereur.gdr
// too when one sits beside it (matched case-insensitively -- dumps and
// SDKs disagree on capitalisation). On failure returns false with a
// user-facing `error`. `copiedItalic` says whether the second file came.
bool InstallFonts(const std::string& ceuropePath, const std::string& destDir, bool& copiedItalic,
                  std::string& error);

// Best-effort guess at a Ceurope.gdr already on this machine: an EKA2L1
// install's ROM drive, or Nokia's Series 60 MIDP SDK emulator at its
// default install path. Empty when there is none -- a first-run
// convenience, never a requirement.
std::string FindExistingFont();

}  // namespace launcher
}  // namespace dawnstar
