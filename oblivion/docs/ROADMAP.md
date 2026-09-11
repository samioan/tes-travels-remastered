# Oblivion -- roadmap

## The target

`roms/TEST-Oblivion.jar` -- a J2ME MIDlet (`MIDP-1.0`/`CLDC-1.0`),
**`Created-By: Superscape build system v.1.4.1`**, MIDlet-Vendor
Superscape, MIDlet-Name "Elder Scrolls". Entry point `blt.Main`
(`MIDlet-1: Elder Scrolls,/icon.png,blt.Main`).

This is a **different engine from dawnstar/stormhold's Vir2L "ngame"**
(different vendor, different package layout, no `ESGame`/`ngame.midlet`
classes at all -- see those two projects' `ROADMAP.md`). Superscape was a
UK middleware company whose mobile 3D engine (marketed as "Swerve"/"BLT")
licensed scene and script data as compiled binary resources rather than
Java code -- consistent with what's here: `blt.Main` is a thin ~10-line
MIDlet shim (`decompiled/blt/Main.java`) that immediately hands off to
class `b` with a `.scr` and a `.cml` path. The real game logic is almost
certainly interpreted from those binary resource files by the (small,
generic) engine classes, not written out in Java per-level -- the 10
top-level classes (`a`..`j`, none over ~57KB) have to cover both engine
*and* interpreter for however much of the game the 60+ `.scr`/`.cml`
files encode. Read `b.java` (the largest, 57KB source, constructed with
the `.scr`/`.cml` paths) first.

## Status

**Phase 0 (this setup) is done:** jar unpacked (`extracted/`), decompiled
to readable-but-unrenamed Java (`decompiled/`, 11 files) via
[`../../tools/decompile.py`](../../tools/decompile.py). Clean recovery.

**Phase 1 (not started): read through and rename**, starting from
`blt/Main.java` -> `b.java`.

**Phase 2 (not started, the hard part): reverse the `.scr`/`.cml`/`.jtm`
binary formats.** See [`ASSET_FORMATS.md`](ASSET_FORMATS.md). Unlike
dawnstar/stormhold's data tables (plain length-prefixed strings, crackable
from a hexdump alone), these are opaque binary streams with no visible
structure -- the per-level content (`l01_*` through `l14_*`, one set per
dungeon/area) is *entirely* encoded in them. This is the actual
decompilation target for this game, analogous to shadowkey's world/model
formats: format understanding has to come from reading how class `b` (or
whichever class owns it) parses the byte stream, not from guessing.

**Phase 3 (not started): PC port.** Scaffold is in `port/` (CMake + Ninja
+ MSVC, matching the shadowkey-decomp port's toolchain). Because content
lives in the `.scr`/`.cml` binary resources rather than Java code, a
faithful port likely means porting the *interpreter* (the engine classes)
to C++ and loading the original resource files directly, rather than
transcribing each level by hand -- confirm this is feasible once the
format is understood.
