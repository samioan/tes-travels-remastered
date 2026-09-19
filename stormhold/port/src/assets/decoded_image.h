#pragma once
#include <cstdint>
#include <vector>

#include "assets/asset_root.h"

namespace stormhold {

// A stb_image-decoded PNG (see ../../third_party/stb/PROVENANCE.md), RGBA8.
// There is no PNG decoder anywhere in the original game (MIDP's own
// built-in Image.createImage() handles that on real hardware) -- this
// exists purely so this port can turn the real .png files GameCanvas's
// plain `Image` fields load (floorTexture/wallTexture/effectImages/
// hotbarIcons -- confirmed by grepping ESGame.java's own asset-loading
// methods for their literal filenames, e.g. "floor3.png"/"newwallsnok.png",
// resolving phase-3 M21's own open question: these are plain PNGs, NOT M7's
// from-scratch RawImage/.cus format) into actual pixels to draw, the same
// ones MIDP's decoder would have produced. Same shape as the sibling
// dawnstar project's own identical DecodedImage (its M10) -- copied
// structurally, adapted to read through AssetRoot (a plain directory, no
// archive) instead of dawnstar's ImgArchive.
struct DecodedImage {
    int width = 0;
    int height = 0;
    // RGBA8, row-major, 4 bytes/pixel (stb_image's native layout when asked
    // for 4 channels).
    std::vector<uint8_t> pixels;

    // Reads `<assets root>/<name>` fully into memory, then decodes it.
    // Throws std::runtime_error on file-open or decode failure.
    static DecodedImage Load(const AssetRoot& assets, const std::string& name);

    uint8_t R(int x, int y) const { return pixels[(static_cast<size_t>(y) * static_cast<size_t>(width) + x) * 4 + 0]; }
    uint8_t G(int x, int y) const { return pixels[(static_cast<size_t>(y) * static_cast<size_t>(width) + x) * 4 + 1]; }
    uint8_t B(int x, int y) const { return pixels[(static_cast<size_t>(y) * static_cast<size_t>(width) + x) * 4 + 2]; }
    uint8_t A(int x, int y) const { return pixels[(static_cast<size_t>(y) * static_cast<size_t>(width) + x) * 4 + 3]; }
};

}  // namespace stormhold
