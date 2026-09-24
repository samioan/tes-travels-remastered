#pragma once

// `launcher.cfg` -- where the launcher remembers what the user plugged in.
// Ported from shadowkey-decomp's port/src/launcher/launcher_config.h/.cpp;
// same plain `key=value` file. M78 brought shadowkey-decomp's own `font`
// key back (see install.h):
//
//     font=fonts/Ceurope.gdr
//
//     gameData=data
//     scale=2
//
// Paths are stored relative to the install root when they live inside it
// (which, after the setup flow unpacks the jar in, they always do), so the
// whole folder can be moved or renamed without breaking the install. An
// absolute path is still read back correctly if one ever gets written.
// Unknown keys are ignored rather than dropped-on-rewrite being a surprise;
// a missing file yields the defaults, which is the first-run state.

#include <string>

namespace dawnstar {
namespace launcher {

struct LauncherConfig {
    // Directory holding the game's own unpacked files (datfiles.lmp,
    // imgfiles.lmp, npcstrings.dat, ...) -- passed to dawnstar_port.exe as
    // argv[1]. Empty until the user has picked a .jar.
    std::string gameData;
    // M78: the installed copy of the phone's Ceurope.gdr (Browsereur.gdr
    // sits beside it when the player had one) -- passed to dawnstar_port.exe
    // as argv[2]. Empty means stand-in letters.
    std::string font;
    // Integer window scale over the native 176x208. main.cpp hardcoded 2
    // before this milestone, which stays the default.
    int scale = 2;

    static constexpr int kMinScale = 1;
    static constexpr int kMaxScale = 8;

    static constexpr const char* kFileName = "launcher.cfg";
};

// Both return false if the file could not be read/written at all. A file
// that exists but is empty or entirely unrecognised is not an error -- it
// reads back as defaults, same as no file.
bool LoadLauncherConfig(const std::string& path, LauncherConfig& out);
bool SaveLauncherConfig(const std::string& path, const LauncherConfig& config);

// Joins `root` and `relative` unless `relative` is already absolute, in
// which case it is returned unchanged. The one place the "relative to the
// install root" convention above is actually applied.
std::string ResolveAgainst(const std::string& root, const std::string& relative);

// The inverse, used before saving: makes `path` relative to `root` when it
// is inside it, and returns it unchanged when it is not.
std::string RelativeToIfInside(const std::string& root, const std::string& path);

}  // namespace launcher
}  // namespace dawnstar
