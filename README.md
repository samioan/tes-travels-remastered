# tes-travels-decomp

Decompilation projects for three *Elder Scrolls Travels* J2ME (mobile
Java) games: **Dawnstar**, **Oblivion**, and **Stormhold**. Sibling to
`shadowkey-decomp` (the Symbian/N-Gage native-binary decompilation of
*Shadowkey*), following the same overall process -- understand the
original game, then build a from-scratch PC port -- adapted for a very
different target format.

## The targets

`roms/*.jar` (kept out of git, see `.gitignore`) -- three standalone
J2ME MIDlet builds:

| Game | Vendor | Engine | Entry point |
|---|---|---|---|
| Dawnstar | mob.ua / "milaro" | Vir2L "ngame" (shared with Stormhold) | `ESGame` |
| Stormhold | **Vir2L Studios** (same studio as Shadowkey) | Vir2L "ngame" | `ESGame` |
| Oblivion | Superscape | Superscape BLT/Swerve | `blt.Main` |

Dawnstar and Stormhold share an engine (`ngame.midlet` base MIDlet class,
`ESGame` subclass, same asset-table conventions) even though they shipped
under different vendors -- see each game's `docs/ROADMAP.md` for the
evidence. Oblivion is unrelated: built with Superscape's own mobile 3D
middleware, storing level/scene/script content as opaque binary resource
files (`.scr`/`.cml`/`.jtm`) rather than in Java code.

Unlike Shadowkey, these are **plain JVM bytecode**, not a native ARM
binary -- there's no disassembler-loader step, no Ghidra project. A Java
decompiler (Vineflower) reconstructs real, close-to-original Java source
directly from the `.class` files.

## Structure

Each game gets an identical layout:

- `<game>/extracted/` -- the jar's raw contents, unzipped (gitignored,
  regenerate with `tools/extract_jar.py`).
- `<game>/decompiled/` -- Java source recovered by Vineflower (tracked;
  this is the actual decompilation output, analogous to shadowkey-decomp's
  Ghidra decompiler output). Classes are still obfuscated to single/double
  letter names (`a.class`, `b.class`, ...) -- renaming them in place as
  their roles are understood is phase 1 of each game's roadmap.
- `<game>/docs/ROADMAP.md`, `<game>/docs/ASSET_FORMATS.md` -- per-game
  status and what's known/unknown about that game's custom binary asset
  formats.
- `<game>/port/` -- the PC port scaffold: CMake + Ninja + MSVC, C++17,
  matching `shadowkey-decomp/port`'s toolchain exactly (same `build.bat`
  invocation of the VS Build Tools). Currently a placeholder Win32 window
  only, proven to build -- no game logic ported yet.

Shared:

- `roms/` -- the original `.jar` files (gitignored -- copyrighted game
  data, never commit).
- `tools/` -- `extract_jar.py`, `decompile.py` (the jar -> Java pipeline,
  see `tools/README.md`), `vineflower.jar` (gitignored, fetched).

## Status

**Phase 0 is done for all three games**: jars unpacked, decompiled to
readable Java, engines and asset-format families identified (see each
game's `docs/`), PC port toolchain proven working (all three `port/`
scaffolds configure, build, and link cleanly with `port/build.bat`).

**Dawnstar's phase 1 is done**: all 13 decompiled classes read through,
understood, and renamed into a compile-checked `dawnstar/src/` reference
tree (see `dawnstar/docs/CLASS_MAP.md` and `dawnstar/src/README.md`).
**Phase 3 (the PC port) is well underway** -- a real, playable build with
its own launcher; see "Playing it" below.

**Stormhold's phase 1 is done** the same way (see
`stormhold/docs/CLASS_MAP.md`), reusing most of Dawnstar's own findings
where the two games' shared Vir2L "ngame" engine matches, documenting the
real differences where it doesn't. **Phase 3 (the PC port) is underway**
-- movement, combat and starting a new character are playable, with its
own launcher; see "Playing it" below.

**Oblivion's phase 1 has not started.** Its `.scr`/`.cml`/`.jtm` formats
are fully opaque and will need the Superscape engine's own parsing code
read through first.

## Playing it

Both Dawnstar and Stormhold have a real, downloadable build: a small
launcher (`Dawnstar.exe`/`Stormhold.exe`) that asks for your own copy of
the matching `.jar`, unpacks it, and starts the game -- no other setup,
and each updates itself from this repo's
[Releases](https://github.com/samioan/tes-travels-remastered/releases)
page (tagged `dawnstar-v*`/`stormhold-v*` respectively, so the two never
fight over "the latest release"). See `dawnstar/port/src/launcher/`/
`stormhold/port/src/launcher/` for how each one works and
`dawnstar/docs/ITCH_PAGE.md` for the itch.io page copy. Stormhold is
still missing saving/loading, the inventory/skills/spells menu, and NPCs
-- see `stormhold/docs/PORT_ROADMAP.md` for exactly what's ported so far.
