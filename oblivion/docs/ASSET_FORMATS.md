# Oblivion -- asset formats

All paths below are relative to `extracted/` (regenerate with
`python ../../tools/extract_jar.py oblivion`).

| File(s) | Format | Notes |
|---|---|---|
| `*.png` (`1.png`, `2.png`, `3.png`, `5.png`, `c1-c9.png`, `icon.png`, `magic.png`, `main.png`, `ts_lvl*.png`, `ts5.png`) | PNG | Standard, no RE needed. `ts_lvl*` naming suggests tilesets, one per level. |
| `lang_0.txt` .. `lang_12.txt` | Plain text, understood | `<index> <English string>\|` per line (e.g. `1 Language\|`, `2 New Game\|`). 13 files, likely 13 UI-string tables (menus, not full dialogue) or one per localization slot with only `lang_0` populated in this build -- check the others' contents to tell which. |
| `start.txt`, `copywrite.txt`, `eso.ver` | Plain text | Not yet read; `eso.ver` is a single version string (`2.424`) in this build. |
| `startup.scr`, `startup2.scr`, `l01_1.scr`, `l01_1b.scr`, `l01_1c.scr`, `l01_1r.scr`, `l02_2.scr`, `l02_2_1.scr`, ... (60+ total, `l01`..`l14` = one set per level/area) | **Binary, unconfirmed.** Superscape script bytecode. | No ASCII structure in a raw hexdump -- e.g. `startup.scr` starts `01 01 0b fe 1e 01 00 01 01 f0 6c 02 00 03 00 04 05 09 00 32 1f 1e ...`. Likely opcode-stream bytecode for Superscape's scripting layer. This is the actual decompilation target -- see `ROADMAP.md` phase 2. |
| `startup.cml`, `startup2.cml`, `oh_menu.cml`, `oh_pc.cml`, `oh_deadroth.cml`, `oh_dremora.cml`, `oh_ghost.cml`, `oh_liches.cml`, `oh_magic.cml`, `oh_ogre.cml`, `oh_scamp.cml`, `l01_l1.cml` (etc., one per level, `l01`..`l14`) | **Binary, unconfirmed.** Superscape scene/markup data. | `oh_menu.cml` starts `00 01 08 2f 74 73 35 2e 70 6e 67 00 ...` -- note `2f 74 73 35 2e 70 6e 67` is the ASCII path `/ts5.png` inline, so `.cml` at least embeds asset path references in a mostly-binary stream; not pure bytecode like `.scr` appears to be. `oh_*` files read as per-monster-type scenes (deadroth, dremora, ghost, liches, ogre, scamp = Oblivion bestiary), `l01`-`l14` as per-level world scenes. |
| `l01_1.jtm`, `l01_r.jtm`, `l02_1.jtm`, ..., `l13_clrl.jtm`, `l14_1.jtm` (one or two per level) | **Binary, unconfirmed.** | Dense byte stream, many `ff` bytes that look like escape/delimiter markers rather than payload (`2c 3b ff 7e 00 ff 24 00 ff 05 01 ff 27 00 01 ff ...`). Name suggests "joint/terrain map" or similar -- guess only, not confirmed. |

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
