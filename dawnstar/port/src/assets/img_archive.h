#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace dawnstar {

// Reads dawnstar's imgfiles.lmp -- a named-blob archive, but
// header-then-data rather than interleaved like datfiles.lmp (see
// ../../docs/ASSET_FORMATS.md). Renamed-source counterpart of
// ESGame.createImageFromFile(): every entry's header (same `'-'name'-'
// <u32 dataOffset><u16 size>` shape as datfiles.lmp) is packed
// contiguously at the start of the file -- read back-to-back until the
// read position reaches the *first* header's u32 value, which is how the
// reader knows where the header block ends without a separate count
// field -- followed by every entry's raw bytes concatenated in the same
// order the headers were read. Unlike datfiles.lmp, both the u32 and u16
// fields are load-bearing here (offset and byte length), and this port
// reads them once at construction rather than DatArchive's per-lookup
// rescan, since ESGame.createImageFromFile() itself is a one-shot full
// decode too (not a repeated-lookup API like getResource()).
//
// Every value is a still-PNG-encoded byte blob (Image.createImage(bytes,
// 0, size) in the Java source) -- no PNG decoding happens here, same as
// this port doesn't decode PNGs anywhere else yet.
class ImgArchive {
public:
    explicit ImgArchive(const std::string& path);

    const std::vector<uint8_t>& Data(const std::string& name) const;
    bool Contains(const std::string& name) const { return images_.count(name) != 0; }
    size_t Count() const { return images_.size(); }

private:
    std::unordered_map<std::string, std::vector<uint8_t>> images_;
};

}  // namespace dawnstar
