#pragma once
#include "assets/decoded_image.h"
#include "assets/img_archive.h"
#include "graphics/backbuffer.h"

namespace dawnstar {

// GameCanvas.iconsSprite/panelImage -- the hotbar's own two image assets
// (ESGame.runImageLoader()'s createImage("icons.png")/("panel.png")),
// decoded once up front like every other image-loading milestone
// (M7/M10/M26).
struct HotbarTextures {
    // 270x24 (confirmed against the real archive, not assumed): 9 icons,
    // 30px each (index 0-8), horizontally tiled -- GameCanvas.
    // drawHotbarIcon()'s own frame math (`x - 30 * iconIdx`). Exactly
    // 24 tall, matching drawHotbarIcon's own 24px clip height, so no
    // separate vertical clip is needed here (Backbuffer::Blit's
    // [clipX0, clipX1) range alone reproduces the original's
    // g.setClip(x, y, 30, 24)).
    DecodedImage icons;
    // 176x52 (confirmed against the real archive): the bottom panel
    // background, drawn unscaled at (0, 156) -- exactly fills the
    // 176x208 backbuffer's last 52 rows.
    DecodedImage panel;

    static HotbarTextures Load(const ImgArchive& archive);
};

// Renamed-source counterpart of GameCanvas.paintHotbar()/
// computeHotbarContext()/drawHotbarIcon() -- the bottom hotbar panel:
// the panel background plus 4 numeric-key prompts (a context-dependent
// digit char, drawn as a black "shadow" 1px down-right of a white fill,
// matching the original's own two-pass drawChar calls exactly) and
// their 4 icons, all at 4 fixed positions.
//
// GameCanvas.paintActionFlashes() is fully ported here (as
// PaintActionFlashIcon below): monsterHitFlash since M32, spellHitFlash/
// selfSpellFlash since M33. The actual keyPressed() dispatch that reads
// hotbarContext back for camp/interact/options
// (campRequested/interactRequested/optionsRequested) is still not
// ported -- no camp/shop/options UI exists yet (cast/cycle don't
// consult hotbarContext at all, so they needed no such UI to wire).
class HotbarRenderer {
public:
    // GameCanvas.computeHotbarContext(): 0 = exploring, 1 = a monster is
    // targeted (combat hotbar), 2 = a chest or NPC is in sight (interact
    // hotbar). `monsterTargeted` is a plain parameter (a plain read of
    // player.monsterTargeted at each of this port's 2 call sites -- see
    // main.cpp) rather than PlayerState itself being threaded straight
    // in, simply because this function only ever needs that one bit of
    // it.
    static int ComputeHotbarContext(bool monsterTargeted, bool chestInSight, int npcInSight);

    // Paints the panel background plus whichever of the 3 digit/icon
    // rows `context` (0/1/2, from ComputeHotbarContext above) selects.
    // An out-of-range context (unreachable -- ComputeHotbarContext only
    // ever returns 0/1/2) draws just the panel background, matching the
    // original's own if/else-if chain with no final else.
    static void Paint(Backbuffer& bb, const HotbarTextures& textures, int context);

    // GameCanvas.paintActionFlashes()'s own reuse of drawHotbarIcon()
    // for its one-shot combat-flash icons -- exposed publicly (unlike
    // Paint's own 4 fixed per-context placements above) since these use
    // per-event RANDOM offsets the original picks at the call site, not
    // a fixed table (see main.cpp's own wiring for all 3 reachable
    // cases: monsterHitFlash iconIdx 6, selfSpellFlash iconIdx 7,
    // spellHitFlash iconIdx 8).
    static void PaintActionFlashIcon(Backbuffer& bb, const HotbarTextures& textures, int iconIdx, int x, int y);
};

}  // namespace dawnstar
