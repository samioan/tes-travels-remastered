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
// paintGameView()'s own compositing step) -- M29, plus the compass
// glyph (M55).
//
// The compass glyph (`g.drawChar(COMPASS_GLYPHS[facing], ...)`, drawn
// alongside the minimap image in both zoom states) went unported at M29
// because this port had no text-rendering system at all yet (same
// reason every message-popup call site across M13-M28 was SIMPLIFIED
// away then). `graphics/bitmap_font.h`'s hand-authored `BitmapFont` --
// added at M31 for the hotbar's own digit glyphs, and already covering
// every letter A-Z including N/E/S/W -- closes that gap; `Composite`
// below now draws it. One real simplification remains: the original
// switches to a second, larger MIDP built-in font
// (`COMPASS_FONT_ZOOMED`, `Font.getFont(64, 1, 16)`) only while zoomed
// out, and `BitmapFont` -- a single hand-invented glyph set, not a real
// recovered font with size variants -- has no larger counterpart to
// switch to, so the same font/size draws the glyph in both zoom states.
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
    // `if (!player.hasAilment(3))` gate, the compass glyph, and both
    // drawImage calls). Draws `surface` onto `bb` at its real fixed
    // screen position: clipped to a 23x23 window at (10,20) when not
    // zoomed out (g.setClip(10, 20, 23, 23) before that drawImage --
    // meaningful, since paintMinimapGrid's own real bugs leave most of
    // the 89x89 surface either black or stale past that window anyway),
    // or the full surface drawn at (15,25) when zoomed out. Also draws
    // the facing-direction compass glyph (see class comment) at its own
    // real fixed position, (16,10) not zoomed / (58,10) zoomed out --
    // `COMPASS_GLYPHS[player.facing]`, white, drawn before the image in
    // the original (paint order doesn't matter here: the glyph and image
    // positions never overlap).
    static void Composite(Backbuffer& bb, const MinimapSurface& surface, const PlayerState& p);
};

}  // namespace dawnstar
