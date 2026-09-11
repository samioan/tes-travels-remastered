# Dawnstar -- asset formats

All paths below are relative to `extracted/` (regenerate with
`python ../../tools/extract_jar.py dawnstar`). Formats below are confirmed
against the loader code in `decompiled/` (see `CLASS_MAP.md` for which
class loads what) -- not guessed from hexdumps alone, except where noted.

## `datfiles.lmp` -- CONFIRMED format

A simple named-blob archive, read with a **linear scan from the start of
the file on every lookup** (`ESGame.getResource(name)` -- no index is
built, it just walks entries until the name matches). Repeating structure:

```
'-' <name, ASCII, no length prefix, terminated by the next '-'>
'-' <u32 BE: absolute byte offset (from file start) where this entry's data begins>
<u16 BE: unused by this reader -- read but never consulted after the match, meaning unconfirmed>
<data, length = next entry's '-' position minus current position>
```

The leading `-` of the *next* entry is exactly where the data ends, so a
reader can also just treat each entry's data as "everything up to the
next `-<name>-`" without using the u32 field at all -- the u32 is what
`getResource` actually uses to `skip()` there directly rather than
scanning byte-by-byte.

Bundles the same per-file data tables Stormhold ships as loose files:
`charin.dat`, `droppeditemsin.dat`, `geomin.dat`, `itemsin.dat`,
`monstersin.dat`, `monsterfilenamesin.dat`, `spellsin.dat`,
`dungnamesin.dat`(?), `npcstrings.dat`, `helptext.dat` -- each read
through `ESGame.getResource("<name>")` and then parsed with the schema
below, exactly as if it were its own standalone file.

## `imgfiles.lmp` -- CONFIRMED format, DIFFERENT from `datfiles.lmp`

Also a named-blob archive, but header-then-data rather than interleaved:
**every entry's header is packed contiguously at the start of the file**,
followed by all the raw image bytes concatenated in the same order.
`ESGame.createImageFromFile()`:

1. Reads headers (same `'-'name'-'<u32><u16>` shape as `datfiles.lmp`)
   back-to-back until the read position reaches the **first** header's
   u32 value -- i.e. that first u32 marks the end of the header block /
   start of the first image's data, which is how the reader knows where
   to stop reading headers without a separate "header count" field.
2. Builds an in-memory map of `u32 dataStartOffset -> "name-size"` from
   every header read in step 1 (size = the u16 field, actually used
   here -- confirms u16 = byte length of the entry's data).
2. Walks the file sequentially from the end of the header block, and
   each time the current read position matches one of the recorded
   offsets, decodes `size` bytes there as a PNG via
   `Image.createImage(bytes, 0, size)`, keyed by name in a `Hashtable`
   for later `createImage(name)` lookups.

So here the u32 is each image's absolute data-start offset and the u16
*is* its byte length -- both fields are load-bearing, unlike
`datfiles.lmp` where the u16 is unused. Confirm whether `datfiles.lmp`'s
u16 is silently the same "length" field before assuming it's dead: it
just isn't read by this particular consumer.

## `npcstrings.dat` -- CONFIRMED format

Loaded via `Shop`'s `npcstrings.dat` loader (originally `k.a(String)`):
10 fixed-size groups, each `<u32 count><UTF x count>`. Group sizes are
hardcoded in the loader (not stored in the file) as `{3,3,3,3,14,16,16,16,
16,77}` and checked against the file's own per-group count -- a mismatch
throws. See `CLASS_MAP.md` for what each of the 10 groups is.

## `helptext.dat` -- CONFIRMED format

`<u32 count=35><UTF x 35>`. `ESGame.loadHelpStrings()` then hand-picks
fixed indices into 12 title strings and concatenates fixed ranges into 12
body strings (both hardcoded index lists, not derived from the file) --
i.e. the file is a flat pool of 35 fragments, and this build's code
decides how they're grouped into help topics.

## `itemsin.dat` -- CONFIRMED format

```
<u16 categoryCount><UTF x categoryCount>      -- Item.n[]
<u16 itemCount>
<UTF x itemCount>                              -- Item.b[]  name
<u8  x itemCount>                              -- Item.j[]  category id
<u8  x itemCount>                              -- Item.c[]  subtype/icon id
<u8  x itemCount>                              -- Item.m[]  quest-turn-in flags (packed 2 bits x 4 shops)
<u16 x itemCount>                              -- Item.f[]  buy price (Shop.dialogue's buy branch)
<u16 x itemCount>                              -- Item.a[]  sell price (Shop.dialogue's sell branch)
<u8  x itemCount>                              -- Item.e[]  equip slot id
```

## `droppeditemsin.dat` -- CONFIRMED format

```
<u16 depthRows><u16 cols>
<u8 x depthRows*cols>   -- Item.h[depthRows][cols], row-major
```

## `spellsin.dat` -- CONFIRMED format

```
<u16 spellCount>
<UTF x spellCount>   -- name
<u8  x spellCount>   -- skill required (index into Player skill table)
<u8  x spellCount>   -- magicka cost
<u8  x spellCount>   -- base power
<u8  x spellCount>   -- school/type id
<u8  x spellCount>   -- duration multiplier
<u8  x spellCount>   -- icon id
<UTF x spellCount>   -- description
```

## `monstersin.dat` -- CONFIRMED format

```
<u32 monsterTypeCount>
<UTF x monsterTypeCount>          -- name
<u8  x monsterTypeCount x 17>     -- per-type stat row, row-major; see
                                      CLASS_MAP.md for the columns pinned
                                      down so far (not all 17)
```

## `geomin.dat` -- CONFIRMED format

```
<u8 x 37 x 6>   -- one 6-byte row per level: [northId, eastId, southId,
                   westId, stairsUpDir, stairsDownDir], 0 = none
```

`stairsUpDir`/`stairsDownDir` are NOT tile columns (despite the name
suggesting a coordinate) -- they're compass direction codes (1=N, 2=E,
3=S, 4=W, same convention as `neighbors[]` itself and `Player` facing)
saying which edge of the level has an internal stairway, at that
direction's fixed tile position ((17,5)=N, (30,17)=E, (17,30)=S,
(5,17)=W). See `Dungeon.tileAt`/`Monster.isStairwayTile` in `../src/`.

## `charin.dat` -- CONFIRMED format

```
<u16 n1><UTF x n1>              -- Player.ak[]  stat labels
<u16 n2><UTF x n2>              -- Player.u[]   attribute names (8)
<u16 races><UTF x races>        -- Player.i[]   race/template names
<u16 n4><UTF x n4>              -- Player.p[]   gender labels (2)
<u16 skills=14><UTF x 14>       -- Player.ax[]  skill names -- loader
                                    throws if this isn't exactly 14
<u16 x 14>                      -- Player.P[]   per-skill scalar
<u16 x races x (13+2*14)>       -- Player.l[][] per-race stat template
                                    row (attributes, skills, starting-spell
                                    thresholds)
```

## `.png` files

Standard PNG, no RE needed. Top-level jar resources (`icon3650.png`,
`splashtop.png`, `splashbot.png`) load directly via
`Image.createImage("/name.png")`; everything else (`floor3.png`,
`wallsr.png`, `icons.png`, `panel.png`, monster sprites, ...) is bundled
inside `imgfiles.lmp` instead and loaded via `ESGame.createImage(name)`.
