// M7 smoke test: loads the real imgfiles.lmp (ImgArchive) and checks a
// handful of known real image names (see ../../src/ESGame.java's own
// this.createImage("...") call sites) round-trip to plausible-looking
// PNG byte blobs -- magic-number check, plus (optionally, if an output
// dir is given) writes a couple out as real .png files so they can be
// opened/viewed directly as an even stronger check than a magic-number
// match.
#include <cstdio>
#include <fstream>

#include "assets/img_archive.h"

namespace {

bool LooksLikePng(const std::vector<uint8_t>& data) {
    static const uint8_t kPngMagic[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    if (data.size() < 8) return false;
    for (int i = 0; i < 8; i++) {
        if (data[i] != kPngMagic[i]) return false;
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";
    std::string outDir = argc > 2 ? argv[2] : "";
    std::string imgPath = root + "/imgfiles.lmp";

    const char* kKnownNames[] = {"floor3.png",  "wallsr.png",  "icons.png",         "panel.png",
                                 "gate.png",    "baglarge.png", "chestnearclosed.png"};

    try {
        dawnstar::ImgArchive archive(imgPath);
        std::printf("images: %zu\n", archive.Count());

        bool ok = true;
        for (const char* name : kKnownNames) {
            if (!archive.Contains(name)) {
                std::printf("  FAIL: missing expected image %s\n", name);
                ok = false;
                continue;
            }
            const std::vector<uint8_t>& data = archive.Data(name);
            bool png = LooksLikePng(data);
            std::printf("  %-20s %6zu bytes  %s\n", name, data.size(), png ? "PNG OK" : "NOT PNG");
            if (!png) ok = false;

            if (!outDir.empty()) {
                std::ofstream out(outDir + "/" + name, std::ios::binary);
                out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
            }
        }

        if (!ok) {
            std::fprintf(stderr, "m7_img_archive_smoke: FAILED\n");
            return 1;
        }
        std::printf("all known images present and PNG-magic-valid\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m7_img_archive_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
