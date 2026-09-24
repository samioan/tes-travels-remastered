# Notices and attribution

This project is a from-scratch, behavioural re-implementation of *The
Elder Scrolls Travels: Dawnstar* (J2ME/mobile Java), written by reading
the original game's own decompiled bytecode (see `dawnstar/docs/`). It is
not affiliated with, endorsed by, or supported by Bethesda Softworks,
ZeniMax, or the game's original developer.

The same repository also contains *The Elder Scrolls Travels: Stormhold*
(see `stormhold/docs/`), which now has its own distributable PC port and
launcher too, and reverse-engineering work in progress on *The Elder
Scrolls Travels: Oblivion* (see `oblivion/docs/`), which does not yet.

## What this project does not contain, and never will

**The game.** No script, image, sound, string table or byte of the
original `TEST-Dawnstar.jar`/`TEST-Stormhold.jar` is committed here, and
none is distributed with any release. Each launcher asks for your own
copy and will not run without it. See `.gitignore`.

**The phone's fonts.** `Ceurope.gdr` and `Browsereur.gdr` are the Nokia
Series 60 phones' own ROM fonts, which both games' text was drawn in.
They are Nokia device firmware: never committed and never shipped. Each
launcher asks for them separately, and the games run (with stand-in
letters) without them. The code that reads them
(`*/port/src/assets/gdr_font.cpp`) is this project's own, written from
Nokia's EPL-licensed Symbian sources rather than taken from any other
font reader.

## Artwork

Each launcher's background is this project's own key art:

| Game | Full-resolution master | Embedded copy | Icon |
|---|---|---|---|
| Dawnstar | `dawnstar/port/src/launcher/assets/banner_source.jpg` | `banner.png` | crop of the same image |
| Stormhold | `stormhold/port/src/launcher/assets/banner_source.jpg` | `banner.png` | crop of the same image |

Each embedded/icon copy is generated from its own master by
`tools/make_banner.py`/`tools/make_icon.py` (both take a `-o` to target
either game's own `assets/` directory).

"The Elder Scrolls", "Dawnstar" and "Stormhold" are trademarks of ZeniMax
Media Inc. The wordmark visible in that artwork is theirs, not this
project's.

## Third-party code

| Component | Licence | Where |
|---|---|---|
| `stb_image` | public domain / MIT | `dawnstar/port/third_party/stb/` and `stormhold/port/third_party/stb/`, see each one's own `PROVENANCE.md` |
| `puff` (zlib's reference inflate) | zlib | `dawnstar/port/third_party/puff/` and `stormhold/port/third_party/puff/`, see each one's own `PROVENANCE.md` |

Each launcher's file/folder pickers use `IFileOpenDialog` (COM, a Windows
system component) and its update check uses WinHTTP (also a system
component) -- nothing extra to redistribute, and no Visual C++ runtime to
install: release builds link the CRT statically.
