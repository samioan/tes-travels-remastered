#pragma once
#include <string>
#include <vector>

#include "assets/asset_root.h"
#include "assets/cml.h"
#include "assets/image.h"
#include "assets/jtm.h"
#include "assets/scr.h"
#include "graphics/backbuffer.h"
#include "world/actor.h"

namespace oblivion {

// The static picture of a level: its .jtm tile map drawn with its .cml tile
// sprites through the game isometric projection (Game.finishMapLoad /
// drawTileLayers / paint). No actors or scripts yet -- those come with the
// interpreter.
class LevelView {
public:
    static constexpr int kCellSize = 128;  // world units per grid cell (Game.cellSize)
    static constexpr int kTileWidth = 32;  // Game.tileWidth
    static constexpr int kTileHeight = 16;

    LevelView(const AssetRoot& assets, ImageCache& images) : assets_(assets), images_(images) {}

    // Loads a level from its .scr (e.g. "/l01_1.scr"): finds the LOAD_MAP the
    // level scripts execute, loads that map and tile sprites.
    void LoadScr(const std::string& scrPath);
    // Loads a .jtm + .cml pair directly.
    void LoadMap(const std::string& jtmPath, const std::string& cmlPath);

    const JtmMap& map() const { return map_; }
    const std::string& jtmPath() const { return jtmPath_; }
    const std::string& cmlPath() const { return cmlPath_; }

    // Screen-space top-left of a cell before the camera offset (Game.cellScreenPos).
    void CellScreenPos(int cx, int cy, int* sx, int* sy) const;

    // The camera offset is added to every cell position (Game.cameraOffset).
    void CenterOnCell(int cx, int cy);
    int camX() const { return camX_; }
    int camY() const { return camY_; }
    void Pan(int dx, int dy) { camX_ += dx; camY_ += dy; }

    // Collision data for ActorSystem::IsBlocked (jtm layer 0).
    Grid grid() const;

    // Centres the camera on an isometric screen position (Game.updateCamera,
    // without its dead zone).
    void CenterOnScreen(int sx, int sy);

    // Draws the visual layers, then the object overlay layer with the actors
    // interleaved by sortCell (Game.paint state 0).
    void Draw(Backbuffer& bb, const std::vector<Actor*>& actors = {});

private:
    const AssetRoot& assets_;
    ImageCache& images_;
    JtmMap map_;
    SpriteSet tiles_;
    std::string jtmPath_, cmlPath_;
    int camX_ = 0, camY_ = 0;
};

}  // namespace oblivion
