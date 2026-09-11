#pragma once
#include <cstdint>
#include <istream>
#include <stdexcept>
#include <string>

namespace dawnstar {

// Reads dawnstar's on-disk data tables, which are Java DataInputStream
// dumps: big-endian integers, and readUTF()'s 2-byte-length-prefixed
// string (java.io.DataOutputStream.writeUTF -- "modified UTF-8", but this
// game's text is all plain ASCII/Latin-1 range in practice, so the raw
// bytes are kept as-is rather than fully decoding modified-UTF-8's 3-byte
// NUL/surrogate encodings).
class BinaryReader {
public:
    explicit BinaryReader(std::istream& in) : in_(in) {}

    uint8_t ReadU8() { return static_cast<uint8_t>(ReadByte()); }
    int8_t ReadS8() { return static_cast<int8_t>(ReadByte()); }

    uint16_t ReadU16() {
        uint16_t hi = ReadU8();
        uint16_t lo = ReadU8();
        return static_cast<uint16_t>((hi << 8) | lo);
    }
    int16_t ReadS16() { return static_cast<int16_t>(ReadU16()); }

    uint32_t ReadU32() {
        uint32_t b0 = ReadU8();
        uint32_t b1 = ReadU8();
        uint32_t b2 = ReadU8();
        uint32_t b3 = ReadU8();
        return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    }
    int32_t ReadS32() { return static_cast<int32_t>(ReadU32()); }

    std::string ReadUTF() {
        uint16_t len = ReadU16();
        std::string s(len, '\0');
        if (len > 0) {
            in_.read(s.data(), len);
            if (!in_) throw std::runtime_error("BinaryReader: truncated readUTF");
        }
        return s;
    }

private:
    int ReadByte() {
        int b = in_.get();
        if (b == std::char_traits<char>::eof()) {
            throw std::runtime_error("BinaryReader: unexpected EOF");
        }
        return b;
    }

    std::istream& in_;
};

}  // namespace dawnstar
