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

**Phase 1 (done): read through and rename.** A full survey pass of every
class is written up in [`CLASS_MAP.md`](CLASS_MAP.md), and all ten classes are
renamed into `../src/` (`Game`, `ScriptInterpreter`, `Actor`, `ActorSystem`,
`DialogueScreen`, `DialogueNode`, `SpriteFrame`, `SpriteRenderer`,
`ProjectileManager`, `Strings`, plus `blt/Main`). The obfuscator reused single
letters across fields of different JVM descriptors, which plain decompiler
output cannot express as Java, so the renaming is done by a descriptor-aware
Vineflower renamer driven by `docs/rename.map` (`tools/decompile_renamed.py`).
The renamed tree compiles as a whole against the MIDP stubs.

**Phase 2 (in progress, the hard part): reverse the `.scr`/`.cml`/`.jtm`
binary formats.** See [`ASSET_FORMATS.md`](ASSET_FORMATS.md). Unlike
dawnstar/stormhold's data tables (plain length-prefixed strings, crackable
from a hexdump alone), these are opaque binary streams with no visible
structure -- the per-level content (`l01_*` through `l14_*`, one set per
dungeon/area) is *entirely* encoded in them. All three formats' field
layouts are now confirmed (by reading `b`/`e`/`g`'s own parsers as part
of the phase-1 survey) and written up in `ASSET_FORMATS.md`. The ~78 `.scr`
bytecode opcodes are named ([`SCR_OPCODES.md`](SCR_OPCODES.md)) and
`tools/parse_scr.py` disassembles all 32 `.scr` files; `tools/parse_jtm.py`
(17 files) and `tools/parse_cml.py` (21 files) parse theirs completely. Each
has a `--check` mode that verifies every extracted file parses exactly to EOF.
**Phase 2's parsing goal is met**, and the semantic questions it left (`table6`,
`pairTable`, the odd opcode operands, states 4/9) were answered by the port; see
`CLASS_MAP.md`. Tile-id / `.cml` group ids are named by use in the port code.

**Phase 3 (in progress): PC port.** See [`PORT_ROADMAP.md`](PORT_ROADMAP.md). The
port in `port/` (CMake + Ninja + MSVC, software-rendered Win32) is an engine
reimplementation that loads the original resources directly. Done so far: the
asset loaders (`.jtm`/`.cml`/`.scr`/PNG, smoke-tested against all 70 extracted
files), sprite drawing, and a level viewer that renders the isometric tile maps
of the 12 story levels. Next: actors and collision, then the script interpreter.
