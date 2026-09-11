#include "assets/img_archive.h"

#include <fstream>
#include <stdexcept>

namespace dawnstar {

namespace {

struct HeaderEntry {
    std::string name;
    uint32_t offset;
    uint16_t size;
};

int ReadByteOrThrow(std::ifstream& file) {
    int c = file.get();
    if (c == std::char_traits<char>::eof()) {
        throw std::runtime_error("ImgArchive: unexpected EOF reading header");
    }
    return c;
}

}  // namespace

ImgArchive::ImgArchive(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("ImgArchive: cannot open " + path);

    // Header block: read '-'-terminated names + <u32 offset><u16 size>
    // back-to-back until the read position reaches the first entry's own
    // recorded offset -- ESGame.createImageFromFile()'s exact loop
    // condition (`while (var4 == -1 || var5 < var4)`).
    std::vector<HeaderEntry> headers;
    int64_t bytesRead = 0;
    int64_t firstOffset = -1;

    while (firstOffset < 0 || bytesRead < firstOffset) {
        std::string name;
        int c = ReadByteOrThrow(file);
        bytesRead++;
        while (c != '-') {
            name += static_cast<char>(c);
            c = ReadByteOrThrow(file);
            bytesRead++;
        }

        // Matches the Java source's `if (var12.length() > 1)` gate --
        // preserved as-is even though real data never seems to hit the
        // else branch (a real name is never that short).
        if (name.size() > 1) {
            uint32_t b0 = static_cast<uint8_t>(ReadByteOrThrow(file));
            uint32_t b1 = static_cast<uint8_t>(ReadByteOrThrow(file));
            uint32_t b2 = static_cast<uint8_t>(ReadByteOrThrow(file));
            uint32_t b3 = static_cast<uint8_t>(ReadByteOrThrow(file));
            uint32_t offset = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;

            uint32_t s0 = static_cast<uint8_t>(ReadByteOrThrow(file));
            uint32_t s1 = static_cast<uint8_t>(ReadByteOrThrow(file));
            uint16_t size = static_cast<uint16_t>((s0 << 8) | s1);

            bytesRead += 6;
            headers.push_back({name, offset, size});
            if (firstOffset < 0) firstOffset = offset;
        }
    }

    // The read position is now exactly at the first entry's data (that's
    // what the loop above just proved: bytesRead >= firstOffset, and by
    // construction of a well-formed archive, ==). Data blocks follow
    // sequentially in the same order their headers were read in, so no
    // sorting/seeking is needed -- just read each in turn.
    for (const HeaderEntry& h : headers) {
        std::vector<uint8_t> data(h.size);
        file.read(reinterpret_cast<char*>(data.data()), h.size);
        if (!file) throw std::runtime_error("ImgArchive: truncated image data for " + h.name);
        images_[h.name] = std::move(data);
    }
}

const std::vector<uint8_t>& ImgArchive::Data(const std::string& name) const {
    auto it = images_.find(name);
    if (it == images_.end()) throw std::runtime_error("ImgArchive: no such image: " + name);
    return it->second;
}

}  // namespace dawnstar
