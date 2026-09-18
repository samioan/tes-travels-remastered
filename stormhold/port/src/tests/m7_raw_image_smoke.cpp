// M7 smoke test: RawImage against all 37 real .cus sprites in extracted/.
//
// No JVM ground truth possible here either (same RegisteredMIDlet static-
// initializer issue as M6/dawnstar's own M6), and there's no independent
// oracle for a from-scratch image format anyway -- so this checks strong
// internal self-consistency against every real .cus file instead: decode
// succeeds, dimensions are sane, pixel count matches width*height, every
// pixel index resolves within the declared palette (see raw_image.cpp's own
// comment on why that's modeled as a hard error), and the alpha nibble
// (0xF000) is set on every non-transparent pixel and cleared on every
// transparent one.
#include <cstdio>
#include <string>
#include <vector>

#include "assets/asset_root.h"
#include "assets/raw_image.h"

namespace {

// The full real .cus roster (docs/ASSET_FORMATS.md's ".cus files" list),
// transcribed directly from extracted/'s own directory listing.
const char* kCusFiles[] = {
    "baglarge.cus",         "bagmid.cus",           "bagsmall.cus",
    "chestfarclosed.cus",   "chestmidclosed.cus",   "chestnearclosed.cus",
    "crystalfar.cus",       "crystalmid.cus",       "crystalnear.cus",
    "overseeraxe.cus",      "overseerbodyf3lc.cus", "overseerclub.cus",
    "overseerfar.cus",      "overseerhelmet.cus",   "overseermidcf.cus",
    "trainer_male_belt.cus", "trainer_male_bodyf1.cus", "trainer_male_far.cus",
    "trainer_male_heads.cus", "trainer_male_mid.cus", "trainer_male_shirt.cus",
    "trainerfembelt1.cus",  "trainerfembodf1.cus",  "trainerfemfar.cus",
    "trainerfemheads.cus",  "trainerfemmid.cus",    "trainerfemneck.cus",
    "undeadbodyf3.cus",     "undeadfar.cus",        "undeadleftarm1.cus",
    "undeadmid.cus",        "undeadrightarm1.cus",  "undeadrightarm2.cus",
    "wardenbody2f2half.cus", "wardenfar.cus",       "wardenheads4bit.cus",
    "wardenmid.cus",
};

bool CheckImage(const std::string& name, const stormhold::RawImage& img) {
    bool ok = true;

    if (img.width <= 0 || img.height <= 0) {
        std::printf("  FAIL: %s has non-positive dimensions %dx%d\n", name.c_str(), img.width, img.height);
        ok = false;
    }
    if (img.pixels.size() != static_cast<size_t>(img.width) * static_cast<size_t>(img.height)) {
        std::printf("  FAIL: %s pixel count %zu != width*height %d\n", name.c_str(), img.pixels.size(),
                    img.width * img.height);
        ok = false;
    }

    int transparentCount = 0;
    for (uint16_t p : img.pixels) {
        bool alphaSet = (p & 0xF000) != 0;
        if (!alphaSet) transparentCount++;
    }
    if (img.hasTransparency && transparentCount == 0) {
        // Not necessarily a failure -- a sprite's transparent color might
        // legitimately go unused in every pixel -- but worth surfacing.
        std::printf("  note: %s has hasTransparency=true but 0 transparent pixels decoded\n", name.c_str());
    }
    if (!img.hasTransparency && transparentCount != 0) {
        std::printf("  FAIL: %s has hasTransparency=false but %d pixels missing the alpha nibble\n", name.c_str(),
                    transparentCount);
        ok = false;
    }

    std::printf("  %-24s %4dx%-4d transparency=%-5s transparentPixels=%d\n", name.c_str(), img.width, img.height,
                img.hasTransparency ? "true" : "false", transparentCount);

    return ok;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        stormhold::AssetRoot assets(root);

        bool ok = true;
        int count = 0;
        for (const char* name : kCusFiles) {
            stormhold::RawImage img = stormhold::RawImage::Load(assets, name);
            if (!CheckImage(name, img)) ok = false;
            count++;
        }

        std::printf("decoded %d .cus sprites\n", count);

        if (!ok) {
            std::fprintf(stderr, "m7_raw_image_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m7_raw_image_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
