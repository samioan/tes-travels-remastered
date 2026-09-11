# Port Roadmap

Tracks `port/`, the actual PC port -- as opposed to [`ROADMAP.md`](ROADMAP.md),
which tracks the reverse-engineering work that feeds it (now fully done: all
13 classes renamed and compile-checked in [`../src/`](../src/)). Follows the
sibling `shadowkey-decomp` project's precedent (its own `docs/PORT_ROADMAP.md`):
milestone by milestone, each one landing a real, runnable/testable slice
rather than a big-bang rewrite, verified with a smoke-test executable per
milestone rather than just read-through.

## Decisions carried through every milestone

- **Behavioral reimplementation, not byte-exact recompilation.** `../src/`'s
  renamed Java is the reference for *what* every system does; the port is
  ordinary modern C++ that reproduces that behavior, not a mechanical
  transliteration of MIDP-era Java idioms (`Hashtable`, `DataInputStream`,
  MIDP `Canvas`/`Displayable`) into C++ syntax.
- **C++17, CMake + Ninja, MSVC**, matching `shadowkey-decomp`'s toolchain
  and this repo's existing `port/` scaffold.
- **Raw Win32 + GDI (`StretchDIBits`), no SDL2/D3D/GL** -- the original is a
  software-rendered MIDP `Canvas`, so a Win32 window blitting a manually
  computed backbuffer reproduces that architecture directly with zero
  external dependencies, same rationale as `shadowkey-decomp`.
- **176x208 virtual canvas.** Unlike `shadowkey-decomp` (a fixed-hardware
  N-Gage title, confirmed 176x208 from `GRAPHICS_FORMAT.md`), Dawnstar's
  `GameCanvas` reads `getWidth()`/`getHeight()` at runtime and lays out
  around whatever it gets -- MIDP `Canvas` is resolution-agnostic by
  design, and no single confirmed hardware target is documented in
  `CLASS_MAP.md`. **176x208 is a port decision, not a confirmed original
  constant** -- picked because `icon3650.png` (the MIDlet's own suite icon)
  names the Nokia 3650, whose display is 176x208, and because
  `splashtop.png`/`splashbot.png` (152x48 / 152x30) fit comfortably inside
  that width with plausible side margins. Revisit if a real target device's
  screenshot or spec surfaces and contradicts it -- nothing downstream
  should assume this is load-bearing precision the way Shadowkey's 176x208
  is.
- **Assets stay out of the repo.** The port reads `datfiles.lmp`/
  `imgfiles.lmp`/`npcstrings.dat`/the `.png` files from a configurable root
  path at runtime (defaulting to `../extracted/`, this project's existing
  `tools/extract_jar.py` output) -- nothing copyrighted is copied into git,
  same as `shadowkey-decomp`.
- **No scripting engine to port.** Dawnstar has no SimKin-equivalent --
  `ESGame`/`GameCanvas`/`Player`/`Shop` *are* the game logic, directly, in
  Java. So there's no M2-style "embed an interpreter" milestone here; the
  port's job is porting those classes' logic straight into C++ systems.

## Milestones done

- [x] **M0 -- scaffold** (`1944dea`, initial project setup). CMake/MSVC/Ninja
      project proves the toolchain: opens a blank Win32 window. No tick
      loop, no backbuffer, no game logic.

- [x] **M1 -- real tick loop + backbuffer + present** (this session). An
      actual `GameClock` running the real 250ms/4Hz cadence
      (`GameCanvas.run()`'s "steady 250ms tick via `Thread.sleep`", see
      `../src/GameCanvas.java` around line 1248), a 176x208 `Backbuffer`,
      and `StretchDIBits` presentation scaled to the window's client area --
      structurally identical to `shadowkey-decomp`'s M0/M1. Still no game
      logic: proves the loop/presentation architecture only. Verified by
      building and launching `dawnstar_port.exe` (stays up, presents a
      solid-color frame every tick).
- [x] **M2 -- asset foundations** (this session). `BinaryReader`
      (big-endian primitives + `readUTF`-style length-prefixed strings,
      matching `Util.java`/`ESGame.java`'s `DataInputStream` usage) and
      `DatArchive` (the `datfiles.lmp` linear-scan-every-lookup reader,
      `docs/ASSET_FORMATS.md`, faithfully including the *lack* of caching --
      these lookups only happen a handful of times at load, so there's no
      reason to diverge from the original just to be "more efficient"), then
      the first two real data loaders on top of it: `ItemDatabase`
      (`itemsin.dat`+`droppeditemsin.dat`, `../src/Item.java`) and
      `SpellDatabase` (`spellsin.dat`, `../src/Spell.java`). Verified by
      `asset_smoke.exe` against the real `extracted/datfiles.lmp`: 101 items
      across 15 categories, a 43-row loot table, and 25 spells, all with
      sane-looking names/prices/stats (`Hatchet`/`Ice Axe`/`Battle Axe`,
      `Frenzy`/`Shield`/`Deft Security`).

- [x] **M3 -- monster type database + dungeon geometry** (this session).
      `MonsterDatabase` (`monstersin.dat`, `../src/Monster.java`'s static
      type table -- count/names/17-byte stat rows; deliberately just the
      per-*type* database, not per-instance Monster spawn/AI/combat, which
      is gameplay logic for a later milestone) and `DungeonGeometry`
      (`geomin.dat`'s 37 six-byte rows, `../src/DungeonGenerator.java`'s
      `loadGeomRows()`/`../src/Dungeon.java`'s `neighbors[]`/
      `stairsUpDir`/`stairsDownDir`). Verified by
      `monster_dungeon_smoke.exe` against the real archive: 42 monster
      types with sane names/stats (`Sickly Bandit`/`Bandit`/`Dark Bandit`),
      and 37 geometry rows whose connectivity is topologically sane (level
      1, the hub town, connects N/E/S/W to levels 2/11/20/29 with no
      stairs; level 2 has real up/down stairs to 3/2).

      Real bug caught building this: `geomin.dat`'s bytes are **signed**
      (`Dungeon.java` declares `neighbors` as `byte[]`, and `Dungeon.tileAt`
      treats *any* value `<= 0` as "no connection", not just literal `0`)
      -- an initial unsigned read printed nonsense like `stairsUp=255`
      instead of the real sentinel `-1`. Fixed in
      `dungeon_geometry.h`/`.cpp` and tightened `docs/ASSET_FORMATS.md`'s
      "0 = none" note to match. `MonsterDatabase`'s stat bytes stay
      unsigned on purpose, though -- `Monster.java`'s own `stat(column)`
      accessor explicitly does `& 0xFF`, i.e. the *intended* reading really
      is unsigned there, unlike `geomin.dat`'s neighbor ids.

- [x] **M4 -- character data (class/race/skill templates)** (this
      session). `CharacterData` (`charin.dat`, `../src/Player.java`'s
      `loadCharacterData()` -- stat labels, attribute names, class names,
      race names, skill names, per-skill governing attribute, and the big
      per-class stat template table). Verified by
      `character_data_smoke.exe` against the real archive: 7 classes
      (Barbarian/Battlemage/.../Spellsword), 6 races (Redguard/Nord/
      Breton/High Elf/Wood Elf/Dark Elf), 14 skills, all with sane labels.

      **Real bug found and fixed in `../src/Player.java` itself** (not
      just the port), the most significant correction of this milestone:
      the "class" and "race" arrays were swapped by the original Phase-1
      rename pass, before `charin.dat` had ever actually been loaded
      against real data. What was named `raceNames`/`raceIndex` turned
      out to be the character CLASS list (`ESGame.java`'s own
      character-creation screen titles this exact list "Select a Class:"
      -- see `this.newGameUI.setupPromptList("New Game", "Select a
      Class:", Player.classNames)`), and what was named `genderNames`/
      `genderIndex` turned out to be the actual RACE list -- a 6-entry
      "gender" array with no "Male"/"Female" string anywhere in the
      corpus should have been the tell. Renamed throughout `Player.java`
      (`raceIndex`->`classIndex`, `raceNames`->`classNames`,
      `raceTemplates`->`classTemplates`, `raceCount`->`classCount`,
      `applyRaceTemplate`->`applyClassTemplate`,
      `raceMagickaFactor`->`classMagickaFactor`,
      `raceUnknownPair`->`classUnknownPair`, and the old `genderIndex`/
      `genderNames`->`raceIndex`/`raceNames`) and `ESGame.java`'s two call
      sites, re-verified with a full standalone `dawnstar/src/` compile
      (zero errors, same 6 pre-existing warnings as every prior run).
      `docs/ASSET_FORMATS.md` and `docs/CLASS_MAP.md` updated to match.
      As a nice independent confirmation: `classTemplates[class][1]`
      (the race each class comes with -- race isn't separately
      player-selectable) now resolves to sensible pairings like Knight/
      Redguard and Nightblade/Wood Elf instead of nonsense.

- [x] **M5 -- bit-exact `java.util.Random`** (this session). `JavaRandom`
      (`port/src/util/java_random.h`, header-only): the 48-bit LCG
      (`next(32)`) plus `LingoRandomInt`/`RandomIntBelow`, matching
      `ESGame.lingoRandomInt`/`ESGame.nextInt(int)` (every real caller in
      `../src/` -- `Item.java`, `DungeonGenerator.java`, `Monster.java`,
      `ESGame.java` -- only ever uses the no-arg `nextInt()` plus its own
      `Math.abs(x % bound)` wrapping, never `Random`'s more involved
      `nextInt(bound)`, so that's the only surface ported). Foundational
      rather than optional: `DungeonGenerator.java` seeds one of these
      **deterministically per level** (`new Random(level.number * 8000)`),
      so a level's room layout/monster spawns/loot are a pure function of
      the level number in the original game -- an approximate-quality
      PRNG would generate a *different*, wrong dungeon for every level.

      Verified the strongest way available: captured real
      `java.util.Random(seed).nextInt()` sequences from an actual JVM (5
      seeds, including `16000` = level 2's real generator seed) and
      checked `JavaRandom` reproduces them **bit-for-bit** --
      `java_random_smoke.exe` matched on the first try. Also ported
      `Math.abs(int)`'s `Integer.MIN_VALUE` quirk explicitly (`JavaAbs`)
      rather than calling `std::abs`, which is undefined behavior for
      `INT_MIN` in C++ where Java's version is well-defined (returns it
      unchanged) -- preserves a real, if obscure, original-game edge case
      instead of silently changing behavior there.

## Milestones next

- [ ] **M6 and beyond (not yet planned in detail):** `imgfiles.lmp`,
      `DungeonGenerator`'s procedural level generator itself (now
      unblocked by M5's `JavaRandom` -- already fully understood,
      `../src/DungeonGenerator.java`, but a real room-carving/loot-
      placement system to port), the rest of `Player`'s runtime instance
      state (stats/inventory/equipment/combat) and the save format,
      `npcstrings.dat`/`helptext.dat`/`Shop` dialogue, the first-person
      corridor renderer (`GameCanvas`'s `CORRIDOR_WALL_TABLE`), and finally
      `ESGame`'s own screen-wiring loop tying it all together. Each gets
      its own milestone once the shape of "how much fits in one slice" is
      clearer -- following `shadowkey-decomp`'s pattern of not
      over-planning milestones far in advance of actually reaching them.
