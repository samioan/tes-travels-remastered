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
    CenterOnCell(map_.width / 2, map_.height / 2);  // viewer default; World overrides
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

void LevelView::SetTile(int x, int y, int layer, int tile) {
    const size_t li = static_cast<size_t>(layer) + 1;
    if (li >= map_.layers.size() || x < 0 || y < 0 || x >= map_.width || y >= map_.height) return;
    map_.layers[li][static_cast<size_t>(x * map_.height + y)] = static_cast<uint8_t>(tile);
}

void LevelView::SetCollision(int x, int y, bool solid) {
    if (map_.layers.empty() || x < 0 || y < 0 || x >= map_.width || y >= map_.height) return;
    map_.layers[0][static_cast<size_t>(x * map_.height + y)] = solid ? 1 : 0;
}

void LevelView::ClearVisualLayers() {
    if (map_.layers.size() > 1) map_.layers.resize(1);
}

void LevelView::Unload() {
    map_ = JtmMap{};
    tiles_ = SpriteSet{};
}

Grid LevelView::grid() const {
    Grid g;
    g.width = map_.width;
    g.height = map_.height;
    if (!map_.layers.empty()) g.collision = &map_.layers[0];
    return g;
}

void LevelView::CenterOnScreen(int sx, int sy) {
    camX_ = Backbuffer::kWidth / 2 - sx;
    camY_ = Backbuffer::kHeight / 2 - sy;
}

void LevelView::Draw(Backbuffer& bb, const std::vector<Actor*>& actors) {
    // Layer 0 is collision, the last layer the object overlay; in between are
    // the visual layers (Game.drawTileLayers).
    const size_t n = map_.layers.size();
    for (size_t li = 1; li + 1 < n; li++) {
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
    if (n < 2) return;
    // Overlay: each cell draws its tile, then the actors sorted into that cell.
    const std::vector<uint8_t>& overlay = map_.layers[n - 1];
    int lastH = 0;
    for (int x = 0; x < map_.width; x++) {
        for (int y = 0; y < map_.height; y++) {
            int tile = overlay[x * map_.height + y];
            int sx, sy;
            CellScreenPos(x, y, &sx, &sy);
            sx += camX_;
            sy += camY_;
            if (tile != 0) lastH = tiles_.Height(tile);
            if (sx > -kTileWidth && sx < Backbuffer::kWidth && sy > -kTileHeight &&
                sy < Backbuffer::kHeight + lastH) {
                if (tile != 0) DrawSprite(bb, images_, tiles_, tile, sx, sy);
                for (Actor* a : actors)
                    if (a && a->sortCell[0] == x && a->sortCell[1] == y)
                        ActorSystem::Draw(*a, bb, images_, camX_, camY_);
            }
        }
    }
}

}  // namespace oblivion
