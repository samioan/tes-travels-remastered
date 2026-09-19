#include "assets/decoded_image.h"

#include <stdexcept>
#include <string>

#include "stb_image.h"

namespace stormhold {

DecodedImage DecodedImage::Load(const AssetRoot& assets, const std::string& name) {
    std::ifstream file = assets.OpenFile(name);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    int w = 0;
    int h = 0;
    int channelsInFile = 0;
    unsigned char* data = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h,
                                                 &channelsInFile, 4);
    if (!data) {
        throw std::runtime_error("DecodedImage: " + name + ": stb_image failed: " + stbi_failure_reason());
    }

    DecodedImage img;
    img.width = w;
    img.height = h;
    img.pixels.assign(data, data + (static_cast<size_t>(w) * static_cast<size_t>(h) * 4));
    stbi_image_free(data);
    return img;
}

}  // namespace stormhold
