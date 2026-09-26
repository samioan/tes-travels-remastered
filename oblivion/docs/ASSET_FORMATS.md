# Oblivion -- asset formats

All paths below are relative to `extracted/` (regenerate with
`python ../../tools/extract_jar.py oblivion`).

| File(s) | Format | Notes |
|---|---|---|
| `*.png` (`1.png`, `2.png`, `3.png`, `5.png`, `c1-c9.png`, `icon.png`, `magic.png`, `main.png`, `ts_lvl*.png`, `ts5.png`) | PNG | Standard, no RE needed. `ts_lvl*` naming suggests tilesets, one per level. |
| `lang_0.txt` .. `lang_12.txt` | Plain text, understood | `<index> <English string>\|` per line (e.g. `1 Language\|`, `2 New Game\|`). 13 files, likely 13 UI-string tables (menus, not full dialogue) or one per localization slot with only `lang_0` populated in this build -- check the others' contents to tell which. |
| `start.txt`, `copywrite.txt`, `eso.ver` | Plain text | Not yet read; `eso.ver` is a single version string (`2.424`) in this build. |
| `startup.scr`, `startup2.scr`, `l01_1.scr`, `l01_1b.scr`, ... (32 files; `l01`..`l12` plus `end_15`) | **Binary, fully parsed** by [`tools/parse_scr.py`](../tools/parse_scr.py) (`--check` parses all 32 with no unknown opcode or operand-width mismatch). Interpreted by `ScriptInterpreter` (`src/`, was `e`). | Layout: `n`; `n x (script id, 16-bit absolute offset)`; then `30, tag, <field-id/value pairs>, 31` table records (tags 0-10, see `CLASS_MAP.md`'s `e` section for what each table is); then `00 01 01`; then the scripts back to back. Each script is a straight run of opcodes (no jumps -- `CALL id` runs another script) ending in `RETURN`, followed by the next script's marker `00 <id> 01`. Opcode and operand table: [`SCR_OPCODES.md`](SCR_OPCODES.md). `tools/parse_scr.py FILE.scr` prints tables and a disassembly (`--json` for tooling). Strings are either literal (byte length) or `0xF000 | id` into the localized string table. |
| `startup.cml`, `startup2.cml`, `oh_menu.cml`, `oh_pc.cml`, `oh_deadroth.cml`, `oh_dremora.cml`, `oh_ghost.cml`, `oh_liches.cml`, `oh_magic.cml`, `oh_ogre.cml`, `oh_scamp.cml`, `l01_l1.cml` (etc., one per level, `l01`..`l14`) | **Binary, confirmed -- record format**, image-group tree. Parsed by `SpriteRenderer.load(String)` (`../src/SpriteRenderer.java`) and standalone by [`tools/parse_cml.py`](../tools/parse_cml.py) (`--check`: all 21 files consume exactly to EOF). | Loaded whole into `b.b` (shared 6144-byte static buffer, via `b.a(String)`). Byte 0 = length of a path-prefix string, prepended to any following name that doesn't start with `/` (explains the raw `/ts5.png`/etc. paths you'll see inline -- most are *not* prefixed). Then a stream of variable-length records, one per named image/animation group, until the buffer is exhausted: `nameId(1) nameLen(1) name(nameLen) <bit-flagged record, see below> colorKeyCount(1) [colorKeyCount * (3-byte RGB, 3-byte RGB)] frameCount(1)`. If `frameCount==0` the record is a single static image (position/size + dims come straight off the image); if `frameCount>0`, that many *frames* follow, each `<bit-flagged record> subframeCount(1) [subframeCount * <bit-flagged record>]` -- i.e. every image-group is really an animation with 1+ frames, each frame 1+ subframes, chained into a linked list (see `SpriteFrame.java`'s doc comment for the two chain fields). The **bit-flagged record** shared by every level: a 16-bit presence mask (MSB-first) gates up to 10 fields in fixed order -- `[0]`=1 byte unsigned (group/frame id), `[1]`/`[2]`=2-byte unsigned big-endian (x/y offset), `[3]`/`[4]`=1 byte unsigned (width/height), `[5..9]`=1 byte signed each (draw-offset x, draw-offset y, a byte reused as either a "loop vs. freeze" flag on frames or a mirror-ish value depending on context -- still not 100% pinned down, see `SpriteFrame.java`, plus a "use MIDP Sprite" flag and one more). Full field-by-field evidence in `../src/SpriteRenderer.java`'s `readRecord`/`unpackFrame`. Colour-key pairs are parsed into `int[]` pairs but their *consumer* wasn't located in this pass (image loading (`Image.createImage(String)`) ignores them; likely dead in this build, or used by a transparency path not yet found). `oh_*` files read as per-monster-type scenes (deadroth, dremora, ghost, liches, ogre, scamp = Oblivion bestiary, plus `oh_pc`=player and `oh_magic`=spell-effect sprites, all confirmed by cross-reference from `decompiled/h.java`/`i.java`), `l01`-`l14` as per-level world scenes. |
| `l01_1.jtm`, `l01_r.jtm`, `l02_1.jtm`, ..., `l13_clrl.jtm`, `l14_1.jtm` (17 files) | **Binary, fully parsed** by [`tools/parse_jtm.py`](../tools/parse_jtm.py) (`--check`: all 17 consume exactly to EOF). Loaded by `Game.loadMap` (`b(String)`). | Byte 0 = width, byte 1 = height. Then layers back to back until EOF, each `width*height` cells, **run-length encoded and stored row by row (y outer, x inner)**: `0xFF count value` = `count` copies of `value` (a run may cross rows; count 0/1 = one cell), any other byte is one literal cell. The engine indexes cells as `layer[x*height + y]`. **Layer 0 is the collision layer** (`Game.collision`: 0 open, 1 solid, 2-5 half-tile diagonals -- confirmed: `l14_1`'s layer 0 is a walled room); every later layer is a visual tile layer (`Game.layers`, 2-4 of them; the last is the object/item overlay). The trigger layers (`enter`/`leave`/`zone`) are not in the file -- scripts fill them with `SET_TRIGGER`. Stubs like `l01_r.jtm` are 1x1 with 2 layers. (An earlier version of this row said column-major with no collision layer; that was wrong.) |

## How to make progress on these

There is no public Superscape format documentation to lean on (same
situation shadowkey-decomp was in for E32Image, except here the *engine*
recovers cleanly via Vineflower even though the *content* formats don't).
The only way in is reading how `decompiled/b.java` (or whichever class it
delegates to) parses these streams -- find the `InputStream`/
`DataInputStream` read calls that consume `.scr`/`.cml`/`.jtm` bytes and
work the field layout out from there, the same way shadowkey-decomp's
`docs/WORLD_MODEL.md` / `docs/ZONE_FORMAT.md` were derived from decompiler
output rather than guesswork. Once a format is confirmed, add a
`tools/parse_*.py` for it.

**Update (phase 1 survey pass, see `CLASS_MAP.md`):** all three formats
above went from "unconfirmed" to at least structurally understood by
reading `decompiled/b.java`, `e.java`, and `g.java`, without writing any
`tools/parse_*.py` yet -- that remains the concrete next step for each
(`.scr` is done -- `tools/parse_scr.py`; `.jtm` -- `tools/parse_jtm.py`; `.cml` -- `tools/parse_cml.py`)
now that the field layouts are written up per-row above.
