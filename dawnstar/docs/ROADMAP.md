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

**Phase 1 is done for 12 of the 13 decompiled classes -- every one
except `ESGame` itself.** Read through class-by-class, worked out each
one's role from field usage and call sites, and produced hand-written,
faithfully-renamed source for all 12 in [`../src/`](../src/) (see its
`README.md` for the exact mapping and a compile-check methodology/
result). Full writeup of every class -- including `ESGame`, the one not
mechanically renamed -- is in [`CLASS_MAP.md`](CLASS_MAP.md).

- **Renamed and compile-checked**: `Item`, `Spell`, `DungeonGenerator`,
  `Monster`, `Util`, `Screen`, `LoadingScreen`, `Dungeon`, `Shop`,
  `GameCanvas` (renamed from `e.java`, 1893 lines -- the renderer, input
  handler, and main ~4Hz tick loop), and now `Player` (renamed from
  `j.java`, 2668 lines -- by far the largest and most central class:
  stats, inventory, equipment, spellbook, movement/combat, and the
  save-game format). Verified against real MIDP/CLDC API stub jars, not
  just read-through -- this caught real transcription mistakes before
  they became "settled" documentation (see `../src/README.md`),
  including a swapped icon-index/frame-count column pair in
  `GameCanvas`'s corridor object renderer. `Screen.java`/
  `LoadingScreen.java` were updated alongside `GameCanvas`'s rename, and
  `Monster.java`/`Shop.java`/`Dungeon.java`/`GameCanvas.java` were all
  updated alongside `Player`'s, so the whole `src/` tree now integrates
  directly using real class names rather than old single-letter ones.
  Renaming `Player` also resolved two of `CLASS_MAP.md`'s longstanding
  open questions: `Player.corridorView` (previously an unconfirmed
  "`ap[9][5]` built from a strange formula") turned out to be the
  corridor tile-occlusion view grid `GameCanvas`'s 3D renderer and
  minimap both sample from; and the "overstayed in one place" ambush
  spawner turned out to be unreachable dead code (its trigger field is
  never set to anything but its inert default anywhere in the codebase).
- **Not mechanically renamed**: `ESGame` alone -- its own member names
  are already readable in the decompiled output (no rename needed
  there), it just hasn't had its own pass yet. `Player.currentDungeon()`
  is the one remaining old-type leak in the renamed tree: it still
  returns the *old* unrenamed Dungeon type because it just forwards
  `ESGame.dungeons[]`'s own element type, which stays `i[]` until
  `ESGame` is renamed -- see `../src/README.md` and `Player.java`'s/
  `GameCanvas.java`'s class header comments for exactly where that
  surfaces.
- A handful of fields/tables across several classes are flagged in
  `CLASS_MAP.md`'s "Open questions" as genuinely unconfirmed (no call
  site found) rather than guessed.

**Phase 2 is substantially done.** See [`ASSET_FORMATS.md`](ASSET_FORMATS.md)
-- `datfiles.lmp`, `imgfiles.lmp` (a *different* format from `datfiles.lmp`,
despite the similar-looking name), `npcstrings.dat`, `helptext.dat`,
`itemsin.dat`, `droppeditemsin.dat`, `spellsin.dat`, `geomin.dat`, and
`charin.dat` all have confirmed binary layouts now, derived from the
loader code rather than guessed from hexdumps.

**Next for phase 1**: a light rename pass on `ESGame.java` itself. Its
own names are already readable, but a handful of field *types* (`static
i[] dungeons`, `e gameCanvas`, and every UI screen field currently typed
`g`) need updating to point at the renamed classes. That's the only
thing left before `src/` becomes a single coherent, compilable tree
instead of needing `decompiled/`'s originals alongside it for `ESGame`'s
own dependencies -- see `../src/README.md`'s compile-check section for
exactly what breaks today without that. With all 12 other classes now
renamed and integrated, this is a much smaller pass than the ones before
it: `ESGame` needs its field/local *types* updated, not the same
from-scratch hand-trace `GameCanvas`/`Player` needed.

**Phase 3 (not started): PC port.** Scaffold is in `port/` (CMake +
Ninja + MSVC, matching the sibling `shadowkey-decomp` project's
toolchain) -- currently just proves the build works, no game logic yet.
Port strategy should follow shadowkey-decomp's precedent: behavioral
reimplementation in C++, not byte-exact recompilation. The procedural
dungeon generator (`DungeonGenerator`), the first-person corridor
renderer's wall-segment lookup table (`GameCanvas.CORRIDOR_WALL_TABLE`,
now fully renamed in `../src/GameCanvas.java`), and the full data-table
formats are all well-understood enough now to start porting logic
directly rather than needing further reverse-engineering first.
