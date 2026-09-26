#pragma once
#include <cstdint>

#include "assets/cml.h"
#include "assets/image.h"
#include "graphics/backbuffer.h"
#include "world/actor.h"

namespace oblivion {

class CombatHost;

// ProjectileManager (src/ProjectileManager.java): a pool of 11 magic-effect /
// ranged-attack slots sharing one sprite set (/oh_magic.cml, group = type).
// Slots are 9 int16 each, exactly like the original's short[99]:
//   [0] packed type + source (high nibble 0xF = 1-based actor slot in bits 8..11,
//       type in the low byte) or a plain type for fixed-position effects,
//   [1]/[2] x/y, [3] ms since the last step, [4] frame (0xFF00 = finished),
//   [5]/[6] origin (or last position), [7] lifetime ms (0 = until it ends),
//   [8] age ms.
// Slot handles are offsets (0, 9, 18 ...), as in the original.
class Projectiles {
public:
    Projectiles() { ClearAll(); }
    void SetFrames(SpriteSet frames) { frames_ = std::move(frames); }

    void ClearAll();
    void Clear(int slot);
    void ClearAt(int x, int y);

    // spawn(dir, subtype, actor[, duration]): at the actor, following it
    // (types 8..10) or flying in the actor's facing (0 / 11 with a subtype).
    int SpawnFromActor(int dir, int subtype, const Actor& a, int duration = 0);
    // spawn(type, x, y) / spawnFixed: a fixed effect at a world position.
    int SpawnFixed(int type, int x, int y, int duration = 0);

    void Tick(int dtMs, CombatHost& host);
    void Draw(Backbuffer& bb, ImageCache& images, int camX, int camY);

private:
    int FindFree() const;
    bool OnExpire(int slot, CombatHost& host);

    int16_t pool_[99];
    SpriteSet frames_;
};

}  // namespace oblivion
