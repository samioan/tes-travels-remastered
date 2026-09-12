#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace dawnstar {

// Mirror of BinaryReader (see binary_reader.h) for writing: big-endian
// integers and a java.io.DataOutputStream.writeUTF()-compatible 2-byte-
// length-prefixed string. Like BinaryReader, text is written as raw
// ASCII/Latin-1 bytes rather than fully modified-UTF-8 encoded -- see
// binary_reader.h's own note on why that's fine for this game's text.
class BinaryWriter {
public:
    explicit BinaryWriter(std::vector<uint8_t>& out) : out_(out) {}

    void WriteU8(uint8_t v) { out_.push_back(v); }
    void WriteS8(int8_t v) { WriteU8(static_cast<uint8_t>(v)); }

    void WriteU16(uint16_t v) {
        WriteU8(static_cast<uint8_t>(v >> 8));
        WriteU8(static_cast<uint8_t>(v));
    }
    void WriteS16(int16_t v) { WriteU16(static_cast<uint16_t>(v)); }

    void WriteU32(uint32_t v) {
        WriteU8(static_cast<uint8_t>(v >> 24));
        WriteU8(static_cast<uint8_t>(v >> 16));
        WriteU8(static_cast<uint8_t>(v >> 8));
        WriteU8(static_cast<uint8_t>(v));
    }
    void WriteS32(int32_t v) { WriteU32(static_cast<uint32_t>(v)); }

    void WriteBool(bool v) { WriteU8(v ? 1 : 0); }

    void WriteUTF(const std::string& s) {
        WriteU16(static_cast<uint16_t>(s.size()));
        out_.insert(out_.end(), s.begin(), s.end());
    }

private:
    std::vector<uint8_t>& out_;
};

}  // namespace dawnstar
