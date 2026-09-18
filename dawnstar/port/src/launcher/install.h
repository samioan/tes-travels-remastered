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
// There is no font concept here at all -- Dawnstar's renderer
// (port/src/graphics/bitmap_font.h/.cpp) draws its own embedded bitmap
// font and reads no external file, unlike Shadowkey's Nokia ROM font
// requirement.

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

}  // namespace launcher
}  // namespace dawnstar
