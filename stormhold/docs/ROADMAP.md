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

**Phase 1 (not started): read through and rename.** `ESGame.java` is the
biggest single class (28KB source) and the obvious starting point;
`ngame/midlet/a.java` next, cross-referenced against dawnstar's
`RegisteredMIDlet.java`.

**Phase 2 (not started): asset formats.** See
[`ASSET_FORMATS.md`](ASSET_FORMATS.md). Two families:
- Flat game-data tables (`itemsin.dat`, `monstersin.dat`, `spellsin.dat`,
  `dungnamesin.dat`, `geomin.dat`, `droppeditemsin.dat`, `charin.dat`,
  `monsterfilenamesin.dat`, `npcstrings.dat`) -- `itemsin.dat`'s framing is
  already worked out (see the doc); the others are presumably the same
  `<u16 length><UTF-8/bytes>`-record style and should fall quickly once one
  is confirmed against its loader.
- `.cus` files (`baglarge.cus`, `chestfarclosed.cus`, `trainer_male_*.cus`,
  `overseer*.cus`, `undead*.cus`, `warden*.cus`, ...) -- per-bodypart/prop
  3D mesh data (the naming reads as equipment/character pieces: heads,
  torsos, weapons, chests). Binary, structure not yet worked out.

**Phase 3 (not started): PC port.** Scaffold is in `port/` (CMake + Ninja
+ MSVC, matching the shadowkey-decomp port's toolchain). Given the shared
Vir2L "ngame" engine with dawnstar, factor out common engine code into a
shared location once both are understood, rather than duplicating a
reimplementation twice.
