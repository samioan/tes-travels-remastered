#include "render/sprite_renderer.h"

namespace oblivion {

int DrawSprite(Backbuffer& bb, ImageCache& images, const SpriteSet& set, int groupId, int x, int y) {
    const SpriteGroup* g = set.Find(groupId);
    if (!g || g->frames.empty()) return 0;
    const SpriteFrame& f = g->frames[g->current];
    std::shared_ptr<Image> img = images.Get(f.imagePath);
    if (f.frameChain == 0) {
        bb.Blit(*img, 0, 0, img->width, img->height, x + f.dx, y + f.dy);
        return f.width;
    }
    int clipX = x + f.dx;
    int clipY = y + f.dy;
    if (clipX < bb.Width() && clipY < Backbuffer::kHeight) {
        int clipW = std::min<int>(bb.Width(), f.width);
        int clipH = std::min<int>(Backbuffer::kHeight, f.height);
        if (f.isSprite == 1) {
            // The whole sheet, mirrored, positioned so this frame window
            // lands on the clip rectangle.
            int px = x + f.width - img->width + f.offsetX + f.dx;
            int py = y - f.offsetY + f.dy;
            bb.Blit(*img, 0, 0, img->width, img->height, px, py, true, clipX, clipY, clipX + clipW, clipY + clipH);
        } else {
            int px = x - f.offsetX + f.dx;
            int py = y - f.offsetY + f.dy;
            bb.Blit(*img, 0, 0, img->width, img->height, px, py, false, clipX, clipY, clipX + clipW, clipY + clipH);
        }
    }
    return f.width;
}

}  // namespace oblivion
