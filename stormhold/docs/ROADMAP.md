# Stormhold -- roadmap

## The target

`roms/TEST-Stormhold.jar` -- a J2ME MIDlet (`MIDP-1.0`/`CLDC-1.0`),
MIDlet-Vendor **Vir2L Studios** (the same studio behind
*The Elder Scrolls Travels: Shadowkey* on N-Gage -- see the sibling
`shadowkey-decomp` project), MIDlet-Name "The Elder Scrolls". Entry point
`ESGame` (`MIDlet-1: The Elder Scrolls, icon3650.png, ESGame`).

`ESGame` extends `ngame.midlet.a` -- structurally identical to how
`dawnstar/`'s `ESGame` extends `ngame.midlet.RegisteredMIDlet`; that class
is unobfuscated over there, so `ngame/midlet/a.java` here is almost
certainly the same class with its real name stripped. This is Vir2L's
in-house "ngame" MIDP engine, shared across (at least) these two titles --
diff the two once both are read through.

## Status

**Phase 0 (this setup) is done:** jar unpacked (`extracted/`), decompiled
to readable-but-unrenamed Java (`decompiled/`, 13 files) via
[`../../tools/decompile.py`](../../tools/decompile.py). Clean recovery,
same as dawnstar -- plain JVM bytecode, no native/ARM step.

**Phase 1 (done): read through and rename.** See
[`CLASS_MAP.md`](CLASS_MAP.md) for the full writeup. All 13 decompiled
classes plus `ngame/midlet/a.java` are renamed, with hand-written source in
`../src/`, and the whole tree **compiles cleanly** (`javac` against
`tools/midp-stubs/*`, zero errors). Several classes turned out field-for-
field identical to dawnstar's own equivalents (same shared "ngame" engine),
though real Stormhold-specific content/mechanic differences were found and
documented rather than assumed away -- a Stormhold-only "Warden visits/
leaves" NPC mechanic, monsters keyed by spawn id rather than tile position
in the live registry, a from-scratch indexed-color image format
(`RawImage`), and a rendering/UI architecture (`ScreenCanvas`/`UIScreen`/
`GameCanvas`) that does NOT map onto dawnstar's Screen/LoadingScreen/
GameCanvas split by content, among others. Left for later phases (not
blocking phase 2/3): `GameCanvas.java`'s ~15 still-stubbed pixel-rendering
methods, `ESGame.loadHelpTopicBodies()`'s remaining help topics, and a set
of individually-flagged lower-confidence names throughout (see each file's
header comment and CLASS_MAP.md).

**Phase 2 (done): asset formats.** See [`ASSET_FORMATS.md`](ASSET_FORMATS.md)
for the full writeup. Turned out to already be substantially resolved as a
byproduct of phase 1's class-by-class read-through (every loader lives in
one of the 13 renamed classes) -- this phase was mostly writing that up
plus a couple of remaining gaps:
- All 9 flat game-data tables (`itemsin.dat`, `monstersin.dat`,
  `spellsin.dat`, `dungnamesin.dat`, `geomin.dat`, `droppeditemsin.dat`,
  `charin.dat`, `monsterfilenamesin.dat`, `npcstrings.dat`) have confirmed
  record layouts, each column-oriented (`<u16/u32 count>` then one
  contiguous array per field), not the flat `itemsin.dat`-per-record style
  originally guessed.
- **Correction to this doc's own earlier guess:** the `.cus` files are
  *not* 3D mesh data despite the naming pattern reading that way -- they're
  a from-scratch 2D indexed-color raw sprite format (`RawImage.java`),
  fully decoded (width/height/transparency flag/palette/1-byte-per-pixel
  indices). `far`/`mid`/`near` in filenames is 2D sprite LOD, not mesh LOD.
- Every non-`.class` resource in `extracted/` (75 files) has a confirmed
  loader call site; nothing orphaned. Left open: a handful of individual
  table columns/groups whose exact meaning isn't pinned down yet (doesn't
  block phase 3) -- see `ASSET_FORMATS.md`'s own "what's actually left"
  section.

**Phase 3 (in progress): PC port.** See
[`PORT_ROADMAP.md`](PORT_ROADMAP.md) for the full milestone-by-milestone
writeup, following dawnstar's own port precedent. M0 (scaffold), M1 (real
tick loop/backbuffer/window, the confirmed 250ms cadence), and M2 (asset
foundations -- `BinaryReader`/`AssetRoot` plus the `ItemDatabase`/
`SpellDatabase` loaders, verified against real `itemsin.dat`/
`droppeditemsin.dat`/`spellsin.dat`) are done. Given the shared Vir2L
"ngame" engine with dawnstar, small identical pieces (tick cadence,
backbuffer, window, binary reader) are copied rather than shared for now --
factoring out a real cross-project engine library is deferred until
Stormhold's own port has enough milestones to show what's actually worth
sharing (see `PORT_ROADMAP.md`'s own note on this).
