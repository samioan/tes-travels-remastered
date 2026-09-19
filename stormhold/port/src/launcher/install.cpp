#include "launcher/install.h"

#include <filesystem>
#include <vector>

#include "launcher/zip_reader.h"

namespace stormhold {
namespace launcher {

namespace {

namespace fs = std::filesystem;

// charin.dat/itemsin.dat/npcstrings.dat: three of the flat top-level
// resources ESGame.requiredResourceNames itself lists (see
// ../../../src/ESGame.java), present at the top level of a real
// TEST-Stormhold.jar (confirmed both against stormhold/extracted/ and by
// unzip -l against the real jar) -- no datfiles.lmp/imgfiles.lmp wrapper
// to check for, unlike dawnstar's own identical marker check.
const char* const kMarkerFiles[] = {
    "charin.dat",
    "itemsin.dat",
    "npcstrings.dat",
};

}  // namespace

bool IsGameDataRoot(const std::string& directory) {
    if (directory.empty()) return false;
    std::error_code error;
    if (!fs::is_directory(fs::path(directory), error) || error) return false;
    for (const char* marker : kMarkerFiles) {
        if (!fs::is_regular_file(fs::path(directory) / marker, error) || error) return false;
    }
    return true;
}

bool InstallGameJar(const std::string& jarPath, const std::string& destDir, std::string& error) {
    error.clear();
    std::error_code code;
    if (!fs::is_regular_file(fs::path(jarPath), code) || code) {
        error = jarPath + " is not a file";
        return false;
    }

    fs::create_directories(fs::path(destDir), code);
    if (code) {
        error = "could not create " + destDir + ": " + code.message();
        return false;
    }

    // A .jar is a plain zip; ExtractZip lands every entry straight into
    // destDir, the same flat layout tools/extract_jar.py's own
    // zipfile.extractall() produces.
    if (!ExtractZip(jarPath, destDir, error)) {
        error = "could not unpack " + jarPath + ": " + error;
        return false;
    }

    if (!IsGameDataRoot(destDir)) {
        error = "that .jar doesn't look like Stormhold -- missing charin.dat/itemsin.dat/"
                "npcstrings.dat after unpacking";
        return false;
    }
    return true;
}

}  // namespace launcher
}  // namespace stormhold
