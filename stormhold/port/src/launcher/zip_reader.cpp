#include "launcher/zip_reader.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>

extern "C" {
#include "puff.h"
}

namespace stormhold {
namespace launcher {

namespace {

namespace fs = std::filesystem;

// --- little-endian readers over a byte vector -------------------------

uint16_t ReadU16(const std::vector<unsigned char>& data, size_t at) {
    if (at + 2 > data.size()) return 0;
    return static_cast<uint16_t>(data[at] | (data[at + 1] << 8));
}

uint32_t ReadU32(const std::vector<unsigned char>& data, size_t at) {
    if (at + 4 > data.size()) return 0;
    return static_cast<uint32_t>(data[at]) | (static_cast<uint32_t>(data[at + 1]) << 8) |
           (static_cast<uint32_t>(data[at + 2]) << 16) |
           (static_cast<uint32_t>(data[at + 3]) << 24);
}

constexpr uint32_t kEndOfCentralDirectory = 0x06054b50;
constexpr uint32_t kCentralFileHeader = 0x02014b50;
constexpr uint32_t kLocalFileHeader = 0x04034b50;

// A parsed central-directory record, which is the authoritative index --
// the local headers are allowed to carry zeroed sizes (the streaming case,
// with a data descriptor after the payload), so sizes are taken from here.
struct CentralEntry {
    std::string name;
    uint16_t method = 0;
    uint16_t flags = 0;
    uint32_t compressedSize = 0;
    uint32_t uncompressedSize = 0;
    uint32_t localHeaderOffset = 0;
};

bool ReadWholeFile(const std::string& path, std::vector<unsigned char>& out, std::string& error) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        error = "could not open " + path;
        return false;
    }
    const std::streamoff size = file.tellg();
    if (size <= 0) {
        error = path + " is empty";
        return false;
    }
    // A generous cap keeps a corrupt or hostile file from being read
    // entirely into memory -- neither a real stormhold .jar nor a release
    // zip comes anywhere close to this.
    if (size > 512ll * 1024 * 1024) {
        error = path + " is implausibly large for this reader";
        return false;
    }
    out.resize(static_cast<size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(out.data()), size);
    if (!file) {
        error = "could not read " + path;
        return false;
    }
    return true;
}

// The end-of-central-directory record sits at the very end, after a
// variable-length comment, so it is found by scanning backwards for its
// signature rather than by seeking to a fixed offset.
bool FindEndOfCentralDirectory(const std::vector<unsigned char>& data, size_t& at) {
    if (data.size() < 22) return false;
    const size_t maxComment = 65535;
    const size_t lowest = data.size() > maxComment + 22 ? data.size() - maxComment - 22 : 0;
    for (size_t candidate = data.size() - 22 + 1; candidate-- > lowest;) {
        if (ReadU32(data, candidate) == kEndOfCentralDirectory) {
            at = candidate;
            return true;
        }
    }
    return false;
}

bool ParseCentralDirectory(const std::vector<unsigned char>& data,
                           std::vector<CentralEntry>& entries, std::string& error) {
    size_t endRecord = 0;
    if (!FindEndOfCentralDirectory(data, endRecord)) {
        error = "not a zip archive (no end-of-central-directory record)";
        return false;
    }
    const uint16_t count = ReadU16(data, endRecord + 10);
    const uint32_t directorySize = ReadU32(data, endRecord + 12);
    const uint32_t directoryOffset = ReadU32(data, endRecord + 16);
    if (directoryOffset > data.size() ||
        static_cast<size_t>(directoryOffset) + directorySize > data.size()) {
        error = "the zip's central directory is outside the file";
        return false;
    }

    size_t at = directoryOffset;
    entries.clear();
    entries.reserve(count);
    for (uint16_t i = 0; i < count; ++i) {
        if (at + 46 > data.size() || ReadU32(data, at) != kCentralFileHeader) {
            error = "the zip's central directory is malformed";
            return false;
        }
        CentralEntry entry;
        entry.flags = ReadU16(data, at + 8);
        entry.method = ReadU16(data, at + 10);
        entry.compressedSize = ReadU32(data, at + 20);
        entry.uncompressedSize = ReadU32(data, at + 24);
        const uint16_t nameLength = ReadU16(data, at + 28);
        const uint16_t extraLength = ReadU16(data, at + 30);
        const uint16_t commentLength = ReadU16(data, at + 32);
        entry.localHeaderOffset = ReadU32(data, at + 42);
        if (at + 46 + nameLength > data.size()) {
            error = "the zip's central directory is truncated";
            return false;
        }
        entry.name.assign(reinterpret_cast<const char*>(data.data()) + at + 46, nameLength);
        entries.push_back(std::move(entry));
        at += 46 + nameLength + extraLength + commentLength;
    }
    return true;
}

// Locates an entry's payload, which starts after its *local* header --
// whose name and extra fields have their own lengths, independent of the
// central directory's.
bool LocatePayload(const std::vector<unsigned char>& data, const CentralEntry& entry,
                   size_t& payloadAt, std::string& error) {
    const size_t at = entry.localHeaderOffset;
    if (at + 30 > data.size() || ReadU32(data, at) != kLocalFileHeader) {
        error = "entry '" + entry.name + "' has no local header";
        return false;
    }
    const uint16_t nameLength = ReadU16(data, at + 26);
    const uint16_t extraLength = ReadU16(data, at + 28);
    payloadAt = at + 30 + nameLength + extraLength;
    if (payloadAt + entry.compressedSize > data.size()) {
        error = "entry '" + entry.name + "' runs past the end of the file";
        return false;
    }
    return true;
}

bool Inflate(const std::vector<unsigned char>& data, size_t at, const CentralEntry& entry,
             std::vector<unsigned char>& out, std::string& error) {
    out.assign(entry.uncompressedSize, 0);
    if (entry.uncompressedSize == 0) return true;

    unsigned long destLen = entry.uncompressedSize;
    unsigned long sourceLen = entry.compressedSize;
    // A zip's deflate member is raw -- no zlib header, no adler-32 trailer
    // -- which is exactly puff's input format.
    const int result = puff(out.data(), &destLen, data.data() + at, &sourceLen);
    if (result != 0 || destLen != entry.uncompressedSize) {
        error = "entry '" + entry.name + "' did not decompress cleanly";
        return false;
    }
    return true;
}

std::string NormaliseSlashes(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

}  // namespace

bool IsSafeRelativePath(const std::string& relative) {
    if (relative.empty()) return false;
    const std::string path = NormaliseSlashes(relative);
    // An absolute path, a UNC path, or a drive letter all escape the
    // destination directory outright.
    if (path[0] == '/') return false;
    if (path.size() >= 2 && path[1] == ':') return false;
    if (path.find("//") == 0) return false;

    size_t start = 0;
    while (start <= path.size()) {
        const size_t slash = path.find('/', start);
        const std::string component =
            path.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
        if (component == "..") return false;
        if (slash == std::string::npos) break;
        start = slash + 1;
    }
    return true;
}

bool ReadZipIndex(const std::string& zipPath, std::vector<ZipEntry>& entries,
                  std::string& error) {
    entries.clear();
    error.clear();
    std::vector<unsigned char> data;
    if (!ReadWholeFile(zipPath, data, error)) return false;
    std::vector<CentralEntry> central;
    if (!ParseCentralDirectory(data, central, error)) return false;

    for (const CentralEntry& entry : central) {
        ZipEntry listed;
        listed.path = NormaliseSlashes(entry.name);
        listed.size = entry.uncompressedSize;
        listed.isDirectory = !listed.path.empty() && listed.path.back() == '/';
        entries.push_back(std::move(listed));
    }
    return true;
}

bool ExtractZip(const std::string& zipPath, const std::string& destDir, std::string& error) {
    error.clear();
    std::vector<unsigned char> data;
    if (!ReadWholeFile(zipPath, data, error)) return false;
    std::vector<CentralEntry> central;
    if (!ParseCentralDirectory(data, central, error)) return false;

    std::error_code code;
    fs::create_directories(fs::path(destDir), code);
    if (code) {
        error = "could not create " + destDir + ": " + code.message();
        return false;
    }

    for (const CentralEntry& entry : central) {
        const std::string name = NormaliseSlashes(entry.name);
        if (!IsSafeRelativePath(name)) {
            error = "the archive contains an unsafe path: '" + entry.name + "'";
            return false;
        }
        // Bit 0 of the general-purpose flags means the entry is encrypted.
        if (entry.flags & 0x0001) {
            error = "entry '" + entry.name + "' is encrypted";
            return false;
        }

        const fs::path target = fs::path(destDir) / fs::path(name);
        if (!name.empty() && name.back() == '/') {
            fs::create_directories(target, code);
            if (code) {
                error = "could not create " + target.string() + ": " + code.message();
                return false;
            }
            continue;
        }

        fs::create_directories(target.parent_path(), code);
        if (code) {
            error = "could not create " + target.parent_path().string() + ": " + code.message();
            return false;
        }

        size_t payloadAt = 0;
        if (!LocatePayload(data, entry, payloadAt, error)) return false;

        std::vector<unsigned char> contents;
        if (entry.method == 0) {
            if (entry.compressedSize != entry.uncompressedSize) {
                error = "entry '" + entry.name + "' is stored but its sizes disagree";
                return false;
            }
            contents.assign(data.begin() + payloadAt,
                            data.begin() + payloadAt + entry.compressedSize);
        } else if (entry.method == 8) {
            if (!Inflate(data, payloadAt, entry, contents, error)) return false;
        } else {
            error = "entry '" + entry.name + "' uses compression method " +
                    std::to_string(entry.method) + ", which this reader does not implement";
            return false;
        }

        std::ofstream out(target, std::ios::binary | std::ios::trunc);
        if (!out) {
            error = "could not write " + target.string();
            return false;
        }
        if (!contents.empty()) {
            out.write(reinterpret_cast<const char*>(contents.data()),
                      static_cast<std::streamsize>(contents.size()));
        }
        if (!out) {
            error = "could not finish writing " + target.string();
            return false;
        }
    }
    return true;
}

}  // namespace launcher
}  // namespace stormhold
