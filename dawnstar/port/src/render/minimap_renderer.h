#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include "dungeon/dungeon_runtime.h"
#include "graphics/backbuffer.h"
#include "player/player_state.h"
#include "world/dungeon_generator.h"

namespace dawnstar {

// Image.createImage(89,89)'s counterpart -- GameCanvas's offscreen
// minimapImage, rebuilt by MinimapRenderer::Refresh below and composited
// onto the main Backbuffer every frame by Composite. A small dedicated
// pixel surface rather than a second Backbuffer, since it's a different
// fixed size and needs no PNG-sprite Blit -- just the two MIDP Graphics
// primitives GameCanvas.paintMinimapGrid actually uses (fillRect/
// drawRect).
class MinimapSurface {
public:
    static constexpr int kSize = 89;

    MinimapSurface() : pixels_(static_cast<size_t>(kSize) * kSize, 0) {}

    // Graphics.fillRect()'s counterpart, clamped to the surface -- same
    // "non-positive w/h is a no-op" convention as Backbuffer::FillRect
    // (graphics/backbuffer.h), and (unlike that one) also the mechanism
    // that makes paintMinimapGrid's real oversized-fill bug harmless
    // here: a request bigger than the 89x89 surface just clips down to
    // "fill everything" instead of overflowing.
    void FillRect(int x, int y, int w, int h, uint16_t rgb565);

    // Graphics.drawRect()'s counterpart: a 1px-thick outline whose
    // corners are (x,y) and (x+w,y+h) -- MIDP's own inclusive-corner
    // convention, so the outline is w+1 by h+1 pixels. A non-positive
    // w/h draws nothing -- both MIDP's own documented behavior for a
    // degenerate rectangle, and (see PaintMinimapGrid's own doc comment)
    // the deliberately-chosen stand-in for the one call site that
    // reaches an even more degenerate (undefined-on-real-hardware
    // anyway) case.
    void DrawRect(int x, int y, int w, int h, uint16_t rgb565);

    const std::vector<uint16_t>& Pixels() const { return pixels_; }

private:
    void SetPixel(int x, int y, uint16_t rgb565);
    std::vector<uint16_t> pixels_;
};

// Renamed-source counterpart of GameCanvas's minimap subsystem
// (refreshMinimap()/paintMinimapGrid()/sampleSquareView() plus
// paintGameView()'s own compositing step) -- M29.
//
// SIMPLIFIED: the compass glyph (`g.drawChar(COMPASS_GLYPHS[facing],
// ...)`, drawn alongside the minimap image in both zoom states) is NOT
// ported. It's real text/font rendering (MIDP's own built-in
// Font.getFont(), not a game-data asset this port has extracted
// anything for) -- this port has no text-rendering system at all yet
// (same reason every message-popup call site across M13-M28 is
// SIMPLIFIED away), not something specific to the minimap.
class MinimapRenderer {
public:
    // GameCanvas.refreshMinimap(): resamples the world around the
    // player (7x7 at 3px/cell normally, 17x17 at 5px/cell zoomed out --
    // p.minimapZoomedOut) via DungeonRuntime::SampleSquareView and
    // repaints `surface` from it. Only call when p.minimapDirty is set
    // (matching the original's own dirty-flag gate around this call,
    // not baked in here so callers can decide their own tick placement)
    // -- clears it either way, matching refreshMinimap()'s own first
    // line.
    static void Refresh(MinimapSurface& surface, PlayerState& p, const std::vector<GeneratedLevel>& levels,
                         const WorldRegistry& world);

    // GameCanvas.paintGameView()'s own minimap-compositing step (the
    // `if (!player.hasAilment(3))` gate and both drawImage calls; the
    // compass glyph is not ported -- see class comment). Draws
    // `surface` onto `bb` at its real fixed screen position: clipped to
    // a 23x23 window at (10,20) when not zoomed out (g.setClip(10, 20,
    // 23, 23) before that drawImage -- meaningful, since
    // paintMinimapGrid's own real bugs leave most of the 89x89 surface
    // either black or stale past that window anyway), or the full
    // surface drawn at (15,25) when zoomed out.
    static void Composite(Backbuffer& bb, const MinimapSurface& surface, const PlayerState& p);
};

}  // namespace dawnstar
