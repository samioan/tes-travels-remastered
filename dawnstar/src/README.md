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

`ESGame`, `GameCanvas` (`e.java`) and `Player` (`j.java`) are **not**
renamed here -- see `CLASS_MAP.md`'s "why e/j aren't renamed yet" for the
reason (Vineflower's output for those two has field names that literally
collide with the single-letter class names, making a mechanical rename
unsafe without the same full hand-trace the 9 classes above got). Files
in this directory that need something from those three reference them by
their **original** names (`e`, `j`, `ESGame` and its members) rather than
inventing renamed-but-nonexistent APIs.

## Compile-checked, not just read-through

Every class here was verified with a real `javac` compile, not just
visual review -- catches real symbol/type errors that a read-through
alone misses (and did: see below). Not part of the normal
extract/decompile pipeline (needs extra fetched dependencies), so it's a
manual step, not a `tools/` script:

```
mkdir -p /tmp/dawnstar_compile_check/ngame/midlet
cp dawnstar/src/*.java dawnstar/decompiled/*.java /tmp/dawnstar_compile_check/
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

Both the renamed files here AND the untouched `decompiled/*.java` need
to be present together: `e.java`/`j.java`/`ESGame.java` still reference
`a`-`k` by their original names, so those originals must stay on the
classpath even though renamed replacements for 9 of them also exist
alongside (different class names, no collision).

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
