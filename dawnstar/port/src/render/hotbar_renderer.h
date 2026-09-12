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
// NOT ported here or anywhere yet: GameCanvas.paintActionFlashes()
// (monsterHitFlash/spellHitFlash/selfSpellFlash -- also drawHotbarIcon-
// based, but driven entirely by the still-unwired attack/spellcast
// combat flow) and the actual keyPressed() dispatch that reads
// hotbarContext back (attackRequested/interactRequested/campRequested --
// same reason). See docs/PORT_ROADMAP.md's M31 entry.
class HotbarRenderer {
public:
    // GameCanvas.computeHotbarContext(): 0 = exploring, 1 = a monster is
    // targeted (combat hotbar), 2 = a chest or NPC is in sight (interact
    // hotbar). `monsterTargeted` is a plain parameter here rather than a
    // PlayerState field -- unlike M28/M29's own "GameCanvas statics
    // folded into PlayerState" precedent -- because nothing in this
    // port ever sets it yet: GameCanvas.monsterTargeted is only ever
    // written by the combat attack-targeting flow, which isn't wired
    // into the live tick loop (see docs/PORT_ROADMAP.md). Adding a
    // PlayerState field nothing writes would just be dead state; every
    // real call site in this port passes `false` until that combat
    // wiring lands, at which point this signature already has the hook
    // ready for it.
    static int ComputeHotbarContext(bool monsterTargeted, bool chestInSight, int npcInSight);

    // Paints the panel background plus whichever of the 3 digit/icon
    // rows `context` (0/1/2, from ComputeHotbarContext above) selects.
    // An out-of-range context (unreachable -- ComputeHotbarContext only
    // ever returns 0/1/2) draws just the panel background, matching the
    // original's own if/else-if chain with no final else.
    static void Paint(Backbuffer& bb, const HotbarTextures& textures, int context);
};

}  // namespace dawnstar
