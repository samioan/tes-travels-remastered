#include "assets/decoded_image.h"

#include <stdexcept>
#include <string>

#include "stb_image.h"

namespace dawnstar {

DecodedImage DecodedImage::FromPng(const std::vector<uint8_t>& pngBytes) {
    int w = 0;
    int h = 0;
    int channelsInFile = 0;
    unsigned char* data = stbi_load_from_memory(pngBytes.data(), static_cast<int>(pngBytes.size()), &w, &h,
                                                 &channelsInFile, 4);
    if (!data) {
        throw std::runtime_error(std::string("DecodedImage: stb_image failed: ") + stbi_failure_reason());
    }

    DecodedImage img;
    img.width = w;
    img.height = h;
    img.pixels.assign(data, data + (static_cast<size_t>(w) * h * 4));
    stbi_image_free(data);
    return img;
}

}  // namespace dawnstar
