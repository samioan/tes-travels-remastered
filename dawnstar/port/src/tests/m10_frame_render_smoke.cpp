// M10 smoke test: decodes the real wall/floor/gate PNGs (stb_image) and
// renders one real frame of a real M6-generated dungeon level, standing
// at that level's own stairway-corridor tile facing down it (same
// position M9 used to demonstrate direction-sensitivity) -- then writes
// the result out as an uncompressed 24-bit BMP so it can actually be
// looked at, the same way M7 verified ImgArchive by viewing extracted
// textures directly instead of only checking their magic bytes.
#include <cstdio>
#include <fstream>

#include "assets/dat_archive.h"
#include "assets/img_archive.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "render/frame_renderer.h"

namespace {

void WriteBmp(const std::string& path, const dawnstar::Backbuffer& bb) {
    const int w = dawnstar::Backbuffer::kWidth;
    const int h = dawnstar::Backbuffer::kHeight;
    const int rowBytes = (w * 3 + 3) & ~3;
    const uint32_t pixelDataSize = static_cast<uint32_t>(rowBytes) * h;
    const uint32_t fileSize = 14 + 40 + pixelDataSize;

    std::ofstream out(path, std::ios::binary);
    auto putU16 = [&](uint16_t v) { out.put(static_cast<char>(v & 0xFF)); out.put(static_cast<char>(v >> 8)); };
    auto putU32 = [&](uint32_t v) {
        out.put(static_cast<char>(v & 0xFF));
        out.put(static_cast<char>((v >> 8) & 0xFF));
        out.put(static_cast<char>((v >> 16) & 0xFF));
        out.put(static_cast<char>((v >> 24) & 0xFF));
    };
    auto putS32 = [&](int32_t v) { putU32(static_cast<uint32_t>(v)); };

    out.put('B');
    out.put('M');
    putU32(fileSize);
    putU32(0);
    putU32(14 + 40);

    putU32(40);           // DIB header size
    putS32(w);
    putS32(-h);            // negative = top-down
    putU16(1);             // planes
    putU16(24);            // bitcount
    putU32(0);             // no compression
    putU32(pixelDataSize);
    putS32(0);
    putS32(0);
    putU32(0);
    putU32(0);

    std::vector<uint8_t> row(rowBytes, 0);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint16_t px = bb.Data()[y * w + x];
            uint8_t r5 = (px >> 11) & 0x1F;
            uint8_t g6 = (px >> 5) & 0x3F;
            uint8_t b5 = px & 0x1F;
            row[x * 3 + 0] = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));
            row[x * 3 + 1] = static_cast<uint8_t>((g6 << 2) | (g6 >> 4));
            row[x * 3 + 2] = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
        }
        out.write(reinterpret_cast<const char*>(row.data()), rowBytes);
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";
    std::string outPath = argc > 2 ? argv[2] : "frame.bmp";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        dawnstar::MonsterDatabase monsters = dawnstar::MonsterDatabase::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);
        dawnstar::ImgArchive images(root + "/imgfiles.lmp");
        dawnstar::FrameTextures textures = dawnstar::FrameTextures::Load(images);

        int levelNumber = argc > 3 ? std::atoi(argv[3]) : 2;
        dawnstar::GeneratedLevel level = levelNumber == 1
                                              ? dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[0])
                                              : dawnstar::DungeonGenerator::PopulateLevel(
                                                    levelNumber, geometry.rows[levelNumber - 1], items, monsters);
        dawnstar::DungeonView view(level);

        int px, py, facing;
        if (argc > 6) {
            // Explicit override: <root> <outPath> <level> <px> <py> <facing>.
            px = std::atoi(argv[4]);
            py = std::atoi(argv[5]);
            facing = std::atoi(argv[6]);
        } else if (levelNumber == 1) {
            // The hub town's hand-carved cross corridor -- near the west
            // end of its long east-west arm, facing east down it.
            px = 2;
            py = 9;
            facing = 2;
        } else {
            int stairsDir = geometry.rows[levelNumber - 1].stairsUpDir;
            px = 17;
            py = 17;
            facing = 1;
            if (stairsDir == 2) {
                px = 31;
                py = 17;
                facing = 2;
            } else if (stairsDir == 4) {
                px = 3;
                py = 17;
                facing = 4;
            } else if (stairsDir == 1) {
                px = 17;
                py = 3;
                facing = 1;
            } else if (stairsDir == 3) {
                px = 17;
                py = 31;
                facing = 3;
            } else {
                px = level.monsters[0].x;
                py = level.monsters[0].y;
                facing = 1;
            }
        }

        std::printf("level %d, standing at (%d,%d) facing %d\n", levelNumber, px, py, facing);

        dawnstar::Backbuffer bb;
        dawnstar::FrameRenderer::Render(bb, textures, view, px, py, facing, level.number);
        WriteBmp(outPath, bb);
        std::printf("wrote %s\n", outPath.c_str());
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m10_frame_render_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
