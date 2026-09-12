#pragma once
#include <cstdint>
#include <vector>

namespace dawnstar {

// A stb_image-decoded PNG (see ../../third_party/stb/PROVENANCE.md),
// RGBA8. There is no PNG decoder anywhere in the original game (MIDP's
// own built-in Image.createImage() handles that on real hardware) --
// this exists purely so this port can turn ImgArchive's raw PNG bytes
// (M7) into actual pixels to draw, the same ones MIDP's decoder would
// have produced.
struct DecodedImage {
    int width = 0;
    int height = 0;
    // RGBA8, row-major, 4 bytes/pixel (stb_image's native layout when
    // asked for 4 channels).
    std::vector<uint8_t> pixels;

    // Throws std::runtime_error on decode failure.
    static DecodedImage FromPng(const std::vector<uint8_t>& pngBytes);

    uint8_t R(int x, int y) const { return pixels[(static_cast<size_t>(y) * width + x) * 4 + 0]; }
    uint8_t G(int x, int y) const { return pixels[(static_cast<size_t>(y) * width + x) * 4 + 1]; }
    uint8_t B(int x, int y) const { return pixels[(static_cast<size_t>(y) * width + x) * 4 + 2]; }
    uint8_t A(int x, int y) const { return pixels[(static_cast<size_t>(y) * width + x) * 4 + 3]; }
};

}  // namespace dawnstar
