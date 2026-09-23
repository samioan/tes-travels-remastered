# Oblivion -- class map (phase 1 findings)

Full read-through of every class in `decompiled/` (`blt/Main.java` + `a`
through `j`, 8542 lines total), done from the code alone -- no external
Superscape/BLT documentation exists. This is a **survey pass**: the overall
architecture, every class's role, and most method/field purposes are
confirmed below with real names proposed, but this is a much more
obfuscated, more field-collision-heavy codebase than dawnstar/stormhold's
Vir2L engine (see "Reading notes" below), so a full mechanical rename into
`../src/` has only been done for the classes called out as "renamed" below.
The rest (`b`, `e`) are documented here at the architecture/method level;
line-by-line field disambiguation is follow-up work, same as dawnstar left
`ESGame`/`Player` for a dedicated later pass.

## Reading notes -- why this is harder than dawnstar/stormhold

Vineflower recovers the *class* structure perfectly, but the original
obfuscator reused single-letter names **across fields of different JVM
descriptors on the same class** (legal at the bytecode level -- field
lookup is by name+type, not name alone -- but not valid input Java, and
very easy to misread). E.g. `b` (the main class) independently declares
`public static byte a`, `public static int a`, `public static short a`,
`public static boolean a`, `public static byte[] a`, `public static j[] a`,
`public static b a`, `public static Vector a`, `public static String[] a`,
`public static Font a`, `public static char[] a`, and more -- twelve+
unrelated fields all spelled `a`. Every one has to be told apart by its
*declared type* at each use site, not by name. `c.java`/`j.java` do the
same at smaller scale. This is why `b` and `e` (the two largest, most
field-collision-heavy classes) are documented but not yet mechanically
renamed -- doing it wrong would be worse than leaving it obfuscated.

## Entry point

`blt.Main extends MIDlet` (`decompiled/blt/Main.java`, already real-named,
10 lines) -- `startApp()` constructs `new b(this, "/startup.scr",
"/oh_menu.cml", <MIDlet-Version>)`, sets it as the `Display`'s current
`Displayable`, calls `.d()` (starts the game thread, see below).
`pauseApp()`/`destroyApp()` forward to `b`. Nothing else here.

## `b` -- the whole engine (canvas + controller + resource loader + renderer)

**Not yet renamed** (see "Reading notes"); proposed name **`Game`**.
`final class b extends Canvas implements Runnable`. Unlike
dawnstar/stormhold where the MIDlet-lifecycle class (`ESGame`) and the
render surface (`GameCanvas`) are separate, this engine centralizes
*everything* in one 3539-line god-class: `b` **is** the `Canvas`, the
`Runnable` game-loop thread, the resource loader, the tilemap/dungeon
generator, the renderer, and the input dispatcher, all at once -- exactly
matching `ROADMAP.md`'s prediction that "the real game logic ... [is]
almost certainly interpreted ... by the (small, generic) engine classes".

Confirmed pieces (method line numbers from `decompiled/b.java`):

- **Ctor** (166): sets full-screen mode, becomes the static singleton
  (`b.a = this`), constructs the script interpreter (`e a = new e(this)`),
  allocates the offscreen draw buffer (`a(width, height+25)`, 379),
  constructs the dialogue/menu screen (`f a = new f("/oh_menu.cml",
  this)`), loads the player's sprite/animation tree (`g.a("/oh_pc.cml")`),
  then loads the boot script (`a("/startup.scr")`, 337) and starts the
  loading-screen state (`b(false)`).
- **Generic resource loader** `static int a(String path)` (2971): reads a
  classpath resource fully into a shared static 6144-byte buffer (`static
  byte[] b`) -- this is what both `e.a(String)` (`.scr` loader) and
  `g.a(String)` (`.cml` loader) call to get raw file bytes. 6144 is a hard
  cap (no resize), consistent with every `.scr`/`.cml` file being small.
- **`.jtm` tile-layer loader**, inside `b(String) throws Exception` (723):
  **format confirmed**, see updated `ASSET_FORMATS.md`. Byte 0/1 of the
  resource = grid width/height (`f`/`g`, both `static byte`). What follows
  is one or more layers of `width*height` tiles, each layer
  run-length-encoded: byte `0xFF` starts an escape `(0xFF, count, tileId)`
  -> `count` copies of `tileId`; any other byte is a literal single tile.
  Storage is column-major (`a[col*height + row]`). Each layer becomes a
  `byte[]` pushed onto the static `Vector a` (layer 0 = base terrain,
  further layers = overlays/decoration -- matches the procedural generator
  below pushing exactly 3 layers: floor, walls, "extras").
- **Procedural dungeon generator**, `a(int[],int[],int,int)` (625) plus
  helpers (420-623): builds a `width*height` grid (same layer-stack
  convention as `.jtm`), carves a random walk from a start to an end point
  (`a(byte[],int[],int[],int,int,int[])`, 428 -- direction-biased random
  walk that also drops side-branch "treasure room" markers into
  `this.m[]`), then post-processes wall-piece selection by 4-neighbor
  pattern matching (`a(byte[],byte[],int[])`, 555 -- picks corner/straight
  wall tile variants, classic autotile logic) and spawns monsters at the
  branch points (`m()`, 388, calls `h.a`/`h.b` to reset/attach actor 0).
  This is the "Level 1"/"Level 2"/... procedural mode referenced in `l()`'s
  string tables (see below) -- as opposed to the fixed, hand-built `.jtm`
  levels loaded by `b(String)` for the story dungeons `l01`-`l14`.
- **`l()`** (245): builds the static `String[][] a` menu-text table:
  level names, the **fixed 12 story-level script paths**
  (`/l01_1.scr` .. `/l12_12.scr`, confirming exactly which `.scr` file is
  the "main" one per level out of each level's several `.scr` variants),
  difficulty names, and main-menu option labels -- assembled from
  `a(int)` (localized string lookup by id, see below) plus a few literal
  option arrays. Also builds `static String[] b`, a 100-entry table of
  known dialogue/HUD strings (rank names, "# 0".."# 9" counters, etc.)
  keyed by id.
- **`run()`** (1413): the MIDP game-loop thread. Computes frame delta,
  ticks the script interpreter (`e.a`) when active, ticks all 25 actor
  slots (`h.a(a[i], dt, ...)`) and the projectile system (`i.a(dt)`),
  handles player movement input via `this.b(dt)` (see next), checks
  distance to level-exit trigger points (static `byte[] d`, up to 75
  bytes = 25 triggers * 3 bytes: gridX, gridY, exit-script-id) to show a
  "press action" prompt, then `repaint()`/`serviceRepaints()`.
- **`b(long)`** (1935, ~490 lines): the input dispatcher, keyed off game
  state `static int m` (screen/mode id: `0`=playing, `1`/others = various
  menus/dialogue/loading -- not all 20+ values enumerated yet). Routes
  D-pad/fire presses to either direct player movement (`h.a(playerActor,
  dir, dt)`) or to whichever UI screen is active (`this.a` of type `f`,
  the dialogue/menu screen -- `this.a.a(char)`, matching `f`'s own input
  handler read separately below). Also handles picking up world items at
  trigger points and NPC/chest interaction (case `var5==7` block, 2006).
- **`paint(Graphics)`** (874, ~520 lines) and `b(Graphics)` (837): the
  renderer -- draws the tile layers, actors (via `h.a(actor, Graphics,
  offset)`), projectiles (`i.a`), and HUD, then blits the offscreen
  buffer. Not traced method-by-method yet.
- Six methods `a()`..`f()` (2446-2782) each return a `String[][]`: these
  are the **per-screen localized UI text tables** (options menu, character
  sheet labels, etc.) -- same pattern as `l()`'s table, split out because
  they're lazily built and dropped via `k()` (233, clears them + `gc()`)
  when not needed. Not individually identified by screen yet.
- `static String a(int id)` (3329) / `static int b(String)` (3348): the
  id<->string lookup, backed by `a.a(int)` (class `e`'s own id table) with
  a `0xF000` "high" bit to distinguish `e`'s per-level local string table
  from `b`'s own global table (`static String[] a`) -- mirrors the same
  `(id & 0xF000) == 0xF000` test seen throughout `e`'s bytecode
  interpreter (see below).
- Persistence: `RecordStore`/`RecordStoreNotFoundException` are imported
  but not yet traced to a specific save/load method -- open question.

**Open questions for `b`:** the full meaning of state id `m` (confirmed
`0` = playing; others unconfirmed), the per-screen `a()`-`f()` table
contents, the full `paint()` HUD layout, and the save/load (`RecordStore`)
code path.

## `e` -- the `.scr` bytecode interpreter (Phase 2's actual target)

**Not yet renamed**; proposed name **`ScriptInterpreter`**. This *is* the
class `ROADMAP.md` Phase 2 is looking for -- confirmed by direct evidence,
not guesswork. Two halves:

1. **Level-data table loader**, `a(String)` (85) -- parses a `.scr` file
   (via `b.a`/`b.b`, the shared resource loader) into 11 fixed-shape
   tables, each populated by its own field-tag parser (`a(int)` through
   `j(int)`, 142-434, tag-per-record: tag `0` sets the record's row index,
   other tags fill specific columns, tag `31` ends a record). Tables (all
   `int[][]`, sizes are `[rows][cols]`): `a`/`b`=`[25][21]` (`a`=live
   copy, `b`=pristine backup restored via opcode 67 -- almost certainly
   the **item/object placement table**, 25 slots), `c`=`[37][8]`
   (**NPC/monster spawn table**, matches the 37-level cap seen in
   dawnstar's `ESGame`, though here per-*level* not global), `d`=`[42][10]`,
   `e`=`[11][14]` (has 3 parallel int[15] side-tables `h`/`i`/`j` per
   record, `i`/`j` being `-1`-terminated lists -- **exits/doors table**,
   `i`/`j` = linked destination coordinate pairs), `f`=`[10][21]`,
   `g`=`[25][7]`, `k`=`[10][15]`, `l`=`[30][4]` (**random-encounter/loot
   table**, confirmed: `a()` at 1240 rolls `b.a.nextInt()` -- yes, `e`'s
   own field `a`, the shared `Random`, is a *third* unrelated field also
   spelled `a` on class `b` -- against `l[][2]` as a modulus and decrements
   a per-row counter `l[][3]`, classic weighted-loot-with-limited-uses).
   Also loads a local string pool (field `a: String[255]`, count `d`) used
   by the `0xF000`-tagged string-id convention shared with `b.a(int)`.
2. **Bytecode interpreter**, `b(long)` (462) -- a `switch` on opcode byte
   `var8` (read via the tiny stack-machine helpers `b()`/`c()`/`d()` =
   read-byte/read-int24/read-short from the script buffer at the current
   program counter, itself stored as `this.a[this.a[this.a-1]]` i.e. a
   **call-stack of PC values**, `this.a-1` = current stack depth -- this
   supports subroutine calls, not just linear execution). ~78 opcodes
   (0-78+) confirmed at the call-site level: most delegate straight into
   `b`'s own methods (dialogue text via `this.a.f(str)`/`this.a.i()`,
   32-40=actor spawn/teleport/palette, `h.a`/`h.b` for actor state and
   position, `i.a` for magic-projectile spawning, `56`=load a different
   language pack (`b.a(name, count)`), `73`/`74`=fade transition flags).
   Case bodies are individually understood (see inline evidence in the
   decompiled source) but **not yet given symbolic opcode names** -- that
   naming pass, plus writing an actual `.scr` disassembler
   (`tools/parse_scr.py`), is the natural next Phase-2 milestone once this
   survey lands.

**Open questions for `e`:** symbolic names for all ~78 opcodes (mechanical
work now that the dispatch table is mapped); which of the 11 tables is
which exact game concept (educated guesses above, not confirmed against a
save file or by cross-referencing a specific `.scr` hexdump yet).

## `a` -> renamed `Strings.java` (localization / text-resource loader)

Loads `/lang_N.txt` (13 files, `N`=0..12 -- **not** confirmed to be one
per human language yet, see `ASSET_FORMATS.md`; could equally be UI-text
categories), `/copywrite.txt` (single string), and `/start.txt` (`|`
-pipe-delimited array, indexed by byte). The two int tables `a(int)`/
`b(int)` (13 entries each, indices 0-12) are **unconfirmed** -- values
don't match the lang file byte sizes, so they're likely font-metric or
buffer-size constants rather than per-language data; left as open question
rather than guessed.

## `c` -> renamed `DialogueNode.java` (dialogue/NPC-choice tree node)

```java
public final class DialogueNode {
   public DialogueNode parent;
   // Single-select "radio group" marker: true for the currently
   // equipped/chosen sibling in an equipment- or spell-slot picker list.
   public boolean marked;
   // Gates whether this leaf can be activated at all (false = shown red,
   // blocked outside the root Buy/Sell screens).
   public boolean available;
   public String text;
   public java.util.Vector children;   // Vector<DialogueNode>; empty = leaf/actionable line
   public String[] answerLines;        // rumor/answer text shown when picked, or null
   public String tooltip;
}
```
Confirmed via `f`'s render/nav code (below) -- the select-leaf handler
(action code 7) walks all siblings clearing `marked`, then sets it on the
picked one, with a dual-slot special case (string id 27, e.g. two ring
slots) that skips clearing a compatible already-marked sibling. This is
Oblivion's equivalent of stormhold/dawnstar's NPC-choices menu (see recent
stormhold milestones M64-66) -- same tree-of-choices shape, different
engine.

## `d` -> renamed `SpriteFrame.java` (`.cml` scene/animation node)

```java
public final class SpriteFrame {
   public String imagePath;
   public byte groupId;                    // top-level nodes only
   public byte frameDx, frameDy;           // draw-position pixel offset
   public byte holdFlag;                   // loop-vs-freeze on anim end; weak confidence
   public byte isSprite;                   // 1 = draw via cached MIDP Sprite (mirrored)
   public byte frameChainFlag;             // always 1 on animated subframes; purpose unconfirmed
   public short offsetX, offsetY, width, height;
   public SpriteFrame nextFrame;    // subframe chain within one animation group ("a" in decompiled)
   public SpriteFrame currentFrame; // playback cursor, self-initialized ("b" in decompiled)
   public SpriteFrame nextGroup;    // sibling group, walked by group id ("c" in decompiled)
}
```
This is the parsed form of a `.cml` file's image-group table. Confirmed
field-for-field against `g`'s parser (see next), see `src/SpriteFrame.java`
for the full per-field evidence. **`ASSET_FORMATS.md` has been updated
with the full binary layout** derived from `g.a(String)`.

## `g` -> renamed `SpriteRenderer.java` (`.cml` parser + image/sprite cache + blitter)

Static utility + cache class. `a(String path)` parses a `.cml` resource
into a `SpriteFrame` tree (confirmed format, written up in
`ASSET_FORMATS.md`); images are lazily `Image.createImage`'d and cached in
a `Hashtable` keyed by path, `javax.microedition.lcdui.game.Sprite`
instances (for mirrored/transformed frames, `setTransform(2)` = flip)
cached in a second `Hashtable`. `a(Graphics, SpriteFrame, groupId, x, y)`
draws a given group's current frame, clipping to screen bounds; `a`/`b(SpriteFrame,
groupId)` return current frame width/height; `a`/`b(SpriteFrame,int)`
advance/reset a group's animation frame pointer.  A CRC32-style table
builder (`a()`, 338, standard polynomial `0xEDB88320` reversed) is built
but its consumer wasn't found in this pass -- likely unused/dead code, or
used by a `.jtm`/`.scr` integrity check not yet located.

## `h` -- movement, combat, AI, leveling, equipment (static utility, not yet renamed)

Proposed name **`ActorSystem`**; **not mechanically renamed this pass** --
2385 lines, the second-heaviest field-collision class after `b` (see
"Reading notes"), and every method takes/returns `j` (`Actor`, also not
yet renamed -- see below), so a safe rename here really wants to happen
together with `Actor.java`'s own field-by-field pass, not before it.
All-static utility class operating on `Actor` (`j`, below) instances.
Confirmed responsibilities, each backed by directly-read code:

- **Construction**: `a(String cmlPath, byte type)` builds a fresh actor
  from a `.cml` sprite path; `a(byte[] data, int offset)` deserializes a
  **saved/pre-built actor record** (used for the player character loaded
  from `charin`-equivalent data or a level's fixed NPC table) -- confirmed
  binary layout: type byte, race/model byte, 3-byte color, then a run of
  big-endian stat shorts (HP-related `o`, six shorts `s,t,u,v,w,x` =
  attribute-like stats, `u/y` faction/behavior flags, `E`/`F` aggro-range
  min/max), followed by a length-prefixed name string and a small
  variable-length equipped-item list.
- **Grid/collision**: actor position is tracked as **three points**
  (`b`=origin, `c`/`d`=two more corners of a bounding footprint) in
  sub-tile-precision coordinates (`>>7` converts to grid cell, matching
  `b`'s tile grid), tested against the level's collision-flag layer
  (`b.a[]`, values 0=open, 1=solid, 2-5=diagonal/half-tile blockers) via
  `a(Actor,byte)`; `a(Actor)` is the "actor is fully off-grid/out of
  bounds or blocked" check used by both player movement and the
  procedural-dungeon monster placement.
- **Movement**: `a(Actor,dir,dt)` (direct 4-way player input movement) and
  the auto-move-toward-target variant inside `a(Actor,dt,boolean)` (the
  main per-frame actor tick) which walks an actor toward `j[0]/j[1]` (a
  set destination cell) at a speed derived from stat `w`.
- **Combat**: `a(int dmgBase, Actor target, Actor attacker, bool, bool)`
  is the core damage-resolution function -- rolls a crit/miss chance,
  applies attacker's damage stat vs. target's defense/`D` multiplier,
  shows floating damage/"MISS"/"BLOCK" text (string ids 470-472), and on
  target death (`q<=0`) triggers XP grant (`c(Actor,int)`, handles level-
  up: recomputes derived stats via the same 8-branch class/level table
  used at creation, appends a "+N Strength, +N ..." level-up message using
  string ids 415-420) and death animation state (`e=6`).
- **AI**: `c(Actor)` (wander/return-to-spawn state machine, `A` flag),
  `b(Actor)` (nearest-hostile-target acquisition + chase/flee decision
  using `E`/`F` aggro range thresholds), `c(Actor,int)` -- unconfirmed
  overload, see below -- and `c(Actor,boolean)` (special-attack trigger:
  damage-over-time poison (`case 0`), fear/flee, spawn-more-actors
  (spawns a scamp via `/oh_scamp.cml`), AOE around self (`case 4`),
  heal-self (`case 3`, string-id-gated between two special effect ids
  `61618`/`61619`), self-heal-HP (`case 5`)).
- **Equipment/inventory**: a fixed-slot array `k: int[255]` stores worn
  items packed as `(slotTag<<8)|itemId` (slot tags `0`=weapon, `1`=spell/
  ability, `2`=armor-with-regen); `a`/`b(Actor,slot,item[])` equip/unequip,
  recomputing derived max-HP/defense (`f(Actor)`, the big per-class,
  per-level stat-bonus lookup switch, 8 cases = 8 character
  classes/races) after every change. `a(Actor, ByteArrayOutputStream)`
  serializes an actor back to the same binary layout `a(byte[],int)`
  parses -- this is very likely the save-game record format, tying back
  to `b`'s unexplored `RecordStore` usage.

**Open questions for `h`:** several `c(Actor,...)` overloads only
partially disambiguated by argument count; the exact meaning of stats
`s/t/u/v/w/x/y` (six attribute-like shorts -- strength/agility/etc. by
strong inference from the level-up message string ids 415-420, but not
matched 1:1 to English attribute names yet); field `f` (monster/class id,
1-8) not yet matched by name to the six `oh_*.cml` bestiary files plus
player -- likely `1`=player, `2..7`=deadroth/dremora/ghost/liches/ogre/
scamp in some order, `8` possibly a boss or unused.

## `j` -> renamed `Actor.java` (player + monster shared data record)

The universal actor struct -- used for the player character (field `c`,
type tag, ="Champion" literal name set in the loader) and every monster
alike; this is why `h`'s combat/AI code is entirely type-agnostic. **Not
mechanically split into per-field-renamed source yet** -- ~90 fields
across 4 overloaded groups (byte/short/int[]/int/self-ref/String), see
`h`'s writeup above for the fields confirmed by usage. Notably the class
also declares a **duplicate, apparently-dead set** of 4 `byte[2]` position
fields (`a,b,c,d`) alongside the *actually used* `int[2]` position fields
of the same names (`b,c,d,e` -- offset by one letter) -- see "Reading
notes" above; every call site read for this survey used the `int[]`
versions, so the `byte[]` ones are presumisingly unused/dead and should be
dropped rather than renamed when `Actor.java` gets its full rename pass.

## `i` -> renamed `ProjectileManager.java` (magic/ranged-attack projectile system)

Static utility + a fixed pool of 11 projectile slots (`short[99]`, 9
shorts/slot: packed type+facing, x, y, elapsed-since-move, animation
state, origin x/y, lifetime, elapsed-total). `a(dir,int,Actor,duration)`/
`a(dir,x,y,duration)` spawn a projectile either homing on a target actor
or thrown in a fixed direction (4-way + 4 diagonal, `oh_magic.cml` frame
ids 0-14); `a(long dt)` advances every live slot, moving it one grid step
at a time, checking tile collision (`g.a`, the animation-frame-advance
helper, doubling as an "animation finished" signal that also ends
lifetime) and actor collision (nearest-enemy-in-250-range hit test via
`h`'s cross-referenced distance helper), applying damage through `h`'s
combat resolver on impact. `a(Graphics,int[])` draws all live projectiles
relative to a camera offset, culling off-screen ones against the level's
pixel dimensions (`b.a`/`b.b`).

## `f` -- NPC dialogue / conversation-tree UI (not yet renamed)

Proposed name **`DialogueScreen`**; **not mechanically renamed this
pass** -- on closer inspection while attempting it, this 380-line class
turned out to declare **eight** unrelated fields all spelled `a`
(`SpriteFrame`, `String`, `DialogueNode[]`, `Game`, `Image`, `byte[]`,
`byte`, and `short`), denser than any other class surveyed except `b`.
Behavior below is confirmed by direct reading; the mechanical rename is
deferred to its own pass rather than risk a wrong field-disambiguation
under the same time pressure that already produced one wrong guess in
this session (caught and fixed, see `ProjectileManager.java`'s header
comment).

Owns the current `DialogueNode` (root swapped per-NPC by `a(byte[]
cmlPixmap, DialogueNode[] roots, String npcName, Image portrait,
Graphics)`, `roots` being one root per NPC/topic, indexed by `this.f`),
renders the scrollable choice list with word-wrap and a scroll indicator
(up/down arrow glyphs, frame ids 53/54), a description/tooltip popup box
for the highlighted entry, and an item-reward panel (label/value pairs,
frame ids 5-13 = UI chrome). Navigation (`a(char keyAction)`, action codes
4=down/3=up/5=prev-NPC/6=next-NPC/7=select) walks the `DialogueNode` tree,
with a special case preventing "buy"/"sell" (string ids 149-152) from
being simultaneously highlighted, and a scroll-then-typewriter reveal
effect (`a(long dt)`) for long lines that get truncated with "..." and
need to auto-scroll before being readable. This is the direct structural
analogue of stormhold's NPC-choices-menu milestones (M64-66), just for a
completely different engine/format.

## Renamed source

`Strings.java`, `DialogueNode.java`, `SpriteFrame.java`,
`SpriteRenderer.java`, `ProjectileManager.java` live in `../src/` --
these four (five with `blt/Main.java`, already real-named) had few or no
same-name field collisions, so renaming them mechanically was safe in
this pass. They still reference `b` (Game) and `j` (Actor) by their
original decompiled names/fields where those two haven't been renamed
yet, on purpose -- see the comment at the top of `ProjectileManager.java`
for why guessing at `j`'s field meanings there would be worse than
leaving them obfuscated.

`Actor.java` (`j`), `Game.java` (`b`), `ScriptInterpreter.java` (`e`),
`ActorSystem.java` (`h`), and `DialogueScreen.java` (`f`) are documented
above but left as `decompiled/{j,b,e,h,f}.java` for now -- all five have
enough same-named-field collisions (from 2 on `j` up to 12+ on `b`) that
a rushed rename is riskier than useful. `Actor`+`ActorSystem` (`j`+`h`)
should be renamed together in one pass next, since almost every `h`
method signature is `j`-shaped; `DialogueScreen` (`f`) next after that
(it only depends on the already-renamed `DialogueNode`/`SpriteRenderer`
plus `Game`); `Game`/`ScriptInterpreter` (`b`/`e`) are their own, larger
follow-up milestones after that -- same incremental cadence as
dawnstar/stormhold.
