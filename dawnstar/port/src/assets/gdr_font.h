#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace dawnstar {

// One typeface out of a Symbian OS bitmap font store (`.gdr`) -- the Nokia
// 3650's own ROM fonts, which the original MIDlet's text really rendered
// in. Nokia device firmware, so never committed or shipped: the launcher
// asks the player for their own copy (launcher/install.h), and
// graphics/bitmap_font.h falls back to a stand-in when there is none.
//
// Why these files and these typefaces: the phone's MIDP runtime
// (kmidrun.dll, `CMIDFont.cpp`, in Nokia's Series 60 MIDP SDK 1.2.1
// emulator) ignores `Font.getFont`'s face argument and maps size x style
// through a fixed table onto ROM typefaces -- see graphics/bitmap_font.h's
// Face enum for the table and docs/PORT_ROADMAP.md (M78) for how it was
// read out of the runtime.
//
// Format -- a clean implementation from Nokia's own EPL-licensed Symbian
// sources (textandloc fontservices/fontstore FNTBODY.CPP/FNTSTORE.CPP for
// the layout, graphics bitgdi TEXT.CPP `CFbsBitGc::DoDrawCharacter` for
// glyph decoding), not derived from any GPL parser:
//   A direct file store: UIDs 0x10000037/0x10000039 at offset 0, root
//   stream id at offset 16. Stream ids ARE file offsets.
//   root stream: fnttran version (39 on S60 v1 -- "KFnttran7650Version"),
//     collection uid, pixel aspect, data stream id.
//   data stream: font bitmap count, then per bitmap: uid, posture, stroke
//     weight, proportional, cell height, ascent, max char width, max normal
//     char width, [5 extra metric bytes if version >= 42], bitmap encoding,
//     metrics stream id + count, code section count, then per section:
//     u16 first/last code, offsets stream id, bitmap stream id. Then the
//     typeface list: SCSU-compressed name, flags, and (uid, width factor,
//     height factor) per bitmap.
//   metrics stream: i32 size, then per metric: ascent, height, left adjust,
//     move (advance), right adjust -- all int8.
//   offsets stream: i32 count, then u16 byte offset per code into the
//     section's bitmap stream (0x7FFF = "fill", draw U+F6DB instead).
//   bitmap stream: i32 size, then per character a 1- or 2-byte metric
//     index (bit 0 selects), then rows LSB-first: a 5-bit header (bit 0 =
//     the next `count` rows are all different, else one row repeated
//     `count` times; bits 1-4 = count) followed by the row bits.
struct GdrGlyph {
    int width = 0;
    int height = 0;
    int leftBearing = 0;  // pen x -> bitmap left edge
    int advance = 0;      // pen x movement after this glyph
    int ascent = 0;       // bitmap top -> baseline
    std::vector<uint8_t> bits;  // width*height, row-major, 1 = ink
};

class GdrFont {
public:
    // Parses `path` and keeps the typeface named `typefaceName` (exact,
    // case-sensitive, e.g. "LatinBold12"). Non-fatal: false on a missing
    // file, anything that isn't a font store, or no such typeface, leaving
    // this font unloaded.
    bool Load(const std::string& path, const std::string& typefaceName);

    bool IsLoaded() const { return loaded_; }
    int CellHeight() const { return cellHeight_; }
    int Ascent() const { return ascent_; }

    // nullptr when this typeface has nothing for `code` (fill codes resolve
    // to U+F6DB, the store's own replacement glyph).
    const GdrGlyph* GetGlyph(uint32_t code) const;

    // Every typeface name in the store at `path`, in file order -- for
    // diagnostics ("that file has no LatinBold12; it has ...") and tests.
    static std::vector<std::string> TypefaceNames(const std::string& path);

private:
    bool loaded_ = false;
    int cellHeight_ = 0;
    int ascent_ = 0;
    std::map<uint32_t, GdrGlyph> glyphs_;
};

}  // namespace dawnstar
