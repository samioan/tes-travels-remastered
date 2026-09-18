#include "launcher/install.h"

#include <filesystem>
#include <vector>

#include "launcher/zip_reader.h"

namespace dawnstar {
namespace launcher {

namespace {

namespace fs = std::filesystem;

const char* const kMarkerFiles[] = {
    "datfiles.lmp",
    "imgfiles.lmp",
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
        error = "that .jar doesn't look like Dawnstar -- missing datfiles.lmp/imgfiles.lmp/"
                "npcstrings.dat after unpacking";
        return false;
    }
    return true;
}

}  // namespace launcher
}  // namespace dawnstar
