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

**Phase 1 (not started): read through and rename.** Obfuscation here is
just single-letter class/field/method names, not control-flow flattening
or string encryption -- go class by class, work out its role from what it
touches (`javax.microedition.lcdui`/`rms` calls, field usage), rename in
place. `ESGame.java` (extends the shared MIDlet base) is the natural
starting point.

**Phase 2 (not started): asset formats.** See
[`ASSET_FORMATS.md`](ASSET_FORMATS.md) -- `datfiles.lmp` and
`imgfiles.lmp` are Dawnstar-specific packed archives (loose files in
stormhold's build, bundled here); need a parser once the loader code
in the decompiled sources is understood.

**Phase 3 (not started): PC port.** Scaffold is in `port/` (CMake +
Ninja + MSVC, matching the sibling `shadowkey-decomp` project's
toolchain) -- currently just proves the build works, no game logic yet.
Port strategy should follow shadowkey-decomp's precedent: behavioral
reimplementation in C++, not byte-exact recompilation.
