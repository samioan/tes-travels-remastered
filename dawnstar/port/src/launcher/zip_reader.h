#pragma once

// The launcher's zip reader -- used twice: unpacking the `.jar` the user
// points the launcher at (a jar *is* a zip; tools/extract_jar.py's own
// loader is a plain `zipfile.extractall`, nothing more) and unpacking an
// update `.zip` downloaded from a GitHub release.
//
// A real reader rather than a shell-out, and a small one: the two things it
// must handle are the only two `Compress-Archive`/`zipfile` ever emit --
// stored (method 0) and deflate (method 8) -- and the inflater is
// `third_party/puff` (zlib's own reference decoder, vendored for exactly
// this). Shelling out to PowerShell's Expand-Archive or tar.exe would mean
// a visible console window, a dependency on execution policy, and no way
// to unit-test it. Ported from shadowkey-decomp's own
// port/src/launcher/zip_reader.h, which this is a near-verbatim copy of --
// nothing in a zip reader is specific to any one game.
//
// What it deliberately refuses:
//
//   * **Paths that escape the destination.** An entry named `..\..\evil.exe`
//     is how a zip overwrites something it was never given access to. Every
//     entry's resolved path is required to stay under the destination.
//   * **Absolute paths and drive letters**, for the same reason.
//   * **Encrypted entries**, which it cannot read and must not half-write.
//
// It does not implement zip64, so it handles archives up to 4 GB -- both a
// dawnstar `.jar` and a release `.zip` are a few MB.

#include <string>
#include <vector>

namespace dawnstar {
namespace launcher {

struct ZipEntry {
    std::string path;              // as stored, normalised to forward slashes
    unsigned long long size = 0;   // uncompressed
    bool isDirectory = false;
};

// Lists what an archive contains without writing anything.
bool ReadZipIndex(const std::string& zipPath, std::vector<ZipEntry>& entries, std::string& error);

// Extracts every entry under `destDir`, creating directories as needed and
// overwriting existing files. On failure returns false with `error` set;
// partial output may exist, which is why both callers extract to a staging
// directory rather than straight into a live install.
bool ExtractZip(const std::string& zipPath, const std::string& destDir, std::string& error);

// Exposed for the smoke test: true if `relative` stays inside a directory
// once appended to it. Rejects absolute paths, drive letters and any `..`
// component.
bool IsSafeRelativePath(const std::string& relative);

}  // namespace launcher
}  // namespace dawnstar
