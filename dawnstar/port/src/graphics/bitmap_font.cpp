#include "graphics/bitmap_font.h"

#include <cstdint>

namespace dawnstar {

namespace {

// Glyph order: space, ', -, !, then A-Z. Each glyph is 7 rows, each
// row's low 4 bits are its pixels (bit 3 = leftmost column, bit 0 =
// rightmost) -- see bitmap_font.h's own doc comment: hand-authored,
// not decompiled data. 4 wide (not the more typical 5) specifically so
// a 1px inter-character gap fits within a still-narrow enough per-
// character advance for real popup content to wrap the way
// bitmap_font.h's own kAdvance doc comment explains.
// clang-format off
constexpr uint8_t kGlyphRows[40][7] = {
    // space
    {0b0000, 0b0000, 0b0000, 0b0000, 0b0000, 0b0000, 0b0000},
    // '
    {0b0100, 0b0100, 0b0000, 0b0000, 0b0000, 0b0000, 0b0000},
    // -
    {0b0000, 0b0000, 0b0000, 0b0110, 0b0000, 0b0000, 0b0000},
    // !
    {0b0100, 0b0100, 0b0100, 0b0100, 0b0000, 0b0100, 0b0000},
    // A
    {0b0110, 0b1001, 0b1001, 0b1111, 0b1001, 0b1001, 0b1001},
    // B
    {0b1110, 0b1001, 0b1001, 0b1110, 0b1001, 0b1001, 0b1110},
    // C
    {0b0111, 0b1000, 0b1000, 0b1000, 0b1000, 0b1000, 0b0111},
    // D
    {0b1110, 0b1001, 0b1001, 0b1001, 0b1001, 0b1001, 0b1110},
    // E
    {0b1111, 0b1000, 0b1000, 0b1110, 0b1000, 0b1000, 0b1111},
    // F
    {0b1111, 0b1000, 0b1000, 0b1110, 0b1000, 0b1000, 0b1000},
    // G
    {0b0111, 0b1000, 0b1000, 0b1011, 0b1001, 0b1001, 0b0111},
    // H
    {0b1001, 0b1001, 0b1001, 0b1111, 0b1001, 0b1001, 0b1001},
    // I
    {0b1111, 0b0100, 0b0100, 0b0100, 0b0100, 0b0100, 0b1111},
    // J
    {0b0011, 0b0001, 0b0001, 0b0001, 0b0001, 0b1001, 0b0110},
    // K
    {0b1001, 0b1010, 0b1100, 0b1000, 0b1100, 0b1010, 0b1001},
    // L
    {0b1000, 0b1000, 0b1000, 0b1000, 0b1000, 0b1000, 0b1111},
    // M
    {0b1001, 0b1111, 0b1111, 0b1001, 0b1001, 0b1001, 0b1001},
    // N
    {0b1001, 0b1101, 0b1101, 0b1011, 0b1011, 0b1001, 0b1001},
    // O
    {0b0110, 0b1001, 0b1001, 0b1001, 0b1001, 0b1001, 0b0110},
    // P
    {0b1110, 0b1001, 0b1001, 0b1110, 0b1000, 0b1000, 0b1000},
    // Q
    {0b0110, 0b1001, 0b1001, 0b1001, 0b1001, 0b1001, 0b0111},
    // R
    {0b1110, 0b1001, 0b1001, 0b1110, 0b1010, 0b1001, 0b1001},
    // S
    {0b0111, 0b1000, 0b1000, 0b0110, 0b0001, 0b0001, 0b1110},
    // T
    {0b1111, 0b0100, 0b0100, 0b0100, 0b0100, 0b0100, 0b0100},
    // U
    {0b1001, 0b1001, 0b1001, 0b1001, 0b1001, 0b1001, 0b0110},
    // V
    {0b1001, 0b1001, 0b1001, 0b1001, 0b1001, 0b0110, 0b0100},
    // W
    {0b1001, 0b1001, 0b1001, 0b1001, 0b1111, 0b1111, 0b1001},
    // X
    {0b1001, 0b1001, 0b0110, 0b0100, 0b0110, 0b1001, 0b1001},
    // Y
    {0b1001, 0b1001, 0b0110, 0b0100, 0b0100, 0b0100, 0b0100},
    // Z
    {0b1111, 0b0001, 0b0010, 0b0100, 0b1000, 0b1000, 0b1111},
    // 0 (M31, for the hotbar's HOTBAR_DIGIT_CHARS)
    {0b0110, 0b1001, 0b1001, 0b1001, 0b1001, 0b1001, 0b0110},
    // 1
    {0b0010, 0b0110, 0b0010, 0b0010, 0b0010, 0b0010, 0b0111},
    // 2
    {0b0110, 0b1001, 0b0001, 0b0010, 0b0100, 0b1000, 0b1111},
    // 3
    {0b1111, 0b0001, 0b0010, 0b0110, 0b0001, 0b1001, 0b0110},
    // 4
    {0b0010, 0b0110, 0b1010, 0b1010, 0b1111, 0b0010, 0b0010},
    // 5
    {0b1111, 0b1000, 0b1110, 0b0001, 0b0001, 0b1001, 0b0110},
    // 6
    {0b0110, 0b1000, 0b1000, 0b1110, 0b1001, 0b1001, 0b0110},
    // 7
    {0b1111, 0b0001, 0b0010, 0b0010, 0b0100, 0b0100, 0b0100},
    // 8
    {0b0110, 0b1001, 0b1001, 0b0110, 0b1001, 0b1001, 0b0110},
    // 9
    {0b0110, 0b1001, 0b1001, 0b0111, 0b0001, 0b0001, 0b0110},
};
// clang-format on

// -1 for a character outside the supported set (see bitmap_font.h's own
// class comment) -- DrawString below skips it, still advancing.
int GlyphIndex(char c) {
    if (c == ' ') return 0;
    if (c == '\'') return 1;
    if (c == '-') return 2;
    if (c == '!') return 3;
    char upper = (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
    if (upper >= 'A' && upper <= 'Z') return 4 + (upper - 'A');
    if (c >= '0' && c <= '9') return 30 + (c - '0');
    return -1;
}

void DrawChar(Backbuffer& bb, int x, int y, char c, uint16_t rgb565) {
    int index = GlyphIndex(c);
    if (index < 0) return;

    const uint8_t* rows = kGlyphRows[index];
    for (int row = 0; row < BitmapFont::kGlyphHeight; row++) {
        for (int col = 0; col < BitmapFont::kGlyphWidth; col++) {
            if ((rows[row] >> (BitmapFont::kGlyphWidth - 1 - col)) & 1) {
                bb.SetPixel(x + col, y + row, rgb565);
            }
        }
    }
}

}  // namespace

void BitmapFont::DrawString(Backbuffer& bb, int x, int y, const std::string& text, uint16_t rgb565) {
    int cursorX = x;
    for (char c : text) {
        DrawChar(bb, cursorX, y, c, rgb565);
        cursorX += kAdvance;
    }
}

int BitmapFont::StringWidth(const std::string& text) { return static_cast<int>(text.size()) * kAdvance; }

}  // namespace dawnstar
