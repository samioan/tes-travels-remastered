# Oblivion -- port roadmap (Phase 3)

The port is C++17, CMake + Ninja + MSVC (`port/build.bat`), software rendered
through raw Win32/GDI, like the sibling ports. Game data is **not** in the repo:
the port reads the jar contents from `oblivion/extracted/` (produce it with
`tools/extract_jar.py`); by default it looks in `../../extracted` relative to
`port/build/`, or pass `--assets DIR`.

Because the level content lives in the `.scr`/`.cml`/`.jtm` files, the port is
an **engine reimplementation that loads the original resources directly**: each
C++ module mirrors a renamed Java class in `oblivion/src/` (`Game`,
`ScriptInterpreter`, `ActorSystem`, ...), and each loader mirrors a
`tools/parse_*.py` parser.

Virtual screen: 176x208, 0x00RRGGBB (the Nokia 3650 canvas; the original asks the
Canvas for its size, so this is a port decision).

## Done

- **M1 -- asset loaders** (`port/src/assets/`): `AssetRoot` (jar-root style
  file access), PNG decoding (vendored `stb_image`, `ImageCache`), `.jtm`
  (`ParseJtm`), `.cml` (`ParseCml`/`SpriteSet`, mirrors `SpriteRenderer.load`
  incl. `findGroup`/`advanceFrame`/`setFrame`), `.scr` (`ParseScr`: the eleven
  record tables + bytecode, and an instruction decoder). Smoke test
  `asset_smoke` parses every extracted file and checks the counts
  (32 `.scr`, 17 `.jtm`, 21 `.cml`, 0 failures) -- same numbers as
  `tools/parse_*.py --check`.
- **M2 -- sprite drawing** (`render/sprite_renderer`, `graphics/backbuffer.h`):
  `SpriteRenderer.draw` semantics -- static images drawn whole at (x+dx, y+dy);
  animation frames are windows into their sheet, optionally mirrored. (Porting
  this exposed a wrong expression in `src/SpriteRenderer.java`'s draw path,
  now fixed: the y offset is `y - offsetY + dy`.)
- **M3 -- level viewer** (`world/level_view`, `main.cpp`): loads a level `.scr`,
  finds its `LOAD_MAP`, and draws the visual tile layers through the game
  isometric projection (`((x-y)>>3, (x+y)>>4)` of the 128-unit cell corner,
  minus half a tile width). Arrow keys pan, PageUp/PageDown switch levels.
  `--dump out.ppm` renders one frame headless. Confirmed by eye on level 1:
  walls, floors and torches line up, so the projection, the `.jtm` layout and
  the `.cml` frame windows are right.

## Next

- **M4 -- actors and the collision layer:** `Actor`/`ActorSystem` movement
  (three-point footprint vs `collision`), the player, monster sprites
  (`oh_*.cml` animation groups: `animStateOffset[state] + facing`), depth
  sorting with the object overlay layer (`sortCell`).
- **M5 -- script interpreter:** port `ScriptInterpreter.step` (call stack of PCs,
  waits, key hooks, walk cutscenes) on top of the decoder in `assets/scr`, plus
  the trigger layers (`SET_TRIGGER`) and level loading (`LOAD_LEVEL`,
  `END_LEVEL`).
- **M6 -- game loop and states:** `Game.run`/`setState`, input (softkeys,
  keypad mapping `mapKey`), the menu/help/loading screens, message line,
  dialogue box and `DialogueScreen` menus (inventory, shop).
- **M7 -- combat/AI/leveling:** the rest of `ActorSystem`, `ProjectileManager`
  and the procedural dungeon generator (`Game.generateDungeon`).
- **M8 -- save/load, fonts, audio, packaging** (the original stores one
  RecordStore, "ESO"; see `Game.saveGame`). Audio: none found in the jar so far.

Open questions carried over from the reverse-engineering docs: `table6`,
`pairTable`, a few opcode operands and `Game` state 4/9 meanings -- rendering
and running real levels should settle them.
