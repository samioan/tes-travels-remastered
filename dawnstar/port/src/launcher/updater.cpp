#include "launcher/updater.h"

#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#include "launcher/zip_reader.h"

namespace dawnstar {
namespace launcher {

namespace {

namespace fs = std::filesystem;

std::wstring Widen(const std::string& text) {
    if (text.empty()) return std::wstring();
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
                                         nullptr, 0);
    std::wstring wide(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), &wide[0], size);
    return wide;
}

// Everything the updater writes lives under here, so a failed attempt is
// one directory to delete and nothing else.
fs::path UpdateDirectory(const std::string& installRoot) {
    return fs::path(installRoot) / "update";
}

struct ProgressRelay {
    UpdateProgressCallback callback = nullptr;
    void* context = nullptr;
    UpdateProgress progress;
};

void ReportDownloadProgress(unsigned long long soFar, unsigned long long total, void* context) {
    ProgressRelay* relay = static_cast<ProgressRelay*>(context);
    if (!relay || !relay->callback) return;
    relay->progress.stage = UpdateStage::Downloading;
    relay->progress.bytesSoFar = soFar;
    relay->progress.bytesTotal = total;
    relay->callback(relay->progress, relay->context);
}

void Report(ProgressRelay& relay, UpdateStage stage, const std::string& message) {
    if (!relay.callback) return;
    relay.progress.stage = stage;
    relay.progress.message = message;
    relay.callback(relay.progress, relay.context);
}

bool EndsWithNoCase(const std::string& text, const std::string& suffix) {
    if (text.size() < suffix.size()) return false;
    for (size_t i = 0; i < suffix.size(); ++i) {
        const char a = static_cast<char>(
            ::tolower(static_cast<unsigned char>(text[text.size() - suffix.size() + i])));
        const char b = static_cast<char>(::tolower(static_cast<unsigned char>(suffix[i])));
        if (a != b) return false;
    }
    return true;
}

}  // namespace

bool UpdatesEnabledForThisBuild(const std::string& version) {
    return version.find("-dev") == std::string::npos && !version.empty();
}

bool LooksLikeDawnstarPackage(const std::vector<std::string>& entryPaths) {
    bool hasLauncher = false;
    bool hasGame = false;
    for (const std::string& path : entryPaths) {
        std::string lowered = path;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                       [](unsigned char c) { return static_cast<char>(::tolower(c)); });
        std::replace(lowered.begin(), lowered.end(), '\\', '/');
        if (lowered == "dawnstar.exe") hasLauncher = true;
        if (lowered == "bin/dawnstar_port.exe") hasGame = true;
    }
    // Both, at the top level, in the layout the launcher expects. A zip
    // that merely contains an executable somewhere is not this program, and
    // refusing early is what keeps a wrong download from being moved over a
    // working install.
    return hasLauncher && hasGame;
}

void CleanUpPreviousUpdate(const std::string& installRoot) {
    std::error_code code;

    // The `.old` files left by the last swap. Anything still locked (a
    // previous instance that has not exited yet) simply stays for next
    // time -- this is best-effort by design.
    fs::recursive_directory_iterator it(fs::path(installRoot), code);
    if (!code) {
        std::vector<fs::path> stale;
        for (const fs::directory_entry& entry : it) {
            if (entry.is_regular_file(code) && !code &&
                EndsWithNoCase(entry.path().filename().string(), ".old")) {
                stale.push_back(entry.path());
            }
        }
        // Collected first, then deleted: removing entries while iterating
        // the same tree is asking for trouble.
        for (const fs::path& path : stale) fs::remove(path, code);
    }

    fs::remove_all(UpdateDirectory(installRoot), code);
}

bool InstallUpdate(const std::string& installRoot, const ReleaseInfo& release,
                   UpdateProgressCallback onProgress, void* context, std::string& error) {
    error.clear();
    ProgressRelay relay;
    relay.callback = onProgress;
    relay.context = context;

    const fs::path updateDir = UpdateDirectory(installRoot);
    const fs::path stagedDir = updateDir / "staged";
    const fs::path zipPath = updateDir / release.assetName;

    std::error_code code;
    fs::remove_all(updateDir, code);
    fs::create_directories(stagedDir, code);
    if (code) {
        error = "could not create " + updateDir.string() + ": " + code.message();
        return false;
    }

    Report(relay, UpdateStage::Downloading, "Downloading " + release.tag + "…");
    if (!DownloadFile(release.downloadUrl, zipPath.string(), ReportDownloadProgress, &relay,
                      error)) {
        fs::remove_all(updateDir, code);
        return false;
    }

    // Check before unpacking: an archive that is not a Dawnstar build must
    // never get as far as being moved over the install.
    Report(relay, UpdateStage::Extracting, "Checking the download…");
    std::vector<ZipEntry> index;
    if (!ReadZipIndex(zipPath.string(), index, error)) {
        fs::remove_all(updateDir, code);
        return false;
    }
    std::vector<std::string> paths;
    paths.reserve(index.size());
    for (const ZipEntry& entry : index) paths.push_back(entry.path);
    if (!LooksLikeDawnstarPackage(paths)) {
        error = "the downloaded archive does not look like a Dawnstar build";
        fs::remove_all(updateDir, code);
        return false;
    }

    Report(relay, UpdateStage::Extracting, "Unpacking…");
    if (!ExtractZip(zipPath.string(), stagedDir.string(), error)) {
        fs::remove_all(updateDir, code);
        return false;
    }

    // --- the swap ------------------------------------------------------
    //
    // Only files the archive contains are touched, which is what keeps
    // data/, user/ and launcher.cfg out of it. Each existing target is
    // renamed aside rather than deleted, because the running launcher is
    // one of them: Windows refuses to delete or overwrite a running image
    // but is perfectly happy to rename it.
    Report(relay, UpdateStage::Installing, "Installing…");
    std::vector<std::pair<fs::path, fs::path>> moves;  // staged -> target
    fs::recursive_directory_iterator walk(stagedDir, code);
    if (code) {
        error = "could not read the unpacked update";
        fs::remove_all(updateDir, code);
        return false;
    }
    for (const fs::directory_entry& entry : walk) {
        if (!entry.is_regular_file(code) || code) continue;
        const fs::path relative = fs::relative(entry.path(), stagedDir, code);
        if (code || relative.empty()) continue;
        moves.emplace_back(entry.path(), fs::path(installRoot) / relative);
    }
    if (moves.empty()) {
        error = "the update contained no files";
        fs::remove_all(updateDir, code);
        return false;
    }

    for (const auto& move : moves) {
        fs::create_directories(move.second.parent_path(), code);
        if (fs::exists(move.second, code)) {
            const fs::path aside = move.second.string() + ".old";
            fs::remove(aside, code);  // a leftover from an earlier update
            if (!MoveFileExW(Widen(move.second.string()).c_str(), Widen(aside.string()).c_str(),
                             MOVEFILE_REPLACE_EXISTING)) {
                error = "could not move " + move.second.string() +
                        " aside (is the game still running?)";
                return false;
            }
        }
        if (!MoveFileExW(Widen(move.first.string()).c_str(), Widen(move.second.string()).c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
            error = "could not install " + move.second.string();
            return false;
        }
    }

    // The staging tree has been emptied of files by the moves; the zip and
    // the directory itself go now. Any `.old` files stay until the next
    // start, when nothing has them open.
    fs::remove_all(updateDir, code);

    Report(relay, UpdateStage::Done, "Updated to " + release.tag + ".");
    return true;
}

bool RelaunchLauncher(const std::string& installRoot) {
    const fs::path exe = fs::path(installRoot) / "Dawnstar.exe";
    std::error_code code;
    if (!fs::is_regular_file(exe, code) || code) return false;

    std::wstring command = L"\"" + Widen(exe.string()) + L"\"";
    std::vector<wchar_t> mutableCommand(command.begin(), command.end());
    mutableCommand.push_back(L'\0');

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    const std::wstring workingDirectory = Widen(installRoot);
    if (!CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, FALSE, 0, nullptr,
                        workingDirectory.c_str(), &startup, &process)) {
        return false;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}

}  // namespace launcher
}  // namespace dawnstar
