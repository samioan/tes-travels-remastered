# Dawnstar -- roadmap

## The target

`roms/TEST-Dawnstar.jar` -- a J2ME MIDlet (`MIDP-1.0`/`CLDC-1.0`), MIDlet-Vendor
`mob.ua`, MIDlet-Name "DawnStar by milaro". Entry point `ESGame`
(`MIDlet-1: DawnStar, icon3650.png, ESGame`).

`ESGame` extends `ngame.midlet.RegisteredMIDlet` -- the same base class
name and package (`ngame.midlet`) the `stormhold/` sibling project uses for
its own MIDlet (there just called `a`, unrenamed). Dawnstar's build kept
`RegisteredMIDlet`'s real name, which is how we know it: both games share
an underlying "ngame" engine, not just similar code independently
obfuscated the same way. Worth diffing `ngame/midlet/RegisteredMIDlet.java`
against stormhold's `ngame/midlet/a.java` once both are read through --
whichever parts are byte-for-byte identical are the shared engine,
everything else is Dawnstar-specific.

## Status

**Phase 0 (this setup) is done:** jar unpacked (`extracted/`), decompiled
to readable-but-unrenamed Java (`decompiled/`, 13 files: `a`..`k`.java,
`ESGame.java`, `ngame/midlet/RegisteredMIDlet.java`) via
[`../../tools/decompile.py`](../../tools/decompile.py). Vineflower recovers
real control flow and types cleanly -- this is plain JVM bytecode, not a
native/ARM target, so there's no disassembler-loader step like
shadowkey-decomp needed.

**Phase 1 is substantially done for 9 of the 13 decompiled classes.**
Read through class-by-class, worked out each one's role from field usage
and call sites, and produced hand-written, faithfully-renamed source for
9 classes in [`../src/`](../src/) (see its `README.md` for the exact
mapping and a compile-check methodology/result). Full writeup of every
class -- including the 4 not mechanically renamed -- is in
[`CLASS_MAP.md`](CLASS_MAP.md).

- **Renamed and compile-checked**: `Item`, `Spell`, `DungeonGenerator`,
  `Monster`, `Util`, `Screen`, `LoadingScreen`, `Dungeon`, `Shop`.
  Verified against real MIDP/CLDC API stub jars, not just read-through --
  this caught two real transcription mistakes before they became "settled"
  documentation (see `../src/README.md`).
- **Not mechanically renamed**: `ESGame` (already has real member names in
  the decompiled output, no rename needed, but not copied into `src/`
  either), and `GameCanvas`/`e.java` + `Player`/`j.java` -- Vineflower's
  output for both has field names that literally collide with the
  single-letter class names they're also used as static references to
  elsewhere in the same file, which makes a *mechanical* rename unsafe.
  Both are fully mapped by role and most fields in `CLASS_MAP.md` from a
  complete read-through, just not renamed line-by-line yet -- that needs
  the same one-class-at-a-time hand treatment the other 9 got, not
  attempted this pass given their size (2668 and 1893 lines).
- A handful of fields/tables across several classes are flagged in
  `CLASS_MAP.md`'s "Open questions" as genuinely unconfirmed (no call
  site found) rather than guessed.

**Phase 2 is substantially done.** See [`ASSET_FORMATS.md`](ASSET_FORMATS.md)
-- `datfiles.lmp`, `imgfiles.lmp` (a *different* format from `datfiles.lmp`,
despite the similar-looking name), `npcstrings.dat`, `helptext.dat`,
`itemsin.dat`, `droppeditemsin.dat`, `spellsin.dat`, `geomin.dat`, and
`charin.dat` all have confirmed binary layouts now, derived from the
loader code rather than guessed from hexdumps.

**Next for phase 1**: rename `GameCanvas`/`Player`, one class at a time,
same hand-trace-then-compile-check discipline as the 9 already done.
Once `ESGame.java` also gets a light rename pass (its own names are
already readable, but a couple of field *types* like `static i[]
dungeons` need updating to point at the renamed classes), `src/` becomes
a single coherent, compilable tree instead of needing `decompiled/`'s
originals alongside it for `e`/`j`'s own dependencies -- see
`../src/README.md`'s compile-check section for exactly what breaks today
without that.

**Phase 3 (not started): PC port.** Scaffold is in `port/` (CMake +
Ninja + MSVC, matching the sibling `shadowkey-decomp` project's
toolchain) -- currently just proves the build works, no game logic yet.
Port strategy should follow shadowkey-decomp's precedent: behavioral
reimplementation in C++, not byte-exact recompilation. The procedural
dungeon generator (`DungeonGenerator`), the first-person corridor
renderer's wall-segment lookup table (`GameCanvas.k[5][6][4]`, mapped
but not yet renamed), and the full data-table formats are all
well-understood enough now to start porting logic directly rather than
needing further reverse-engineering first.
