#pragma once
#include <cstdint>

#include "graphics/backbuffer.h"

namespace dawnstar {

// LoadingScreen.java's own `mode` values, renamed from its raw ints. Only the
// four progress-bar modes (8-11) are ported -- see LoadingScreen's own class
// comment below for what happened to modes 1/2.
enum class LoadingScreenMode {
    // `new LoadingScreen(this, 8, ...)` -- ESGame's own createGameUI, the
    // "Creating New Game" bar shown while createNewGame() generates all 37
    // levels on its background thread.
    CreatingNewGame = 8,
    // mode 9 / secondaryParam 302 -- `loadGameUI`, "Loading Game".
    LoadingGame = 9,
    // mode 10 / secondaryParam 303 -- `saveGameUI`, "Saving Game".
    SavingGame = 10,
    // mode 11 -- "Loading Dungeon", the per-level bar GameCanvas drives while
    // walking between levels.
    LoadingDungeon = 11,
};

// Renamed-source counterpart of LoadingScreen.java (`../src/LoadingScreen.java`,
// itself renamed from decompiled/h.java) -- but ONLY its modes 8-11, the plain
// "<action>... Please Wait" progress bars. That's the whole of what the Options
// menu's own "Save Game"/"Load Game" actions (M42) actually use: ESGame builds a
// fresh `new LoadingScreen(this, 10, 303)`/`(this, 9, 302)`, shows it, and drives
// its `percent` field from the background thread doing the real work.
//
// Modes 1/2 live in ui/boot_splash.h (M48), not here. What follows is the
// original reasoning for leaving them out of THIS class:
// DELIBERATELY NOT PORTED HERE -- modes 1/2 and everything that comes with them:
// `runSplashSequence()`'s own startup timing loop (hold until percent==100 AND
// 4s have passed, then the Vir2L/ZeniMax copyright card for 2s, the carrier logo
// for 1s), `startThread()`/`stopThread()`/`run()`/`waitAtLeast()`, and
// `renderSplash()`. That whole sequence is ESGame's own boot flow, which this
// port doesn't reproduce (main.cpp goes straight to M38's own MenuFlow), and it
// needs four images this port never loads (`splashImageTop`/`splashImageBot`/
// `vir2lLogoImage`/`carrierLogoImage`) plus `ESGame.copyString`. `unusedHook1()`/
// `unusedHook2()` are empty no-ops in LoadingScreen.java itself, and
// `onEnter()`/`onExit()` only ever start/stop the mode-2 thread, so none of them
// has anything to port for modes 8-11 either.
//
// A standalone class, NOT a `Screen` subclass as in the original: mode 8-11's own
// `renderProgress()` reads NOTHING from Screen's state -- no title, no items, no
// soft-key commands, no scroll position. It paints the whole backbuffer itself
// and draws its own two lines of text plus the bar, so inheriting Screen's ~20
// fields here would carry nothing but dead weight. (The original only extends
// Screen because Screen is where its `width()`/`height()`/`canvas`/`game`
// plumbing lives -- all four of which this port already replaces with
// Backbuffer's own constants, per ui/screen.h's own class comment.)
//
// Text rendering reuses M30's `BitmapFont`, the same single invented monospace
// font every other text-drawing module in this port uses in place of
// LoadingScreen.java's own `LARGE_TEXT_FONT` (an unrecoverable MIDP
// `Font.getFont(...)` system font -- see ui/screen.h's own class comment on why
// all four of Screen's fonts collapse onto it here too).
class LoadingScreen {
public:
    explicit LoadingScreen(LoadingScreenMode mode) : mode_(mode) {}

    LoadingScreenMode Mode() const { return mode_; }

    // LoadingScreen.java's own `volatile int percent` -- written by whatever is
    // doing the work (ESGame's background thread there; GameSave's own
    // ProgressCallback here) and read by paint(). Deliberately NOT clamped on
    // either set or draw: the original clamps only at its own call sites (e.g.
    // openAndRepopulateDungeons()'s `if (percent > 100) percent = 100`), and
    // `renderProgress()` itself happily computes `percent * 88 / 100` for any
    // value -- an over-100 percent simply draws a bar wider than its outline box,
    // which Backbuffer::FillRect's own clipping makes harmless.
    int Percent() const { return percent_; }
    void SetPercent(int percent) { percent_ = percent; }

    // `renderProgress(g)`: the shared 2510210-blue full-screen background, the
    // mode's own white centered action line at y=30, "Please Wait" at y=45, a
    // white 90x20 outline box at y=60, and the blue `percent * 88 / 100`-wide,
    // 18px-tall bar inset 1px inside it.
    void Render(Backbuffer& bb) const;

    // Screen.java's own `width()` -- this port's fixed 176 (see ui/screen.h's
    // own class comment on why that's a constant here rather than a queried
    // device value). Exposed so a caller can reproduce the same centering
    // arithmetic the original's call sites do.
    static int Width() { return Backbuffer::kWidth; }

private:
    LoadingScreenMode mode_;
    int percent_ = 0;
};

}  // namespace dawnstar
