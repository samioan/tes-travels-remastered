#include "world/level_view.h"

#include <stdexcept>

#include "render/sprite_renderer.h"

namespace oblivion {

void LevelView::LoadScr(const std::string& scrPath) {
    Scr scr = ParseScr(assets_.Read(scrPath));
    // Scripts reach LOAD_MAP through CALL chains; the first LOAD_MAP anywhere
    // in the file is the level map.
    for (int id = 0; id < 256; id++) {
        for (const ScrInsn& in : scr.Script(id)) {
            if (in.op == OP_LOAD_MAP) {
                LoadMap(in.strings.at(0).text, in.strings.at(1).text);
                return;
            }
        }
    }
    throw std::runtime_error("LevelView: " + scrPath + " has no LOAD_MAP");
}

void LevelView::LoadMap(const std::string& jtmPath, const std::string& cmlPath) {
    jtmPath_ = jtmPath;
    cmlPath_ = cmlPath;
    map_ = ParseJtm(assets_.Read(jtmPath));
    tiles_ = ParseCml(assets_.Read(cmlPath), images_);
    CenterOnCell(map_.width / 2, map_.height / 2);
}

void LevelView::CellScreenPos(int cx, int cy, int* sx, int* sy) const {
    // World position of the cell corner, then Game.worldToIso.
    int wx = cx * kCellSize;
    int wy = cy * kCellSize;
    *sx = ((wx - wy) >> 3) - (kTileWidth >> 1);
    *sy = (wx + wy) >> 4;
}

void LevelView::CenterOnCell(int cx, int cy) {
    int sx, sy;
    CellScreenPos(cx, cy, &sx, &sy);
    camX_ = Backbuffer::kWidth / 2 - sx;
    camY_ = Backbuffer::kHeight / 2 - sy;
}

void LevelView::Draw(Backbuffer& bb) {
    // Layers 1..n are the visual layers; the last one is the object overlay
    // (drawn together with the actors in the original, at the same depth here).
    for (size_t li = 1; li < map_.layers.size(); li++) {
        const std::vector<uint8_t>& layer = map_.layers[li];
        for (int x = 0; x < map_.width; x++) {
            for (int y = 0; y < map_.height; y++) {
                int tile = layer[x * map_.height + y];
                if (tile == 0) continue;
                int sx, sy;
                CellScreenPos(x, y, &sx, &sy);
                sx += camX_;
                sy += camY_;
                int h = tiles_.Height(tile);
                // The engine own visibility test (Game.drawTileLayers).
                if (sx > -kTileWidth && sx < Backbuffer::kWidth && sy > -kTileHeight &&
                    sy < Backbuffer::kHeight + h) {
                    DrawSprite(bb, images_, tiles_, tile, sx, sy);
                }
            }
        }
    }
}

}  // namespace oblivion
