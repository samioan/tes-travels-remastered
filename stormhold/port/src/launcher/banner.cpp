#include "launcher/banner.h"

#include <windows.h>

#include <climits>
#include <new>

#include "launcher/resource.h"
#include "stb_image.h"

namespace stormhold {
namespace launcher {

bool DecodeBanner(const void* data, size_t size, Banner& out) {
    out = Banner{};
    if (!data || size == 0 || size > static_cast<size_t>(INT_MAX)) return false;

    int width = 0, height = 0, channels = 0;
    unsigned char* decoded = stbi_load_from_memory(static_cast<const unsigned char*>(data),
                                                    static_cast<int>(size), &width, &height,
                                                    &channels, STBI_rgb_alpha);
    if (!decoded) return false;
    if (width <= 0 || height <= 0 || width > kMaxBannerDimension || height > kMaxBannerDimension) {
        stbi_image_free(decoded);
        return false;
    }

    std::vector<uint32_t> pixels;
    try {
        pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
    } catch (const std::bad_alloc&) {
        stbi_image_free(decoded);
        return false;
    }

    // stb_image always hands back RGBA (STBI_rgb_alpha); StretchDIBits
    // wants BGRA (B,G,R,X byte order) in a top-down 32bpp DIB -- swap R
    // and B per pixel, alpha unused (the panel painter never blends).
    const unsigned char* src = decoded;
    for (size_t i = 0; i < pixels.size(); ++i) {
        const unsigned char r = src[0];
        const unsigned char g = src[1];
        const unsigned char b = src[2];
        pixels[i] = (static_cast<uint32_t>(b)) | (static_cast<uint32_t>(g) << 8) |
                    (static_cast<uint32_t>(r) << 16);
        src += 4;
    }
    stbi_image_free(decoded);

    out.width = width;
    out.height = height;
    out.pixels = std::move(pixels);
    return true;
}

Banner LoadEmbeddedBanner() {
    Banner banner;
    // MAKEINTRESOURCEW(10) rather than RT_RCDATA: that macro follows the
    // UNICODE define, and this file is also compiled into the smoke test,
    // which does not necessarily set it -- so the named constant would be
    // an LPSTR there and an LPWSTR here.
    HRSRC found = FindResourceW(nullptr, MAKEINTRESOURCEW(IDR_BANNER), MAKEINTRESOURCEW(10));
    if (!found) return banner;
    const DWORD size = SizeofResource(nullptr, found);
    HGLOBAL loaded = LoadResource(nullptr, found);
    if (!loaded || size == 0) return banner;
    const void* blob = LockResource(loaded);
    if (!blob) return banner;
    // No FreeResource/UnlockResource: since Win32 those are no-ops, and the
    // blob stays mapped for the life of the module either way.
    DecodeBanner(blob, size, banner);
    return banner;
}

}  // namespace launcher
}  // namespace stormhold
