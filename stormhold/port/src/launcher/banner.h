#pragma once

// The launcher's banner artwork.
//
// Ported from dawnstar's own port/src/launcher/banner.h (itself ported
// from shadowkey-decomp), decoded through **stb_image** (already vendored
// in this repo, third_party/stb/, for the game's own PNG assets) rather
// than Windows' WIC. The source key art (assets/banner_source.jpg) is a
// JPEG, but the embedded resource (assets/banner.png) is a PNG converted
// from it by tools/make_banner.py -- this port's stb_image is built with
// STBI_ONLY_PNG (see third_party/stb/stb_image_impl.cpp's own doc
// comment), so converting once at prep time avoids touching that shared
// build flag just for the launcher, and needs no COM/wincodecs dependency
// for decoding at all (IFileOpenDialog, used elsewhere in the launcher for
// the file/folder pickers, still needs COM -- that part is unchanged).
//
// The image is embedded as an RCDATA resource (assets/banner.png) and
// decoded at startup. `banner_source.jpg` (not embedded, kept only for
// attribution/history, see NOTICE.md) is the original the user supplied.

#include <cstddef>
#include <cstdint>
#include <vector>

namespace stormhold {
namespace launcher {

// Decoded pixels in the layout `StretchDIBits` wants for a top-down 32bpp
// DIB: one `uint32_t` per pixel, bytes in B,G,R,X order, rows top to
// bottom, no row padding.
struct Banner {
    int width = 0;
    int height = 0;
    std::vector<uint32_t> pixels;

    bool valid() const {
        return width > 0 && height > 0 &&
               pixels.size() == static_cast<size_t>(width) * static_cast<size_t>(height);
    }
};

// A sanity bound on the decoded dimensions, so a corrupt or hostile image
// header cannot ask for a multi-gigabyte allocation.
constexpr int kMaxBannerDimension = 8192;

// Decodes a PNG from memory. Returns false and leaves `out` empty on
// anything that goes wrong: a truncated buffer, an unrecognised format,
// implausible dimensions, a failed allocation. The launcher then draws a
// flat background rather than refusing to start, the same "optional asset"
// tolerance the game itself applies to every real asset it loads.
bool DecodeBanner(const void* data, size_t size, Banner& out);

// Same, reading the image from the executable's own RCDATA resource.
Banner LoadEmbeddedBanner();

}  // namespace launcher
}  // namespace stormhold
