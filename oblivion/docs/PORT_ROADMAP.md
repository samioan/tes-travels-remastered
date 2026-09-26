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

- **M4 -- actors and collision** (`world/actor`, `LevelView::Draw`): the
  movement/animation/draw subset of `Actor`/`ActorSystem` -- `setPosition`
  (three-point footprint via `isoToWorld`), `updateCells`/`updateSortCell`,
  `isBlocked`/`footBlocked` (collision values 1 = solid, 2..5 = diagonal
  half-cells), `moveDir` (50 ms step gate, undo when blocked), `setMoveTarget`
  auto-walk, `setAnimState`, `update` (animation + walking), `draw` (shadow
  group -56, body group `facing + animStateOffset[state]`, corpse -55).
  `LevelView::Draw` now draws the overlay layer with actors interleaved by
  `sortCell`, as `Game.paint` does. `main` spawns a test player (`oh_pc.cml`,
  speed 42) you walk with the arrow keys; camera follows. `actor_smoke` walks
  a player into walls on all 12 levels and checks it is never blocked. Health
  bar, floating text and status icons wait for M7.

- **M5 -- script interpreter** (`script/interpreter`, `world/game_world`,
  `assets/lang`): `ScriptInterpreter` mirrors `ScriptInterpreter.step` on the
  decoded instructions -- call-stack of PCs, one instruction per tick, `WAIT`,
  `WAIT_ACTORS_STOP`, walk cutscenes, `WAIT_KEY`, key hooks, dialogue/shop
  blocking, monster scaling/restore. It talks to the game through `ScriptHost`;
  `World` (the playing field of `Game`) implements it: level load/reset,
  actor slots + spawn from monster-type rows, trigger/zone layers and the
  player's enter/leave/zone scripts, camera (`updateCamera` dead zone),
  dialogue, messages, text-screen placeholders, lang packs (`Strings`).
  Facts learned while running real levels:
  - `startup.scr` carries the big item/monster/class tables; level `.scr`
    files only define a few rows and are **merged** over the previous tables
    (`Scr::tables.has`), and the interpreter's `strings[]` persists across
    loads (overwritten from index 0). The boot is `startup.scr` (splash
    `END_LEVEL` images) -> `startup2.scr` (`LOAD_LANG 0`, `OPEN_MENU`); "New
    Game" is `loadLevel("/l01_1.scr")`.
  - `OPEN_SHOP_MENU` parks the game in state 3; the script resumes after the
    shop closes, so the `*_cr` between-level scripts need M6.
  - Text screens (states 9/10/4) end by scrolling off; a timer stands in.
  `script_smoke` boots and runs all 30 level scripts headless: 22 reach the
  playing state, 5 park on a menu/shop screen, 3 are patch scripts (`*r`).
  `oblivion_port.exe` now boots, loads `--level`, and plays it (dialogue and
  messages show in the window title until fonts land); `--dump --run-ms N`
  renders after N simulated ms. Ops still skipped (recorded, not crashing):
  `GIVE_ITEM`, `PLACE_ITEM`, `GENERATE_DUNGEON`, `SPAWN_*PROJECTILE`, menus.

## Next

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
