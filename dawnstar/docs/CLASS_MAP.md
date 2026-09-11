# Dawnstar -- class map (phase 1 findings)

Full read-through of every class in `decompiled/`, done from the code alone
(no external docs exist for this engine). Confidence is high for the small
classes (traced essentially every line), now also for `e`/`GameCanvas`
(same full hand-trace), and high-but-not-exhaustive for the two remaining
large ones (`ESGame`, `j`) -- their overall architecture and most fields
are confirmed, a handful of fields are still genuinely unclear and are
marked as such rather than guessed.

Renamed, hand-written source for the fully-understood classes lives in
`../src/`. `e` (renamed `GameCanvas.java`) has now had the same
hand-trace-then-compile-check treatment as the other 9; `j`/`ESGame`
are still NOT mechanically renamed (see "Why `j` (and, previously, `e`)
weren't renamed by mechanical means" at the bottom) -- this document
remains their authoritative map.

## The shared "ngame" engine

`ESGame extends ngame.midlet.RegisteredMIDlet` (real names, unobfuscated --
this base class is Vir2L's shared MIDlet-lifecycle/licensing shim, also
used unobfuscated-in-name by `stormhold/decompiled/ngame/midlet/a.java`).
`RegisteredMIDlet` handles the MIDlet lifecycle (`startApp`/`pauseApp`/
`destroyApp`), trial/unlock-code gating (`verifyLicence`, unused in this
retail build), and generic exit/error-alert plumbing. Nothing here is
Dawnstar-specific.

## `ESGame` (the MIDlet / main controller)

Unlike `a`-`k`, `ESGame`'s own members were **not** heavily obfuscated --
public/package fields and methods mostly kept real names
(`getResource`, `gameCanvas`, `chests`, `dungeons`, `debugCode`, ...).
Reading it directly confirmed almost everything inferred from `a`-`k`.

- Owns every `Screen`(`g`)/`LoadingScreen`(`h`) instance as a named field:
  `mainMenuUI`, `newGameUI`, `characterMainUI`, `InventoryUI`,
  `SpellsListUI`, `NPCChoicesUI[9]` (one per shop/NPC, indices matching
  `Shop.NAMES`), `LevelUpUI`, `OptionsUI`, `GenericInfoUI` (generic
  message popup, used for dialogue, errors, and one-off notices), etc.
- Owns the shared world state as static arrays, one slot per dungeon level
  (1-37): `dungeons: Dungeon[37]`, `monsters: Hashtable[37]` (key
  `"x,y"` string via `Util.posKey`, value = 28-byte packed `Monster`
  record), `chests: Hashtable[37]`, `droppedItems: Vector[37]`.
- `getResource(String name)` -- confirmed exact format of `datfiles.lmp`,
  see `ASSET_FORMATS.md`.
- `createImage(String)` / `createImageFromFile()` -- confirmed exact format
  of `imgfiles.lmp` (different from `datfiles.lmp`!), see
  `ASSET_FORMATS.md`.
- Vestigial/dead code: a `Pluto-Server-URL` / `Mserver-User-Id` JAD-property
  mechanism (`SERVER_DATAFILE_BASE_URL`, `j.N`) that's read at startup but
  never used for anything in this build beyond a debug `println` -- looks
  like leftover scaffolding from a server-backed variant of the "ngame"
  engine. Safe to drop entirely in the port.
- `run()` is a state machine over `helperThreadState` (1=unused legacy
  download path, 2=`runAppload` app boot/asset load, 4=`createNewGame`,
  5=save, 6=load) -- this is the background-thread half of loading
  screens; the foreground half is `LoadingScreen`.
- Startup order (`runAppload` -> `allocateESGame` -> `allocateAllUIs` ->
  `allocAllDungeons`): load `charin.dat`/help text/`itemsin.dat`/
  `spellsin.dat`/`monstersin.dat` (`j.s()`, `Item.load()`, `Spell.load()`,
  `Monster.load()`), then every UI screen's static text, then
  `imgfiles.lmp` images, then `new DungeonGenerator(dungeons, splashUI)`
  which procedurally builds all 37 levels from `geomin.dat`.

## `a` -> `Item` (item database + loot tables)

Fully renamed, see `../src/Item.java`.

Loads `itemsin.dat`: `n` category names (weapon/armor/etc. group labels),
then `k` item records, column-oriented (all names, then all of column
`j`, then all of `c`, ...): `name`(UTF), `j`=**category id** (11 =
"gift"/camp-consumable category, matches `l[]` flavor text below),
`c`=**subtype/icon id**, `m`=**quest-turn-in flags** (2 bits per shop,
packed for shops 5-8 -- see `Shop.questFlagsFor(shopId,itemId)`),
`f`=**buy price** (confirmed against `Shop.dialogue`'s buy branch),
`a`=**sell price** (confirmed against the sell branch -- name collides
with the class itself, valid in Java, just confusing), `e`=**equip slot
id** (0=weapon,1=shield/offhand,2..6=armor slots, used as index into
`Player.ag[7]`).

`l[13]` = fixed flavor-text array for the 13 "gift"/special consumable
effects, item ids 87-99 in order (confirmed against `Player.a(int,Monster)`
switch on those exact ids): `"Warp to camp"`, `"Cures ailment"`,
`"Restores Health"`, `"Restores Magicka"`, `" "` (unused slot, id 91 is
Fatigue-restore with no distinct flavor line), `"Grants level experience"`,
`"Health & Magicka"`, `"Increase harm"`, `"Increase armor"`,
`"Safe camping"`, `"Kills weak monster"`, `"Kills normal monster"`,
`"Kills strong monster"`.

`droppeditemsin.dat` loader (`c()`/field `h`): a `[depth][3]` byte matrix,
one row per dungeon depth tier, used by `dropItem(Random,depth)` as a
percentile loot table (roll rarity tier via 4-sample-max percentile, roll
depth-adjusted row, read `h[row][col]` as the item id, with a 2-byte
extended-id encoding when the low byte is 86).

## `b` -> `Spell` (spell database)

Fully renamed, see `../src/Spell.java`.

Loads `spellsin.dat`: count, then column-oriented fields per spell --
`c`=**name**(UTF), `h`=**skill required** (index into `Player.au[14]`,
via `Shop`'s skill-name table `ax`), `e`=**magicka cost**, `f`=**base
power**, `d`=**school/type id** (`isOffensive()` checks `== 2`),
`j`=**duration multiplier**, `g`=**icon id**, `a`=**description**(UTF,
shown in the spell-info screen with `"Spell: "` prefix in
`Player.b(int)`'s item-tooltip builder).

## `c` -> `DungeonGenerator` (procedural level generation)

Not a data table -- the actual room/corridor/loot generator, run once at
startup for all 37 levels. Confirmed algorithm:

1. Builds a fixed 19x19 tile template for level 1 (Dawnstar, the hub
   town) by hand-carving specific corridors into an all-walkable grid --
   this is level 1's fixed layout, not random.
2. Loads `geomin.dat`: 37 rows x 6 bytes. Row layout (confirmed against
   `Dungeon.tileAt`'s boundary-crossing logic): `[0]`=north neighbor
   level id, `[1]`=east neighbor, `[2]`=south neighbor, `[3]`=west
   neighbor, `[4]`=up-stairs direction code, `[5]`=down-stairs direction
   code (0 = none). The stairs fields are compass codes (1=N,2=E,3=S,4=W,
   same convention as the neighbor ids and `Player` facing), not tile
   columns -- see `ASSET_FORMATS.md`.
3. For levels 2-37 (35x35 each): places up to 15 non-overlapping
   rectangular rooms at random, connects them with an L-shaped-corridor
   minimum-spanning-tree pass (nearest-unconnected-room heuristic, not a
   true MST), carves stairwell corridors at the fixed edge positions
   matching each level's `geomin.dat` neighbor entries, and drops 5
   treasure chests per level (weighted by a per-level room-count/size
   class table, `d[36]`, values 1/3/5) with encoded loot (`Item`'s loot
   roll, extended-id when byte 86).
4. Special-cases 4 "key" levels (3, 12, 21, 30 -- every 9th level, i.e.
   the last level of each of the first 4 zones) by recording their central
   room's coordinates into `Shop.SHOP_X[5..8]`/`SHOP_Y[5..8]` -- these are
   the 4 named shopkeepers (`Alhavara`/`Beatrice`/`Chung`/`Delacroix`) who
   live inside a dungeon room rather than the hub town.

## `d` -> `Monster` (monster instances + AI)

Fully renamed, see `../src/Monster.java`.

Loads `monstersin.dat`: count, names, then 17-byte stat rows per monster
*type* (columns referenced piecemeal throughout combat/AI code -- not all
17 are pinned down; confirmed ones: col2=detection-range related,
col3=?, col4=base defense/to-hit offset, col5=attack stat, col11=on-hit
status-effect id (1-8, matches `Player`'s 8 named ailments), col14=max
HP, col15/16=death-drop chance and loot-table row).

Each `Monster` *instance* is a 28-byte packed record (`toBytes()`/
`fromBytes()`) stored directly as the hashtable value in
`ESGame.monsters[level]` -- no separate object graph, the byte array
*is* the storage format (this is also exactly the save-game wire format
for monster state, `writeTo(DataOutputStream)`/static
`readFrom(DataInputStream)`, but two different serializations: 28-byte
in-memory vs. explicit stream form -- same fields either way):
type id (short), level(byte, redundant with `n`? -- reads `a` field as
"item/type", `l`=**monster type id**, `g`=**current HP**, `o`/`m`=**tile
position (x,y)**, `i`=**boolean, unconfirmed** (serialized as a whole
byte, `readBoolean`/`writeBoolean`), `n`=**dungeon level number**,
`c[10]`=**AI/loot scratch bytes** (index 5/6/7 seen used for a lingering
"marked"/cooldown flag pair, rest unconfirmed), `b`=**attack-animation
frame counter**, `f`=**AI tick phase** (0=idle,1=cooldown,2=acting, used
by `tick(Player,long)`).

`tick(Player,long)` (originally `a(j,long)`) is the per-monster combat/AI
step, called once per nearby monster per game tick from `Dungeon`'s
proximity scan: rolls detection based on distance and the player's
stealth-ish stat, then either attacks (damages `Player.E[2]`, i.e.
current Health, and can inflict one of the 8 status ailments per col11)
or chases (one step toward the player, `Player`-relative axis pick).

`onDeath(boolean guaranteedDrop)` rolls the monster's death drop via
`Item`'s loot table and adds it to `Dungeon.droppedItems`.

## `e` -> `GameCanvas` (renderer + input + tick loop)

Fully renamed, see `../src/GameCanvas.java`. `extends
com.nokia.mid.ui.FullCanvas implements Runnable`. Unlike the other 9
renamed classes it still depends on `Player`'s own unrenamed API for
any value that crosses that boundary (the current `Dungeon`, the
targeted `Monster`, etc. all keep their old single-letter types there)
-- see the file's own header comment for exactly which fields that
applies to, and CLASS_MAP's `j`/`Player` note below for why.

- **First-person corridor renderer**: `CORRIDOR_WALL_TABLE[5][6][4]`
  (was `k`) is a fixed lookup table of wall-segment draw commands for 5
  possible forward-visibility patterns (how far you can see down a
  straight corridor before a wall/junction), used by
  `paintCorridorWalls` every frame. Reads tile-occlusion bits from a
  `Dungeon`-populated 17x17 (or 7x7 zoomed-out) view grid
  (`visibleTileGrid`, was `C`), not the full level array directly.
  **Newly confirmed while renaming**: the floor/wall texture pair used
  (`floorTexture`+`wallTexture`, "floor3.png"/"wallsr.png", vs.
  `floorIceTexture`+`wallIceTexture`, "floorIce.png"/"wallsi.png") is
  selected by `Dungeon.e` (the *dungeon's own level number*, confirmed
  via the field-order cross-check against `Dungeon.java`'s `number`
  field -- **not** an ice/rock type flag as an earlier pass guessed):
  level 1 (the hub town) uses the plain floor/wall images, every other
  level (2-37, all icy) uses the Ice-suffixed ones. `gateTexture`
  ("gate.png") is selected by tile bit 6 (the edge/transition marker)
  taking priority over the wall-texture choice.
- **HUD**: 3-bar Health/Magicka/(Fatigue?) meter reading `Player.l(2)`
  etc. against `Player.E[3]/E[5]/E[7]`; a compass-direction glyph +
  minimap thumbnail (`minimapImage`, was `at`, an 89x89 offscreen
  `Image` rebuilt by `refreshMinimap` (was `p()`) whenever the player
  moves); a numeric hotbar (keys matching whichever of 4
  context-dependent action sets is active, see `computeHotbarContext`,
  was `j()`); a 2-line popup-message system (`messageLines`/
  `messagePriority`, was `d[]`/`V` -- `messagePriority` is a priority
  gate on which message can *replace* the current one, not a countdown;
  auto-hide after 3000ms is driven separately by `messageShownAt`) that
  backs every player-facing HUD string constant, now named `MSG_*`
  (`MSG_CANNOT_CAMP`, `MSG_NO_SPELLS`, `MSG_NOT_ENOUGH_MAGICKA`,
  `MSG_NO_MONSTER`, `MSG_REST_DISTURBED`/`MSG_REST_COMPLETE`,
  `MSG_CREATURE_DEAD`/`MSG_CREATURE_ATTACKS`, `MSG_CHEST`/
  `MSG_CHEST_LOCKED`, `MSG_INVENTORY_FULL`, `MSG_FOUND_ITEM`/
  `MSG_FOUND_SEVERAL_ITEMS`, `MSG_ENEMY_ARRIVED`).
- **Input**: numeric keys 1-9,0 are spell/item/camp/options hotkeys
  (mapping depends on `computeHotbarContext`); `*` toggles the
  zoomed-out minimap; arrow/game-action keys move/turn (delegated to
  `Player.a(dir,strafe)`, this class's field `pendingMoveDir`, was `n`,
  1-4).
- **Main tick loop** (`run()`, ~4Hz/250ms tick): drives camping
  (interruptible timed rest, `campState` field 0-3, was `c` -- 1=rolling
  for interruption, 2=safe/undisturbed wait, 3=a rare scripted
  "disturbed" event; see the field's doc comment in `GameCanvas.java`
  for exactly when each is entered), death/respawn (`deathState`, was
  `aM`), per-tick status-effect countdowns (`tickStatusCountdowns`, was
  `e(long)`) and once-per-real-second passive regen/drain
  (`tickPerSecond`, was `l()`), and a **scripted ambush system**: a
  per-second counter (`Player.Q`) that spawns extra monsters at
  hardcoded elapsed-second checkpoints (two different checkpoint
  schedules depending on `Player.ah`, presumably normal vs. "New
  Game+"), ending in a game-over if too many monsters end up alive at
  once. This looks like a scripted "you've overstayed in one place"
  penalty rather than a per-level trigger -- confirm against where `Q` is
  set to non -1 before treating it as universal.
- Delegates entirely to the active `Screen` instance (field
  `activeScreen`, was `Y`) when one is open (menus, dialogs, NPC
  dialogue) -- the 3D view only paints when `activeScreen == null`.
- A handful of fields turned out to be genuinely dead code while
  tracing every reference for the rename (declared, never read anywhere
  in the class, and not reachable externally): the `UNUSED_FONT`
  constant (was `J`), `UNUSED_TABLE` (was `m`, an unused `int[3][3]`),
  `unusedP`/`unusedAI` (was `P`/`aI`), `unusedAe` (was `ae`), and the
  `unusedKey9Request` flag (was `aG` -- captured from key '9' outside
  the chest/NPC context, but the tick dispatcher's branch for it is
  empty).

## `f` -> `Util` (misc helpers, no state)

Fully renamed, see `../src/Util.java`.

Grab-bag static helper class: `String` find/replace (`f.a(s, tag,
replacement)`, used for `<TAG>` substitution in NPC dialogue), bit
set/clear/test on `byte`/`int` (`setBit`/`clearBit`/`testBit`), big-endian
`long` decode from a byte offset (used for `Monster.k`, an 8-byte field
whose purpose is unconfirmed -- a timestamp of some kind given the type),
and thin wrappers around `ESGame`'s resource/RNG statics.

## `g` -> `Screen` (generic menu/dialog framework)

Fully renamed, see `../src/Screen.java`. One class implements every
non-3D-view UI screen via a `mode` int fixed at construction; which
`setup*` method a caller uses is a *separate*, orthogonal choice (only
certain combinations are actually used by convention -- e.g. `ESGame`
always builds its `GenericInfoUI`-style popups with `mode=4` +
`setupMessage`, never `mode=3`):

- `mode` 3, 5, 6 render through the shared highlighted-row renderer
  (`renderItemRows`, visible-selection box behind the current row/group).
- `mode` 4 has its own separate, non-highlighted list renderer
  (`renderPlainList`) -- despite being the mode most naturally called a
  "list", it's the one *without* a selection highlight.
- `setupList` populates a plain scrollable `items[]` (10 visible).
  `setupMessage` word-wraps a body string into `items[]` (11 visible).
  `setupPromptList` (two overloads) sets a word-wrapped prompt
  (`promptLines`, mode 5/6) optionally plus a second footer block
  (`footerLines`, mode 6 only) above a selectable, per-item-word-wrapped
  `items[]` list (9 visible; `itemGroupStart[]` tracks which wrapped
  visual lines belong to the same logical item, so up/down navigation
  moves by item not by line).

Soft-key commands (bottom-left/bottom-right) auto-derived from whichever
of `okCommand`/`selectCommand`/`cancelCommand`/`backCommand` were added,
so screens don't hardcode key positions. Word-wrapping calls through to
`GameCanvas`'s own wrap helper (still `e.a(text,maxWidth,font)`, not
renamed -- see "why e/j aren't renamed yet").

`onEnter`/`onExit` (originally `e()`/`l()`, in that declaration order --
renamed to the opposite order since the *names* had to match their
confirmed behavior in `LoadingScreen`, not their declaration order) are
empty hook points in `Screen` itself.

## `h` -> `LoadingScreen` (extends `Screen`)

Fully renamed, see `../src/LoadingScreen.java`. Reuses `Screen`'s mode
dispatch for two unrelated purposes: mode 2 is the startup splash/
carrier-logo sequence (`onEnter` starts a background `Thread`, `onExit`
stops it), modes 1/8-11 are plain "&lt;action&gt;... Please Wait"
progress bars driven by `percent` (originally `G`, updated externally by
whatever's loading). Two always-empty methods (`unusedHook1`/
`unusedHook2`, originally `c()`/`j()`) are called externally right after
construction (`ESGame.initSplash()`) by their original names but do
nothing -- purpose unconfirmed, preserved as no-ops rather than guessed.

## `i` -> `Dungeon` (one dungeon level's live state + queries)

Fully renamed, see `../src/Dungeon.java`.

- `NAMES[36]` = every non-hub level's display name (confirms the game's
  world structure: hub town "Dawnstar" + 12 zones x 3 levels each --
  "North Creek", "Ice Spike", "Blind Fjord", "Slipneck Fjord",
  "Troll Pace", "Ice Tribe Haven", "Dawnstar Run", "Massacre Caves",
  "Frostheim", "Glacier Run", "Troll Hole", "Ice Council").
- `tiles[width][height]` = the tile bitflag grid: bit0=wall, bit1=monster
  present, bit2=dropped item, bit3=no-spawn/special, bit4=chest,
  bit5=blocked marker (used for the special shop-room tiles), bit6=
  level-edge/transition marker (only meaningful transiently during
  cross-level boundary checks).
- `tier` = a *permuted* difficulty index in [1,36] (from
  `DIFFICULTY_TIER_LOOKUP`), **not** the level number -- this is what
  `Monster`'s type/loot tables and `DungeonGenerator`'s room-count-weight
  table are actually keyed by. Grouped into 4 bands of 9; the bands don't
  obviously line up with anything else observed yet.
- `neighbors[6]` = the level's own `geomin.dat` row: north/east/south/west
  neighbor level ids, then up/down stairway direction codes -- see
  `ASSET_FORMATS.md`.
- `sampleCorridorView`/`sampleSquareView` (originally two overloads of
  `a(...)`) sample tiles **through level boundaries into the neighboring
  `Dungeon`** via `tileAt` when the requested coordinate falls outside
  this level's own grid. These are the methods behind both
  `GameCanvas`'s 3D corridor view and its minimap.
- `tickNearbyMonsters(long now, Player p)` -- the per-tick "scan nearby
  tiles for monsters and run their AI" driver that calls into
  `Monster.tick`/`Monster.chase`.

## `j` -> `Player` (player state, inventory, combat, spellcasting)

By far the largest and most central class; see "why not renamed" below.
Confirmed structure:

- `charin.dat` loader (`loadCharacterData`/static block): race/template
  names `i[]`, gender labels `p[]` (2 entries), attribute names `u[]` (8),
  skill names `ax[]` (14, must match exactly or the loader throws), stat
  labels `ak[]` (used by the character-sheet string builder), and the big
  per-race stat template table `l[races][41]` (base attributes, base
  skills, starting spell-knowledge thresholds).
- Core stats: `E[10]` = level, level-exp, curHP, maxHP, curMagicka,
  maxMagicka, curFatigue(?), maxFatigue(?), and two more slots whose use
  is unconfirmed (`E[8]`/`E[9]`, zeroed on rest, never otherwise touched
  in what's been traced). `n[16]` = 8 attributes as base+bonus pairs.
  `au[14][3]` = skills as rank/bonus/exp-toward-next-rank.
- Inventory: `af[24]`=item-type ids (negative = currently equipped),
  `ae[24]`=packed value/charge per slot, `aq`=slot count, `ag[7]`=equipped
  item-type per equip slot (indexed by `Item.e`, the item's equip-slot
  column).
- Position/world: `x`/`w`=current tile, `aw`=facing (1-4, N/E/S/W per
  `GameCanvas.L[]`), `ao`=current level number. `k`/`j`/`h`/`W` are the
  *pending* target tile/level/facing used mid-move (`move(dir)` computes
  them, `commitMove()` applies them) -- letting a move be validated
  before committing, including the cross-level-boundary math.
  `f`/`d`=previous tile (for the "just arrived here" chest/item
  auto-trigger check).
- Status: `r` = 8-bit active-ailment mask (matches `an[8]`'s named
  debuffs: Frost Limbs, Snow Mirage, Blind, Troll Thirst, Glacier Curse,
  Grievous Harm, Terrified, Winter Worn); `ar`/`O`/`J` = countdown timers
  for 3 of those 8 (bits 3/4/6 specifically -- the other 5 ailments appear
  to be binary/durationless in what's been traced, or use `D[]` instead);
  `D[25]` = generic spell/effect duration timers, `-1`=until cured,
  `-2`=until a condition check (`r(int)`) rather than a countdown.
- `visibleObjects` (originally `al`, a **static** 13-element `Vector`
  shared by the one live `Player`) -- the "what's renderable at each of
  the 13 3D-view object slots this frame" cache, rebuilt by
  `refreshVisibleObjects()` (originally `h()`) every move: does its own
  wall-occlusion pass (reusing `Dungeon`'s tile bits) to decide which
  slots are blocked, then `placeVisibleObject(kind, obj)` (originally
  `a(int,Object)`) is called separately per monster/chest/NPC to drop it
  into the correct slot based on its position relative to the player's
  facing. `GameCanvas` reads this cache directly (`Player.al.elementAt(n)`)
  to paint monster/chest/NPC sprites at the right screen column.
- Combat: `attack(Monster)` (originally `a(d)`) and `castOnMonster`/
  `castOnSelf` (originally `b(int,d)`/`m(int)`) all share the same
  hit-tier lookup (`rollOutcome(atk,def)`, originally the static
  `d(int,int)` -- returns 0=miss..3=crit, via two independent percentile
  rolls) and the same `damage = max(power - defense, 4) * multiplier/100`
  formula. Spell effects are a big switch on spell id (1-25) covering
  buffs (`D[]` duration), direct heals (E[2]/E[4] restore), and
  status-cure.
- `useItem(slot, Monster)` (originally `a(int,d)`) handles the 13
  "gift"/special consumable ids 87-99 by exact id -- confirms `Item.l[]`'s
  flavor text 1:1 (87=Warp to Camp, 88=cure random ailment, 89=full heal
  HP, 90=full heal Magicka, 91=Fatigue+3xMagicka, 92=+1 level-exp,
  93=full HP+Magicka, 94=Increase Harm buff (`C` flag, +25 weapon
  damage), 95=Increase Armor buff (`ac` flag, +15 armor), 96=Safe
  Camping flag (`b` flag, skips camp-interruption roll), 97/98/99=instant-kill
  scrolls gated on monster difficulty <=13/22/29).
- Save format: two serializations from the same `toBytes`/`fromBytes`
  pair, switched by a boolean (`true`=full in-progress save: everything
  including inventory/position/status; `false`=lightweight "character
  summary" with no position/inventory -- likely a high-score/leaderboard
  record, needs confirming against where the `false` path is actually
  called from `ESGame`).

## `k` -> `Shop` (NPC dialogue, shop transactions, quest/rumor tracking)

Fully renamed, see `../src/Shop.java`. Corrected twice from the initial
pass while writing that file -- worth reading the source comments
directly, but summarized here:

- `NAMES[9]` -- the 9 shop/NPC identities: 4 generic hub peddlers (Weapon/
  Heavy Armor/Light Armor/"Jakar's"), then 5 named NPCs (Eustacia,
  Alhavara, Beatrice, Chung, Delacroix). `SHOP_X[9]`/`SHOP_Y[9]` -- world
  positions; indices 0-4 are fixed hub coordinates, 5-8 get overwritten
  at dungeon-generation time by `DungeonGenerator` (they live inside
  levels 3/12/21/30, not the hub).
- `SHOP_STOCK[4][]` (originally field `n`) is each **generic** shop's
  (0-3) sellable item-id list, indexed by a catalog slot -- e.g.
  `SHOP_STOCK[3]` (Jakar's) is the 87-96 "gift"/special-consumable range.
  Misidentified in an earlier pass as "rumor level ids" from the raw
  values alone (87-96 look like level numbers out of context); only
  confirmed once `dialogue()`'s buy branch (`action==14`) was traced.
- `RUMOR_STRING_OFFSET[4][6]` (originally field `m`) maps
  `[Player.ai][revealStep]` to an offset into `dialogue[9]` for that
  step's rumor fragment. `Player.ai` is the hidden "traitor" index (0-3,
  rolled at character creation, `System.out.println("traitor is "...)`
  in the original) -- the rumor system is a mystery-subplot hint feed,
  not generic flavor text.
- `npcstrings.dat` loads 10 groups of dialogue strings (`dialogue[10][]`)
  -- groups 0-3 are per-generic-shop-type flavor lines, 4 is Jakar's
  (main quest-giver/rumor-mill NPC), 5-8 are the 4 named shopkeepers, 9
  is the generic/rumor pool (77 entries, `<TAG>`-templated).
- Per-named-shop quest-turn-in tracking: `rewardsGiven[4]`/
  `questState1[4]`/`questState2[4]`/`interactionCount[4]` for shops 5-8,
  checked against `Item.questFlags` (2-bit-per-shop field, decoded via
  `questFlagsFor(shopId, itemId)`).
- `dialogue(Player, shopId, action, extra)` (originally `a(j,int,int,
  int)`) is the single dispatcher for every shop/NPC interaction: greet,
  buy (`action==14`, catalog slot into `SHOP_STOCK`), sell (`action==15`,
  blocks selling "gift"-category items), and the richer scripted
  branches for shops 5-8 (deliver a specific quest item for a reward).
  The quest-turn-in outcome roll is **`Player`'s own** `c(shopId, action)`
  method (not renamed, still `j.c`), not anything on `Shop` itself --
  caught and fixed after an initial wrong guess wired it to a
  same-shaped-looking table inside `Shop` instead (`shopActionCode`/
  `isValidShopAction`, which turn out to have no confirmed caller at
  all -- kept in `Shop.java` since they're clearly a matched pair, but
  flagged as dead-in-what's-been-traced).

## Open questions (deliberately left unrenamed/undocumented rather than guessed)

- `Monster.i` (boolean), `Monster.c[]` indices beyond 5-8, `Monster.k`
  (8-byte value, likely a timestamp).
- `Player.E[8]`/`E[9]`, `S`, `y[0]`/`y[1]` -- read/written but never
  observed being used meaningfully in what's been traced.
- `Player.ap[9][5]` -- built from a strange formula
  (`a(shopId,slot){ return slot<4 ? ap[shopId+slot+1][slot] : ap[shopId+slot][slot]; }`)
  that reads like a deliberately-obfuscated lookup, not naturally-shaped
  game data. Needs dedicated tracing of every write site before renaming.
- `GameCanvas`'s `OBJECT_DRAW_TABLE`/`OBJECT_EXTRA_FLAGS`/
  `OBJECT_ICON_TABLE` (was `ad[][]`/`G[][]`/`a[][]`) -- structure is
  clear (per-position-code base sprite + up to 4 extra decorations,
  each an (dx,dy,icon) triple, consumed by `paintObjectAtPosition`) but
  the exact per-column meaning of each isn't pinned down. `UNUSED_TABLE`
  (was `m[][]`) is confirmed dead code (declared, never read).
- The exact ambush-system trigger condition (`Player.Q >= 0`) -- when
  does `Q` actually get set to a non -1 value? Not yet located.
- `Shop.UNCONFIRMED_A`/`UNCONFIRMED_B` (originally `k.a[24]`/`k.i[24]`) --
  24 entries each (matches `Player`'s 24 inventory slots), values in
  13-60, no confirmed read site. Plausibly inventory-screen layout
  coordinates given the count, but not verified.
- `Shop.shopActionCode`/`isValidShopAction` (originally `k.b(int,int)`/
  `k.c(int,int)`) -- a clearly-matched pair, but no confirmed caller
  anywhere traced (the quest-turn-in roll in `dialogue()` that looked
  like it should call these actually calls `Player.c(shopId,action)`
  instead -- see the `Shop` section above).
- `Screen.secondaryParam`/`unused1` (originally `g.s`/`g.i`) -- read
  and stored, no confirmed use beyond storage.

## Why `j` (and, previously, `e`) weren't renamed by mechanical means

Vineflower's decompiled source for both classes contains **field names
that collide with the single-letter class names** (`e.a` was a
`byte[][]` field on `GameCanvas` itself, `j.a` is a static `Integer`
field on `Player` itself) which are *also* used elsewhere in the same
file as bare `a.` static references to the `Item` class. This only
type-checks in real Java because the compiled bytecode fully qualifies
every field/method owner -- Vineflower prints it as if unqualified,
which means the source as printed doesn't actually recompile, and more
importantly means a mechanical (regex/sed) rename of the single-letter
tokens is unsafe: the same literal token means two different things
depending on position. Safe renaming requires the same hand-traced,
one-class-at-a-time treatment the small classes got, done field by
field and cross-checked against every call site rather than any
automated substitution.

`GameCanvas` (`e.java`, 1893 lines) got exactly that treatment and is
now `../src/GameCanvas.java`. `Player` (`j.java`, 2668 lines -- by far
the largest and most central class, note the file sizes here were
previously swapped in this document/the roadmap) has not yet -- it's
next. Until then, any file that needs something from `Player` (which
now includes `GameCanvas.java` itself) references it by its **original**
single-letter members rather than inventing renamed-but-nonexistent
APIs -- see each such file's own header comment for specifics.
