#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "assets/asset_root.h"

namespace stormhold {

// Stormhold's from-scratch indexed-color sprite format (.cus files),
// decoded by RawImage.load() in ../src/RawImage.java -- no dawnstar analog,
// see docs/ASSET_FORMATS.md's ".cus files" section (that doc's own
// correction of the original "per-bodypart 3D mesh" guess). On-disk layout:
//   s32 BE width, s32 BE height, u8 hasTransparency flag,
//   s16 BE transparentColorValue, u8 colorCount (<=255),
//   colorCount x s16 BE palette entries, width*height x u8 palette indices.
//
// Palette entries are resolved to decoded pixels at load time, exactly like
// the original -- each pixel is the matching palette entry with the alpha
// nibble (0xF000) forced on, except pixels whose index equals the FIRST
// palette entry matching transparentColorValue, which get 0xF000 cleared
// instead: binary on/off transparency baked directly into the pixel value
// (RawImage.load()'s `pixel & -61441` / `pixel | 61440`), no separate alpha
// mask. Kept as the same raw ARGB4444-ish uint16_t the Java stores, not
// converted to RGB565 here -- compositing onto Backbuffer (alpha test +
// format conversion) is a later milestone's job, same as
// docs/PORT_ROADMAP.md's Backbuffer header comment already flags.
class RawImage {
public:
    static RawImage Load(const AssetRoot& assets, const std::string& name);

    int width = 0;
    int height = 0;
    bool hasTransparency = false;
    int16_t transparentColorValue = 0;
    std::vector<uint16_t> pixels;  // width * height entries, row-major
};

}  // namespace stormhold
