#include "world/projectiles.h"

#include "render/sprite_renderer.h"
#include "world/combat.h"

namespace oblivion {

namespace {

constexpr int kSlots = 99;   // 11 * 9
constexpr int kStride = 9;

int16_t S(int v) { return static_cast<int16_t>(v); }

}  // namespace

int Projectiles::FindFree() const {
    for (int i = 0; i < kSlots - kStride; i += kStride)
        if (pool_[i] == -1) return i;
    return -1;
}

void Projectiles::ClearAll() {
    for (int i = 0; i < kSlots; i += kStride) Clear(i);
}

void Projectiles::Clear(int slot) {
    if (slot < 0 || slot >= kSlots) return;
    for (int k = 0; k < kStride; k++) pool_[slot + k] = -1;
}

void Projectiles::ClearAt(int x, int y) {
    for (int i = 0; i < kSlots; i += kStride) {
        if (pool_[i] != -1 && pool_[i + 1] == x && pool_[i + 2] == y) {
            Clear(i);
            return;
        }
    }
}

int Projectiles::SpawnFromActor(int dir, int subtype, const Actor& a, int duration) {
    const int slot = FindFree();
    if (slot == -1) return slot;
    if (dir == 0) {
        if (subtype == 2) dir = 0;
        else if (subtype == 1) dir = 2;
        else if (subtype == 3) dir = 4;
        else if (subtype == 4) dir = 6;
    } else if (dir == 11) {
        if (subtype == 2) dir = 11;
        else if (subtype == 1) dir = 12;
        else if (subtype == 3) dir = 13;
        else if (subtype == 4) dir = 14;
    }
    pool_[slot + 0] = S(-4096 | (a.slot << 8) | dir);
    pool_[slot + 1] = S(a.pos[0]);
    pool_[slot + 2] = S(a.pos[1]);
    pool_[slot + 5] = S(a.pos[0]);
    pool_[slot + 6] = S(a.pos[1]);
    pool_[slot + 3] = 0;
    pool_[slot + 4] = 0;
    pool_[slot + 7] = S(duration);
    pool_[slot + 8] = 0;
    return slot;
}

int Projectiles::SpawnFixed(int type, int x, int y, int duration) {
    const int slot = FindFree();
    if (slot == -1) return -1;
    pool_[slot + 0] = S(type);
    pool_[slot + 1] = S(x);
    pool_[slot + 2] = S(y);
    pool_[slot + 5] = S(x);
    pool_[slot + 6] = S(y);
    pool_[slot + 3] = 0;
    pool_[slot + 4] = 0;
    pool_[slot + 7] = S(duration);
    pool_[slot + 8] = 0;
    return slot;
}

// A move step ended (or the timer ran out): find the nearest enemy of the
// source within 200 and hit it.
bool Projectiles::OnExpire(int slot, CombatHost& host) {
    const int pos[2] = {pool_[slot + 1], pool_[slot + 2]};
    const int sourceIndex = (pool_[slot + 0] & 4095) >> 8;
    if (sourceIndex <= 0 || sourceIndex >= host.SlotCount()) {
        Clear(slot);
        return false;
    }
    Actor* source = host.ActorSlot(sourceIndex - 1);
    if (!source) {
        Clear(slot);
        return false;
    }
    int bestSlot = -1, bestDist = 16777215;
    for (int i = 0; i < host.SlotCount(); i++) {
        Actor* o = host.ActorSlot(i);
        if (!o || o->dead == 1 || o == source || source->team == o->team) continue;
        const int dist = Combat::Distance(pos, o->pos);
        if (dist < 200 && dist < bestDist) {
            bestSlot = i;
            bestDist = dist;
        }
    }
    if (bestSlot == -1) return false;
    Combat::Attack(*source, *host.ActorSlot(bestSlot), false, host);
    return true;
}

void Projectiles::Tick(int dt, CombatHost& host) {
    for (int i = 0; i < kSlots; i += kStride) {
        if (pool_[i] == -1) continue;
        pool_[i + 3] = S(pool_[i + 3] + dt);
        pool_[i + 8] = S(pool_[i + 8] + dt);
        if (pool_[i + 7] > 0 && pool_[i + 8] >= pool_[i + 7]) {
            pool_[i + 4] = 0;
            pool_[i + 8] = 0;
        }
        if ((pool_[i + 4] & 0xFF00) == 0xFF00 || pool_[i + 3] <= 100) continue;

        pool_[i + 3] = 0;
        pool_[i + 4]++;
        const int dir = pool_[i] & 255;
        if ((dir >= 0 && dir <= 6) || (dir >= 11 && dir <= 14)) {
            // Flying (and, on odd dirs, the impact animation).
            if (dir == 0) pool_[i + 2] = S(pool_[i + 2] - 60);
            else if (dir == 2) pool_[i + 2] = S(pool_[i + 2] + 60);
            else if (dir == 4) pool_[i + 1] = S(pool_[i + 1] + 60);
            else if (dir == 6) pool_[i + 1] = S(pool_[i + 1] - 60);
            if (dir == 11) pool_[i + 2] = S(pool_[i + 2] - 150);
            else if (dir == 12) pool_[i + 2] = S(pool_[i + 2] + 150);
            else if (dir == 13) pool_[i + 1] = S(pool_[i + 1] + 150);
            else if (dir == 14) pool_[i + 1] = S(pool_[i + 1] - 150);

            if (frames_.SetFrame(dir, pool_[i + 4])) {
                if (dir != 1 && dir != 3 && dir != 5 && dir != 7) {
                    pool_[i + 4] = 0;
                    frames_.SetFrame(dir, 0);
                } else {
                    Clear(i);
                    continue;
                }
            }
            const int pos[2] = {pool_[i + 1], pool_[i + 2]};
            const int origin[2] = {pool_[i + 5], pool_[i + 6]};
            if (Combat::Distance(pos, origin) > 750 || OnExpire(i, host)) {
                if (pool_[i] == -1) continue;  // OnExpire cleared it
                if (dir >= 11 && dir <= 14) Clear(i);
                else if (dir == 0 || dir == 2 || dir == 4 || dir == 6) pool_[i]++;
            }
        } else {
            // Effect that sits on its source actor (types 8..10) or on a fixed spot.
            if ((pool_[i] & -4096) == -4096) {
                const int src = ((pool_[i] & 4095) >> 8) - 1;
                Actor* a = src < 0 || src >= host.SlotCount() ? nullptr : host.ActorSlot(src);
                if (!a) {
                    Clear(i);
                    continue;
                }
                pool_[i + 1] = S(a->pos[0]);
                pool_[i + 2] = S(a->pos[1]);
            }
            if (frames_.SetFrame(dir, pool_[i + 4])) {
                if (pool_[i + 7] <= 0) Clear(i);
                else pool_[i + 4] = S(pool_[i + 4] | 0xFF00);
            }
        }
    }
}

void Projectiles::Draw(Backbuffer& bb, ImageCache& images, int camX, int camY) {
    for (int i = 0; i < kSlots; i += kStride) {
        if (pool_[i] == -1 || (pool_[i + 4] & 0xFF00) == 0xFF00) continue;
        const int sx = ((pool_[i + 1] - pool_[i + 2]) >> 3) + camX;
        const int sy = ((pool_[i + 1] + pool_[i + 2]) >> 4) + camY;
        if (sx >= 0 && sx <= bb.Width() && sy >= 0 && sy <= Backbuffer::kHeight) {
            const int type = pool_[i] & 255;
            frames_.SetFrame(type, pool_[i + 4]);
            DrawSprite(bb, images, frames_, type, sx, sy);
        }
    }
}

}  // namespace oblivion
