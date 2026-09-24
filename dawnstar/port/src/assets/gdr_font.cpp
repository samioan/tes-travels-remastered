#include "assets/gdr_font.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iterator>

namespace dawnstar {

namespace {

constexpr uint32_t kDirectFileStoreUid = 0x10000037;
constexpr uint32_t kFontStoreFileUid = 0x10000039;
constexpr int kFnttranMetricsVersion = 42;  // KFnttranVersion: adds 5 metric bytes per bitmap
constexpr uint16_t kFillCharacterOffset = 0x7FFF;
constexpr uint32_t kReplacementCharacter = 0xF6DB;

// Bounds-checked little-endian reader over the whole file. Every read
// reports failure instead of throwing, so a truncated or foreign file just
// makes Load() return false.
class Reader {
public:
    Reader(const std::vector<uint8_t>& data, size_t pos) : data_(data), pos_(pos) {}

    bool U8(uint8_t& out) {
        if (pos_ + 1 > data_.size()) return false;
        out = data_[pos_++];
        return true;
    }
    bool I8(int& out) {
        uint8_t v;
        if (!U8(v)) return false;
        out = static_cast<int8_t>(v);
        return true;
    }
    bool U16(uint16_t& out) {
        if (pos_ + 2 > data_.size()) return false;
        out = static_cast<uint16_t>(data_[pos_] | (data_[pos_ + 1] << 8));
        pos_ += 2;
        return true;
    }
    bool U32(uint32_t& out) {
        if (pos_ + 4 > data_.size()) return false;
        out = static_cast<uint32_t>(data_[pos_]) | (static_cast<uint32_t>(data_[pos_ + 1]) << 8) |
              (static_cast<uint32_t>(data_[pos_ + 2]) << 16) |
              (static_cast<uint32_t>(data_[pos_ + 3]) << 24);
        pos_ += 4;
        return true;
    }
    bool I32(int& out) {
        uint32_t v;
        if (!U32(v)) return false;
        out = static_cast<int32_t>(v);
        return true;
    }
    bool Skip(size_t n) {
        if (pos_ + n > data_.size()) return false;
        pos_ += n;
        return true;
    }

    // TCardinality: 1, 2 or 4 bytes, the low bits saying which.
    bool Cardinality(uint32_t& out) {
        uint8_t b0;
        if (!U8(b0)) return false;
        if ((b0 & 1) == 0) {
            out = b0 >> 1;
            return true;
        }
        if ((b0 & 2) == 0) {
            uint8_t b1;
            if (!U8(b1)) return false;
            out = (static_cast<uint32_t>(b0) | (static_cast<uint32_t>(b1) << 8)) >> 2;
            return true;
        }
        uint8_t b1, b2, b3;
        if (!U8(b1) || !U8(b2) || !U8(b3)) return false;
        out = (static_cast<uint32_t>(b0) | (static_cast<uint32_t>(b1) << 8) |
               (static_cast<uint32_t>(b2) << 16) | (static_cast<uint32_t>(b3) << 24)) >> 3;
        return true;
    }

    // A 16-bit descriptor as Symbian streams externalize it: a cardinality
    // (length << 1) then the text in Symbian's SCSU-style compression. Only
    // the two forms these ASCII typeface names use are handled -- plain
    // single-byte passthrough, and SCU (0x0F, "switch to Unicode") followed
    // by raw UTF-16BE -- and anything else is reported as a failure rather
    // than guessed at. Non-ASCII code units come back as '?'.
    bool Descriptor16(std::string& out) {
        uint32_t header;
        if (!Cardinality(header)) return false;
        const uint32_t length = header >> 1;
        out.clear();
        bool unicodeMode = false;
        while (out.size() < length) {
            uint8_t b;
            if (!U8(b)) return false;
            if (unicodeMode) {
                if (b >= 0xE0 && b <= 0xF2) return false;  // window tags -- not used by these files
                uint8_t lo;
                if (!U8(lo)) return false;
                const uint16_t unit = static_cast<uint16_t>((b << 8) | lo);
                out.push_back(unit < 0x80 ? static_cast<char>(unit) : '?');
            } else if (b == 0x0F) {
                unicodeMode = true;
            } else if (b == 0x00 || b == 0x09 || b == 0x0A || b == 0x0D || (b >= 0x20 && b < 0x80)) {
                out.push_back(static_cast<char>(b));
            } else {
                return false;
            }
        }
        return true;
    }

    size_t Pos() const { return pos_; }
    const uint8_t* At(size_t pos) const { return data_.data() + pos; }
    size_t Size() const { return data_.size(); }

private:
    const std::vector<uint8_t>& data_;
    size_t pos_;
};

struct Metric {
    int ascent = 0, height = 0, leftAdjust = 0, move = 0, rightAdjust = 0;
};

struct CodeSection {
    uint16_t first = 0, last = 0;
    uint32_t offsetsId = 0, bitmapId = 0;
};

struct FontBitmap {
    uint32_t uid = 0;
    int cellHeight = 0, ascent = 0;
    uint32_t metricsId = 0;
    int metricsCount = 0;
    std::vector<CodeSection> sections;
};

struct Typeface {
    std::string name;
    std::vector<uint32_t> bitmapUids;
};

struct Store {
    int version = 0;
    std::vector<FontBitmap> bitmaps;
    std::vector<Typeface> typefaces;
};

bool ReadFile(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    out.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return true;
}

bool ParseStore(const std::vector<uint8_t>& data, Store& store) {
    Reader header(data, 0);
    uint32_t uid1, uid2, uid3, checksum, rootId;
    if (!header.U32(uid1) || !header.U32(uid2) || !header.U32(uid3) || !header.U32(checksum) ||
        !header.U32(rootId))
        return false;
    if (uid1 != kDirectFileStoreUid || uid2 != kFontStoreFileUid) return false;

    Reader root(data, rootId);
    uint32_t collectionUid, dataId;
    int pixelAspect;
    if (!root.I32(store.version) || !root.U32(collectionUid) || !root.I32(pixelAspect) ||
        !root.U32(dataId))
        return false;

    Reader r(data, dataId);
    uint32_t bitmapCount;
    if (!r.U32(bitmapCount) || bitmapCount > 1024) return false;
    store.bitmaps.resize(bitmapCount);
    for (FontBitmap& fb : store.bitmaps) {
        int posture, weight, proportional, maxWidth, maxNormalWidth, encoding, sectionCount;
        if (!r.U32(fb.uid) || !r.I8(posture) || !r.I8(weight) || !r.I8(proportional) ||
            !r.I8(fb.cellHeight) || !r.I8(fb.ascent) || !r.I8(maxWidth) || !r.I8(maxNormalWidth))
            return false;
        if (store.version >= kFnttranMetricsVersion && !r.Skip(5)) return false;
        if (!r.I32(encoding) || !r.U32(fb.metricsId) || !r.I32(fb.metricsCount) ||
            !r.I32(sectionCount) || sectionCount < 0 || sectionCount > 4096)
            return false;
        fb.sections.resize(static_cast<size_t>(sectionCount));
        for (CodeSection& s : fb.sections) {
            if (!r.U16(s.first) || !r.U16(s.last) || !r.U32(s.offsetsId) || !r.U32(s.bitmapId))
                return false;
        }
    }

    uint32_t typefaceCount;
    if (!r.U32(typefaceCount) || typefaceCount > 1024) return false;
    store.typefaces.resize(typefaceCount);
    for (Typeface& tf : store.typefaces) {
        uint8_t flags;
        uint32_t count;
        if (!r.Descriptor16(tf.name) || !r.U8(flags) || !r.U32(count) || count > 1024) return false;
        for (uint32_t i = 0; i < count; i++) {
            uint32_t uid;
            int widthFactor, heightFactor;
            if (!r.U32(uid) || !r.I8(widthFactor) || !r.I8(heightFactor)) return false;
            tf.bitmapUids.push_back(uid);
        }
    }
    return true;
}

// Reads one glyph whose data starts at `pos` -- CBitmapFont's metric-index
// prefix, then CFbsBitGc::DoDrawCharacter's row decoding.
bool DecodeGlyph(const std::vector<uint8_t>& data, size_t pos, size_t end,
                 const std::vector<Metric>& metrics, GdrGlyph& out) {
    if (pos >= end) return false;
    size_t index = data[pos] >> 1;
    if (data[pos] & 1) {
        if (pos + 1 >= end) return false;
        index += static_cast<size_t>(data[pos + 1]) * 128;
        pos += 2;
    } else {
        pos += 1;
    }
    if (index >= metrics.size()) return false;
    const Metric& m = metrics[index];

    out = GdrGlyph{};
    out.width = m.move - m.leftAdjust - m.rightAdjust;
    out.height = m.height;
    out.leftBearing = m.leftAdjust;
    out.advance = m.move;
    out.ascent = m.ascent;
    if (out.width <= 0 || out.height <= 0) {
        out.width = out.height = 0;  // e.g. space: advance only
        return true;
    }
    out.bits.assign(static_cast<size_t>(out.width) * out.height, 0);

    const size_t bitLimit = (end - pos) * 8;
    size_t bit = 0;
    auto readBits = [&](int count, uint32_t& value) {
        if (bit + static_cast<size_t>(count) > bitLimit) return false;
        value = 0;
        for (int i = 0; i < count; i++, bit++) {
            value |= static_cast<uint32_t>((data[pos + (bit >> 3)] >> (bit & 7)) & 1) << i;
        }
        return true;
    };
    auto readRow = [&](int row) {
        for (int x = 0; x < out.width; x++) {
            uint32_t v;
            if (!readBits(1, v)) return false;
            out.bits[static_cast<size_t>(row) * out.width + x] = static_cast<uint8_t>(v);
        }
        return true;
    };

    int row = 0;
    while (row < out.height) {
        uint32_t header;
        if (!readBits(5, header)) return false;
        const bool distinctRows = (header & 1) != 0;
        const int count = static_cast<int>(header >> 1);
        if (count == 0) return false;  // malformed; would never advance
        if (distinctRows) {
            for (int i = 0; i < count && row < out.height; i++, row++) {
                if (!readRow(row)) return false;
            }
        } else {
            if (!readRow(row)) return false;
            for (int i = 1; i < count && row + i < out.height; i++) {
                std::copy_n(out.bits.begin() + static_cast<ptrdiff_t>(row) * out.width, out.width,
                            out.bits.begin() + static_cast<ptrdiff_t>(row + i) * out.width);
            }
            row += count;
        }
    }
    return true;
}

}  // namespace

bool GdrFont::Load(const std::string& path, const std::string& typefaceName) {
    loaded_ = false;
    glyphs_.clear();

    std::vector<uint8_t> data;
    if (!ReadFile(path, data)) return false;
    Store store;
    if (!ParseStore(data, store)) {
        std::printf("GdrFont: %s is not a readable Symbian font store\n", path.c_str());
        return false;
    }

    const FontBitmap* bitmap = nullptr;
    for (const Typeface& tf : store.typefaces) {
        if (tf.name != typefaceName || tf.bitmapUids.empty()) continue;
        for (const FontBitmap& fb : store.bitmaps) {
            if (fb.uid == tf.bitmapUids[0]) bitmap = &fb;
        }
    }
    if (!bitmap) {
        std::printf("GdrFont: %s has no typeface '%s'\n", path.c_str(), typefaceName.c_str());
        return false;
    }

    std::vector<Metric> metrics;
    Reader m(data, bitmap->metricsId);
    int metricsSize;
    if (!m.I32(metricsSize)) return false;
    metrics.resize(static_cast<size_t>(bitmap->metricsCount));
    for (Metric& metric : metrics) {
        if (!m.I8(metric.ascent) || !m.I8(metric.height) || !m.I8(metric.leftAdjust) ||
            !m.I8(metric.move) || !m.I8(metric.rightAdjust))
            return false;
    }

    std::vector<uint32_t> fillCodes;
    for (const CodeSection& section : bitmap->sections) {
        Reader offsets(data, section.offsetsId);
        uint32_t offsetCount;
        if (!offsets.U32(offsetCount)) return false;
        Reader bitmapStream(data, section.bitmapId);
        uint32_t bitmapSize;
        if (!bitmapStream.U32(bitmapSize)) return false;
        const size_t bitmapStart = bitmapStream.Pos();
        const size_t bitmapEnd = bitmapStart + bitmapSize;
        if (bitmapEnd > data.size()) return false;

        for (uint32_t code = section.first; code <= section.last; code++) {
            if (code - section.first >= offsetCount) return false;
            uint16_t offset;
            if (!offsets.U16(offset)) return false;
            if (offset == kFillCharacterOffset) {
                fillCodes.push_back(code);
                continue;
            }
            GdrGlyph glyph;
            if (!DecodeGlyph(data, bitmapStart + offset, bitmapEnd, metrics, glyph)) return false;
            glyphs_[code] = std::move(glyph);
        }
    }
    // CFontBitmap::CharacterMetrics: a fill code draws KReplacementCharacter.
    auto replacement = glyphs_.find(kReplacementCharacter);
    if (replacement != glyphs_.end()) {
        const GdrGlyph copy = replacement->second;
        for (uint32_t code : fillCodes) glyphs_[code] = copy;
    }

    cellHeight_ = bitmap->cellHeight;
    ascent_ = bitmap->ascent;
    loaded_ = true;
    return true;
}

const GdrGlyph* GdrFont::GetGlyph(uint32_t code) const {
    auto it = glyphs_.find(code);
    return it == glyphs_.end() ? nullptr : &it->second;
}

std::vector<std::string> GdrFont::TypefaceNames(const std::string& path) {
    std::vector<std::string> names;
    std::vector<uint8_t> data;
    Store store;
    if (!ReadFile(path, data) || !ParseStore(data, store)) return names;
    for (const Typeface& tf : store.typefaces) names.push_back(tf.name);
    return names;
}

}  // namespace dawnstar
