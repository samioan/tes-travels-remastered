# Stormhold -- asset formats

All paths below are relative to `extracted/` (regenerate with
`python ../../tools/extract_jar.py stormhold`). Formats below marked
CONFIRMED are read straight off the loader code in `../src/` (see
`CLASS_MAP.md` for which class loads what), not guessed from hexdumps
alone, except where noted.

**No `datfiles.lmp`/`imgfiles.lmp` archive here, unlike dawnstar.** Every
`*in.dat` table below is its own top-level jar resource, read directly via
`ESGame.getResource(name)` (`getClass().getResourceAsStream()` + full
buffering, no archive indirection) -- see `ESGame.java`'s own header
comment. `npcstrings.dat` is loaded the same direct way, via
`Util.openResource` -- there's no dawnstar-style "is it bundled or not"
distinction to make here, since nothing is bundled.

## `itemsin.dat` -- CONFIRMED format

Loaded by `Item.loadItems()`.

```
<u16 categoryCount><UTF x categoryCount>   -- Item.categoryNames[]
<u16 itemCount>
<UTF x itemCount>                          -- Item.name[]
<u8  x itemCount>                          -- Item.category[]
<u8  x itemCount>                          -- Item.subtype[]
<u8  x itemCount>                          -- Item.questFlags[]
<u16 x itemCount>                          -- Item.buyPrice[]
<u16 x itemCount>                          -- Item.sellPrice[]
<u8  x itemCount>                          -- Item.equipSlot[]
```

Same column order as dawnstar's `itemsin.dat` (see
`../../dawnstar/docs/ASSET_FORMATS.md`). Item ids 87-99 are the 13 "gift"/
special-consumable items -- their flavor text isn't in this file at all,
it's hardcoded in `Item.specialEffectText[13][2]` (each entry pre-split
into up to 2 display lines).

## `droppeditemsin.dat` -- CONFIRMED format

Loaded by `Item.loadLootTable()`.

```
<u16 depthRows><u16 cols>
<u8 x depthRows*cols>   -- Item.lootTable[depthRows][cols], row-major
```

## `monstersin.dat` -- CONFIRMED format

Loaded by `Monster.loadTypes()`.

```
<u32 typeCount>
<UTF x typeCount>        -- Monster.typeNames[]
<u8  x typeCount x 17>   -- Monster.typeStats[][], row-major per-type stats
```

Columns pinned down so far (0-indexed, not all 17 -- see `Monster.java`'s
own call sites for the rest): `[2]` a damage-mitigation cap used in attack-
chance calc, `[3]` base attack chance, `[4]` base defense, `[5]` base
attack power, `[11]` inflicted-ailment id, `[14]` starting/max HP (used to
init `currentHp` on spawn), `[15]` loot drop chance, `[16]` loot-table
bonus. `stat(int)` is the masked (`&0xFF`) public-style accessor; a few
call sites (`tick()`/`onDeath()`) read the raw signed byte directly
instead.

## `spellsin.dat` -- CONFIRMED format

Loaded by `Spell.load()`. Identical column layout to dawnstar's own
`spellsin.dat` (see `../../dawnstar/docs/ASSET_FORMATS.md`):

```
<u16 spellCount>
<UTF x spellCount>   -- name
<u8  x spellCount>   -- skill required (index into Player skill table)
<u8  x spellCount>   -- magicka cost
<u8  x spellCount>   -- base power
<u8  x spellCount>   -- school/type id (== 2 marks "offensive")
<u8  x spellCount>   -- duration multiplier
<u8  x spellCount>   -- icon id
<UTF x spellCount>   -- description
```

## `dungnamesin.dat` -- CONFIRMED format

Loaded by `Dungeon.loadNames()`.

```
<UTF x 37 x 2>   -- Dungeon.NAMES[37][2], row-major, no count prefix.
                    Column 0 confirmed as the level's display name
                    (Dungeon.displayNames()); column 1's exact use is
                    unconfirmed (read but not yet traced to a caller).
```

## `geomin.dat` -- CONFIRMED format

Loaded by `ESGame.loadDungeonGeometryRows()`.

```
<s8 x 37 x 6>   -- ESGame.geomRows[37][6], one row per level:
                   [northNeighborId, eastNeighborId, southNeighborId,
                   westNeighborId, stairsUpDir, stairsDownDir]
```

Column order confirmed against `Dungeon.tileAt`'s cross-level lookup
(`x<0`->`neighbors[3]` west, `x>=width`->`neighbors[1]` east, `y<0`->
`neighbors[0]` north, `y>=height`->`neighbors[2]` south) and
`Dungeon.allocateAndGenerate()` (`neighbors[4]`/`[5]` passed into
`generate()` as `stairsUp`/`stairsDown`). `neighborId <= 0` means "no
neighbor in that direction" (same convention as dawnstar). Unlike
dawnstar, stairway placement isn't a fixed per-direction tile coordinate --
`generate()` calls `carveStairwell(direction)` to place each stairwell
procedurally as part of level generation, gated by `isValidDirection()`.

## `charin.dat` -- CONFIRMED format

Loaded by `Player.loadCharacterData()`. Same overall shape as dawnstar's
`charin.dat` (see `../../dawnstar/docs/ASSET_FORMATS.md`), field names
carried over directly since dawnstar's own naming-history fix (class vs.
race vs. gender mixup) already applies here too:

```
<u16 n1><UTF x n1>              -- Player.statLabels[]
<u16 n2><UTF x n2>              -- Player.attributeNames[]
<u16 classes><UTF x classes>    -- Player.classNames[]
<u16 races><UTF x races>        -- Player.raceNames[]
<u16 skills=14><UTF x 14>       -- Player.skillNames[] -- loader throws if
                                    this isn't exactly 14
<u16 x 14>                      -- Player.skillGoverningAttribute[]
<u16 x classes x (13+2*14)>     -- Player.classTemplates[][], per-class
                                    stat template row
```

## `monsterfilenamesin.dat` -- CONFIRMED format

Loaded by `ESGame.loadMonsterImageFileNames()` -- note this one is read
via a raw `getResourceAsStream` + `Util.readAll` + wrap in a
`ByteArrayInputStream`, not `Util.openResource`, but same underlying
per-resource read either way.

```
<UTF x 5 x 7>   -- ESGame.monsterImageFileNames[5][7], row-major, no count
                   prefix -- same "5 buckets x 7 slots, not every slot
                   used" shape as dawnstar's own monsterfilenamesin.dat.
```

## `npcstrings.dat` -- CONFIRMED format

Loaded by `Shop.loadDialogue()`/`Shop.load()`. 8 fixed-size groups (not
10, unlike dawnstar), each `<u32 count><UTF x count>`. Group sizes are
hardcoded in `Shop.GROUP_SIZES = {20, 20, 20, 20, 5, 22, 5, 41}` and
checked against the file's own per-group count in `loadGroup()` -- a
mismatch throws. Group-to-NPC mapping not yet fully traced (Shop.java's
own header comment documents the 7-NPC roster and shop-category split;
cross-reference that against `dialogue[group][...]` call sites for a
future pass).

## `.cus` files -- CONFIRMED format (correction: NOT 3D meshes)

`baglarge.cus`, `bagmid.cus`, `bagsmall.cus`, `chestfarclosed.cus`,
`chestmidclosed.cus`, `chestnearclosed.cus`, `crystalfar.cus`,
`crystalmid.cus`, `crystalnear.cus`, `overseeraxe.cus`,
`overseerbodyf3lc.cus`, `overseerclub.cus`, `overseerfar.cus`,
`overseerhelmet.cus`, `overseermidcf.cus`, `trainer_male_*.cus`,
`trainerfem*.cus`, `undead*.cus`, `wardenbody2f2half.cus`,
`wardenfar.cus`, `wardenheads4bit.cus`, `wardenmid.cus`.

**Corrects this doc's earlier guess** (and the ROADMAP's) that these were
per-bodypart/prop 3D mesh data going by the `far`/`mid`/`near` LOD-style
naming -- that was a naming-pattern guess never checked against a loader.
They're actually 2D sprites: `ESGame.java` calls `RawImage.load("...cus")`
for all of them (`GameCanvas.bagImages`/`crystalImages`/`chestImages`/
`monsterImages`, the last driven by `monsterfilenamesin.dat`'s filename
table), and `RawImage.java` (renamed from `decompiled/g.java`, no dawnstar
analog) is a from-scratch indexed-color raw image decoder, fully traced:

```
<s32 BE width>
<s32 BE height>
<u8   hasTransparency flag (0/non-zero)>
<s16 BE transparentColorValue>            -- meaningful only if hasTransparency
<u8   colorCount>                         -- <= 255
<s16 BE x colorCount>                     -- palette, ARGB4444-ish shorts
<u8  x width*height>                      -- pixel data, 1 byte/pixel palette index
```

Each palette index is resolved to a pixel value at load time (not stored
as raw index): the alpha nibble (`0xF000`) is set on every pixel except
those whose palette index matches `transparentColorValue`'s first match,
which get it cleared -- binary on/off transparency baked directly into the
decoded `short[] pixels` array, no separate alpha mask. `far`/`mid`/`near`
in the filename is still an LOD-style naming convention, just for 2D
sprite detail level (smaller sprite = more distant), not mesh LOD.

## `.png` files

Standard PNG, no RE needed. Top-level jar resources
(`icon3650.png`, `splashtop.png`, `splashbot.png`, and the rest) load
directly via `Image.createImage("/name.png")` -- there's no `imgfiles.lmp`
bundle here either.

## What's actually left for phase 2

With every `*in.dat` table and `.cus` now confirmed, phase 2 is
substantially further along than this doc previously suggested -- most of
that work rode along with phase 1's class-by-class read-through, it just
hadn't been written up here yet. What's genuinely still open:

- `dungnamesin.dat` column 1's exact use (read into `Dungeon.NAMES[][1]`
  but no confirmed caller yet).
- `npcstrings.dat`'s group-to-NPC/purpose mapping (sizes and framing are
  confirmed; *which* group is which NPC's lines isn't).
- `monstersin.dat`'s `typeStats` columns not listed above (0, 1, 6-10,
  12-13) -- trace remaining call sites in `Monster.java`/`GameCanvas.java`.

**Resource-listing check (done this pass):** diffed every non-`.class`
file in `extracted/` (75 total, `META-INF/` excluded) against every
`getResource`/`createImage`/`RawImage.load` literal-string call site in
`src/`. Every `.dat`, `.cus`, and `.png` resource has a confirmed loader;
the only file with no in-code loader is `icon3650.png`, which is the
MIDlet suite icon referenced from the jar's own manifest
(`MIDlet-1: The Elder Scrolls, icon3650.png, ESGame`), not loaded by
`ESGame` itself -- expected, not a gap.
