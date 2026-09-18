#include "assets/raw_image.h"

#include <array>
#include <stdexcept>

#include "assets/binary_reader.h"

namespace stormhold {

RawImage RawImage::Load(const AssetRoot& assets, const std::string& name) {
    std::ifstream file = assets.OpenFile(name);
    BinaryReader reader(file);

    RawImage img;
    img.width = reader.ReadS32();
    img.height = reader.ReadS32();
    // RawImage.load() also stores a `widthAgain` field, set from the same
    // read as `width` and never used anywhere else in ../src/ -- a dead
    // field, not ported.
    uint8_t transparencyFlag = reader.ReadU8();
    img.hasTransparency = transparencyFlag != 0;
    img.transparentColorValue = reader.ReadS16();

    uint8_t colorCount = reader.ReadU8();

    // RawImage.load()'s Java palette scratch is a *static* array reused
    // across every image load, so a pixel index >= colorCount would read a
    // stale entry left over from whichever image loaded before it --
    // load-order-dependent behavior no well-formed .cus file should ever
    // actually rely on. Modeled here as a hard error instead of silently
    // reproducing that nondeterminism; the M7 smoke test confirms it never
    // fires against any real .cus file.
    std::array<int16_t, 256> palette{};
    int transparentPaletteIndex = -1;
    for (int i = 0; i < colorCount; i++) {
        int16_t color = reader.ReadS16();
        palette[static_cast<size_t>(i)] = color;
        if (img.hasTransparency && transparentPaletteIndex < 0 && img.transparentColorValue == color) {
            transparentPaletteIndex = i;
        }
    }

    int pixelCount = img.width * img.height;
    img.pixels.resize(static_cast<size_t>(pixelCount));
    for (int i = 0; i < pixelCount; i++) {
        int index = reader.ReadU8();
        if (index >= colorCount) {
            throw std::runtime_error("RawImage: " + name + " pixel index " + std::to_string(index) +
                                      " out of range for palette of " + std::to_string(colorCount) + " colors");
        }
        uint16_t pixel = static_cast<uint16_t>(palette[static_cast<size_t>(index)]);
        if (img.hasTransparency && index == transparentPaletteIndex) {
            pixel &= 0x0FFF;
        } else {
            pixel |= 0xF000;
        }
        img.pixels[static_cast<size_t>(i)] = pixel;
    }

    return img;
}

}  // namespace stormhold
