#include "assets/image.h"

#include <stdexcept>

#include "stb_image.h"

namespace oblivion {

std::shared_ptr<Image> DecodePng(const std::vector<uint8_t>& bytes) {
    int w = 0, h = 0, n = 0;
    unsigned char* rgba = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h, &n, 4);
    if (!rgba) throw std::runtime_error("DecodePng: not a decodable PNG");
    auto img = std::make_shared<Image>();
    img->width = w;
    img->height = h;
    img->pixels.resize(static_cast<size_t>(w) * h);
    for (size_t i = 0; i < img->pixels.size(); i++) {
        const unsigned char* p = rgba + i * 4;
        img->pixels[i] = (uint32_t(p[3]) << 24) | (uint32_t(p[0]) << 16) | (uint32_t(p[1]) << 8) | p[2];
    }
    stbi_image_free(rgba);
    return img;
}

std::shared_ptr<Image> ImageCache::Get(const std::string& path) {
    auto it = images_.find(path);
    if (it != images_.end()) return it->second;
    auto img = DecodePng(assets_.Read(path));
    images_[path] = img;
    return img;
}

void ImageCache::Evict(const std::string& pathPrefix) {
    for (auto it = images_.begin(); it != images_.end();) {
        if (it->first.compare(0, pathPrefix.size(), pathPrefix) == 0) it = images_.erase(it);
        else ++it;
    }
}

}  // namespace oblivion
