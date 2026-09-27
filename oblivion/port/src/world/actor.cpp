#include "world/actor.h"

#include <algorithm>
#include <cmath>

#include "graphics/text.h"
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

int GroupOf(const Actor& a, int state) { return a.facing + kAnimStateOffset[state]; }

int SpriteHeight(const Actor& a) { return a.animState == 6 ? 0 : a.sprite.Height(GroupOf(a, a.animState)); }
int SpriteWidth(const Actor& a) { return a.animState == 6 ? 0 : a.sprite.Width(GroupOf(a, a.animState)); }

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

bool WouldBeBlocked(Actor& a, int x, int y, const Grid& grid) {
    const int savedPos[2] = {a.pos[0], a.pos[1]};
    const int savedFootB[2] = {a.footB[0], a.footB[1]};
    const int savedFootC[2] = {a.footC[0], a.footC[1]};
    const int savedScreenPos[2] = {a.screenPos[0], a.screenPos[1]};
    const int8_t savedCell[2] = {a.cell[0], a.cell[1]};
    const int8_t savedFootBCell[2] = {a.footBCell[0], a.footBCell[1]};
    const int8_t savedFootCCell[2] = {a.footCCell[0], a.footCCell[1]};
    const int8_t savedSortCell[2] = {a.sortCell[0], a.sortCell[1]};

    SetPosition(a, x, y);
    const bool blocked = IsBlocked(a, grid);

    a.pos[0] = savedPos[0]; a.pos[1] = savedPos[1];
    a.footB[0] = savedFootB[0]; a.footB[1] = savedFootB[1];
    a.footC[0] = savedFootC[0]; a.footC[1] = savedFootC[1];
    a.screenPos[0] = savedScreenPos[0]; a.screenPos[1] = savedScreenPos[1];
    a.cell[0] = savedCell[0]; a.cell[1] = savedCell[1];
    a.footBCell[0] = savedFootBCell[0]; a.footBCell[1] = savedFootBCell[1];
    a.footCCell[0] = savedFootCCell[0]; a.footCCell[1] = savedFootCCell[1];
    a.sortCell[0] = savedSortCell[0]; a.sortCell[1] = savedSortCell[1];
    return blocked;
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

bool MoveAnalog(Actor& a, const Grid& grid, float dx, float dy) {
    a.moveFracX += dx;
    a.moveFracY += dy;
    int ix = static_cast<int>(a.moveFracX), iy = static_cast<int>(a.moveFracY);
    a.moveFracX -= static_cast<float>(ix);
    a.moveFracY -= static_cast<float>(iy);
    bool moved = false;
    // Small steps: a long frame must not jump through a wall.
    while (ix != 0 || iy != 0) {
        const int sx = std::max(-4, std::min(4, ix)), sy = std::max(-4, std::min(4, iy));
        ix -= sx;
        iy -= sy;
        if (sx != 0) {
            MoveBy(a, sx, 0);
            if (IsBlocked(a, grid)) UndoMove(a);
            else moved = true;
        }
        if (sy != 0) {
            MoveBy(a, 0, sy);
            if (IsBlocked(a, grid)) UndoMove(a);
            else moved = true;
        }
    }
    return moved;
}

void FaceWorldDir(Actor& a, float wx, float wy) {
    if (std::abs(wx) < 1e-4f && std::abs(wy) < 1e-4f) return;
    if (std::abs(wx) >= std::abs(wy)) a.facing = wx > 0 ? 3 : 4;
    else a.facing = wy > 0 ? 1 : 2;
}

void SetMoveTarget(Actor& a, int x, int y) {
    a.moveTarget[0] = x;
    a.moveTarget[1] = y;
    a.animState = 1;
}

void SetStat(Actor& a, int stat, int value) {
    switch (stat) {
        case 2: a.level = static_cast<int8_t>(value); break;
        case 3: a.strength = value; break;
        case 4: a.intelligence = value; break;
        case 5: a.willpower = value; break;
        case 6: a.agility = value; break;
        case 7: a.speed = value; break;
        case 8: a.endurance = value; break;
        case 9: a.personality = value; break;
        case 10: a.weapon = static_cast<int8_t>(value); break;
        case 13: a.team = static_cast<int8_t>(value); break;
        case 14: a.sightRange = value; break;
        case 15: a.attackRange = value; break;
        case 18:
            a.aiType = static_cast<int8_t>(value);
            a.ranged = a.aiType == 4 ? 1 : 0;
            break;
        case 20: a.attackInterval = value * 1000; break;
        default: break;
    }
    a.maxHp = a.level * 4 + (a.strength + a.buffStrength) * 2 + a.endurance * 2 + a.bonusMaxHp;
    a.hp = std::min(a.hp, a.maxHp);
    a.maxMp = a.level * 4 + a.intelligence * 2 + a.bonusMaxMp;
    a.mp = std::min(a.mp, a.maxMp);
    if (a.sightRange == 0) a.sightRange = 300;  // defaultSightRange
    if (a.attackRange == 0) a.attackRange = 200;  // defaultAttackRange
}

void SetStatusIcon(Actor& a, int icon) {
    switch (icon) {
        case 0: a.statusIcon = -1; break;
        case 1: a.statusIcon = -53; break;
        case 2: a.statusIcon = -52; break;
        case 3: a.statusIcon = -51; break;
        case 4: a.statusIcon = -2; break;
        default: break;
    }
}

int8_t CheckTriggerTiles(Actor& a, const std::vector<int8_t>& enter, const std::vector<int8_t>& leave,
                         int gridHeight) {
    a.enterScript = -1;
    a.leaveScript = -1;
    if (enter.empty() || leave.empty()) return -1;
    const int idx[3] = {a.cell[0] * gridHeight + a.cell[1], a.footBCell[0] * gridHeight + a.footBCell[1],
                        a.footCCell[0] * gridHeight + a.footCCell[1]};
    for (int i : idx) {
        if (i < 0 || i >= static_cast<int>(enter.size())) return -1;
        int8_t e = enter[static_cast<size_t>(i)];
        if (e != 0 && e != -1) {
            a.enterScript = e;
            a.leaveScript = leave[static_cast<size_t>(i)];
            return e;
        }
    }
    return -1;
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

    // Enemy health bar.
    if (a.dead == 0 && a.slot != 1 && a.team == 0) {
        const int bx = sx + (a.sprite.Width(a.facing) >> 1) - 10;
        const int by = sy - a.sprite.Height(a.facing) - 6;
        bb.DrawRect(bx, by, 20, 3, 0xFFFFFF);
        bb.FillRect(bx + 1, by + 1, 19 * a.hp / std::max(1, a.maxHp), 2, 0xFF0000);
    }

    // Floating damage text: starts above the head, drifts up and fades.
    if (a.dead == 0 && !a.floatText.empty()) {
        if (a.floatTextY == 0) {
            a.floatTextStartY = a.floatTextY =
                a.screenPos[1] - a.sprite.Height(a.facing) - (a.slot == 1 ? 6 : 10);
            if (a.floatKind == 1) {  // dodged
                a.floatTextColor = 0x00FF00;
                a.floatTextShadow = 8704;
            } else if (a.floatKind == 2) {  // blocked
                a.floatTextColor = 0x0000FF;
                a.floatTextShadow = 34;
            } else {
                a.floatTextColor = 0xFF0000;
                a.floatTextShadow = 2228224;
            }
        }
        Text::DrawString(bb, sx + (a.sprite.Width(a.facing) >> 1) - 10, a.floatTextY + camY, a.floatText,
                         static_cast<uint32_t>(std::max(0, a.floatTextColor)), Text::Face::SmallPlain);
    }

    // Status icon (poison, buff ...) over the head.
    if (a.statusIcon != -1) {
        const int ix = sx + a.sprite.Width(a.facing) - 4;
        int iy = sy - a.sprite.Height(g) - a.sprite.Width(-54) - 4;
        if (a.slot != 1) iy -= 8;
        DrawSprite(bb, images, a.sprite, -54, ix, iy);
        if (a.statusIcon != -2) DrawSprite(bb, images, a.sprite, a.statusIcon, ix, iy);
    }
}

}  // namespace ActorSystem
}  // namespace oblivion
