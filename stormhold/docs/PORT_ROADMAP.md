# Port Roadmap

Tracks `port/`, the actual PC port -- as opposed to [`ROADMAP.md`](ROADMAP.md),
which tracks the reverse-engineering work that feeds it (now fully done: all
13 classes renamed/compile-checked in [`../src/`](../src/), all asset formats
confirmed in [`ASSET_FORMATS.md`](ASSET_FORMATS.md)). Follows the sibling
`dawnstar` project's own precedent (`../../dawnstar/docs/PORT_ROADMAP.md`,
which in turn follows `shadowkey-decomp`'s): milestone by milestone, each one
landing a real, runnable/testable slice rather than a big-bang rewrite,
verified with a smoke-test executable per milestone rather than just
read-through.

## Decisions carried through every milestone

- **Behavioral reimplementation, not byte-exact recompilation.** `../src/`'s
  renamed Java is the reference for *what* every system does; the port is
  ordinary modern C++ that reproduces that behavior, not a mechanical
  transliteration of MIDP-era Java idioms into C++ syntax. Same rule as
  dawnstar's own port.
- **C++17, CMake + Ninja, MSVC**, matching `dawnstar`/`shadowkey-decomp`'s
  toolchain and this repo's existing `port/` scaffold.
- **Raw Win32 + GDI (`StretchDIBits`), no SDL2/D3D/GL** -- same rationale as
  dawnstar: the original is a software-rendered MIDP `Canvas`, so a Win32
  window blitting a manually computed backbuffer reproduces that
  architecture directly with zero external dependencies.
- **176x208 virtual canvas.** Same reasoning as dawnstar's own port decision
  (`../../dawnstar/docs/PORT_ROADMAP.md`): `GameCanvas` reads
  `getWidth()`/`getHeight()` at runtime rather than assuming a fixed
  hardware size (`../src/GameCanvas.java` lines 253-278), and this jar's own
  `MIDlet-1` manifest line names `icon3650.png` -- the Nokia 3650, whose
  display is 176x208. **This is a port decision, not a confirmed original
  constant**, same caveat as dawnstar's.
- **Assets stay out of the repo.** The port reads `itemsin.dat`/
  `spellsin.dat`/etc. and (later) `.cus`/`.png` files directly from a
  configurable root path at runtime (defaulting to `../extracted/`, this
  project's existing `tools/extract_jar.py` output) -- nothing copyrighted
  is copied into git.
- **No archive indirection, unlike dawnstar.** Stormhold has no
  `datfiles.lmp`/`imgfiles.lmp` -- every resource is its own top-level jar
  entry (`ASSET_FORMATS.md`'s header note). So there's no `DatArchive`
  equivalent to port; `AssetRoot` (`src/assets/asset_root.h`) is a plain
  root-directory-relative file opener instead, structurally simpler than
  dawnstar's linear-scan archive reader.
- **No scripting engine to port.** Same as dawnstar -- `ESGame`/`GameCanvas`/
  `Player`/`Monster`/`Dungeon` *are* the game logic, directly, in Java. No
  SimKin-equivalent, no "embed an interpreter" milestone.
- **Shared "ngame" engine code is duplicated for now, not factored out.**
  `../docs/ROADMAP.md` originally suggested factoring shared engine code
  into one location "once both are understood" -- both now are, but
  dawnstar's port is a mature, 45+-milestone codebase already shipping a
  launcher/release pipeline. Retrofitting a cross-project shared library
  underneath it is a real, risky refactor of working code, not a
  reverse-engineering task -- deliberately deferred rather than done as a
  side effect of starting Stormhold's own port. In the meantime, small
  engine-level pieces that turned out byte-for-byte identical (`GameClock`,
  `Backbuffer`, `Window`, `BinaryReader` -- all confirmed identical cadence/
  format between the two games' decompiled sources) are copied rather than
  shared; revisit consolidation once Stormhold's port has enough of its own
  milestones to show which pieces actually *stay* identical under real
  gameplay-logic weight, the same way dawnstar's own split between
  `player/`/`monster/`/`combat/` only became obvious once M14/M15 needed it.

## Milestones done

- [x] **M0 -- scaffold** (pre-existing). CMake/MSVC/Ninja project proves the
      toolchain: opens a blank Win32 window. No tick loop, no backbuffer, no
      game logic.

- [x] **M1 -- real tick loop + backbuffer + present** (this session). A
      `GameClock` running the real 250ms/4Hz cadence
      (`GameCanvas.run()`'s "steady 250ms tick", `../src/GameCanvas.java`
      around line 505 -- confirmed identical to dawnstar's own `GameCanvas.
      run()` cadence, see that method's own header comment), a 176x208
      `Backbuffer`, and `StretchDIBits` presentation scaled to the window's
      client area -- structurally identical to dawnstar's M1 (same engine,
      same architecture). Still no game logic: proves the loop/presentation
      architecture only. Verified by building and launching
      `stormhold_port.exe` (stays up, presents a solid-color frame every
      tick, closes cleanly).

      `Backbuffer` here has no `Blit()`/`DecodedImage` yet, unlike
      dawnstar's own M1-era backbuffer had grown by M10 -- no PNG decoder
      exists yet in this port, so there's nothing to composite beyond flat
      fills this milestone.

- [x] **M2 -- asset foundations** (this session). `BinaryReader`
      (big-endian primitives + `readUTF`-style length-prefixed strings,
      matching `Util.java`/`Item.java`/`Spell.java`'s `DataInputStream`
      usage -- byte-for-byte identical to dawnstar's own `BinaryReader`,
      confirmed by diffing the two) and `AssetRoot` (the direct-file
      resource opener described above), then the first two real data
      loaders on top of it: `ItemDatabase` (`itemsin.dat`+
      `droppeditemsin.dat`, `../src/Item.java`) and `SpellDatabase`
      (`spellsin.dat`, `../src/Spell.java`). Data-only for now --
      RNG-driven methods (`Item.java`'s `randomGiftItemOfSubtype()`/
      `rollLoot()`) are deferred to whichever later milestone actually
      needs dungeon generation, matching how dawnstar's own M2 deferred its
      equivalents to M6.

      Verified by `asset_smoke.exe` against the real `extracted/`: 109
      items across 17 categories, a 43-row loot table, and 25 spells, all
      with sane-looking names/prices/stats (`Miner Pick`/`Woodman's Axe`/
      `Battle Axe`, `Frenzy`/`Shield`/`Deft Security`) -- the same 3 spell
      names dawnstar's own M2 smoke test happened to print first, a nice
      independent confirmation that `spellsin.dat`'s early rows are shared
      content between the two games, not a coincidence of this port's own
      code.

## What's next

M3 onward: monster type database + dungeon geometry (`monstersin.dat`,
`geomin.dat` -- `../src/Monster.java`/`../src/Dungeon.java`), following
dawnstar's own M3 as a template, then character data (`charin.dat`),
`java.util.Random`'s bit-exact LCG (needed the moment dungeon generation is
ported, since `Dungeon.java`'s level layouts are seeded per-level the same
way dawnstar's are), and Stormhold-specific systems dawnstar has no
equivalent of yet -- the Warden visits/leaves NPC mechanic and
`RawImage`'s indexed-color sprite decoder (`.cus` files), both flagged in
`ROADMAP.md`'s phase 1 writeup as real Stormhold-only content.
