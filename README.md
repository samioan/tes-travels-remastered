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

**Dawnstar's phase 1+ is underway**: 9 of its 13 decompiled classes are
read through, understood, and mechanically renamed into a compile-checked
`dawnstar/src/` reference tree (see `dawnstar/docs/CLASS_MAP.md` and
`dawnstar/src/README.md`); its asset formats (`datfiles.lmp`,
`imgfiles.lmp`, and every `*.dat` data table) are fully confirmed. Given
Dawnstar and Stormhold share Vir2L's "ngame" engine, Stormhold should
reuse most of this work rather than starting from scratch.

**Oblivion and Stormhold's phase 1+ has not started.** Stormhold's
data-table formats are partially crackable from a hexdump alone the same
way Dawnstar's were; Oblivion's `.scr`/`.cml`/`.jtm` formats are fully
opaque and will need the Superscape engine's own parsing code read
through first.
