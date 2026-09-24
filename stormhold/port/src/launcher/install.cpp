#include "launcher/install.h"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <vector>

#include "assets/gdr_font.h"
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

namespace {

std::string Lower(std::string text) {
    for (char& ch : text) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return text;
}

// The file in `directory` named `name`, ignoring case, or an empty path.
fs::path FindCaseInsensitive(const fs::path& directory, const std::string& name) {
    std::error_code error;
    const std::string wanted = Lower(name);
    for (const fs::directory_entry& entry : fs::directory_iterator(directory, error)) {
        if (Lower(entry.path().filename().string()) == wanted) return entry.path();
    }
    return fs::path();
}

std::string EnvOrEmpty(const char* name) {
    char* value = nullptr;
    size_t size = 0;
    if (_dupenv_s(&value, &size, name) != 0 || !value) return std::string();
    std::string result(value);
    free(value);
    return result;
}

bool CopyInto(const fs::path& from, const fs::path& to, std::string& error) {
    std::error_code code;
    if (fs::exists(to, code) && fs::equivalent(from, to, code) && !code) return true;
    fs::copy_file(from, to, fs::copy_options::overwrite_existing, code);
    if (code) {
        error = "could not copy " + from.string() + ": " + code.message();
        return false;
    }
    return true;
}

}  // namespace

bool IsUsableFont(const std::string& path) {
    if (path.empty()) return false;
    std::error_code error;
    if (!fs::is_regular_file(fs::path(path), error) || error) return false;
    GdrFont font;
    return font.Load(path, "LatinBold12");
}

bool InstallFonts(const std::string& ceuropePath, const std::string& destDir, bool& copiedItalic,
                  std::string& error) {
    error.clear();
    copiedItalic = false;
    if (!IsUsableFont(ceuropePath)) {
        error = "that file isn't a font this game can use -- it has to be the phone's own "
                "Ceurope.gdr (a Symbian font store containing LatinBold12)";
        return false;
    }
    std::error_code code;
    fs::create_directories(fs::path(destDir), code);
    if (code) {
        error = "could not create " + destDir + ": " + code.message();
        return false;
    }
    if (!CopyInto(fs::path(ceuropePath), fs::path(destDir) / "Ceurope.gdr", error)) return false;

    const fs::path italic = FindCaseInsensitive(fs::path(ceuropePath).parent_path(), "Browsereur.gdr");
    if (!italic.empty()) {
        if (!CopyInto(italic, fs::path(destDir) / "Browsereur.gdr", error)) return false;
        copiedItalic = true;
    }
    return true;
}

std::string FindExistingFont() {
    std::vector<fs::path> directories;
    // EKA2L1 keeps an extracted ROM per device under its data directory;
    // an N-Gage or any other S60 v1 phone carries the same font files.
    for (const char* variable : {"APPDATA", "LOCALAPPDATA", "USERPROFILE"}) {
        const std::string base = EnvOrEmpty(variable);
        if (base.empty()) continue;
        std::error_code error;
        for (const fs::directory_entry& device :
             fs::directory_iterator(fs::path(base) / "EKA2L1" / "data" / "drives" / "z", error)) {
            directories.push_back(device.path() / "System" / "Fonts");
            directories.push_back(device.path() / "Resource" / "Fonts");
        }
    }
    // Nokia's Series 60 MIDP SDK 1.2.1 at its default install path -- the
    // emulator the M78 font investigation itself read these files out of.
    directories.push_back(fs::path("C:\\MIDP_Emulators\\Series_60_MIDP_SDK_for_Symbian_OS_v_1_2_1\\bin\\"
                                   "Series_60_MIDP_SDK_for_Symbian_OS_v_1_2_1\\epoc32\\release\\wins\\udeb\\z\\"
                                   "system\\fonts"));
    for (const fs::path& directory : directories) {
        const fs::path candidate = FindCaseInsensitive(directory, "Ceurope.gdr");
        if (!candidate.empty() && IsUsableFont(candidate.string())) return candidate.string();
    }
    return std::string();
}

}  // namespace launcher
}  // namespace stormhold
