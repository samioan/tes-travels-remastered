# stormhold/src

Hand-written, renamed reconstruction of the fully-understood classes from
`../decompiled/`, mirroring `../../dawnstar/src/README.md`'s role for that
sibling project. This is a **human-maintained reference tree**, not
pipeline output -- never regenerated, never overwritten by
`tools/decompile.py`. See `../docs/CLASS_MAP.md` for the full writeup of
what each class does and how confident each renaming is.

| Renamed | Original | Role |
|---|---|---|
| `Item.java` | `a.java` | Item database + loot tables |
| `Spell.java` | `b.java` | Spell database |
| `Monster.java` | `d.java` | Monster instances + AI |
| `Util.java` | `f.java` | Misc stateless helpers |
| `Shop.java` | `k.java` | NPC dialogue, quest tracking, Warden world event |
| `Dungeon.java` | `i.java` | One dungeon level's live state **and** generation (no separate generator class here) |
| `RawImage.java` | `g.java` | From-scratch indexed-color image decoder |
| `ScreenCanvas.java` | `c.java` | Thin `FullCanvas` adapter hosting a `UIScreen` |
| `UIScreen.java` | `h.java` | Menu/dialog/splash framework (LCDUI widgets as data containers, hand-painted rendering) |
| `GameCanvas.java` | `e.java` | First-person renderer + input + main tick loop (**partial**: ~15 pixel-rendering methods still stubbed) |
| `Player.java` | `j.java` | Player stats, inventory, combat, spellcasting |
| `ESGame.java` | `ESGame.java` | MIDlet entry point: UI screen wiring, save/load, image loading |
| `ngame/midlet/RegisteredMIDlet.java` | `ngame/midlet/a.java` | Shared Vir2L MIDlet-lifecycle base class |

Unlike dawnstar, **every one of these 13 classes needed a real rename
pass** -- this build's `ESGame` and `ngame/midlet/a` are single-letter
obfuscated too (dawnstar's copies came out of Vineflower with readable
names already). `../src/` compiles cleanly end-to-end:

```
cd stormhold
javac -cp "../tools/midp-stubs/*" -d out src/*.java src/ngame/midlet/*.java
```

(same `tools/midp-stubs/*` stub jars dawnstar's own compile-check uses --
see `../../dawnstar/src/README.md` for how to fetch them if missing.)

What "compiles" does *not* mean here: `GameCanvas.java`'s pixel-rendering
methods and a scattering of individually-flagged `TODO_`/`unconfirmed*`
names throughout are real, tracked gaps -- see `../docs/CLASS_MAP.md` and
each file's own header comment before trusting a specific name as final.
