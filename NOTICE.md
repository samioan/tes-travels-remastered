# Notices and attribution

This project is a from-scratch, behavioural re-implementation of *The
Elder Scrolls Travels: Dawnstar* (J2ME/mobile Java), written by reading
the original game's own decompiled bytecode (see `dawnstar/docs/`). It is
not affiliated with, endorsed by, or supported by Bethesda Softworks,
ZeniMax, or the game's original developer.

The same repository also contains reverse-engineering work in progress on
*The Elder Scrolls Travels: Oblivion* and *The Elder Scrolls Travels:
Stormhold* -- see `oblivion/docs/` and `stormhold/docs/` -- neither of
which has a distributable PC port yet.

## What this project does not contain, and never will

**The game.** No script, image, sound, string table or byte of the
original `TEST-Dawnstar.jar` is committed here, and none is distributed
with any release. The launcher asks for your own copy and will not run
without it. See `.gitignore`.

## Artwork

The launcher's background is this project's own key art. The
full-resolution master is committed at
`dawnstar/port/src/launcher/assets/banner_source.jpg`; the downscaled copy
the launcher actually embeds is `banner.png`, and the application icon is
a crop of the same image (see `tools/make_banner.py` and
`tools/make_icon.py`).

"The Elder Scrolls" and "Dawnstar" are trademarks of ZeniMax Media Inc.
The wordmark visible in that artwork is theirs, not this project's.

## Third-party code

| Component | Licence | Where |
|---|---|---|
| `stb_image` | public domain / MIT | `dawnstar/port/third_party/stb/`, see its own `PROVENANCE.md` |
| `puff` (zlib's reference inflate) | zlib | `dawnstar/port/third_party/puff/`, see its own `PROVENANCE.md` |

The launcher's file/folder pickers use `IFileOpenDialog` (COM, a Windows
system component) and its update check uses WinHTTP (also a system
component) -- nothing extra to redistribute, and no Visual C++ runtime to
install: release builds link the CRT statically.
