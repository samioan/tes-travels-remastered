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

**Phase 1 (in progress): read through and rename.** A full survey pass
of every class (`blt/Main.java`, `a`-`j`) is done -- architecture, every
class's role, and most fields/methods are understood and written up in
[`CLASS_MAP.md`](CLASS_MAP.md). Five classes are mechanically renamed
into `../src/` (`Strings`, `DialogueNode`, `SpriteFrame`,
`SpriteRenderer`, `ProjectileManager`); the rest (`Actor`/`j`,
`ActorSystem`/`h`, `DialogueScreen`/`f`, `Game`/`b`,
`ScriptInterpreter`/`e`) are documented but deliberately left unrenamed
-- this codebase reuses single-letter field names across *different JVM
descriptors on the same class* far more aggressively than
dawnstar/stormhold's engine (`Game`/`b` alone has 12+ fields all spelled
`a`), so those five need their own careful, one-at-a-time passes rather
than a single rushed sweep. Suggested order in `CLASS_MAP.md`'s "Renamed
source" section: `Actor`+`ActorSystem` together next, then
`DialogueScreen`, then `Game`/`ScriptInterpreter` last (largest, most
collision-heavy).

**Phase 2 (in progress, the hard part): reverse the `.scr`/`.cml`/`.jtm`
binary formats.** See [`ASSET_FORMATS.md`](ASSET_FORMATS.md). Unlike
dawnstar/stormhold's data tables (plain length-prefixed strings, crackable
from a hexdump alone), these are opaque binary streams with no visible
structure -- the per-level content (`l01_*` through `l14_*`, one set per
dungeon/area) is *entirely* encoded in them. All three formats' field
layouts are now confirmed (by reading `b`/`e`/`g`'s own parsers as part
of the phase-1 survey) and written up in `ASSET_FORMATS.md`; what's left
is naming `.scr`'s ~78 bytecode opcodes and writing standalone
`tools/parse_scr.py`/`parse_cml.py`/`parse_jtm.py` parsers instead of
only having the logic live inside the game engine's own loaders.

**Phase 3 (not started): PC port.** Scaffold is in `port/` (CMake + Ninja
+ MSVC, matching the shadowkey-decomp port's toolchain). Because content
lives in the `.scr`/`.cml` binary resources rather than Java code, a
faithful port likely means porting the *interpreter* (the engine classes)
to C++ and loading the original resource files directly, rather than
transcribing each level by hand -- confirm this is feasible once the
format is understood.
