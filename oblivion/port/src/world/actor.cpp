#include "world/actor.h"

#include <algorithm>

#include "render/sprite_renderer.h"

namespace oblivion {
namespace ActorSystem {

namespace {

// ActorSystem.animStateOffset
const int8_t kAnimStateOffset[] = {0, 4, 8, 12, 16, 20, 24, 25};

// Game.isoToWorld: screen-space delta -> world-space delta.
void IsoToWorld(int out[2], int isoX, int isoY) {
    out[0] = (isoX << 2) + (isoY << 3);
    out[1] = (isoY << 3) - (isoX << 2);
}

void UpdateScreenPos(Actor& a) {
    a.screenPos[0] = (a.pos[0] - a.pos[1]) >> 3;
    a.screenPos[1] = (a.pos[0] + a.pos[1]) >> 4;
}

// Draw-order cell: the footprint cell nearest the camera (updateSortCell).
void UpdateSortCell(Actor& a) {
    if (a.footCCell[0] > a.footBCell[0]) {
        a.sortCell[0] = a.footCCell[0];
        a.sortCell[1] = a.footCCell[1];
    } else if (a.footBCell[1] <= a.footCCell[1] && a.footBCell[0] <= a.cell[0]) {
        a.sortCell[0] = a.cell[0];
        a.sortCell[1] = a.cell[1];
    } else {
        a.sortCell[0] = a.footBCell[0];
        a.sortCell[1] = a.footBCell[1];
    }
}

void MoveBy(Actor& a, int dx, int dy) {
    a.prevPos[0] = a.pos[0];
    a.prevPos[1] = a.pos[1];
    a.pos[0] += dx;
    a.pos[1] += dy;
    a.footB[0] += dx;
    a.footB[1] += dy;
    a.footC[0] += dx;
    a.footC[1] += dy;
    UpdateScreenPos(a);
    UpdateCells(a);
    if (dx > 0) a.facing = 3;
    else if (dx < 0) a.facing = 4;
    else if (dy > 0) a.facing = 1;
    else if (dy < 0) a.facing = 2;
    a.idleTimer = 500;
}

// footBlocked: tests one footprint point against its cell's collision value.
// 1 = solid; 2..5 are diagonal half-cells decided by the point's offset inside
// the 128-unit cell.
bool FootBlocked(const Grid& grid, const int8_t cell[2], const int p[2]) {
    int idx = cell[0] * grid.height + cell[1];
    if (idx < 0 || idx >= static_cast<int>(grid.collision->size())) return true;
    int v = (*grid.collision)[static_cast<size_t>(idx)];
    if (v == 0) return false;
    int px = p[0] % 128, py = p[1] % 128;
    switch (v) {
        case 1: return true;
        case 2: return px <= py;
        case 3: return py >= px;
        case 4: return py <= px;
        case 5: return px >= py;
        default: return false;
    }
}

}  // namespace

int GroupOf(const Actor& a, int state) { return a.facing + kAnimStateOffset[state]; }

void Init(Actor& a, const std::string& cmlPath, int8_t slot, const SpriteSet& sprite) {
    a.cmlPath = cmlPath;
    a.slot = slot;
    a.sprite = sprite;
    a.sortCell[0] = a.sortCell[1] = 0;
    a.spriteWidth = static_cast<int8_t>(a.sprite.Width(1));
    a.spriteHalfWidth = a.spriteWidth >> 1;
}

void UpdateCells(Actor& a) {
    a.cell[0] = static_cast<int8_t>(a.pos[0] >> 7);
    a.cell[1] = static_cast<int8_t>(a.pos[1] >> 7);
    a.footBCell[0] = static_cast<int8_t>(a.footB[0] >> 7);
    a.footBCell[1] = static_cast<int8_t>(a.footB[1] >> 7);
    a.footCCell[0] = static_cast<int8_t>(a.footC[0] >> 7);
    a.footCCell[1] = static_cast<int8_t>(a.footC[1] >> 7);
    UpdateSortCell(a);
}

void SetPosition(Actor& a, int x, int y) {
    int half[2], full[2];
    IsoToWorld(half, a.spriteHalfWidth, 0);
    IsoToWorld(full, a.spriteWidth, 0);
    a.pos[0] = x;
    a.pos[1] = y;
    a.footB[0] = x + half[0];
    a.footB[1] = y + half[1];
    a.footC[0] = x + full[0];
    a.footC[1] = y + full[1];
    UpdateScreenPos(a);
    UpdateCells(a);
}

void UndoMove(Actor& a) { SetPosition(a, a.prevPos[0], a.prevPos[1]); }

bool IsBlocked(const Actor& a, const Grid& grid) {
    if (a.collides == 0) return false;
    if (a.cell[0] < 0) return true;
    if (a.cell[1] >= grid.height) return true;
    if (a.footCCell[0] >= grid.width) return true;
    if (a.footCCell[1] < 0) return true;
    if (!grid.collision) return false;
    return FootBlocked(grid, a.cell, a.pos) || FootBlocked(grid, a.footBCell, a.footB) ||
           FootBlocked(grid, a.footCCell, a.footC);
}

void MoveDir(Actor& a, const Grid& grid, int dir, int dtMs) {
    a.moveTimer += dtMs;
    if (a.moveTimer <= 50) return;
    if (a.moveTimer > 400) a.moveTimer = 50;
    int step = a.speed / (1000 / a.moveTimer);
    switch (dir) {
        case 1: MoveBy(a, 0, step); break;
        case 2: MoveBy(a, 0, -step); break;
        case 3: MoveBy(a, step, 0); break;
        case 4: MoveBy(a, -step, 0); break;
        default: break;
    }
    if (IsBlocked(a, grid)) UndoMove(a);
    a.moveTimer = 0;
}

void SetMoveTarget(Actor& a, int x, int y) {
    a.moveTarget[0] = x;
    a.moveTarget[1] = y;
    a.animState = 1;
}

void SetAnimState(Actor& a, int8_t state) {
    if (state == 6) {
        a.dead = 1;
    } else if (a.animState != state) {
        a.sprite.ResetFrame(GroupOf(a, state));
    }
    a.animState = state;
}

void Update(Actor& a, int dtMs) {
    a.animTimer += dtMs;
    if (a.animTimer > 125 && a.dead == 0) {
        a.sprite.AdvanceFrame(GroupOf(a, a.animState));
        a.animTimer = 0;
    }
    if (a.dead != 0) return;

    if (a.moveTarget[0] != -1) {
        a.moveTimer += dtMs;
        if (a.moveTimer >= 50) {
            if (a.moveTimer > 100) a.moveTimer = 100;
            int step = a.speed / (1000 / a.moveTimer);
            int dx = 0, dy = 0;
            // One axis at a time: x first, then y.
            if (a.pos[0] < a.moveTarget[0]) dx = std::min(step, a.moveTarget[0] - a.pos[0]);
            else if (a.pos[0] > a.moveTarget[0]) dx = std::max(-step, a.moveTarget[0] - a.pos[0]);
            else if (a.pos[1] < a.moveTarget[1]) dy = std::min(step, a.moveTarget[1] - a.pos[1]);
            else if (a.pos[1] > a.moveTarget[1]) dy = std::max(-step, a.moveTarget[1] - a.pos[1]);
            else {
                a.moveTarget[0] = -1;
                if (a.animState != 2) a.animState = 0;
            }
            MoveBy(a, dx, dy);
            a.moveTimer = 0;
        }
    } else if (a.slot == 1 && a.idleTimer > 0) {
        a.idleTimer -= dtMs;
        if (a.idleTimer <= 0) a.animState = 0;
    }
}

void Draw(Actor& a, Backbuffer& bb, ImageCache& images, int camX, int camY) {
    const int sx = a.screenPos[0] + camX;
    const int sy = a.screenPos[1] + camY;
    const int idleW = a.sprite.Width(GroupOf(a, 0));
    if (a.dead == 1 || a.animState == 6) {
        // Corpse (group -55).
        DrawSprite(bb, images, a.sprite, -55, sx + (idleW >> 1) - (a.sprite.Width(-55) >> 1),
                   sy - a.sprite.Height(-55) + 3);
        return;
    }
    // Shadow (group -56), then the body.
    DrawSprite(bb, images, a.sprite, -56, sx + (idleW >> 1) - (a.sprite.Width(-56) >> 1),
               sy - a.sprite.Height(-56) + 3 + (a.animState == 2 || a.animState == 3 ? 3 : 0));
    const int g = GroupOf(a, a.animState);
    DrawSprite(bb, images, a.sprite, g, sx, sy - a.sprite.Height(g));
}

}  // namespace ActorSystem
}  // namespace oblivion
