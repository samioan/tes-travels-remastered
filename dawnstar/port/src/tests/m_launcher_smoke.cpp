// Launcher smoke test -- the launcher's non-visual half.
//
// The launcher is a GUI binary, so the window itself is verified by hand.
// What can be pinned down here is everything underneath it: the artwork
// blob actually decodes to the pixels it claims to, launcher.cfg survives
// a round trip, and game-data discovery/install accepts the shapes people
// really point a file dialog at while rejecting things that merely look
// plausible. Ported from shadowkey-decomp's own
// src/tests/m113_launcher_smoke.cpp, dropping every font-related check
// (Dawnstar has no font concept -- see launcher/install.h) and replacing
// the folder-discovery checks with jar-unpacking ones.
//
// Headless and window-free, so it needs no game data and no network -- the
// two things that make it safe to run in CI.

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "launcher/banner.h"
#include "launcher/install.h"
#include "launcher/launcher_config.h"
#include "launcher/update_check.h"
#include "launcher/updater.h"
#include "launcher/zip_reader.h"

namespace {

namespace fs = std::filesystem;
using namespace dawnstar::launcher;

int g_Checks = 0;
int g_Failed = 0;

void Check(bool ok, const std::string& what) {
    ++g_Checks;
    if (!ok) ++g_Failed;
    std::printf("  [%s] %s\n", ok ? "ok" : "FAIL", what.c_str());
}

std::vector<unsigned char> ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};
    return std::vector<unsigned char>((std::istreambuf_iterator<char>(file)),
                                      std::istreambuf_iterator<char>());
}

std::string ReadWholeText(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return std::string();
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

// --- a minimal zip writer, for parts 6 and 9 ----------------------------
//
// Writing a zip is much simpler than reading one, so the reader is tested
// against bytes whose every field is known here rather than against a
// checked-in binary fixture nobody can inspect.

void PutU16(std::string& out, unsigned value) {
    out.push_back(static_cast<char>(value & 0xff));
    out.push_back(static_cast<char>((value >> 8) & 0xff));
}

void PutU32(std::string& out, unsigned long value) {
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<char>((value >> (8 * i)) & 0xff));
}

unsigned long Crc32(const std::string& data) {
    unsigned long crc = 0xFFFFFFFFul;
    for (unsigned char byte : data) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320ul & (~(crc & 1) + 1));
        }
    }
    return crc ^ 0xFFFFFFFFul;
}

// A DEFLATE stream of one uncompressed block: BFINAL=1, BTYPE=00, then
// LEN/~LEN and the bytes. Still the method-8 path through puff, which is
// what needs covering -- without needing a compressor in the test.
std::string DeflateStored(const std::string& data) {
    std::string out;
    out.push_back(static_cast<char>(0x01));
    PutU16(out, static_cast<unsigned>(data.size()));
    PutU16(out, static_cast<unsigned>(~data.size() & 0xffff));
    out += data;
    return out;
}

struct ZipMember {
    std::string name;
    std::string body;
    bool deflate = false;
};

bool WriteTestZip(const std::string& path, const std::vector<ZipMember>& members) {
    struct Written {
        std::string name, payload;
        unsigned method;
        unsigned long crc, uncompressed;
        unsigned long localOffset = 0;
    };
    std::vector<Written> written;
    for (const ZipMember& member : members) {
        written.push_back({member.name, member.deflate ? DeflateStored(member.body) : member.body,
                           member.deflate ? 8u : 0u, Crc32(member.body),
                           static_cast<unsigned long>(member.body.size())});
    }

    std::string out;
    for (Written& member : written) {
        member.localOffset = static_cast<unsigned long>(out.size());
        PutU32(out, 0x04034b50);                                  // local file header
        PutU16(out, 20);                                          // version needed
        PutU16(out, 0);                                           // flags
        PutU16(out, member.method);
        PutU16(out, 0);                                           // mod time
        PutU16(out, 0);                                           // mod date
        PutU32(out, member.crc);
        PutU32(out, static_cast<unsigned long>(member.payload.size()));
        PutU32(out, member.uncompressed);
        PutU16(out, static_cast<unsigned>(member.name.size()));
        PutU16(out, 0);                                           // extra length
        out += member.name;
        out += member.payload;
    }

    const unsigned long directoryOffset = static_cast<unsigned long>(out.size());
    for (const Written& member : written) {
        PutU32(out, 0x02014b50);                                  // central file header
        PutU16(out, 20);                                          // version made by
        PutU16(out, 20);                                          // version needed
        PutU16(out, 0);                                           // flags
        PutU16(out, member.method);
        PutU16(out, 0);
        PutU16(out, 0);
        PutU32(out, member.crc);
        PutU32(out, static_cast<unsigned long>(member.payload.size()));
        PutU32(out, member.uncompressed);
        PutU16(out, static_cast<unsigned>(member.name.size()));
        PutU16(out, 0);                                           // extra
        PutU16(out, 0);                                           // comment
        PutU16(out, 0);                                           // disk number
        PutU16(out, 0);                                           // internal attrs
        PutU32(out, 0);                                           // external attrs
        PutU32(out, member.localOffset);
        out += member.name;
    }
    const unsigned long directorySize = static_cast<unsigned long>(out.size()) - directoryOffset;

    PutU32(out, 0x06054b50);                                      // end of central directory
    PutU16(out, 0);                                               // this disk
    PutU16(out, 0);                                               // disk with directory
    PutU16(out, static_cast<unsigned>(written.size()));
    PutU16(out, static_cast<unsigned>(written.size()));
    PutU32(out, directorySize);
    PutU32(out, directoryOffset);
    PutU16(out, 0);                                               // comment length

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) return false;
    file.write(out.data(), static_cast<std::streamsize>(out.size()));
    return static_cast<bool>(file);
}

// Finds a path this repo's own smoke tests conventionally assume relative
// to build/ (two levels above dawnstar/), by trying every plausible
// working directory rather than assuming one. Developer builds run this
// from build/ by hand; CI (both ci.yml and release.yml) invokes the exe by
// a path from the repo root instead, without cd'ing into build/ first --
// a real CWD mismatch this test hit in its first CI run, not a
// hypothetical one. `relativeToRepoRoot` is the same path spelled from the
// repo root, for when that's where we actually are.
std::string FindAsset(const std::string& relativeToBuildDir,
                      const std::string& relativeToRepoRoot) {
    const char* candidates[] = {
        relativeToBuildDir.c_str(),   // CWD == dawnstar/port/build(-dist)/
        relativeToRepoRoot.c_str(),   // CWD == repo root (CI's own convention)
    };
    std::error_code error;
    for (const char* candidate : candidates) {
        if (fs::is_regular_file(candidate, error) && !error) return candidate;
    }
    return relativeToBuildDir;  // let the caller's own Check() report the miss
}

// A scratch directory of our own under TEMP -- never the repo, never a
// real install.
fs::path ScratchDirectory() {
    const char* temp = std::getenv("TEMP");
    fs::path base = temp ? fs::path(temp) : fs::path(".");
    fs::path directory = base / "dawnstar_launcher_smoke";
    std::error_code error;
    fs::remove_all(directory, error);
    fs::create_directories(directory, error);
    return directory;
}

}  // namespace

int main(int, char**) {
    const fs::path scratch = ScratchDirectory();

    // -- 1. the banner artwork --
    //
    // The image is committed (assets/banner.png) and the launcher embeds
    // it as RCDATA; decoding it from the file here is the same code path
    // LoadEmbeddedBanner() takes, minus the resource lookup. Unlike
    // shadowkey-decomp's WIC-based decoder, DecodeBanner here goes through
    // stb_image and needs no COM.
    std::printf("\n-- 1. the banner artwork (stb_image -> DecodeBanner) --\n");
    const std::string bannerPath =
        FindAsset("../src/launcher/assets/banner.png",
                  "dawnstar/port/src/launcher/assets/banner.png");
    const std::vector<unsigned char> blob = ReadFile(bannerPath);
    Check(!blob.empty(), "launcher/assets/banner.png is present (looked for it at " +
                             bannerPath + ")");
    if (!blob.empty()) {
        Check(blob.size() > 8 && blob[0] == 0x89 && blob[1] == 'P' && blob[2] == 'N' &&
                  blob[3] == 'G',
              "starts with a PNG signature");

        Banner banner;
        Check(DecodeBanner(blob.data(), blob.size(), banner), "DecodeBanner succeeds");
        Check(banner.valid(), "the decoded banner is self-consistent");
        Check(banner.pixels.size() ==
                  static_cast<size_t>(banner.width) * static_cast<size_t>(banner.height),
              "one pixel per width*height, no row padding");

        // A corrupt image must leave the launcher drawing a flat background
        // rather than a screenful of uninitialised memory.
        Banner rejected;
        Check(!DecodeBanner(blob.data(), 4, rejected), "a 4-byte fragment is rejected");
        std::vector<unsigned char> corrupt = blob;
        corrupt[1] = 'X';  // no longer a PNG signature
        Check(!DecodeBanner(corrupt.data(), corrupt.size(), rejected),
              "a file with no recognisable header is rejected");
        Check(!DecodeBanner(nullptr, 1024, rejected), "a null pointer is rejected");
        Check(!DecodeBanner(blob.data(), 0, rejected), "a zero length is rejected");
    }

    // -- 2. launcher.cfg --
    std::printf("\n-- 2. launcher.cfg round trip --\n");
    {
        const std::string path = (scratch / "launcher.cfg").string();
        LauncherConfig written;
        written.gameData = "data";
        written.font = "fonts/Ceurope.gdr";
        written.scale = 4;
        Check(SaveLauncherConfig(path, written), "SaveLauncherConfig writes the file");

        LauncherConfig read;
        Check(LoadLauncherConfig(path, read), "LoadLauncherConfig reads it back");
        Check(read.gameData == written.gameData, "gameData survives the round trip");
        Check(read.font == written.font, "font survives the round trip (M78)");
        Check(read.scale == written.scale, "scale survives the round trip");

        LauncherConfig missing;
        Check(!LoadLauncherConfig((scratch / "nope.cfg").string(), missing),
              "a missing file reports failure");
        Check(missing.scale == 2 && missing.gameData.empty(),
              "...and still yields the first-run defaults");

        // Hand-edited files are a thing people do, so neither an unknown
        // key nor a nonsense value may take the launcher down with it.
        const std::string odd = (scratch / "odd.cfg").string();
        {
            std::ofstream file(odd);
            file << "# a comment\n\nsomeFutureKey=1\ngameData = spaced \nscale=99\n";
        }
        LauncherConfig loaded;
        Check(LoadLauncherConfig(odd, loaded), "a file of unknown keys still loads");
        Check(loaded.gameData == "spaced", "whitespace around a value is trimmed");
        Check(loaded.scale == 2, "an out-of-range scale keeps the default");
    }

    // -- 3. paths relative to the install root --
    std::printf("\n-- 3. install-relative paths --\n");
    {
        const std::string root = scratch.string();
        Check(ResolveAgainst(root, "data") == (scratch / "data").string(),
              "a relative path resolves against the install root");
        Check(ResolveAgainst(root, "").empty(), "an unset path stays unset");
        const std::string absolute = (scratch / "elsewhere" / "x.jar").string();
        Check(ResolveAgainst(root, absolute) == absolute, "an absolute path is left alone");

        Check(RelativeToIfInside(root, (scratch / "data").string()) == "data",
              "a path inside the install root is stored relative");
        const std::string outside = (scratch.parent_path() / "somewhere_else").string();
        Check(RelativeToIfInside(root, outside) == outside,
              "a path outside it is stored absolute");
    }

    // -- 4. finding the game --
    std::printf("\n-- 4. game-data discovery --\n");
    {
        Check(!IsGameDataRoot(scratch.string()), "an empty folder is not the game");
        Check(!IsGameDataRoot(""), "an empty path is not the game");

        // A folder holding one marker file but not the rest must not pass.
        const fs::path decoy = scratch / "decoy";
        std::error_code error;
        fs::create_directories(decoy, error);
        std::ofstream(decoy / "datfiles.lmp") << "not really";
        Check(!IsGameDataRoot(decoy.string()), "one marker file alone is not enough");

        const std::string real = "../../extracted";
        if (fs::is_directory(real, error)) {
            Check(IsGameDataRoot(real), "the real extracted/ folder is recognised");
        } else {
            std::printf("  [skip] no extracted assets at %s -- positive discovery check "
                        "skipped\n",
                        real.c_str());
        }
    }

    // -- 5. installing from a .jar --
    //
    // Against a synthetic jar rather than the real one: what is being
    // checked is that InstallGameJar unpacks it, validates the result, and
    // rejects a jar that doesn't contain the game -- none of which needs
    // the real ~1MB TEST-Dawnstar.jar.
    std::printf("\n-- 5. installing from a .jar --\n");
    {
        const fs::path jarPath = scratch / "fake.jar";
        Check(WriteTestZip(jarPath.string(),
                           {{"datfiles.lmp", "not really", false},
                            {"imgfiles.lmp", "nor this, but it inflates", true},
                            {"npcstrings.dat", "still not really", false}}),
              "a synthetic jar is written");

        const fs::path destination = scratch / "install" / "data";
        std::string error;
        Check(InstallGameJar(jarPath.string(), destination.string(), error),
              "InstallGameJar succeeds (" + error + ")");
        Check(IsGameDataRoot(destination.string()), "the unpacked jar validates as the game");
        Check(ReadWholeText((destination / "imgfiles.lmp").string()) ==
                  "nor this, but it inflates",
              "a deflated entry round-trips through puff");

        // Re-running setup on an install that is already set up is a thing
        // people do; it must overwrite cleanly rather than fail.
        Check(InstallGameJar(jarPath.string(), destination.string(), error),
              "installing again over an existing install is fine");
        Check(IsGameDataRoot(destination.string()), "...and the install still validates");

        std::string missingError;
        Check(!InstallGameJar((scratch / "does_not_exist.jar").string(), destination.string(),
                              missingError) &&
                  !missingError.empty(),
              "a missing source reports an error rather than failing silently");

        const fs::path notTheGame = scratch / "wrong.jar";
        Check(WriteTestZip(notTheGame.string(), {{"readme.txt", "hello", false}}),
              "a jar missing the game's own files is written");
        std::string wrongError;
        Check(!InstallGameJar(notTheGame.string(), (scratch / "install2").string(), wrongError) &&
                  !wrongError.empty(),
              "a jar that isn't Dawnstar is rejected after unpacking");
    }

    // -- 6. version comparison --
    std::printf("\n-- 6. version comparison --\n");
    {
        Check(CompareVersions("0.1.0", "0.2.0") < 0, "0.1.0 is older than 0.2.0");
        Check(CompareVersions("0.9.0", "0.10.0") < 0,
              "0.9.0 is older than 0.10.0 (numeric, not lexical)");
        Check(CompareVersions("0.2.0", "0.1.0") > 0, "the comparison is symmetric");
        Check(CompareVersions("0.1.0", "0.1.0") == 0, "equal versions compare equal");
        Check(CompareVersions("v0.1.0", "0.1.0") == 0, "a leading v is ignored");
        Check(CompareVersions("1.0.0", "0.999.999") > 0, "major wins over minor and patch");
        Check(CompareVersions("0.1.1", "0.1.0") > 0, "patch is compared");

        Check(CompareVersions("1.0.0-beta1", "1.0.0") < 0, "a pre-release precedes its release");
        Check(CompareVersions("1.0.0", "1.0.0-beta1") > 0, "...and the reverse");
        Check(CompareVersions("1.0.0-beta1", "1.0.0-beta2") < 0,
              "pre-releases order among themselves");
        Check(CompareVersions("0.1.0-dev", "0.1.0") < 0,
              "a -dev build is older than the release of the same number");
    }

    // -- 7. reading the release list, filtered by tag prefix --
    //
    // The real difference from shadowkey-decomp's own version of this test:
    // this repo's releases endpoint can list more than one game, so the
    // parser has to skip a release that isn't this one's.
    std::printf("\n-- 7. the GitHub release payload, tag-prefix filtered --\n");
    {
        // Two releases in the payload -- a newer oblivion-v one first, then
        // the dawnstar-v one that should actually be picked.
        const std::string json =
            R"([{"tag_name":"oblivion-v0.2.0","name":"Oblivion v0.2.0",)"
            R"("draft":false,"prerelease":true,"assets":[)"
            R"({"name":"TesTravelsRemastered-oblivion-v0.2.0-win64.zip",)"
            R"("browser_download_url":)"
            R"("https://github.com/o/r/releases/download/oblivion-v0.2.0/x.zip"}]},)"
            R"({"tag_name":"dawnstar-v0.1.0","name":"Dawnstar v0.1.0",)"
            R"("draft":false,"prerelease":true,"assets":[)"
            R"({"name":"checksums.txt","browser_download_url":)"
            R"("https://github.com/o/r/releases/download/dawnstar-v0.1.0/checksums.txt"},)"
            R"({"name":"DawnstarRemastered-dawnstar-v0.1.0-win64.zip",)"
            R"("browser_download_url":)"
            R"("https://github.com/o/r/releases/download/dawnstar-v0.1.0/DawnstarRemastered-dawnstar-v0.1.0-win64.zip"}]}])";
        ReleaseInfo info;
        Check(ParseReleaseList(json, "dawnstar-v", info),
              "the dawnstar-v release is found despite not being first in the payload");
        Check(info.tag == "dawnstar-v0.1.0", "the right tag is read");
        Check(info.version == "0.1.0", "the version drops the dawnstar-v prefix");
        Check(info.prerelease, "the prerelease flag is read");
        Check(info.assetName == "DawnstarRemastered-dawnstar-v0.1.0-win64.zip",
              "the asset name comes from the URL");
        Check(info.downloadUrl.find(".zip") != std::string::npos &&
                  info.downloadUrl.find("checksums") == std::string::npos,
              "the .zip asset is chosen, not the first asset listed");

        ReleaseInfo none;
        Check(!ParseReleaseList(json, "stormhold-v", none),
              "a prefix with no matching release is rejected, not mismatched to another game's");

        ReleaseInfo rejected;
        Check(!ParseReleaseList("[]", "dawnstar-v", rejected), "an empty release list is rejected");
        Check(!ParseReleaseList("", "dawnstar-v", rejected), "an empty body is rejected");
        Check(!ParseReleaseList(R"([{"tag_name":"dawnstar-v1.0.0","assets":[]}])", "dawnstar-v",
                                rejected),
              "a release with no assets is rejected");
        Check(!ParseReleaseList(
                  R"([{"tag_name":"dawnstar-v1.0.0","assets":[{"browser_download_url":)"
                  R"("https://x/y/source.tar.gz"}]}])",
                  "dawnstar-v", rejected),
              "a release with no .zip asset is rejected");
        // GitHub escapes nothing in these URLs today, but a parser that
        // cannot survive an escape is a parser waiting to break.
        ReleaseInfo escaped;
        Check(ParseReleaseList(
                  R"([{"tag_name":"dawnstar-v2.0.0","assets":[{"browser_download_url":)"
                  R"("https:\/\/github.com\/o\/r\/a.zip"}]}])",
                  "dawnstar-v", escaped) &&
                  escaped.downloadUrl == "https://github.com/o/r/a.zip",
              "escaped forward slashes in a URL are unescaped");
    }

    // -- 8. the zip reader --
    std::printf("\n-- 8. the zip reader --\n");
    {
        Check(IsSafeRelativePath("bin/dawnstar_port.exe"), "a normal path is safe");
        Check(!IsSafeRelativePath("../evil.exe"), "a parent-directory escape is refused");
        Check(!IsSafeRelativePath("bin/../../evil.exe"), "...including a buried one");
        Check(!IsSafeRelativePath("/etc/passwd"), "an absolute path is refused");
        Check(!IsSafeRelativePath("C:\\Windows\\System32\\evil.dll"),
              "a drive-letter path is refused");
        Check(!IsSafeRelativePath("..\\evil.exe"), "a backslash escape is refused too");
        Check(!IsSafeRelativePath(""), "an empty path is refused");

        const fs::path zipPath = scratch / "test.zip";
        const std::string storedName = "Dawnstar.exe";
        const std::string storedBody = "not really an executable";
        const std::string deflateName = "bin/dawnstar_port.exe";
        const std::string deflateBody = "nor is this one, but it inflates";
        Check(WriteTestZip(zipPath.string(), {{storedName, storedBody, false},
                                              {deflateName, deflateBody, true}}),
              "a two-entry test archive is written");

        std::vector<ZipEntry> index;
        std::string zipError;
        Check(ReadZipIndex(zipPath.string(), index, zipError),
              "ReadZipIndex succeeds (" + zipError + ")");
        Check(index.size() == 2, "both entries are listed");

        std::vector<std::string> paths;
        for (const ZipEntry& e : index) paths.push_back(e.path);
        Check(LooksLikeDawnstarPackage(paths), "the archive is recognised as a Dawnstar build");
        Check(!LooksLikeDawnstarPackage({"readme.txt"}), "an unrelated archive is not");
        Check(!LooksLikeDawnstarPackage({"Dawnstar.exe"}),
              "the launcher alone is not enough -- the game has to be there too");

        const fs::path out = scratch / "unzipped";
        Check(ExtractZip(zipPath.string(), out.string(), zipError),
              "ExtractZip succeeds (" + zipError + ")");
        Check(ReadWholeText((out / storedName).string()) == storedBody,
              "the stored entry round-trips");
        Check(ReadWholeText((out / "bin" / "dawnstar_port.exe").string()) == deflateBody,
              "the deflated entry round-trips through puff");

        std::string truncatedError;
        std::vector<ZipEntry> nothing;
        Check(!ReadZipIndex((scratch / "not_a_zip.bin").string(), nothing, truncatedError),
              "a missing file is reported, not crashed on");
        {
            std::ofstream(scratch / "not_a_zip.bin", std::ios::binary) << "definitely not a zip";
        }
        Check(!ReadZipIndex((scratch / "not_a_zip.bin").string(), nothing, truncatedError) &&
                  !truncatedError.empty(),
              "a non-zip file is rejected with a reason");
    }

    // -- 9. the dev-build guard --
    std::printf("\n-- 9. the dev-build guard --\n");
    {
        Check(!UpdatesEnabledForThisBuild("0.1.0-dev"), "a -dev build does not self-update");
        Check(!UpdatesEnabledForThisBuild(""), "an empty version does not self-update");
        Check(UpdatesEnabledForThisBuild("0.1.0"), "a release build does");
    }

    // -- 10. the device font (M78) --
    //
    // The rejections always run. The accept/copy half needs a real Nokia
    // Ceurope.gdr, which is firmware and never committed -- it runs only
    // when a developer has put their own in port/assets/fonts/.
    std::printf("\n-- 10. the device font --\n");
    {
        Check(!IsUsableFont(""), "an empty path is not a font");
        Check(!IsUsableFont((scratch / "nope.gdr").string()), "a missing file is not a font");
        std::ofstream(scratch / "fake.gdr", std::ios::binary) << "definitely not a font store";
        Check(!IsUsableFont((scratch / "fake.gdr").string()), "a file that merely ends in .gdr is not a font");
        bool italic = true;
        std::string error;
        Check(!InstallFonts((scratch / "fake.gdr").string(), (scratch / "fonts").string(), italic, error) &&
                  !error.empty(),
              "installing a non-font is refused with a reason");

        const std::string real = FindAsset("../assets/fonts/Ceurope.gdr", "dawnstar/port/assets/fonts/Ceurope.gdr");
        if (fs::is_regular_file(real)) {
            Check(IsUsableFont(real), "the real Ceurope.gdr is accepted");
            const fs::path dest = scratch / "install" / "fonts";
            const bool installed = InstallFonts(real, dest.string(), italic, error);
            Check(installed, "InstallFonts copies it" + (error.empty() ? std::string() : ": " + error));
            Check(IsUsableFont((dest / "Ceurope.gdr").string()), "...and the copy is usable");
            Check(italic == fs::is_regular_file(fs::path(real).parent_path() / "Browsereur.gdr") &&
                      italic == fs::is_regular_file(dest / "Browsereur.gdr"),
                  "Browsereur.gdr comes along exactly when it sat beside the source");
        } else {
            std::printf("  (skipped the accept/copy half: no dev copy of Ceurope.gdr)\n");
        }
    }

    std::error_code cleanup;
    fs::remove_all(scratch, cleanup);

    std::printf("\nlauncher_smoke: %d/%d checks passed -- %s\n", g_Checks - g_Failed, g_Checks,
                g_Failed == 0 ? "OK" : "FAILED");
    return g_Failed == 0 ? 0 : 1;
}
