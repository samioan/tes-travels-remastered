# dawnstar/src

Hand-written, renamed reconstruction of the fully-understood classes from
`../decompiled/`. This is a **human-maintained reference tree**, not
pipeline output -- never regenerated, never overwritten by
`tools/decompile.py`. See `../docs/CLASS_MAP.md` for the full writeup of
what each class does and how confident each renaming is.

| Renamed | Original | Role |
|---|---|---|
| `Item.java` | `a.java` | Item database + loot tables |
| `Spell.java` | `b.java` | Spell database |
| `DungeonGenerator.java` | `c.java` | Procedural level generation |
| `Monster.java` | `d.java` | Monster instances + AI |
| `Util.java` | `f.java` | Misc stateless helpers |
| `Screen.java` | `g.java` | Generic menu/dialog framework |
| `LoadingScreen.java` | `h.java` | Splash + progress-bar screens |
| `Dungeon.java` | `i.java` | One dungeon level's live state |
| `Shop.java` | `k.java` | NPC dialogue, shops, quest tracking |
| `GameCanvas.java` | `e.java` | Renderer + input handler + main tick loop |
| `Player.java` | `j.java` | Player stats, inventory, combat, spellcasting |
| `ESGame.java` | `ESGame.java` | MIDlet entry point: UI screen wiring, save/load, image loading |

All 13 decompiled classes are now renamed here (`k` was Shop, not a
13th class on top of these -- see `CLASS_MAP.md`'s class-by-class
writeup). `Player` was the last large hand-trace (2668 lines) -- see
`CLASS_MAP.md`'s "why `j`/`ESGame` weren't renamed by mechanical means"
for why it needed the same full treatment as `GameCanvas` rather than
a mechanical rename. `ESGame` itself needed no such treatment -- its
own member names were already readable in the decompiled output -- but
its pass turned out bigger than a pure retype: a handful of its own
methods called `Player` predicates (`canEquipOrUnequip`/`canUseItem`/
`canLearnSpell`) that had never been written yet, since nothing
`Player.java` itself or any already-renamed class called them before
ESGame did.

Every class now integrates directly using real names: `Player.java`'s
`currentDungeon()` returns the real `Dungeon` (it used to return the
old `i`, forwarding `ESGame.dungeons[]`'s then-unrenamed element type),
which in turn made `Dungeon.tickNearbyMonsters` and the handful of
other `Dungeon`/`Player` cross-calls reachable through `GameCanvas` and
`Player` themselves -- both updated alongside this pass. `src/` is now
a single coherent, compilable tree: `decompiled/*.java` is no longer
needed on the classpath to build it.

## Compile-checked, not just read-through

Every class here was verified with a real `javac` compile, not just
visual review -- catches real symbol/type errors that a read-through
alone misses (and did: see below). Not part of the normal
extract/decompile pipeline (needs extra fetched dependencies), so it's a
manual step, not a `tools/` script:

```
mkdir -p /tmp/dawnstar_compile_check/ngame/midlet
cp dawnstar/src/*.java /tmp/dawnstar_compile_check/
cp dawnstar/decompiled/ngame/midlet/RegisteredMIDlet.java /tmp/dawnstar_compile_check/ngame/midlet/
cd /tmp/dawnstar_compile_check && mkdir out

# One-time: fetch MIDP 2.0 / CLDC 1.1 / Nokia UI API *stub* jars (just
# enough for javac to resolve javax.microedition.*/com.nokia.mid.ui.*
# symbols -- not a real implementation, don't need one to compile-check).
# Not tracked in git, same as tools/vineflower.jar.
mkdir -p tools/midp-stubs
curl -L -o tools/midp-stubs/midpapi20.jar  https://repo1.maven.org/maven2/org/microemu/midpapi20/2.0.4/midpapi20-2.0.4.jar
curl -L -o tools/midp-stubs/cldcapi11.jar  https://repo1.maven.org/maven2/org/microemu/cldcapi11/2.0.4/cldcapi11-2.0.4.jar
curl -L -o tools/midp-stubs/nokiaui.jar    https://repo1.maven.org/maven2/org/microemu/microemu-nokiaui/2.0.4/microemu-nokiaui-2.0.4.jar

javac -cp "tools/midp-stubs/*" -d out *.java ngame/midlet/*.java
```

Now that all 13 classes are renamed, `dawnstar/src/*.java` alone is
enough -- `dawnstar/decompiled/*.java` (except `RegisteredMIDlet.java`,
the shared base class that was never obfuscated) no longer needs to be
on the classpath at all. Earlier runs below, made while classes were
still missing their rename pass, needed the untouched originals
alongside for whatever hadn't been done yet; kept for history.

**Result (last run while writing `Dungeon.java`/`Monster.java`):** the
untouched `a`/`c`/`e`/`h`/`i`/`j`/`ESGame.java` fail to compile as-is --
expected, that's exactly the field/class-name-collision issue documented
in `CLASS_MAP.md` (not something introduced here, it's inherent to
Vineflower's output whenever a short field name matches a short class
name it's also used as a static reference elsewhere). Of the 9 renamed
files, **7 compiled with zero errors** (`Item`, `Spell`, `Util`, `Screen`,
`LoadingScreen`, `Shop`, `DungeonGenerator`). The other 2
(`Dungeon.java`, `Monster.java`) have exactly one recurring error class,
not a logic bug:

```
Dungeon.java:542: error: incompatible types: i cannot be converted to Dungeon
```

`ESGame.dungeons` is declared `static i[] dungeons;` in the untouched
`ESGame.java` (type `i`, the *original* Dungeon class) -- since `ESGame`
isn't renamed, that field's declared type can't be `Dungeon[]` too. This
is a real integration boundary, not a mistake in `Dungeon.java`/
`Monster.java`'s own logic: both files are internally consistent and
correct reconstructions, they just can't type-check against
`ESGame.dungeons` specifically until `ESGame.java` itself gets a rename
pass (tracked as future work, not attempted here to avoid the same
scope/risk that kept `e`/`j` unrenamed).

This compile pass also caught and fixed two real transcription mistakes
before they'd have sat undetected in `CLASS_MAP.md`/the source: an
early draft of `Monster.java` swapped which field held the monster's
*type* vs. the *dungeon level number* it lives on, and `Shop.java`'s
quest-turn-in roll was first wired to a same-shaped-looking table
inside `Shop` itself instead of `Player`'s own `c(shopId,action)` method.
Both are noted inline where they were caught.

**Re-run after adding `GameCanvas.java`:** `Dungeon.java`/`Monster.java`
now also compile with zero errors (the `i cannot be converted to
Dungeon` error above no longer reproduces against the current tree).
`GameCanvas.java` itself compiles with **zero errors**, and the
`Screen.java`/`LoadingScreen.java` updates needed to point `canvas` at
the real `GameCanvas` type (instead of the old unrenamed `e`) introduced
no new errors either. All errors from this run are confined to the
still-untouched `a`/`c`/`e`/`h`/`i`/`j`/`ESGame.java` -- the same
inherent single-letter-field-vs-class-name collision issue as before,
now also affecting `e.java` itself for the same reason it was never
mechanically renamed in the first place. A careful re-read (not the
compiler -- this was a semantic swap, not a type error) also caught one
real transcription mistake in `GameCanvas.java`'s corridor
object-renderer (`paintObjectAtPosition`): an early draft swapped the
icon-index and frame-count columns read from `OBJECT_DRAW_TABLE`.

**Re-run after adding `Player.java`:** same error counts and same 7
files (`a`/`c`/`e`/`h`/`i`/`j`/`ESGame.java`) as every run above --
`Player.java` itself, and the `GameCanvas.java`/`Monster.java`/
`Shop.java`/`Dungeon.java` updates needed to integrate with it (Player's
own methods now use the real `Monster`/`Item`/`Spell`/`Shop`/`Util`
types and names throughout), all compile with **zero errors**. This was
the last of the 12 non-`ESGame` classes renamed -- `dawnstar/src/`'s
only remaining integration debt was the single documented `i`-typed
leak from `Player.currentDungeon()` (see above), expected to resolve
once `ESGame.java` got its own rename pass.

**Re-run after adding `ESGame.java` (the last class):** `dawnstar/src/`
now compiles **standalone**, with zero errors, against just the MIDP
stub jars -- `dawnstar/decompiled/*.java` is no longer needed on the
classpath at all (confirmed by dropping it from the command above and
re-running). Getting there needed more than retyping ESGame's own
fields: `Player.currentDungeon()` now genuinely returns `Dungeon`
instead of the old `i` (since `ESGame.dungeons` is finally `Dungeon[]`),
which surfaced a chain of old-type leaks that had been hiding behind
that one return type -- half a dozen `i`-typed locals and old
`i`-lettered method calls in `Player.java` and `GameCanvas.java`
(`Dungeon.tickNearbyMonsters`, `sampleSquareView`,
`trySpawnMonsterNear`, `addDroppedItem`, `removeChest`, `displayName`)
all needed updating to the real `Dungeon` API in the same pass. Also
added three `Player` predicate methods (`canEquipOrUnequip`/
`canUseItem`/`canLearnSpell`) that only `ESGame`'s inventory-item menu
ever called, so nothing had written them yet; and renamed two
previously-unconfirmed fields once ESGame's call sites confirmed their
purpose: `Player.traitorSuspicionCount` (was `unconfirmedB`) and
`Screen.contextIndex`/`Screen.backTarget` (were `unused1`/`unused2`).
