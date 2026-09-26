#pragma once
#include <cstdint>
#include <string>

#include "assets/cml.h"
#include "assets/image.h"
#include "graphics/backbuffer.h"

namespace oblivion {

// Collision + trigger data of the loaded level, as the engine's static
// Game.gridWidth/gridHeight/collision. `collision` is jtm layer 0, indexed
// [x * height + y]; null means "no map" (nothing blocks).
struct Grid {
    int width = 0, height = 0;
    const std::vector<uint8_t>* collision = nullptr;
};

// The movement / collision / animation / drawing subset of Actor + ActorSystem
// (src/Actor.java, src/ActorSystem.java). Combat, stats, AI and items arrive
// with M7; the fields they need are added then.
struct Actor {
    // Sub-tile world points (128 per grid cell): pos, and the two other feet
    // of the sprite's isometric footprint (footB half, footC full sprite width).
    int pos[2] = {0, 0}, footB[2] = {0, 0}, footC[2] = {0, 0}, prevPos[2] = {0, 0};
    int screenPos[2] = {0, 0};
    int8_t cell[2] = {0, 0}, footBCell[2] = {0, 0}, footCCell[2] = {0, 0};
    int8_t sortCell[2] = {0, 0};
    int spriteWidth = 0, spriteHalfWidth = 0;

    int8_t slot = 0;       // 1-based index in the actor table (1 = player)
    int8_t facing = 2;     // 1 +y, 2 -y, 3 +x, 4 -x
    int8_t animState = 0;  // see animStateOffset
    int8_t collides = 1;
    int8_t dead = 0;
    int8_t team = 1;
    int speed = 0;          // world units per second
    int moveTimer = 0;      // ms
    int idleTimer = 0;      // ms until animState returns to 0 (player only)
    int animTimer = 0;      // ms since last frame advance
    int moveTarget[2] = {-1, -1};  // [0] == -1: none

    // Trigger tiles under the actor (checkTriggerTiles).
    int8_t enterScript = -1, leaveScript = -1;

    std::string cmlPath;
    SpriteSet sprite;
};

namespace ActorSystem {

// animStateOffset[state] + facing = sprite group id.
int GroupOf(const Actor& a, int state);

// createFromCml: loads the sprite set and measures the footprint width from group 1.
void Init(Actor& a, const std::string& cmlPath, int8_t slot, const SpriteSet& sprite);

// setPosition / updateCells / updateSortCell / undoMove.
void SetPosition(Actor& a, int x, int y);
void UpdateCells(Actor& a);
void UndoMove(Actor& a);

// isBlocked: is any of the three footprint points inside a blocking cell or
// out of the grid.
bool IsBlocked(const Actor& a, const Grid& grid);

// moveDir(dir 1..4): one movement step, gated by the 50 ms move timer; undone
// when it ends up blocked. `dtMs` is the time since the last call.
void MoveDir(Actor& a, const Grid& grid, int dir, int dtMs);

void SetMoveTarget(Actor& a, int x, int y);
void SetAnimState(Actor& a, int8_t state);

// The per-frame part of ActorSystem.update that concerns animation and
// auto-walking to moveTarget (regen, dot, AI, floating text come with M7).
void Update(Actor& a, int dtMs);

// ActorSystem.draw: shadow, sprite, and (dead) corpse group. `cam` is the
// camera offset. Health bar, floating text and status icon come with M7.
void Draw(Actor& a, Backbuffer& bb, ImageCache& images, int camX, int camY);

}  // namespace ActorSystem
}  // namespace oblivion
