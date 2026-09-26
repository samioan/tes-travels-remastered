#pragma once
#include "assets/cml.h"
#include "assets/image.h"
#include "graphics/backbuffer.h"

namespace oblivion {

// SpriteRenderer.draw: draws the current frame of group `groupId` with its
// origin at (x, y); returns the frame width (0 if the group is missing).
// Static images are drawn whole at (x+dx, y+dy). Animation frames are windows
// (offsetX, offsetY, width, height) into their sheet drawn at (x+dx, y+dy),
// mirrored when isSprite == 1 (Sprite.setTransform(TRANS_MIRROR)).
int DrawSprite(Backbuffer& bb, ImageCache& images, const SpriteSet& set, int groupId, int x, int y);

}  // namespace oblivion
