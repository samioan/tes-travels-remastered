#pragma once
#include "graphics/backbuffer.h"
#include "render/flash_overlay_assets.h"
#include "util/java_random.h"

namespace stormhold {

// GameCanvas's 3 one-shot flash-overlay trigger flags (unconfirmed_S/
// unconfirmed_ao/unconfirmed_am) -- GameCanvas *static* fields in the
// original, folded into their own small state struct the same way
// MessagePopupState (M30)/HudState (M29) already do for their own
// GameCanvas statics.
struct FlashOverlayState {
    bool hit = false;              // unconfirmed_S: monster-hit flash
    bool spellHitMonster = false;  // unconfirmed_ao: spell-hit-monster flash
    bool spellHitSelf = false;     // unconfirmed_am: self-spell-hit flash
};

// Renamed-source counterpart of GameCanvas.paintFlashOverlays() (M22's
// own confirmed transcription -- see its own header comment there; this
// milestone only ports the body, the Java side was already done). Three
// independent one-shot flash overlays, each self-clearing its own flag
// once drawn, at a small random jittered position via Util.randomInt()
// (RandomInt1Based, ESGame.randomInt()'s own 1-based convention -- see
// java_random.h's own header comment).
//
// **A deliberate divergence from the real game's own shared static RNG:**
// the original draws these jitter offsets from ESGame's single
// process-wide `Random rng` -- the SAME stream combat/dungeon generation
// etc. all draw from -- consumed once per rendered frame purely for
// cosmetic screen-space jitter with no gameplay effect whatsoever. This
// port already keeps combat/ambush spawning on their own dedicated
// JavaRandom instances rather than one shared stream (M16/M37/M42's own
// "confirmed different RNG fidelity" findings), so reusing the caller's
// own combat RNG here for this purely-visual jitter is consistent with
// that split, not a new divergence.
//
// Only `hit` (unconfirmed_S) has a wired trigger site anywhere in this
// port so far -- CombatResolution::ResolveAttackInput's own return value
// (M39), previously discarded by main.cpp. `spellHitMonster`/
// `spellHitSelf` are painted for real here but stay permanently
// false/unreachable until spell casting itself is wired (see
// docs/PORT_ROADMAP.md's own "what's next").
class FlashOverlay {
public:
    static void Paint(Backbuffer& bb, FlashOverlayState& state, const FlashOverlayAssets& assets, JavaRandom& rng);
};

}  // namespace stormhold
