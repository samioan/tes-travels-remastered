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

## `h` -> renamed `ActorSystem.java` (movement, combat, AI, leveling, equipment)

All-static logic over `Actor` records. Now fully renamed (see
`docs/rename.map` for the old-name -> new-name table and "Renaming
tooling" below for how). Findings, several of which **correct** the
first-pass survey:

- **Construction**: `createFromCml(path, slot)` builds a bare actor from a
  `.cml` sprite; `fromRecord(byte[], off)` loads a saved/pre-built record and
  `serialize(actor, out)` writes the same layout -- the player save format:
  `slot, classId, xp(3 bytes), level(2), strength(2), intelligence(2),
  agility(2), speed(2), endurance(2), willpower(2), weapon(2), sightRange(2),
  attackRange(2), team(1), gold(2), <len><cml path>, <n items>, n x
  {(equipped?0x80:0)|category, id}`. (The length-prefixed string is the
  **sprite path**, not the display name; the player's name is the literal
  "Champion".)
- **Geometry**: `pos`/`footB`/`footC` are three sub-tile world points (128 per
  cell) forming an isometric footprint (`setPosition` derives footB/footC from
  the sprite width); `cell`/`footBCell`/`footCCell` are the same points >> 7
  and are what `isBlocked`/`footBlocked` test against `Game.collision`
  (0 open, 1 solid, 2-5 half-tile diagonals via the position within the
  cell). `screenPos` = isometric projection `((x-y)>>3, (x+y)>>4)`;
  `sortCell` picks the draw-order cell. `undoMove` restores `prevPos`.
- **Attributes** (verified against the lang strings and Game's character
  sheet): `strength` 415, `intelligence` 416, `willpower` 417, `agility` 418,
  `endurance` 419, `personality` 420, plus `speed` (walk speed), `dodgeChance`
  (471 "Dodge"), `blockChance` (470 "Block"), `defenseRating` 431,
  `attackRating` 432. `maxHp = level*4 + (strength+buff)*2 + endurance*2 +
  bonusMaxHp`, `maxMp = level*4 + intelligence*2 + bonusMaxMp`; regen interval
  = 40000/max ms per point. Class ids 1-8 = Monk, Nightblade, Barbarian,
  Archer, Knight, Spellsword, Sorcerer, Battlemage; `recalcDerivedStats`
  applies the per-class, per-level dodge/block/attack/defense breakpoints and
  `applyLevelUpBonus` the extra bonus at levels 5/10/15/20. XP: `xpForLevel[]`
  thresholds (level cap 25), `xpReward[]` indexed by the victim's level.
- **Combat**: `attack` picks weapon or special damage (`(strength + buff +
  weaponPower >> 1) + buffs`, scaled by `attackRating`%, 1-in-16 crit),
  `applyDamage` rolls dodge, then block, then subtracts
  `(agility + armor + buffArmor >> 3) + buffDefense`; handles death (xp to the
  killer or its `owner`, death script, loot drop via `ScriptInterpreter.rollLoot`).
  Damage-over-time (`applyPoison`), buffs (`useConsumable`, `buffTimer`) and
  potions (`quaffPotion`) are timers on the actor updated in `update`.
- **AI**: `update` also drives monsters: `findNearestEnemy`, `aiThink`
  (chase inside `sightRange`, attack inside `attackRange`, else retreat via
  `stepAwayFrom`), `useSpecialAttack` (special-row types: 0/1 buffs+projectile,
  2 summon a scamp, 3 heal/AOE/projectile by id 61618/61619, 4 AOE, 5 dodge
  buff, 6 self-heal), `teleportStep` for `aiType` 2 blinkers.
- **Inventory**: `inventory[255]` packs `(category<<8)|id` (0 weapon, 1 armor,
  2 consumable); armor goes to one of 8 `wornArmor` slots (row column 3);
  `addItem`/`removeItem`/`canUseItem` (class restrictions via
  `ScriptInterpreter.classAllows`), `reselectBestWeapon`, `equipFromString`
  (script strings prefixed "Spell: " / "Weapon: " / "Bow: ").
- **Player input**: `moveDir(actor, dir, dt)` and `handleAction(actor, action,
  dt)` (2 toggle special, 3-6 move, 7 interact).

**Open**: exact semantics of `aiType` 3, `animState` 2/3/5, the special-attack
row columns, and the `classBase`/`classLists`/`specials` tables in
`ScriptInterpreter` (only their use here is known).

## `j` -> renamed `Actor.java` (player + monster shared data record)

The universal actor struct; every field is now named and commented in
`src/Actor.java`. The four `byte[2]` fields `a..d` that the first survey called
"dead" are the **grid cells** (`sortCell`, `cell`, `footBCell`, `footCCell`)
and are heavily used; nothing in the class is dead. Same descriptor-collision
situation as the rest of the codebase, resolved with the descriptor-aware
renamer below.

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

## Renaming tooling

`docs/rename.map` + `tools/MapRenamer.java` + `tools/decompile_renamed.py`:
a Vineflower identifier-renamer keyed on (class, name, JVM descriptor), which
is the only way to give names to the obfuscator's same-letter fields and
return-type-only method overloads. `python oblivion/tools/decompile_renamed.py
<outdir> [--uniquify]` re-decompiles `extracted/` with the map applied
(`--uniquify` suffixes every still-unmapped field with its type, e.g. `a_aBy`,
so a class can be read unambiguously before it is named). Add entries to the
map, regenerate, and copy the result. `Game`/`ScriptInterpreter`/
`DialogueScreen` members that `Actor`/`ActorSystem` touch are already mapped.

## Renamed source

`Strings.java`, `DialogueNode.java`, `SpriteFrame.java`,
`SpriteRenderer.java`, `ProjectileManager.java`, **`Actor.java`** and
**`ActorSystem.java`** live in `../src/`. They still refer to `b` (Game) and
`e` (ScriptInterpreter) by their decompiled names, plus the few mapped members
(`b.actors`, `b.collision`, `b.random`, `e.runScript`, ...).

`Game.java` (`b`), `ScriptInterpreter.java` (`e`) and `DialogueScreen.java`
(`f`) are still `decompiled/{b,e,f}.java`. Next: `DialogueScreen` (`f`), then
`Game`/`ScriptInterpreter` with the renamer (`--uniquify` output first).
