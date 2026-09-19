#pragma once

// `launcher.cfg` -- where the launcher remembers what the user plugged in.
// Ported from dawnstar's own port/src/launcher/launcher_config.h/.cpp
// (itself from shadowkey-decomp); same plain `key=value` file, `font`
// dropped (Stormhold needs none -- see install.h).
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

namespace stormhold {
namespace launcher {

struct LauncherConfig {
    // Directory holding the game's own unpacked files (charin.dat,
    // itemsin.dat, npcstrings.dat, ...) -- passed to stormhold_port.exe as
    // argv[1]. Empty until the user has picked a .jar.
    std::string gameData;
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
}  // namespace stormhold
