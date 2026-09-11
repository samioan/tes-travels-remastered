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

- [x] **M6 -- procedural dungeon generation** (this session).
      `DungeonGenerator` (`port/src/world/dungeon_generator.h`/`.cpp`):
      the full room-carving/corridor-connection/monster-placement/
      chest-placement pipeline from `../src/DungeonGenerator.java`, plus
      `Dungeon.java`'s `MONSTER_TABLE`/`DIFFICULTY_TIER_LOOKUP`/`initTier`
      and two `Item.java` loot-roll methods added onto `ItemDatabase`
      (`RollLoot`/`RandomGiftItemOfSubtype`, M2's data-only struct's first
      added logic methods, mirroring how M4 added `Player` predicate
      methods once something needed them).

      **No bit-exact JVM ground truth was possible for this one** --
      unlike M2/M3/M4 (verified against real *data*) and M5 (verified
      against a real JVM's own `java.util.Random` output), attempting to
      actually *run* anything that touches `ESGame` (even just
      `Item.load()`, which calls `ESGame.getResource()`) against the MIDP
      stub jars throws `java.lang.Error: API Stub has been used` --
      confirmed by trying it this session. `ESGame extends
      RegisteredMIDlet`, whose static initializer constructs three real
      `javax.microedition.lcdui.Command` objects
      (`../decompiled/ngame/midlet/RegisteredMIDlet.java`), and these
      stub jars are compile-only: they exist so `javac` can resolve MIDP
      symbols, and are designed to throw the instant any of that surface
      is actually *executed*, not just linked against. So this was
      verified the way Phase 1's own hand-trace rename work was before
      any of this port existed: careful line-by-line transcription plus
      strong internal self-consistency checks (`dungeon_generator_smoke.exe`,
      against the hub level plus 2/3/12/15/21/30/37 -- all 4 special-shop
      levels and a spread of ordinary ones): exactly 15 rooms/monster
      spawns, exactly 5 chests (exactly 1 marked "guaranteed gift"), every
      monster/chest position walkable/in-bounds with the right tile bit
      set, every chest's rolled item id valid (including catching an early
      *test* bug -- not a generator bug -- around `Item.rollLoot`'s 2-byte
      "extended id" packing, where the real id to validate is always the
      low byte, not the packed low+high value), special-room marking only
      on levels 3/12/21/30, no undocumented tile bits, and tier values
      matching `DIFFICULTY_TIER_LOOKUP` by hand for every tested level.

      Two things deliberately simplified rather than guessed at, both
      called out in `dungeon_generator.h`/`.cpp`'s comments: the 4 special
      "shopkeeper room" levels report their position as plain output
      fields instead of writing into `Shop.SHOP_X`/`SHOP_Y[5..8]` (no
      `Shop` class ported yet), and chest `spawnId` is a per-level-local
      counter (1-5) rather than the original's single counter shared
      cumulatively across all 37 levels generated in one pass (that
      counter is pure bookkeeping with no RNG involved, so it can't affect
      generation correctness -- it only matters once a real save format
      needs it, later).

- [x] **M7 -- real image archive (`imgfiles.lmp`)** (this session).
      `ImgArchive` (`port/src/assets/img_archive.h`/`.cpp`), ported from
      `ESGame.createImageFromFile()`: the header-then-data layout (every
      entry's `'-'name'-'<u32 offset><u16 size>` header packed
      contiguously up front, ending exactly at the first entry's own
      recorded offset, followed by every entry's raw bytes in the same
      order), decoded once at construction into a name -> raw-PNG-bytes
      map (no PNG decoding here, same as nowhere else in this port yet).

      Verified two ways, stronger than M2-M4's "sane-looking values"
      standard: every byte blob's first 8 bytes are a real PNG magic
      number, *and* -- since real image bytes can actually be looked at,
      unlike a data table -- a few (`panel.png`, `icons.png`,
      `wallsr.png`) were written out and viewed directly: real,
      recognizable game art (a UI panel texture, an icon strip with a
      spell/rock/question-mark/sleep-Z/fire icon row, and stone
      wall/gate corridor textures), not just plausible-looking bytes.
      `img_archive_smoke.exe` checks 7 known names
      (`this.createImage("...")` call sites in `../src/ESGame.java`)
      round-trip to valid, correctly-sized PNGs: 43 images total.

- [x] **M8 -- NPC dialogue + help text** (this session). `ShopDialogue`
      (`port/src/assets/shop_dialogue.h`/`.cpp`, `npcstrings.dat` --
      `Shop.java`'s `loadDialogue()`/`load()`/`loadGroup()`: 10 fixed-size
      groups, sizes `{3,3,3,3,14,16,16,16,16,77}` checked against the
      file's own per-group count) and `HelpText`
      (`port/src/assets/help_text.h`/`.cpp`, `helptext.dat` --
      `ESGame.loadHelpStrings()`: a flat pool of 35 fragments the code
      groups into 12 help topics via two hardcoded index lists, ported
      verbatim). Verified against the real files via `dialogue_smoke.exe`:
      all 10 dialogue groups at their exact expected sizes with sane,
      in-character NPC lines (a weapon peddler's "Welcome! Please peruse
      our manufactured mayhem makers.", a named shopkeeper's "You are from
      Dawnstar? That place is dead to me."), and all 12 help topics with
      correct, sensible titles (Goal/Combat/Experience/Items/Gift
      Items/Maps/Monsters/Movement/Spells/Trainers/Camp/Website) and
      non-trivial concatenated bodies.

      **Real doc bug found and fixed**: `docs/ASSET_FORMATS.md` had
      claimed `npcstrings.dat` was bundled inside `datfiles.lmp` like
      every other `*in.dat` table. Reading `Shop.java` directly for this
      milestone showed `Shop.load()` actually opens it via
      `Util.openResource("/npcstrings.dat")` (a direct top-level jar
      resource stream), never `ESGame.getResource()` -- and
      `../extracted/npcstrings.dat` independently confirms it, sitting
      there as its own top-level file rather than nested inside
      `datfiles.lmp`. `ShopDialogue::Load()` takes a plain file path
      rather than a `DatArchive&` because of this, unlike every other
      loader in `port/src/assets/`.

## Milestones next

- [ ] **M9 and beyond (not yet planned in detail):** the rest of
      `Player`'s runtime instance state (stats/inventory/equipment/
      combat) and the save format, the first-person corridor renderer
      (`GameCanvas`'s `CORRIDOR_WALL_TABLE`, unblocked by M7's real
      wall/floor/gate textures), and finally `ESGame`'s own screen-wiring
      loop tying it all together. Each gets its own milestone once the
      shape of "how much fits in one slice" is clearer -- following
      `shadowkey-decomp`'s pattern of not over-planning milestones far in
      advance of actually reaching them.
