#include "assets/dat_archive.h"

#include <stdexcept>

namespace dawnstar {

DatArchive::DatArchive(const std::string& path) : file_(path, std::ios::binary) {
    if (!file_) throw std::runtime_error("DatArchive: cannot open " + path);
}

BinaryReader DatArchive::OpenResource(const std::string& name) {
    file_.clear();
    file_.seekg(0);

    for (;;) {
        std::string entryName;
        int c = file_.get();
        while (c != '-') {
            if (c == std::char_traits<char>::eof()) {
                throw std::runtime_error("DatArchive: resource not found: " + name);
            }
            entryName += static_cast<char>(c);
            c = file_.get();
        }

        if (entryName == name) {
            uint32_t b0 = static_cast<uint8_t>(file_.get());
            uint32_t b1 = static_cast<uint8_t>(file_.get());
            uint32_t b2 = static_cast<uint8_t>(file_.get());
            uint32_t b3 = static_cast<uint8_t>(file_.get());
            uint32_t dataOffset = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;

            // u16 length field -- read but unused by this reader, matching
            // ESGame.getResource() (see ../../docs/ASSET_FORMATS.md).
            file_.get();
            file_.get();

            file_.seekg(dataOffset);
            return BinaryReader(file_);
        }
    }
}

}  // namespace dawnstar
