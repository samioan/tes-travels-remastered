#pragma once
#include <cstdint>
#include <ostream>
#include <string>

namespace stormhold {

// Mirror of BinaryReader (see binary_reader.h) for writing: big-endian
// integers and a java.io.DataOutputStream.writeUTF()-compatible 2-byte-
// length-prefixed string. Like BinaryReader, text is written as raw
// ASCII/Latin-1 bytes rather than fully modified-UTF-8 encoded -- see
// BinaryReader's own note on why that's fine for this game's text.
//
// No `WriteU64`/`WriteS64` -- nothing in this port needs one on its own;
// Monster.java's `writeTo()` writes its 8-byte `unconfirmedTimestamp` as
// two `WriteU32` halves instead (see monster/monster_runtime.cpp's
// `WriteTo`), matching dawnstar's own port's identical choice for the
// same field.
class BinaryWriter {
public:
    explicit BinaryWriter(std::ostream& out) : out_(out) {}

    void WriteU8(uint8_t v) { out_.put(static_cast<char>(v)); }
    void WriteS8(int8_t v) { WriteU8(static_cast<uint8_t>(v)); }
    void WriteBool(bool v) { WriteU8(v ? 1 : 0); }

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

    void WriteUTF(const std::string& s) {
        WriteU16(static_cast<uint16_t>(s.size()));
        out_.write(s.data(), static_cast<std::streamsize>(s.size()));
    }

private:
    std::ostream& out_;
};

}  // namespace stormhold
