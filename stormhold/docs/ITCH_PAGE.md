# itch.io page copy

The text for the store page, kept in the repo so it can be revised like
anything else rather than living only in a web form. Paste into itch's
description field; it takes Markdown. Adapted from dawnstar's own
`dawnstar/docs/ITCH_PAGE.md` (itself from shadowkey-decomp's version of
this doc).

Page settings that matter:

- **Kind of project**: Downloadable. Not "HTML" -- there is nothing to run
  in a browser.
- **Pricing**: free. This is someone else's game; charging for a port of
  it would be indefensible whatever the licence situation.
- **Platforms**: Windows only for now.
- **Upload**: the `StormholdRemastered-<tag>-win64.zip` from the matching
  GitHub release, marked "Windows" and **not** "This file will be played in
  the browser".
- **Cover image**: `stormhold/port/src/launcher/assets/banner_source.jpg`,
  cropped to itch's 630x500.
- Leave **"Generate itch.io app manifest"** off -- the launcher already
  updates itself from GitHub, and two update mechanisms fighting over the
  same folder is a bug waiting to happen.

---

## The Elder Scrolls Travels: Stormhold -- Remastered

**Stormhold on PC, rebuilt from the original game's own code.**

*The Elder Scrolls Travels* was a short-lived series of Elder Scrolls
spin-offs for mobile Java (J2ME) phones -- full first-person RPGs, stat
systems, spellcasting and all, squeezed onto phone hardware from the
mid-2000s. Stormhold is one of them, and it has been effectively
unplayable for years unless you still have a compatible phone (or
emulator) and the original game file.

This is that game, running natively on Windows: not an emulator, but a
re-implementation written by reading the original game's own decompiled
bytecode and rebuilding what it does, piece by piece.

### What you need

**This download does not include the game.** It cannot -- the game
belongs to Bethesda. You supply your own copy of `TEST-Stormhold.jar`.

1. Download and unzip anywhere.
2. Run **Stormhold.exe**.
3. Choose your `.jar` file -- the launcher unpacks it for you.
4. Press **Play**.

From the Main Menu, choose New Game, pick a class, name your character,
and read through (or skip past) the introduction to start playing.

### Notes

- **Portable.** No installer, no registry keys, nothing in AppData, no
  Visual C++ redistributable. Your game file is unpacked into `data\`.
  Uninstalling is deleting the folder.
- **Pick your window size** -- 2x, 3x or 4x the original resolution.
- **It updates itself** from the GitHub releases, and asks first.

### This is an early work in progress

Movement, combat, and starting a new character all work. **Saving and
loading a game do not exist yet** -- there's nothing to lose, but there's
also no way to keep playing across two sessions. Also not yet ported: the
inventory/skills/spells menu, shops and NPCs, and camping.

If something looks wrong, `user\stormhold_port.log` records what the game
did and is the right thing to attach to a bug report.

Source, the full milestone-by-milestone record of how it was reverse
engineered, and the issue tracker:
<https://github.com/samioan/tes-travels-remastered>

---

*The Elder Scrolls* and *Stormhold* are trademarks of ZeniMax Media Inc.
This project is not affiliated with, endorsed by, or supported by
Bethesda Softworks, ZeniMax or the game's original developer.
