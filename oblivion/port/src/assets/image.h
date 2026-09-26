#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "assets/asset_root.h"

namespace oblivion {

// A decoded PNG: 8-bit RGBA, row-major. The game's PNGs are palettised with
// (at most) 1-bit transparency, so alpha is treated as a test (>= 128).
struct Image {
    int width = 0;
    int height = 0;
    std::vector<uint32_t> pixels;  // 0xAARRGGBB

    uint32_t At(int x, int y) const { return pixels[static_cast<size_t>(y) * width + x]; }
};

std::shared_ptr<Image> DecodePng(const std::vector<uint8_t>& bytes);

// Image.createImage(path) + SpriteRenderer's image Hashtable: decode once,
// keep by path, drop by prefix (SpriteRenderer.evict).
class ImageCache {
public:
    explicit ImageCache(const AssetRoot& assets) : assets_(assets) {}

    std::shared_ptr<Image> Get(const std::string& path);
    void Evict(const std::string& pathPrefix);
    void Clear() { images_.clear(); }

private:
    const AssetRoot& assets_;
    std::map<std::string, std::shared_ptr<Image>> images_;
};

}  // namespace oblivion
