#pragma once

// Replacing the install with a newer one, including the executable doing
// the replacing. Ported from dawnstar's own port/src/launcher/updater.h/
// .cpp (itself from shadowkey-decomp) -- none of this is game-specific,
// only the package-shape check (LooksLikeStormholdPackage) and the
// relaunch target name.
//
// The awkward part is that `Stormhold.exe` cannot overwrite itself while
// it is running -- but Windows *will* let a running image be **renamed**.
// So the swap is: rename each file that is being replaced to `<name>.old`,
// move the new one into its place, relaunch, and delete the leftovers on
// the next start (by which time nothing has them open).
//
// Everything is staged under `update/` first and only moved into place once
// the download has been unpacked and checked, so a failed or interrupted
// download cannot leave a half-replaced install. The player's own files are
// never touched: only paths present in the downloaded archive are moved,
// and the archive contains no `data/`, `user/` or `launcher.cfg`.

#include <string>
#include <vector>

#include "launcher/update_check.h"

namespace stormhold {
namespace launcher {

// Progress for the whole operation, reported to the UI thread.
enum class UpdateStage {
    Downloading,
    Extracting,
    Installing,
    Done,
    Failed,
};

struct UpdateProgress {
    UpdateStage stage = UpdateStage::Downloading;
    unsigned long long bytesSoFar = 0;
    unsigned long long bytesTotal = 0;  // 0 when the server did not say
    std::string message;
};

using UpdateProgressCallback = void (*)(const UpdateProgress&, void*);

// Deletes the `.old` files a previous update left behind. Cheap, and safe
// to call on every start -- anything still locked is simply skipped and
// tried again next time.
void CleanUpPreviousUpdate(const std::string& installRoot);

// True if this build should offer updates at all. A developer build (a
// version carrying `-dev`) says no: its install layout is a build tree, and
// dropping a release on top of it would overwrite the binaries someone is
// working on.
bool UpdatesEnabledForThisBuild(const std::string& version);

// Downloads, verifies and installs `release` into `installRoot`. Blocking
// -- run it on a worker thread. On success the caller should relaunch (see
// RelaunchLauncher) because the running executable has just been replaced.
bool InstallUpdate(const std::string& installRoot, const ReleaseInfo& release,
                   UpdateProgressCallback onProgress, void* context, std::string& error);

// Checks that an unpacked archive actually looks like a Stormhold build
// before anything is overwritten with it. Exposed for the smoke test.
bool LooksLikeStormholdPackage(const std::vector<std::string>& entryPaths);

// Starts the (new) launcher from `installRoot` and returns true if it
// started. The caller then exits, leaving the replacement running.
bool RelaunchLauncher(const std::string& installRoot);

}  // namespace launcher
}  // namespace stormhold
