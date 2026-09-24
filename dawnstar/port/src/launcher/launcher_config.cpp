#include "launcher/launcher_config.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace dawnstar {
namespace launcher {

namespace {

namespace fs = std::filesystem;

void TrimInPlace(std::string& text) {
    const auto notSpace = [](unsigned char c) { return c != ' ' && c != '\t' && c != '\r'; };
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), notSpace));
    text.erase(std::find_if(text.rbegin(), text.rend(), notSpace).base(), text.end());
}

// Paths go into the file with forward slashes so a config written on one
// machine reads identically everywhere and so the file is not full of
// escaped-looking backslashes. std::filesystem accepts either on Windows.
std::string ToForwardSlashes(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

}  // namespace

bool LoadLauncherConfig(const std::string& path, LauncherConfig& out) {
    out = LauncherConfig{};
    std::ifstream file(path);
    if (!file) return false;

    std::string line;
    while (std::getline(file, line)) {
        TrimInPlace(line);
        if (line.empty() || line[0] == '#') continue;
        const size_t equals = line.find('=');
        if (equals == std::string::npos) continue;
        std::string key = line.substr(0, equals);
        std::string value = line.substr(equals + 1);
        TrimInPlace(key);
        TrimInPlace(value);

        if (key == "gameData") {
            out.gameData = value;
        } else if (key == "font") {
            out.font = value;
        } else if (key == "scale") {
            // A garbage or out-of-range scale keeps the default rather than
            // opening a 1-pixel or 20000-pixel window.
            try {
                const int scale = std::stoi(value);
                if (scale >= LauncherConfig::kMinScale && scale <= LauncherConfig::kMaxScale) {
                    out.scale = scale;
                }
            } catch (const std::exception&) {
                // keep the default
            }
        }
        // Anything else is someone else's key (or a future one) -- ignored.
    }
    return true;
}

bool SaveLauncherConfig(const std::string& path, const LauncherConfig& config) {
    std::ofstream file(path, std::ios::trunc);
    if (!file) return false;
    file << "# Dawnstar Remastered launcher settings.\n"
         << "# Rewritten by the launcher; hand-edits to known keys are kept.\n"
         << "gameData=" << ToForwardSlashes(config.gameData) << "\n"
         << "font=" << ToForwardSlashes(config.font) << "\n"
         << "scale=" << config.scale << "\n";
    return static_cast<bool>(file);
}

std::string ResolveAgainst(const std::string& root, const std::string& relative) {
    if (relative.empty()) return std::string();
    fs::path candidate(relative);
    // make_preferred so the result reads with one kind of slash throughout.
    if (candidate.is_absolute()) return candidate.make_preferred().string();
    return (fs::path(root) / candidate).make_preferred().string();
}

std::string RelativeToIfInside(const std::string& root, const std::string& path) {
    if (path.empty()) return std::string();
    std::error_code error;
    // weakly_canonical so this still works for a path that does not exist
    // yet (the setup flow computes the destination before creating it).
    const fs::path absoluteRoot = fs::weakly_canonical(fs::path(root), error);
    if (error) return path;
    const fs::path absolutePath = fs::weakly_canonical(fs::path(path), error);
    if (error) return path;

    const fs::path relative = fs::relative(absolutePath, absoluteRoot, error);
    if (error || relative.empty()) return path;
    // `relative` climbing out of the root with ".." means the path is not
    // inside it, and an absolute path is the honest answer for those.
    if (*relative.begin() == "..") return path;
    return relative.string();
}

}  // namespace launcher
}  // namespace dawnstar
