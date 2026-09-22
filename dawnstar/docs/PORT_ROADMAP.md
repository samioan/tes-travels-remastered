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

- [x] **M9 -- corridor wall-segment selection logic** (this session).
      `DungeonView` (`port/src/world/dungeon_view.h`, `Dungeon.java`'s
      runtime `tileAt()`/`sampleCorridorView()` -- as opposed to
      `DungeonGenerator`, which only builds a level's *initial* state) and
      `CorridorRenderPlan` (`port/src/render/corridor_render_plan.h`/
      `.cpp`, `GameCanvas.java`'s `CORRIDOR_WALL_TABLE`-driven
      `paintCorridorWalls()`/`drawWallSegment()`/`resolveWallFrame()`) --
      the wall-segment *selection* logic: for a given player position/
      facing/level, which of the 3 wall textures (plain/ice/gate) to draw
      at which screen column and pixel offset, including the
      forward/mirrored-scan dedup toggle that was one of `CLASS_MAP.md`'s
      longstanding "exact geometry not fully traced" notes.

      **Deliberately stops short of drawing actual pixels** -- this port
      has no PNG decoder yet (M7's `ImgArchive` only extracts raw,
      still-PNG-encoded byte blobs), so `CorridorRenderPlan::Plan()`
      returns the same "draw this texture at this x,y" decisions
      `GameCanvas.drawWallSegment()` would have made, as data, for a
      later milestone's real pixel renderer to consume directly. Floor/
      ceiling and object/monster/chest sprites are out of scope too
      (both need `Player`, not ported yet).

      Same verification story as M6 (no bit-exact JVM ground truth
      possible -- this sits downstream of `Item.load()`/`Monster.load()`
      too): self-consistency (at most one segment per corridor step) plus
      hand-checked geometric plausibility, this time against two
      positions in a real M6-generated level -- a small room's door tile
      (facings look similar to each other, as expected: small 2-5-tile
      rooms don't have much room to differentiate) and, more tellingly,
      that level's own stairway-corridor tile (a straight, 1-wide,
      multi-tile-deep passage in exactly one compass direction per
      `DungeonGeometry`): facing down the long corridor produces a
      visibly different draw pattern than facing back into the open
      interior or across the 1-wide passage into an immediate wall --
      confirming the renderer is actually direction-sensitive, not
      coincidentally producing the same output regardless of facing.
      `corridor_plan_smoke.exe`.

- [x] **M10 -- real rendered frames** (this session). Vendored
      `stb_image.h` (`port/third_party/stb/`, see its own
      `PROVENANCE.md` -- pinned commit, dual MIT/public-domain, same
      rationale `shadowkey-decomp` vendored `puff`/`stb_vorbis` for: a
      small well-known library for a solved problem rather than a
      from-scratch PNG/DEFLATE decoder). `DecodedImage`
      (`port/src/assets/decoded_image.h`/`.cpp`) decodes M7's raw PNG
      bytes to RGBA8; `Backbuffer::Blit()` (`port/src/graphics/
      backbuffer.h`) composites a `DecodedImage` onto the RGB565
      backbuffer with binary (on/off) transparency -- matching real MIDP
      hardware this old, which doesn't alpha-blend PNGs either -- and an
      optional column clip range; `FrameRenderer`
      (`port/src/render/frame_renderer.h`/`.cpp`) ties it together with
      M9's `CorridorRenderPlan`: black background, the floor loop
      (ice/plain by dungeon number, default no-`Player`-ailment path
      only), then every wall/gate segment blitted with its own 18px clip
      window (`GameCanvas.drawWallSegment()`'s `g.setClip(x, 0, 18,
      screenHeight)` -- M9 had missed that this clip position is a
      *different* value than the image's draw origin; fixed there as
      part of this milestone, see `WallDrawCall::clipX`).

      **First real rendered frames of the actual game** -- verified by
      looking at them, same spirit as M7: a real M6-generated level's
      stairway corridor renders as a correctly-perspective receding ice
      corridor (walls narrowing toward a vanishing point, tiled floor);
      the hub town (level 1) renders with the *plain* brick/cobblestone
      textures instead of ice, confirming `dungeonNumber`-based texture
      selection; and a small room's door tile renders as an
      immediate close-up wall, consistent with M9's own finding that
      small rooms don't leave much room to see down. `frame_render_smoke.exe`
      writes an uncompressed BMP (no encoder dependency needed) for
      direct viewing.

- [x] **M11 -- player character creation** (this session). `PlayerState`
      (`port/src/player/player_state.h`) and `PlayerCreation`
      (`port/src/player/player_creation.h`/`.cpp`) port the real
      character-creation pipeline out of `Player.java`:
      `applyClassTemplate()` (attributes/skills/gold/traitor-index roll/
      starting known-spell mask) fused with `resetState(false)`'s
      character-creation path (hub-town spawn position, then
      `grantStartingItems()` -> `addInventoryItem()`/`equipItem()`) --
      the real game calls these from two separate UI steps
      (class-select, then confirm), fused here since this port doesn't
      model the UI screens between them, only the resulting character
      data. Also added `ItemDatabase::IsEquippable()`/`EquipSlotOf()`
      (M2's data-only struct's second round of added logic methods, same
      pattern as M6's `RollLoot`/`RandomGiftItemOfSubtype`).

      Verified against real `CharacterData`/`ItemDatabase` for all 7
      classes via `player_creation_smoke.exe`: gold always 50, hub-town
      spawn position, attributes/skills copied exactly from
      `classTemplates`, starting items granted and auto-equipped into
      the correct equip slots. The strongest confirmation, though, is
      thematic coherence across *all* the systems this milestone and M4
      touch together: Sorcerer/High Elf starts with 3 known spells and
      light armor (the classic pure-caster pairing), Knight/Redguard
      starts with heavy armor and no spells, Battlemage/Breton is a
      balanced hybrid with real starting spells -- exactly the
      archetypes their names promise, which only happens if the M4
      class/race fix, `CharacterData`'s template columns, and this
      milestone's equip/spell-mask logic are all simultaneously correct.

- [x] **M12 -- player save format** (this session). `PlayerSave`
      (`port/src/player/player_save.h`/`.cpp`) ports `Player.java`'s
      `toBytes(true)`/`fromBytes(data, true)` -- the "full" in-progress
      save format actually used by save/load (the game's the lightweight
      `toBytes(false)`/`fromBytes(data, false)` "character summary"
      format, most likely a high-score/leaderboard record per
      `Player.java`'s own comment, is deferred: round-tripping it
      meaningfully needs the `applyClassTemplate`+`resetState`
      reconstruction path `fromBytes(..., false)` leans on, which this
      milestone doesn't otherwise need). `PlayerState` grew the rest of
      the full save format's fields (camp bookmark, ailment/effect
      timers, event flags, combat/buff scratch state). Added
      `assets/binary_writer.h`, a `BinaryReader`-mirroring big-endian
      writer (same ASCII-only `writeUTF` simplification `BinaryReader`
      already documents).

      **A real original-game bug, found and faithfully preserved:**
      `toBytes(true)`'s `traitorIndex`/`traitorSuspicionCount` packing
      line reads `packed = (byte)(this.traitorIndex << 2 +
      this.traitorSuspicionCount)`. Java's `+` binds tighter than `<<`,
      so this parses as `traitorIndex << (2 + traitorSuspicionCount)`,
      *not* the `(traitorIndex << 2) + traitorSuspicionCount` the
      read-back side (`traitorSuspicionCount = packed % 4; traitorIndex =
      (packed >> 2) % 4`) clearly assumes -- a "2 bits each" scheme the
      write side doesn't actually implement. This is a genuine
      operator-precedence bug in the original decompiled source (not a
      decompiler/rename artifact), and it's already lossy on real
      hardware: e.g. `(traitorIndex=2, traitorSuspicionCount=1)` writes
      packed byte 16, which reads back as `(traitorIndex=0,
      traitorSuspicionCount=0)`. Ported as-is rather than "fixed" --
      `player_save.cpp`'s `ToBytes`/`FromBytes` reproduce the exact same
      shift/mask arithmetic, including the `specialEncounterResolved`/
      `roamingSpecialMonsterPresent` flag bits (16/32) landing in the same
      byte and being able to collide with the shift's output.

      Verified via `player_save_smoke.exe`: builds a real character (M11)
      for each of the 7 classes, fills in every full-save-only field with
      varied non-default values, round-trips through `ToBytes`/`FromBytes`,
      and asserts exact field-by-field equality for everything *except*
      the traitor-packing group -- which is instead checked against an
      independently hand-traced copy of the exact (buggy) Java formula,
      including the concrete `(2,1)->(0,0)` corruption example above and a
      boundary case (`traitorSuspicionCount==0`) where the bug happens not
      to manifest. All checks passed. Tier-1-strength verification (like
      M5's `java.util.Random`) despite `ESGame`'s stub-jar execution block
      still applying -- the packing formula is a small, fully
      hand-computable integer expression, not something that needs a JVM
      to trace.

- [x] **M13 -- player movement** (this session). `PlayerMovement`
      (`port/src/player/player_movement.h`/`.cpp`) ports `Player.java`'s
      `computeMoveTarget()`/`commitMove()`/`move()`/`isWalkable()`:
      forward/backward stepping, in-place turning (with the fallthrough
      `case 1: delta=1; case 2: ...` Java switch faithfully reproduced),
      strafing (turn/step/turn-back), and the cross-level boundary
      stitching that walks off one `GeneratedLevel`'s edge onto its
      `geomin.dat` neighbor -- including the coordinate recentering
      needed when crossing between the 19x19 hub town and a 35x35
      standard level. `levels` (mirroring `ESGame.dungeons[]`) is passed
      in rather than owned, matching M11/M12's style of taking real data
      by reference. `GeneratedLevel` grew a `visited` flag and
      `PlayerState` grew `prevTileX/Y`, `corridorView`, and
      `suppressStrafeAdjust` -- all directly touched by this milestone's
      ported code (see their doc comments for why each is there before
      any consumer of them exists).

      Three real behavioral gaps deferred, each documented in
      `player_movement.h`'s class comment rather than silently dropped:
      dropped-item auto-loot on arrival, the instant-lethal-tile
      camp-and-return-to-town trigger, and the "remove roaming gehen on
      level change" cleanup -- all three need runtime systems
      (dropped-item registry, camp/town-return, live monster instances)
      this port doesn't have yet. Also noted: `computeMoveTarget`'s own
      neighbor-level lookup is unguarded against a `<=0` neighbor in the
      original (would throw in Java, relying on the implicit level-design
      invariant that no walkable tile ever borders a "no neighbor" edge)
      -- this port guards it defensively since C++ has no equivalent
      safety net for an out-of-bounds vector index.

      Verified via `player_movement_smoke.exe` against the real generated
      37-level world (M6's `DungeonGenerator` + M9's `DungeonView`): all 8
      documented tile-bit combinations checked against `isWalkable()`
      individually; turning wraps 1↔4 correctly and never moves or costs
      fatigue; fatigue<=0 blocks every direction including turns; a
      successful forward step costs exactly 1×`fatigueCostMultiplier()`
      fatigue and records `prevTileX/Y`, a blocked one costs nothing;
      strafing always restores the original facing (turns are never
      wall-blocked); and, strongest of all, **all 4 of the hub town's
      real border exits** (found by scanning its actual tile data rather
      than assumed) were walked through into their real
      `geomin.dat`-declared neighbor levels (2, 11, 20, 29) with the
      hub↔standard-level recentering math checked against an
      independently hand-traced copy of `computeMoveTarget`'s formula,
      matching exactly. All checks passed.

- [x] **M14 -- player combat/skill stat math** (this session).
      `PlayerCombatStats` (`port/src/player/player_combat_stats.h`/`.cpp`)
      ports the entirely self-contained half of `Player.java`'s combat
      system -- `skillValue()`/`skillBonus()`/`weaponSkillValue()`/
      `baseEvasion()`/`bestArmorSkillIndex()`/`activeWeaponSkillIndex()`/
      `offhandSkillIndex()`/`attackPower()`/`attackAccuracy()`/
      `weaponDamage()`/`armorValue()`/`isEffectActive()`/`clearEffect()`/
      `hasAilment()`/`gainSkillExp()`, plus the static `rollOutcome()`
      hit-tier roll -- deliberately *not* `attack()` itself, which needs a
      live `Monster` target (`stat()`/`takeDamage()`/`store()`) this port
      has no runtime counterpart for yet (only M3/M6's static
      `MonsterDatabase`/generation-time spawn list). Splitting the combat
      system exactly along this line let a real, useful slice land now
      instead of waiting on a full Monster port. `PlayerState` grew
      `levelUpPending`/`starFrostBonusActive` (touched by this milestone's
      code, their real producers -- `grantStarFrostItem()`,
      `ESGame`'s level-up UI -- not ported yet).

      Preserved verbatim rather than tidied: `armorValue()` only sums
      equip slots 1-5's magnitude column (weighted 4/2/2/1/1, `/10`) --
      slot 6 is never read, exactly as the Java source has it.

      Verified via `player_combat_stats_smoke.exe` with no JVM ground
      truth available (same reason as M6/M9/M11/M13): `isEffectActive`'s
      three duration conventions (-1/-2/>0) and `hasAilment`'s bit test
      checked directly; `gainSkillExp` checked against a hand-computed
      multi-rank-up-in-one-call case (+25 exp triggering 3 rank-ups, an
      attribute flag, and a level-up in a single call); `rollOutcome`
      checked bit-exact (build on M5's proven `JavaRandom`) against 4
      chance combinations chosen to force each of its 4 outcome branches
      regardless of the actual dice, plus 2 that compare against an
      independently-drawn hand trace of the same roll; and all the
      equipment-dependent stats checked for all 7 classes' real starting
      gear (M11) against an independently-transcribed copy of the
      category-to-skill dispatch table, plus targeted checks that each
      effect/buff bonus (harm/armor buffs, effects 1/2/14/17) actually
      applies on top of a real character's base stats. All checks passed.

- [x] **M15 -- Monster runtime + combat resolution** (this session).
      `MonsterState`/`MonsterRuntime` (`port/src/monster/`) port
      `Monster.java` in full except `tick()`/`attack()`'s Monster-side
      half; a new `CombatResolution` (`port/src/combat/`) holds those two
      -- `Player.attack(Monster)` and `Monster.tick(Player, now)` -- since
      each needs both `Player` and `Monster` state. Splitting it this way
      keeps `player/` and `monster/` siblings with no dependency on each
      other (mirroring M14's split of combat stats away from `attack()`
      itself); only `combat/` depends on both, and only `dawnstar_world`
      (needed for `DungeonView`/tile data) is a shared dependency
      underneath. Ported: spawn/type-selection, the packed 28-byte
      `toBytes`/`fromBytes` record *and* the separate unpacked
      `readFrom`/`writeTo` stream format (two genuinely different
      serializations in the source), movement + stairway-tile detection,
      chase AI (with its own 1-in-5 move cadence, distinct from
      `tick()`'s 800ms wind-up cadence), and death-loot rolling.

      **A real, deliberately-preserved distinction found while
      transcribing this class:** `Monster.java`'s own methods read its
      static `typeStats` table two different ways depending on which
      method. `stat(column)` (the public accessor `Player.attack()`
      calls via `target.stat(...)`) masks every byte `& 0xFF`. But
      `tick()` and `onDeath()` bypass `stat()` entirely and read
      `typeStats[...]` directly as a private field of their own class --
      getting the *raw signed byte* instead. These give different `int`
      values whenever a column's stored byte is >=128 as an unsigned
      reading. `MonsterRuntime::Stat()` (masked) and `::RawStat()` (raw)
      keep both conventions distinct and route every call site to the
      one the real method actually used -- `CombatResolution::PlayerAttack`
      uses `Stat()`, `::MonsterTick`/`MonsterRuntime::OnDeath` use
      `RawStat()`.

      **A second real finding, caught by testing every generated level
      rather than just one:** `isStairwayTile()`'s cascading if-chain
      checks stairway directions in a fixed priority order (N, then S,
      then W, then E) using `stairsUpDir`/`stairsDownDir` together. A
      level whose up-stairway and down-stairway point in two *different*
      directions can only ever register the higher-priority one at this
      method -- e.g. an up-stair East + down-stair West level's East
      coordinate (30,17) never reads as a stairway, because the chain's
      West check (`stairsDownDir==4`) is reached and returns first. This
      is a real property of the original nested-if, not a bug introduced
      here; ported byte-for-byte rather than "fixed" into an
      order-independent check.

      Verified via `monster_combat_smoke.exe` against the real 37-level
      generated world (no JVM ground truth, same reason as M6/M9/M11/
      M13/M14): `PickMonsterType` checked against an independently
      hand-traced tier/bucket roll; `Spawn`/`IsUndead`/`TakeDamage`
      checked directly; both save formats round-tripped at sign-bit
      boundaries (a negative `spawnId`, `hp=0xFF`, a negative
      `timestamp`); `Move` checked for tile-bit mutation and blocking;
      `IsStairwayTile` cross-checked for *all 37 levels x all 4 canonical
      coordinates* against an independently-transcribed copy of the
      cascading logic (this is what caught the priority-order finding
      above); `Chase`'s 1-in-5 cadence checked exactly over 10 calls;
      `OnDeath` checked for a valid, correctly-flagged guaranteed drop;
      and `PlayerAttack`/`MonsterTick` integration-tested against a real
      character and a real monster over enough attempts that damage,
      fatigue cost, and HP floor-clamping were all actually exercised.
      All checks passed.

      Simplifications carried over from earlier milestones (no live
      per-level monster registry, so `store()`/`Dungeon.
      populateRandomMonsters()`/dropped-item storage are all no-ops here)
      are documented in `monster_runtime.h`'s and `combat_resolution.h`'s
      class comments rather than repeated here.

- [x] **M16 -- Player spellcasting** (this session). `PlayerSpellcasting`
      (`port/src/player/player_spellcasting.h`) ports `Player.java`'s
      self-targeted spell system: `spellSkillIndexFor`, `castOnSelf`,
      `activeAilmentCount`/`cureRandomAilment`, `canLearnSpell`/
      `learnSpellFromScroll`, and the known-spell bookkeeping
      (`knownSpellsSummary`/`nthKnownSpellId`/`cycleSelectedSpell`/
      `spellTooltip`). `castOnMonster` -- the offensive counterpart --
      went into `CombatResolution` instead (`combat_resolution.h`/`.cpp`)
      alongside `PlayerAttack`/`MonsterTick`, for the same reason those
      two live there: it needs both `Player` and `Monster` state (and it
      can itself call back into `PlayerAttack`, spell 14), so it can't
      live in `player/` or `monster/` without creating a library cycle.
      This is the third time this exact three-module split (self-
      contained player logic / self-contained monster logic / a small
      shared "needs both" module) has paid for itself, following M14's
      original split and M15's monster-side mirror of it.

      Along the way, `castOnSelf`'s case-6 branch (learning "cure
      poison" from a granted scroll) needed `addInventoryItem`/
      `equipItem`, which previously existed only as small private copies
      inside `player_creation.cpp` (for `grantStartingItems`). Rather
      than duplicate them a second time, they were pulled out into a new
      `PlayerInventory` (`player/player_inventory.h`/`.cpp` --
      `AddItem`/`Equip`/`UnequipSlot`/`RemoveSlot`/`IsEquipped`/
      `CanEquipOrUnequip`/`EquipLastPickedUpItem`), and `player_creation.cpp`
      was refactored to call it too. All of `player_creation_smoke`/
      `player_save_smoke`/`player_movement_smoke`/`player_combat_stats_smoke`/
      `monster_combat_smoke` were rerun afterward to confirm the refactor
      changed nothing observable.

      **Two real findings, both preserved rather than "fixed":**
      1. `castOnSelf` reads `Spell.byId(spellId).magickaCost` into a
         local Java calls `power`, and `castOnMonster` reads
         `Spell.byId(spellId).power` into a local it calls `school` --
         and then **never uses that local again** in either method.
         `castOnSelf` actually spends Magicka scaled by `school`
         (`Spell.power`), and `castOnMonster` spends it scaled by
         `power` (`Spell.magickaCost`) -- the two methods use the *other*
         field than the one their same-named local variable's read
         suggests. Confirmed by grepping both method bodies for the
         local's name after its declaration line. Not ported (same
         treatment as M15's unused `Monster.attack()` `type` read),
         documented inline at both call sites.
      2. Because of that, and because `castOnSelf`/`castOnMonster` only
         ever clamp Magicka with `Math.max(coreStats[4], 0)` and never
         `Math.min(...,maxMagicka)`, a spell whose scaling field's stored
         byte is >=128 (a negative `int8_t` reading) makes that spend
         *negative* -- casting it actually **refunds** Magicka above the
         normal maximum, with no cap. This is real, observable behavior
         of the original game (confirmed via `m16_spellcasting_smoke`'s
         integration loop, which found the initial test's "Magicka never
         exceeds max" assumption false against real spell data and had to
         be corrected to the real invariant, "Magicka never goes
         negative" -- same discipline as M15's `isStairwayTile` catch).

      Verified via `spellcasting_smoke.exe` (no JVM ground truth, same
      reason as M6/M9/M11/M13/M14/M15) against real `SpellDatabase`/
      `CharacterData`/`ItemDatabase`/`MonsterDatabase` data and real
      characters/monsters: `SpellSkillIndexFor`'s bucket boundaries
      hand-traced; `ActiveAilmentCount`/`CureRandomAilment` hand-traced
      including the RNG-free `active==1` special case and a probe-RNG
      cross-check for `active>1`; `CanLearnSpell`/`LearnSpellFromScroll`
      checked against a real category-12 scroll item (category gate,
      known-spell gate, skill-point gate, and slot-compaction on
      consumption); `KnownSpellsSummary`/`NthKnownSpellId`/
      `CycleSelectedSpell`/`SpellTooltip` hand-traced against a
      hand-picked `knownSpellsMask` (ordering, the "R: " prefix, wrap-
      around, and the no-spells-known/invalid-selection cases); one
      spell each for `CastOnSelf`/`CastOnMonster` fully hand-derived
      (chance clamps, a same-seeded `RollOutcome` probe, and the
      resulting Magicka/HP/heal-or-damage arithmetic) plus an
      integration loop over every non-offensive/offensive spell id on a
      real character (and, for `CastOnMonster`, a real spawned monster),
      confirming spell 14 genuinely re-enters `PlayerAttack`. All checks
      passed.

      Simplifications: `Player.dropInventoryItem()`/`addGold()` weren't
      needed by anything here and remain unported (`PlayerInventory`'s
      class comment says why); `castOnMonster`/`castOnSelf` still skip
      every `target.store()` call, same as M15/M14's `PlayerAttack`.

- [x] **M17 -- the lightweight "character summary" save format** (this
      session). `PlayerSave::ToBytesSummary`/`FromBytesSummary`
      (`player/player_save.h`/`.cpp`) port `Player.java`'s
      `toBytes(false)`/`fromBytes(data,false)`, the format M12
      deliberately deferred because round-tripping it meaningfully
      requires re-deriving the class-template reconstruction path
      (`applyClassTemplate`+`resetState`) `fromBytes(...,false)` leans
      on. That's no longer a blocker: `fromBytes(...,false)` needs
      exactly what M11's `PlayerCreation::CreateCharacter` already does
      (roll a fresh `traitorIndex`, re-grant starting items, place the
      character at the hub spawn) before layering the serialized
      summary fields on top, so `FromBytesSummary` just calls it
      directly. The one new piece of logic needed was
      `computeStartingSpellMask()`, previously a small private helper
      inside `player_creation.cpp`'s anonymous namespace -- exposed as
      `PlayerCreation::ComputeStartingSpellMask` (a public static
      method) instead of duplicating it a third time (following the
      same reuse-over-duplication call M16 made for
      `addInventoryItem`/`equipItem` -> `PlayerInventory`), and
      `CreateCharacter`'s own call site now goes through the same public
      method.

      **A real, surprising finding, preserved rather than "fixed":**
      `toBytes(false)` computes its serialized known-spell mask by
      calling `this.computeStartingSpellMask()` -- on the LIVE character
      being saved, not a scratch one. That method has a genuine side
      effect (it sets `selectedSpellId` to the class's first starting
      spell, the same mechanism M16's `PlayerSpellcasting` doc comment
      already flagged). So producing a nominally read-only "character
      summary" actually **mutates the live character**: it silently
      resets whatever spell the player currently has selected back to
      their class's default -- but only for a class that actually has a
      starting spell; a pure-warrior class (Barbarian/Knight/Rogue, all
      three confirmed via `summary_save_smoke.exe`) never reaches that
      assignment at all, since `computeStartingSpellMask`'s `first` flag
      is only ever consumed inside the `if (bit != -1 && threshold > 0)`
      branch. The serialized mask itself is *also* just the class's
      starting spells, silently discarding anything actually learned
      since character creation -- there is no way to save which spells a
      character has learned in this format at all.

      Verified via `summary_save_smoke.exe` (no JVM ground truth, same
      reason as M6/M9/M11/M13/M14/M15/M16): for all 7 classes, a real
      character (M11's `PlayerCreation`) is advanced away from a fresh
      spawn (moved off the hub tile, extra gold/skill rank/inventory, a
      hand-picked non-starting `selectedSpellId`, and a spell learned
      well outside the class's starting set) and round-tripped through
      the summary format with a deliberately *different* RNG seed on
      load than on creation. Checked: every field the format actually
      carries round-trips exactly (name/classIndex/raceIndex/gold/
      attributes/classMagickaFactor/classUnknownPair/skills);
      `normalizeForSummary`'s exact behavior (current HP/Magicka/Fatigue
      normalized to max, `coreStats[8]` zeroed, `coreStats[9]` left
      untouched -- checked by setting it to a hand-picked nonzero value
      beforehand); the known-spell mask on reload matches an
      independently-recomputed (against a scratch `PlayerState`, so it
      couldn't be perturbed by the save call's own mutation)
      class-starting mask, not the live one with the extra spell;
      inventory/position/`traitorIndex` are all confirmed freshly
      regenerated rather than carried over; and the `selectedSpellId`
      mutation-on-save is checked to actually happen for spellcasting
      classes and to NOT happen for the three classes with no starting
      spells at all. All checks passed.

- [x] **M18 -- the "gift"/special-consumable `useItem` switch and the
      camp system** (this session). `CombatResolution::UseItem`
      (`combat/combat_resolution.h`/`.cpp`) ports `Player.java`'s
      `useItem(slot, target)`: the 87-99 "gift" item switch (warp/mark
      camp, ailment cure, HP/Magicka/Fatigue/level-exp restoratives,
      harm/armor/safe-camping buffs, and three instant-kill scrolls
      gated on a monster's difficulty stats). Items 97-99 need a live
      `Monster` target, so -- like `PlayerAttack`/`MonsterTick`/
      `CastOnMonster` before it -- this is the fourth entry point that
      needs both `Player` and `Monster`, and it lives in `combat/` for
      the same reason those three do. `PlayerInventory::CanUseItem`
      (the `canUseItem()` menu gate, Player-only) went into
      `player_inventory.h` instead, alongside its sibling gating
      methods.

      Item 87 ("Warp to Camp") needed the camp/hub-positioning system
      `Player.java` keeps as its own small cluster of methods
      (`hasCampMark`/`resetToHubPosition`/`markCampAndReturnToTown`/
      `warpToCampMark`) -- these went into `player/player_movement.h`
      (position management is already that module's job, and
      `resetToHubPosition` reuses its existing private
      `RefreshCorridorView` directly, no new dependency needed).
      SIMPLIFIED the same way `PlayerMovement`'s existing methods
      already are: the roaming-special-monster cleanup and rendering-
      refresh calls (chest/NPC visibility) are no-ops, for the same
      reasons documented in that header's class comment.

      Nothing here needed fixing or reinterpreting -- every branch
      ported directly matches the source, including two habits worth
      noting even though they aren't bugs: item 91's Fatigue restore
      (`+3*maxMagicka`) has no upper clamp against `maxFatigue` (unlike
      most of this codebase's other stat changes), and item 92's
      level-exp point is a bare `coreStats[1]++` with no level-up check
      of its own (unlike `PlayerCombatStats::GainSkillExp`, which does
      check).

      Verified via `use_item_smoke.exe` (no JVM ground truth, same
      reason as M6/M9/M11/M13/M14/M15/M16/M17) against the real 37-level
      generated world, real `ItemDatabase`/`MonsterDatabase` data, a real
      character (M11's `PlayerCreation`), and real spawned monsters
      (M15's `MonsterRuntime::Spawn`): confirmed all of ids 87-99 are
      really category-13 in the real item table; every camp-system
      method checked for its exact position/flag effects, including both
      branches of item 87 (mark vs. warp); each of items 88-96 checked
      individually (including 96's real, singular "not consumed"
      exception, and the unclamped/no-level-up-check quirks above);
      items 97/98/99 checked against monster types found by directly
      querying real `MonsterRuntime::Stat` data for one type within and
      one type beyond the instant-kill threshold, plus an explicit
      null-target call (matching the original's own `Monster target`
      nullability) to confirm it's still handled safely and the item is
      still consumed. All checks passed.

- [x] **M19 -- `DungeonView`'s real cross-level `tileAt` stitching**
      (this session). Since M9, `DungeonView` had stood in for
      `Dungeon.java`'s `tileAt()`/`isWalkable()`/`sampleCorridorView()`
      with a documented simplification: no multi-level "world" object
      existed to stitch into, so any out-of-bounds query just returned
      1 (wall) unconditionally, even at a level edge that really opens
      into a neighboring level. Every real caller (`PlayerMovement::
      RefreshCorridorView`, `MonsterRuntime::Move`) already held the
      full `std::vector<GeneratedLevel>& levels`, so this milestone
      only had to change how a `DungeonView` is constructed --
      `DungeonView(levels, levelIndex)` instead of
      `DungeonView(oneLevel)` -- and give `TileAt()` `Dungeon.java`'s
      real neighbor-following logic: crossing into `neighborNorth/East/
      South/West`, the hub town's (19x19) width/height-mismatch
      recentering when either side of the crossing is level 1, and the
      "edge marker" (64) sentinel for the exact boundary tile just
      before a size-mismatched neighbor. `IsWalkable()` itself needed
      no change -- the original never stitches across levels for that
      one, only for `tileAt()`/`sampleCorridorView()`.

      `Dungeon.java`'s own `populated` flag (a lazy per-level generation
      marker `tileAt()` also consults) still has no equivalent here --
      same already-documented `player_movement.h` simplification that
      this port always generates every level upfront, so every entry in
      `levels` is always "populated".

      Verified via the new `dungeon_view_smoke.exe` (no JVM ground
      truth, same reason as M6/M9/M11/M13-M18) against the real
      37-level generated world: 64 real standard-to-standard (35x35)
      border crossings checked against an independent mirrored-tile
      hand-derivation (no recentering expected there), plus all 4 real
      hub-town border crossings checked against an independently
      hand-derived recentering-formula expectation, including the edge-
      marker sentinel. All checks passed. The three existing rendering/
      movement smoke tests that construct single-level `DungeonView`s
      (M9/M10/M13) were updated for the new constructor signature; M9
      and M10's single-level scenarios needed their synthetic level's
      neighbor ids zeroed out first (they were never meant to exercise
      cross-level stitching, and would otherwise index into a `levels`
      vector that doesn't actually hold their real neighbors).

- [x] **M20 -- wire the real pipeline into the actual windowed
      `dawnstar_port.exe`** (this session). Until now `main.cpp` was
      still M1's placeholder: a solid-color `Backbuffer::Fill` and an
      empty tick, with every real milestone (M2-M19) only ever exercised
      through console smoke tests. This milestone made the windowed exe
      itself do something: load the real extracted assets, build the
      real 37-level world (M6's `DungeonGenerator`), create a real
      class-0 character (M11's `PlayerCreation` -- there's no character-
      creation UI yet, so the class is a fixed stand-in), and on every
      `GameClock` tick (M1's real 250ms cadence) read arrow-key state
      (`GetAsyncKeyState`, polled once per tick so a held key advances
      once per tick rather than as fast as the message pump spins) into
      `PlayerMovement::Move` (M13), then render the player's live
      position/facing through `FrameRenderer::Render` (M9/M10) via
      `DungeonView` (M19, so the corridor view now correctly opens into
      neighboring levels at real level boundaries too) and present it
      through the existing GDI `Window::Present`. `CMakeLists.txt`
      links `dawnstar_port` against `dawnstar_player`/`dawnstar_render`
      (previously linked against neither -- only `user32`/`gdi32`).

      No new gameplay logic was ported here -- this is pure wiring of
      already-verified pieces. Verified by actually running
      `dawnstar_port.exe` and screen-capturing its real window: it
      shows the real first-person corridor view (real floor/wall/gate
      textures from the hub town's own layout), and holding the up
      arrow for 1.5s (six ticks) visibly changes thousands of sampled
      pixels versus the initial frame -- confirming movement input
      actually reaches `PlayerMovement::Move` and a new frame is
      rendered from the result, not just a static placeholder.

- [x] **M21 -- the Health/Magicka/Fatigue HUD status-bar overlay**
      (this session). `HudRenderer::PaintStatusBars`
      (`render/hud_renderer.h`, a header-only class) ports
      `GameCanvas.paintStatusBars()`'s 3-bar meter exactly: a shared
      40x7 yellow background per stat, then a red/green/blue fill
      inside sized to `effectiveStat(index) * 38 / max`. That needed
      `Player.java`'s `effectiveStat()` itself, which wasn't ported yet
      -- added as `PlayerCombatStats::EffectiveStat` (`player/
      player_combat_stats.h`/`.cpp`), reusing the already-verified
      `SkillValue`/`IsEffectActive` primitives from M14. `Backbuffer`
      (`graphics/backbuffer.h`) gained a small `FillRect` primitive
      alongside its existing `SetPixel`/`Blit`, the natural counterpart
      of `Graphics.fillRect()`.

      This is the first module that genuinely needs both a `Backbuffer`
      (render/) and player game-state math (player/), so
      `dawnstar_render` gained a `PUBLIC` dependency on `dawnstar_player`
      for it -- no cycle results, since nothing in `player/`/`world/`
      depends back on `render/`. Wired into `main.cpp`'s per-frame
      render call, right after `FrameRenderer::Render`.

      Ported exactly, including one real quirk confirmed against the
      source rather than assumed: the HP and Magicka bars' fill width
      has no upper clamp against the 40px background, so a stat
      temporarily above its own max (e.g. M16's documented unclamped-
      Magicka-refund spellcasting bug) draws a fill past the background
      -- but the Fatigue bar alone explicitly clamps its fill at 40px.
      Not a rendering bug to unify away; the original really does treat
      the third bar differently from the first two.

      Verified via the new `hud_renderer_smoke.exe` (no JVM ground
      truth, same reason as M6/M9/M11/M13-M20) against a real created
      character: fill widths cross-checked against an independent
      hand-transcription of `effectiveStat()`'s formula for a baseline
      character, again with effect 23 ("Regeneration") forced active to
      exercise `effectiveStat()`'s only real branch, and again with
      every stat forced to 3x its own max to confirm the HP/Magicka
      overflow-with-no-clamp vs. Fatigue-clamps-at-40 quirk really
      reproduces. All checks passed. Also confirmed visually: screen-
      captured the real `dawnstar_port.exe` window and saw the 3 bars
      rendered correctly (full red/green/blue on a fresh character) in
      their real position, over the real corridor view from M20.

- [x] **M22 -- the live per-level monster/chest/dropped-item registry**
      (this session). `DungeonRuntime` (a new `src/dungeon/` module,
      `dungeon_runtime.h`/`.cpp`) ports `Dungeon.java`'s live state:
      `ESGame.monsters`/`chests`/`droppedItems`, the Hashtable/Vector
      registries `player_movement.h`/`monster_runtime.h`/
      `combat_resolution.h`'s class comments have been flagging as
      missing since M13/M15/M18. A `WorldRegistry` struct holds one
      registry set per level (position-keyed 28-byte monster records
      and 8-byte chest records, plus an unordered 7-byte dropped-item
      list), and `DungeonRuntime` ports `populateRandomMonsters`/
      `trySpawnMonsterNear`/`addDroppedItem`/`removeChest`/
      `removeDroppedItem`/`clearDroppedItemFlag`/`droppedItemsAt`/
      `refreshTileFlags` against it. This is the third module needing
      two of the existing sibling modules at once (world's
      `GeneratedLevel`/`DungeonView` and monster's `MonsterRuntime`),
      so it followed `combat/`'s established pattern: its own directory,
      depending on both, with neither depending back.

      Deliberately NOT wired into `PlayerMovement`/`MonsterRuntime`/
      `CombatResolution`'s own existing "no live registry" simplifications
      in this milestone -- `Move()`/`Chase()`/`OnDeath()`/`UseItem()` live
      in modules that can't depend on `dawnstar_dungeon` without cycling
      back into it. Traced `Monster.onDeath()`'s own call site to confirm
      it's only ever invoked from `GameCanvas`, never from `Player`/
      `Monster`'s own methods -- so that wiring genuinely belongs to a
      future `GameCanvas`/`ESGame` screen-wiring milestone, not this one.

      Two real quirks confirmed against the source rather than assumed:
      `trySpawnMonsterNear`'s `forcedTypeOrSentinel` parameter only
      literally forces a type for the special values 41/42 (the roaming
      monster's own ids) -- any OTHER "forced" value is actually used as
      a REPLACEMENT DIFFICULTY TIER for a random weighted roll, traced
      through `Monster.spawn()`'s real parameter order. And
      `removeDroppedItem`'s guard clears the registry entry but never
      the tile's own dropped-item presence bit (a separate
      `clearDroppedItemFlag` call does that) -- ported as two genuinely
      independent operations, not merged into one "remove" convenience.
      SIMPLIFIED: `removeDroppedItem`'s original `Vector.removeElement()`
      matches by Java reference identity (the exact same `byte[]`
      instance); this port matches by content equality instead, since a
      value type has no pointer-identity equivalent -- indistinguishable
      from the original unless two genuinely distinct dropped items ever
      have bit-for-bit identical records at once.

      Verified via the new `dungeon_runtime_smoke.exe` (no JVM ground
      truth, same reason as M6/M9/M11/M13-M21) against the real 37-level
      world: `populateRandomMonsters` checked for exact count, real
      walkable placement, and registry-key/stored-position agreement;
      forcing type 41 checked to bypass the roll; the tier-substitution
      quirk checked by independently reproducing the exact roll a fresh,
      identically-seeded RNG should produce; dropped-item add/remove/
      query and the presence-bit-survives-removal quirk; chest removal's
      wall-tile guard; and `refreshTileFlags` checked to both clear a
      deliberately-planted stale bit and re-derive every real bit from
      the registries alone. All checks passed.

- [x] **M23 -- wiring `DungeonRuntime` into `PlayerMovement`'s
      dropped-item auto-loot, instant-lethal-tile camp trigger, and
      roaming-monster cleanup** (this session). Closed three of
      M13/M18's own documented "no live registry"/"no camp system yet"
      simplifications, now that M22's registry and M18's camp system
      both exist:
      - `CommitMove`'s dropped-item auto-loot-on-arrival now really
        runs: every record `DungeonRuntime::DroppedItemsAt` finds on the
        arrival tile is offered to `PlayerInventory::AddItem`, looted
        records are removed via `RemoveDroppedItem` (awarding
        `giftPointsFound` for category-11 items whose record flags say
        so), and the tile's presence bit only clears via
        `ClearDroppedItemFlag` once every record there was actually
        picked up -- an inventory-full item genuinely stays on the
        floor, exactly like the original.
      - `CommitMove`'s instant-lethal-tile (bit 8) case now really calls
        `MarkCampAndReturnToTown(false)` (M18) instead of only
        committing position/facing -- the one piece M18 itself couldn't
        close yet, since `CommitMove` hadn't been touched since M13.
      - `ComputeMoveTarget`'s "remove roaming gehen on level change"
        cleanup, and the identical block `Player.java`'s
        `resetToHubPosition` itself also runs, now really search the
        LEAVING level's live monster registry for a type-41 monster and
        remove it via the new `DungeonRuntime::RemoveMonster`
        (`ESGame.removeMonster`'s port) -- factored into one shared
        private `CleanupRoamingMonsterIfPresent` helper since both call
        sites are the exact same block traced from the source.

      This made `PlayerMovement`/`CombatResolution` the third/fourth
      modules needing two of the existing sibling modules at once (here
      player/combat + the new `dungeon/`), so `dawnstar_player` and
      `dawnstar_combat` both picked up a dependency on `dawnstar_dungeon`
      -- no cycle results, since neither `dawnstar_world` nor
      `dawnstar_monster` depends back on `dawnstar_player`.
      `CombatResolution::UseItem`'s item-87 handling (already calling
      `MarkCampAndReturnToTown` since M18) needed a `WorldRegistry&`
      parameter added through it for the same reason.

      Verified via the new `movement_registry_wiring_smoke.exe` (no JVM
      ground truth, same reason as M6/M9/M11/M13-M22) against the real
      37-level world: full-loot and inventory-full dropped-item
      pickup (including the gift-points branch); the lethal-tile trigger
      landing at the correct alt-spawn point with the correct camp-mark
      bookmark; the roaming-monster cleanup actually removing the
      registered type-41 monster and clearing the flag on a real level
      crossing, and confirming a plain turn (no level change) touches
      neither. One test-assumption bug caught and fixed along the way:
      an initial assertion expected `suppressStrafeAdjust` to still read
      `true` after `Move()` returned from the lethal-tile trigger --
      wrong, since `Move()` itself (matching `Player.move()`)
      unconditionally resets that flag to `false` before returning
      regardless of what `CommitMove` did inside; the flag is only ever
      observable as `true` mid-strafe-sequence within the same call.

- [x] **M24 -- populate the live registry from world generation itself**
      (this session). Since M6, `GeneratedLevel::monsters`/`::chests`
      (`world/dungeon_generator.h`) have held the room-monster/chest
      spawns `DungeonGenerator::PopulateLevel`/`PlaceChests` compute as
      plain output data -- a deliberate placeholder, since no live
      registry existed yet to put them in. In the original,
      `DungeonGenerator.populateLevel`'s room-monster loop and
      `placeChests` register directly into `ESGame.monsters[]`/
      `chests[]` (`Monster.spawn(...).store()`, `ESGame.chests[...]
      .put(...)`) as part of generation itself -- so until this
      milestone, M22's `WorldRegistry` stayed permanently empty in the
      real windowed app: nothing had ever actually put the world's own
      pre-placed monsters/chests into it, only whatever a future
      screen-wiring milestone might dynamically add later. New
      `DungeonRuntime::RegisterGeneratedSpawns(level, world)`
      (`dungeon/dungeon_runtime.h`/`.cpp`) closes that gap: called once
      per level right after generation (`main.cpp`'s `BuildWorld`, which
      now takes the `WorldRegistry&` it builds), it converts each
      `GeneratedMonsterSpawn` into a full 28-byte `Monster.toBytes()`
      record (`MonsterRuntime::ToBytes`) and each `GeneratedChestSpawn`
      into the real 8-byte chest record layout, keyed by position exactly
      like every other registry entry. Tile bits are left untouched here
      -- `DungeonGenerator` already sets them (bit 2/16) while building
      `level.tiles` itself, so this only adds the missing registry side.
      Couldn't live inside `DungeonGenerator` itself: `dawnstar_world` is
      a dependency *of* `dawnstar_dungeon`, so the reverse would cycle --
      same constraint M22's own entry already documents.

      Needed `GeneratedMonsterSpawn` to grow a `spawnId` field (mirroring
      `GeneratedChestSpawn`'s existing one) since nothing had assigned
      room-monsters an id before. Both remain per-level-local counters
      (1..N) rather than the original's single global counter shared
      across all 37 levels generated in one pass (`Monster
      .nextSpawnIdCounter`/`Item.nextSpawnId` respectively) -- unchanged
      from `GeneratedChestSpawn`'s own already-documented reasoning:
      `WorldRegistry` keys everything by position, never by spawnId, so
      this has no observable effect on registry correctness, only on the
      id's own numeric value.

      **A real doc bug caught and fixed along the way, unrelated to any
      code change:** `WorldRegistry`'s own class comment (written in
      M22) claimed chest record byte 2 is "always 0 despite the
      'guaranteed gift' comment" -- checking `DungeonGenerator.java`'s
      `placeChests` directly (needed to get this milestone's byte-packing
      right) shows byte 2 is written as `first ? 1 : 0` and never
      touched again, so it's actually 1 for the guaranteed-gift chest,
      not always 0. Grepping all of `../src/` confirms byte 2 is
      write-only -- nothing ever reads it back -- so the *practical*
      conclusion ("this byte has no observable effect") was right, but
      the stated reason was wrong. Corrected in place.

      Verified via the new `registered_spawns_smoke.exe` (no JVM ground
      truth, same reason as M6/M9/M11/M13-M23) against the real 37-level
      world: the hub town (no room-monster/chest generation at all)
      registers nothing; every one of the 36 standard levels' registered
      monster/chest counts match their generated counts exactly, every
      generated spawn's registry entry round-trips back to the exact
      same type/hp/position/spawnId (monsters) or
      position/guaranteedGift/tier/itemId/spawnId bytes (chests,
      including the extended-itemId high-byte branch, actually exercised
      by 17 of the 180 real generated chests) with the presence tile bit
      already set from generation; exactly one guaranteed-gift chest per
      level; and, as an integration check going beyond M22's own test
      (which only ever exercised dynamically-spawned monsters),
      `DungeonRuntime::RemoveMonster` correctly removes a monster this
      milestone registered from real world generation, not one
      `DungeonRuntime` itself created. All checks passed. Full clean
      rebuild stayed at zero warnings; all 22 smoke tests (including the
      new one) pass; the real windowed `dawnstar_port.exe` re-verified
      via screen capture to still render correctly.

- [x] **M25 -- the 13-slot `visibleObjects` cache (data model only)**
      (this session). `VisibleObjects` (`player/visible_objects.h`/
      `.cpp`) ports `Player.java`'s `tickVisibleObjects()`/
      `refreshVisibleObjects()`/`placeVisibleObject()`/`markLooted()` --
      the system `GameCanvas.paintVisibleObjects()` paints from directly.
      Deliberately the DATA-MODEL half only, mirroring the M9/M10 split
      (M9 = corridor wall-segment *selection*, M10 = actually drawing
      it): this milestone produces a correctly-populated
      `PlayerState::visibleObjects` every tick, with no pixel drawn
      anywhere. A new `VisibleSlot`/`VisibleSlotKind` tagged-enum struct
      (`player_state.h`) replaces Java's `EMPTY_SLOT`/`WALL_BLOCKED_SLOT`/
      `OCCLUDED_SLOT` Integer-reference-identity sentinels, and
      `visibleObjects` itself moved from a Java `static` field into
      `PlayerState` (harmless single-player simplification, same
      treatment every other Java-`static`-but-really-per-player field in
      this port already gets). Wired into `main.cpp`'s tick loop right
      after movement, matching `GameCanvas.run()`'s own per-tick order.
      Needed the hub town's 5 fixed peddler positions
      (`Shop.SHOP_X`/`SHOP_Y[0..4]`) as plain constant data -- no full
      `Shop` class exists in this port yet (M8's `ShopDialogue` only
      holds the dialogue text), so these are hardcoded directly in
      `visible_objects.cpp` rather than standing up a whole Shop module
      for 10 bytes of position data.

      **A real, surprising finding, confirmed by grepping every read
      site of `Monster.flag` (`rec[6]`) across `../../../src/`:** the
      only place that ever sets it true is `markLooted`, called
      immediately after ANY monster is newly placed into a visible slot
      -- every tick it stays in view -- and nothing ever sets it back to
      false. So despite the name, and despite this port's own earlier
      `MonsterState::flag` doc comment ("collected/looted marker") and
      `Dungeon.java`'s own "unconfirmed exact meaning" note, it does NOT
      track combat or loot state at all: it tracks "has the player ever
      seen this monster", and because Java's `rec` there is the literal
      same array reference sitting in both the slot and the registry
      (mutate-in-place aliasing), the very same tick that notices a
      monster also flags-and-renders it, forever after --
      `GameCanvas`'s `rec[6] != 0` rendering gate is therefore true for
      essentially every monster that has ever been on screen, not just
      "attacking" ones. Ported exactly (`VisibleObjects::MarkLooted`
      writes the flag back into the live registry, matching
      `Monster.store()`), not reinterpreted. SIMPLIFIED for dropped
      items only: Java's `rec` there is likewise the same array
      reference the registry's `Vector` holds, so `markLooted`'s bit-set
      incidentally reaches the registry too; this port's registry holds
      independent copies, so it doesn't -- confirmed by the same grep
      that nothing else ever reads that particular bit, so the
      divergence has no observable effect.

      Verified via the new `visible_objects_smoke.exe` (no JVM ground
      truth, same reason as M6/M9/M11/M13-M24): `refreshVisibleObjects`'s
      occlusion cascade checked against 4 independently hand-traced
      cases (a full-wipe wall directly ahead; two direct, single-target
      occlusions; and a deep chained cascade where an earlier slot's
      occlusion enables a later slot's own check to cascade further --
      all re-derived from Java's exact sequential re-read order, not
      consulted from the port's own implementation) -- all matched
      exactly. Integration-tested against the real 37-level generated
      world (M6/M24) and a real character (M11): a real pre-placed
      monster approached from a genuinely walkable adjacent tile lands in
      the closest slot and its "seen" flag round-trips into the live
      registry; the hub town's shop-0 peddler and level 3's named
      shopkeeper (via `GeneratedLevel::specialShopX/Y`) both resolve to
      the correct `npcShopIndex`. All checks passed. Full clean rebuild
      zero warnings; all 23 smoke tests pass; the real windowed app
      re-verified via screen capture (unchanged visually, as expected --
      no sprite drawing yet).

- [x] **M26 -- far/mid/close monster-chest-dropped-item-NPC sprite
      rendering** (this session). `VisibleObjectRenderer`
      (`render/visible_object_renderer.h`/`.cpp`) ports the bulk of
      `GameCanvas.paintVisibleObjects()`: far (slots 8-12) and mid (4-6)
      distance icons for monsters/chests/dropped items/NPCs, plus the
      closest slot's (1) chest/dropped-item icon -- consuming M25's
      `PlayerState::visibleObjects` directly, the first module to do so.
      Needed a genuinely new asset: `monsterfilenamesin.dat` (5x7 grid of
      PNG filenames, one row per monster-type "bucket" --
      `assets/monster_image_names.h`/`.cpp`, documented in
      `docs/ASSET_FORMATS.md`) -- `ESGame`'s own lazy per-level loader
      (`runImageLoader`, MIDP-memory-constrained, loads only the buckets
      the current level's monsters actually need) has no PC-memory-
      constraint equivalent to preserve, so `VisibleObjectTextures::Load`
      just decodes all 26 `objectSprites` + 3 chest + 3 bag images
      eagerly up front, the same simplification this port already made
      for floor/wall textures (M10). Wired into `main.cpp` right after
      `FrameRenderer::Render`, before the HUD bars -- matching
      `paintGameView()`'s own call order.

      DEFERRED (see the header's own class comment): the closest slot's
      MONSTER case (`paintObjectAtPosition`, which needs the large
      `OBJECT_DRAW_TABLE`/`OBJECT_ICON_TABLE`/`OBJECT_EXTRA_FLAGS` static
      tables and `drawSpriteFrame`'s multi-frame sprite-sheet slicing --
      genuinely unlike everything else this milestone drew, which are
      all single-frame plain image blits) and, by extension, the stairs
      icon (only ever reached through that same code path) and full NPC
      portraits (`paintNpcPortrait`, actually keyed by a completely
      separate `npcInSight` mechanism this port hasn't traced at all,
      *not* by `visibleObjects`).

      **Two real findings, confirmed against the source rather than
      assumed:**
      1. `monsterfilenamesin.dat`'s stored filenames carry a leading `/`
         (e.g. `/ban_male_body.png`) that `ImgArchive`'s own name lookup
         doesn't expect -- `ESGame.createImage()` itself strips exactly
         one leading `/` before looking anything up, a step invisible to
         every other image lookup in this port (which all come from
         `ESGame`'s own call-site string literals, never a leading
         slash). Caught immediately as a real exception on first run
         (`no such image: /ban_male_body.png`), not silently swallowed;
         fixed with the same stripping `VisibleObjectTextures::Load` now
         does before every lookup.
      2. NPCs placed into a far/mid `visibleObjects` slot render as a
         generic monster-shaped silhouette icon (never a real portrait),
         chosen by a genuinely asymmetric condition ported straight from
         `GameCanvas`'s own tag comparison: far icon 6 (mid 5) only for
         the hub's `Shop.NAMES[0]`/`NAMES[1]` peddlers and levels 21/30's
         named shopkeepers ("C"/"D") -- every other real NPC tag (the
         hub's `NAMES[2..4]` and levels 3/12's "A"/"B") gets far icon 13
         (mid 12) instead, with no discernible reason for the split.
         Ported as the real, uneven condition it is rather than "cleaned
         up" into something symmetric; Java's own dead `"W"` tag branch
         (never actually placed by `placeVisibleObject`) isn't reachable
         through this port's data model at all, since `VisibleSlotKind::Npc`
         only ever gets constructed for a real placed tag.

      Verified via the new `visible_object_renderer_smoke.exe` against
      the real extracted textures (M7/M10's own "look at the actual
      decoded pixels" standard, not just "something non-black got
      drawn"): far/mid/close monster, chest, dropped-item, and both NPC
      icon-set cases each checked against their sprite's own first
      opaque pixel at the exact expected screen offset; the `rec[6]!=0`
      "seen" gate confirmed to actually suppress an unflagged monster;
      the closest slot's still-deferred monster case confirmed to draw
      nothing at all yet. Every scenario renders on its own freshly-
      cleared frame rather than combining several slots at once -- this
      real asset set's sprites turned out to be far larger than their
      "icon" names suggest (a mid-distance monster sprite can be 36x96
      pixels, drawn with zero clipping, matching `drawMonsterMid`'s own
      plain `drawImage()` exactly), so a mid-distance sprite legitimately
      overpaints a chunk of the far-distance row behind it -- real,
      intentional back-to-front layering (paint far things first, then
      nearer ones on top), not a rendering bug; an early version of this
      test combined multiple slots in one frame and mistook that real
      overlap for a defect before this was traced down. All checks
      passed. Full clean rebuild zero warnings; all 24 smoke tests pass;
      the real windowed app re-verified via screen capture, including
      after walking a real border crossing into a generated level.

- [x] **M27 -- the closest slot's monster sprite + the stairs icon**
      (this session). `VisibleObjectRenderer::PaintObjectAtPosition`
      (`render/visible_object_renderer.h`/`.cpp`) ports
      `GameCanvas.paintObjectAtPosition()`: the `OBJECT_DRAW_TABLE`/
      `OBJECT_ICON_TABLE`/`OBJECT_EXTRA_FLAGS` static tables (a per-bucket
      base sprite + optional second sprite + up to 4 extra decorations,
      each an (dx, dy, icon) triple) and `drawSpriteFrame`'s multi-frame
      horizontal-strip slicing -- which turned out to need no new
      `Backbuffer` method at all: `Blit`'s existing column-clip range is
      exactly `g.setClip(x, y, frameWidth, frameHeight)`'s mechanism here
      (no Y-clipping needed, since these sheets are only ever
      horizontally- not vertically-tiled, so a frame's height always
      equals the sheet's own). Wired into `VisibleObjectRenderer::Render`'s
      closest slot (1), replacing M26's stub there -- gated on the same
      `rec[6]!=0` "seen" flag every other monster slot already uses.
      Still DEFERRED (see the header's own updated class comment): full
      NPC portraits, actually keyed by a completely separate `npcInSight`
      mechanism this port hasn't traced at all, not by `visibleObjects`.

      **Two real findings, confirmed against the source rather than
      assumed:**
      1. `OBJECT_DRAW_TABLE`'s first two columns of every row look
         exactly like a "posCode range" label (row index 2 opens `{11,
         25, ...}`, row index 3 opens `{26, 40, ...}`) -- but grepping
         every read site shows those two columns are NEVER actually
         read by any code; only `positionBucketFor`'s own independent
         range checks decide which row applies. And the labels are
         wrong for the row they sit on besides: row index 2 (which
         `positionBucketFor` actually assigns to types 26-40) opens with
         "11, 25", and row index 3 (types 11-25) opens with "26, 40" --
         the middle two rows' own label values are swapped relative to
         how they're actually dispatched to. Since nothing ever reads
         them, this can't be a functional bug -- just confusing dead
         data, preserved verbatim rather than reordered or "corrected".
      2. Monster types 41/42 -- confirmed via `MonsterDatabase` against
         the real extracted data to be genuine named creatures
         ("Gehenoth"/"Gehenoth Thriceborn", with their own stats like
         any other type, not some kind of stairway sentinel) -- render
         the STAIRS ICON instead of any monster sprite at all when nearby
         and closest. Traced exactly rather than assumed a mislabeling:
         `paintStairsIcon` is only ever called from this one branch,
         gated on nothing but the closest monster's TYPE being 41 or 42.
         So in the original game, standing right next to this specific
         rare monster renders a staircase icon in its place -- ported
         faithfully as this real, striking behavior, not "fixed" into
         drawing a creature sprite instead.

      Also confirmed (not merely assumed from the flags table's shape):
      `OBJECT_EXTRA_FLAGS`'s 4th "extra decoration" column is `false`
      for every one of its 41 rows -- the 4th extra slot two of the four
      `OBJECT_DRAW_TABLE` rows allocate space for is never actually
      triggered by any real monster type in the whole game. And neither
      table has an entry for type 42 at all (only 41 rows, covering
      types 1-41) -- harmless, since type 41/42 both dispatch to the
      stairs-icon branch before either table is ever indexed.

      Verified via the new `object_at_position_smoke.exe` against the
      real extracted textures (M7/M10/M26's own "look at the actual
      decoded pixels" standard): one posCode from each of the 4 sprite
      buckets (including a non-zero-frame case for both the base and
      second sprite, and an all-extras-false case confirming nothing
      extra is drawn), a case exercising 2 of the 4 possible extra
      decorations together, and both monster types 41 and 42 confirmed
      to draw the stairs icon's up/down frame respectively -- every
      expected draw independently hand-derived from the tables' own
      values (not read back from the port's implementation) and checked
      against the sprite's own first opaque pixel within the correct
      frame slice at the exact expected screen offset. All checks
      passed. Full clean rebuild zero warnings; all 25 smoke tests pass
      (M26's own now-obsolete "closest-slot monster is deferred" check
      removed and superseded by this milestone's test); the real
      windowed app re-verified via screen capture.

- [x] **M28 -- NPC-in-sight detection + NPC portraits** (this session).
      Ports `GameCanvas.refreshNpcInSight()`/`Player.npcInFront()`
      (`PlayerMovement::RefreshNpcInSight`/`NpcInFront`,
      `player/player_movement.h`/`.cpp`) and `GameCanvas.
      paintNpcPortrait()` (`VisibleObjectRenderer::PaintNpcPortrait`,
      `render/visible_object_renderer.h`/`.cpp`) -- the piece M26/M27
      explicitly deferred, and a genuinely separate mechanism from
      `visibleObjects` (see M26/M27's own doc comments): `npcInSight` is
      set from a completely independent tile-bit test one step ahead of
      the player, not from anything in the 13-slot cache.

      `NpcInFront` re-derives the look-ahead tile via the already-private
      `ComputeMoveTarget(1, ...)` (M23) -- reusing it directly rather
      than duplicating its recentering/level-crossing logic a second
      time -- and checks it against either the hub town's 5 fixed
      peddler positions (`Shop.SHOP_X/Y[0..4]`, a third independent
      inlined copy of the same 2 arrays `player/visible_objects.cpp` and
      `world/dungeon_generator.cpp` already each carry their own copy of
      -- still no shared `Shop` class exists to consolidate them into)
      or, for levels 3/12/21/30, the real generated
      `GeneratedLevel::specialShopX/Y` position (not a static
      `Shop.SHOP_X/Y[5..8]` -- those get overwritten by
      `DungeonGenerator` at generation time in the original, so reading
      the generated field directly is the correct equivalent, exactly
      matching `player/visible_objects.cpp`'s own NPC-tagging code).
      `RefreshNpcInSight` is wired into `main.cpp` right after a move is
      *attempted* each tick (matching `commitMove()`'s own
      `pendingMoveDir != 0` gate -- called unconditionally whenever a
      direction key was pressed, regardless of whether the move actually
      committed), and `PaintNpcPortrait` is painted right after
      `VisibleObjectRenderer::Render` and before `HudRenderer::
      PaintStatusBars`, matching `paintGameView()`'s own draw order.
      SIMPLIFIED: the original's `showMessage`/`messagePriority` popup
      (shows the shop's greeting the moment it comes into sight) and its
      "tile says NPC but npcInFront() disagrees" console diagnostic are
      both skipped -- no message-popup system is ported yet (still on
      the flagged hotbar/message-popup/minimap list below), and no other
      module in this port uses a stdout channel for diagnostics either
      (same precedent as `CleanupRoamingMonsterIfPresent`'s own skipped
      console message, M23).

      A real, faithfully-preserved quirk, confirmed rather than
      "fixed": since `RefreshNpcInSight` always runs right after a move
      has already committed, `NpcInFront`'s own `ComputeMoveTarget(1,
      ...)` call recomputes a look-ahead from the *already-new*
      position -- which can in principle cross yet another level
      boundary and re-trigger `CleanupRoamingMonsterIfPresent` a second
      time in the same tick. Harmless in practice: that cleanup is
      idempotent (guarded by `p.roamingSpecialMonsterPresent`, already
      cleared by the real move's own call if it fired), so this is
      ported exactly as shaped in the original rather than collapsed
      into a single call.

      `PaintNpcPortrait` itself is a thin, fully data-driven wrapper: a
      9-entry `shopId -> (posCode, frameOverride)` table (transcribed
      directly from `paintNpcPortrait`'s own switch) dispatching straight
      into M27's `PaintObjectAtPosition` -- the original reuses the
      generic corridor-object renderer for portraits wholesale, with no
      NPC-specific sprite/draw path of its own at all.

      Verified via the new `npc_portrait_smoke.exe`: `RefreshNpcInSight`
      against 3 real integration cases (the hub's shop 0, level 3's
      named shopkeeper, and a real "nothing ahead" position, using the
      same `FindApproach`/turn-right-then-left-to-refresh approach M25's
      own test established), and `PaintNpcPortrait` against 2 hand-derived
      shopId cases spanning both buckets 0 and 1 (the only two portraits
      ever use) with `frameOverride` deliberately chosen to differ from
      `OBJECT_ICON_TABLE`'s own default in each case -- the one behavior
      M27's own test never exercised (it always passes `frameOverride`
      -1) -- plus an out-of-range-shopId no-op check. All checks passed.
      Full clean rebuild zero warnings; all 27 smoke tests pass; the
      real windowed app re-verified via screen capture.

- [x] **M29 -- the minimap** (this session). Ports `GameCanvas.
      sampleSquareView()` (`DungeonRuntime::SampleSquareView`,
      `dungeon/dungeon_runtime.h`/`.cpp`) and `refreshMinimap()`/
      `paintMinimapGrid()`/`paintGameView()`'s own minimap-compositing
      step (`MinimapSurface`/`MinimapRenderer`,
      `render/minimap_renderer.h`/`.cpp`) -- closing the "minimap" third
      of M22's long-flagged hotbar/message-popup/minimap entry (the
      other two remain open).

      `SampleSquareView` needed a live `WorldRegistry` (to test a seen
      monster's own `flag`) alongside `world/dungeon_view.h`'s
      `DungeonView::TileAt` (M19) -- the same "needs two existing
      sibling modules at once" shape `dungeon/dungeon_runtime.h` itself,
      `player/player_movement.cpp`, and `combat/combat_resolution.cpp`
      already have, so it lives on `DungeonRuntime` (`dawnstar_dungeon`,
      which already depends on `dawnstar_world`) rather than beside
      `SampleCorridorView` in `dawnstar_world` itself, which cannot
      depend back on `dawnstar_dungeon` without cycling.
      `render/minimap_renderer.cpp` is the second module (after
      `player/player_movement.cpp`) needing both `dawnstar_dungeon` and
      rendering at once, so `dawnstar_render` gained a direct link to
      `dawnstar_dungeon` too (previously only reached transitively
      through `dawnstar_player`).

      SIMPLIFIED: the compass glyph GameCanvas draws right alongside the
      minimap image in both zoom states (`g.drawChar(COMPASS_GLYPHS[...]
      , ...)`) is NOT ported -- it's real MIDP built-in-font text
      rendering, and this port still has no text-rendering system at
      all (the same underlying reason every message-popup call site
      across M13-M28 has been SIMPLIFIED away, not something specific to
      the minimap). The minimap-zoom-toggle key (`key == 42`, MIDP's
      numeric-keypad `*`) is remapped to `'M'` for a PC keyboard, with
      real edge-detection against a held key (unlike movement, which
      polls every tick on purpose) since the original's `keyPressed()`
      is a one-shot event, not a per-tick poll -- not a behavior
      simplification, the same kind of physical-key remap
      VK_UP/DOWN/LEFT/RIGHT already are.

      **Two real, faithfully-preserved bugs found while transcribing
      `paintMinimapGrid`, both the same classic Java trap** (`<<` binds
      looser than `+`, so `a + b << c` parses as `(a + b) << c`, not
      `a + (b << c)`):
      1. The background fill's size, `gridSize*cellSize+border << 1`,
         evaluates to `(gridSize*cellSize+border) << 1` -- DOUBLE the
         apparently-intended `gridSize*cellSize + 2*border` (44 instead
         of 23 when zoomed in; 174 instead of 89 when zoomed out).
         Proven observable (not merely theoretical) by pre-dirtying a
         fresh `MinimapSurface` and confirming a real `Refresh()` call
         actually erases a marker pixel at (35,35) -- inside the buggy
         44px reach, outside the "intended" 23px one -- while a pixel at
         (60,60), outside even the buggy reach, survives untouched.
      2. The very next `drawRect`'s size uses `<< 0` (a no-op) instead
         of doubling `border` either, so the white outer border is
         exactly `border` pixels short of `gridSize*cellSize + 2*border`
         -- missing its own second border-width on the right/bottom
         edge. Also proven observable: the border's real (buggy) right
         edge sits at x=22 (zoomed in), not x=23 where a correctly
         doubled computation would have put it.
      3. A related, purely arithmetic finding: `border==2`'s (zoomed-out
         only) INNER `drawRect` call has a literal `<<-1` in the source.
         Java's `<<` masks its shift amount to the low 5 bits (JLS
         15.19), so `<<-1` means `<<31`, not a right-shift or a no-op --
         for this call's one real value (`base`=87, odd), that comes out
         to exactly `Integer.MIN_VALUE`. Reproduced with an unsigned
         intermediate (C++ gives UB for a negative/out-of-range shift
         count) and absorbed as a no-op by `MinimapSurface::DrawRect`'s
         own non-positive-dimension guard -- a reasonable stand-in for
         "undefined MIDP behavior no real device's `drawRect` contract
         covers anyway," not an attempt to reproduce one vendor's exact
         garbage output.
      4. Also independently deduced (not tested -- see the reasoning
         directly in `minimap_renderer.cpp`'s own `PaintMinimapGrid` doc
         comment): bug 1's background-fill overshoot is completely
         UNOBSERVABLE in the zoomed-out case specifically, because both
         the buggy value (174) and the "intended" correct one
         (`17*5+2*2=89`) meet or exceed the 89px surface, converging to
         "fill the whole surface" either way. It's only bug-vs-intended-
         DISTINGUISHABLE in the zoomed-in case (22 vs 23 not compared to
         44 vs 89), which is exactly the case this milestone's test
         exercises.

      Also a real, faithfully-ported quirk in `sampleSquareView` itself:
      its monster-presence lookup always queries the registry for the
      level SAMPLING STARTED from, using the sampled tile's own raw
      (possibly cross-level-stitched) coordinates -- so right at a level
      boundary, a neighboring level's real tile bits can show up on the
      minimap without ever resolving to a "seen monster" red square (see
      `DungeonRuntime::SampleSquareView`'s own doc comment).

      Verified via the new `minimap_smoke.exe`: pure `MinimapSurface`
      primitive checks (`FillRect`/`DrawRect` clipping and inclusive-
      corner shape, no game data needed); both real size bugs above,
      demonstrated precisely via a pre-dirtied surface and a real
      `Refresh()` call; `SampleSquareView` against real generated chest/
      no-spawn-room/seen-monster positions (centered in the sample
      window, so the center cell always maps back to the sampled
      position regardless of facing -- no col/row math needed to
      predict where to look); and `Composite`'s ailment-3 visibility
      gate plus its zoomed-in-clipped-to-23x23 vs. zoomed-out-full-89x89
      draw. All checks passed. Full clean rebuild zero warnings; all 28
      smoke tests pass; the real windowed app re-verified via screen
      capture in BOTH zoom states (including pressing 'M' live to
      confirm the toggle itself works).

- [x] **M30 -- text rendering + the message popup** (this session).
      This port's first text-rendering system: a hand-authored
      monospace pixel font (`BitmapFont`, `graphics/bitmap_font.h`/
      `.cpp`) and `Backbuffer::FillRoundRect`, used to port
      `GameCanvas.showMessage()`/`wordWrap()`/`wrapToTwoLines()`/
      `paintMessagePopup()` plus `run()`'s own per-tick auto-hide
      timeout (`MessagePopup`, `render/message_popup.h`/`.cpp`) --
      closing the message-popup half of M22's long-flagged hotbar/
      message-popup pair (the hotbar itself remains open) and unblocking
      the shop-greeting popup M28's own SIMPLIFIED note had flagged as
      waiting on exactly this.

      **A genuine architecture choice, put to the user rather than
      decided silently**: MIDP's `Font`/`Graphics.drawChar`/
      `drawString` have no real recoverable asset (a system font is
      platform/device-dependent, was never bundled game data the way
      every other visual in this project has been) -- the two live
      options were a hand-rolled bitmap font baked into the existing
      `Backbuffer` pipeline (keeping every future text-touching
      milestone testable via the same pixel-level `Backbuffer`
      assertions everything else already uses) or drawing text via GDI
      directly onto the window (less code, but untestable the same way,
      and a second rendering path alongside the backbuffer blit). Asked
      the user directly; they chose the bitmap-font route, which is
      what's built here.

      `BitmapFont` covers only space/`'`/`-`/`!` and A-Z (30 glyphs,
      4x7 pixels each, monospace) -- confirmed against a temporary
      diagnostic dump of every real character actually appearing in
      Item/Monster names and `Shop.NAMES` (not assumed) that this is the
      complete real alphabet needed. Lowercase is folded to uppercase
      before drawing rather than separately hand-authoring a second
      full glyph set purely for cosmetic case-fidelity on an already-
      invented font -- a real, visible (`"WEAPON PEDDLER"` instead of
      `"Weapon Peddler"`), but harmless, simplification. The per-
      character advance (`kAdvance` = 5px) is itself invented (no real
      SMALL_FONT metric survives), but was deliberately tuned so real
      content -- Shop.NAMES' own longest entry, "Heavy Armor Peddler" --
      still wraps to exactly 2 lines through `WrapToTwoLines`'s fixed
      69px width, rather than gratuitously overflowing a 3rd line (which
      `WrapToTwoLines` silently discards) that a real device's own
      unrecoverable metric probably wouldn't have needed either.

      `WordWrap` is `wordWrap()` transcribed directly, substituting
      `BitmapFont::kAdvance` for every real `Font.charWidth()`/
      `stringWidth()`/`substringWidth()` call (valid since SMALL_FONT is
      itself `FACE_MONOSPACE`, so every character really did have one
      constant width in the original too) -- including its own hard-
      break inner loop (a real, if rare, "single word longer than the
      whole popup width" fallback) and the subtle "the final pushed
      line can carry a trailing space character" behavior the original
      algorithm produces (verified by hand-tracing a synthetic example
      step by step, not just trusted from re-reading the algorithm).

      `Show()` folds in the "if (showMessage(...)) { messageShownAt =
      now; messageVisible = true; }" pattern every one of GameCanvas's
      own call sites repeats identically right after calling
      `showMessage()` -- a SIMPLIFIED but exactly behavior-preserving
      consolidation, confirmed by grep that no real call site ever
      diverges from that exact follow-up.

      Wired into the three call sites already reachable from this
      port's live tick loop (the rest of `showMessage()`'s ~15 call
      sites remain unreachable until attack/spellcast/camp/menu actions
      themselves get wired, a separate, larger milestone): the found-
      item message (diffing `player.inventoryCount` before/after
      `Move()`, exactly mirroring `commitMove()`'s own `slotsBefore`
      diff rather than threading a count out of `Move()` itself), the
      chest-in-sight popup (`PlayerMovement::ChestInFront`, a new method
      mirroring `NpcInFront`'s own `ComputeMoveTarget(1,...)` re-
      derivation and its same harmless double-cleanup quirk), and the
      NPC shop-greeting popup (`RefreshNpcInSight` itself still only
      sets `player.npcInSight`, M28 -- the actual `showMessage()` call
      for the greeting now happens in `main.cpp`, using a small new
      `kShopNames` table, since `PlayerMovement`/`dawnstar_player` cannot
      depend on `render/message_popup.h`/`dawnstar_render` without
      cycling back through it).

      A design note, not a bug: `MessagePopupState` is deliberately kept
      as its own struct rather than folded into `PlayerState` as a 4th
      instance of the `npcInSight`/`minimapDirty`/`minimapZoomedOut`
      pattern (M28/M29) -- see its own doc comment for why, and for the
      flagged future cleanup (one dedicated UI-state struct for all of
      GameCanvas's leftover statics) this isn't yet worth doing on its
      own.

      Verified via the new `message_popup_smoke.exe`: `BitmapFont`/
      `FillRoundRect` primitive checks; `WordWrap` against a fully
      hand-traced synthetic case (exercising the space-boundary,
      forced-hard-break, and trailing-space-on-the-final-line paths all
      in one string) plus 2 real `Shop.NAMES` strings (independently
      re-derived by hand, not read back from `message_popup.cpp`);
      `Show`/`Tick`'s exact priority-gate and 3000ms timeout arithmetic;
      and `Paint`'s visible-vs-hidden gating. All checks passed. Full
      clean rebuild zero warnings; all 29 smoke tests pass. The real
      windowed app was also driven end-to-end with a temporary
      diagnostic (a real BFS path to the hub's real shop 0, computed
      against the real generated level rather than guessed -- live
      keyboard-injection timing turned out too unreliable for
      navigation, so the diagnostic called `PlayerMovement::Move`
      directly instead) and rendered one real frame showing the actual
      shopkeeper portrait alongside a real "WEAPON PEDDLER" popup,
      confirming the whole pipeline end-to-end before the diagnostic
      was removed.

- [x] **M31 -- the hotbar panel** (this session). Closes the last piece
      of M22's long-flagged hotbar/message-popup pair (the message
      popup half landed in M30). Ports `GameCanvas.paintHotbar()`/
      `computeHotbarContext()`/`drawHotbarIcon()` (`HotbarRenderer`,
      `render/hotbar_renderer.h`/`.cpp`): the bottom panel background
      (`panel.png`, real 176x52, drawn unscaled at (0,156), exactly
      filling the 176x208 backbuffer's last 52 rows) plus 4 fixed-
      position numeric-key prompts, each a digit char (a black "shadow"
      1px down-right of a white fill, exactly reproducing the original's
      own two-pass `drawChar` calls) and an icon frame from `icons.png`
      (real 270x24 -- 9 icons, 30px each, confirmed against the real
      archive rather than assumed; its 24px height exactly matches
      `drawHotbarIcon`'s own clip height, so `Backbuffer::Blit`'s
      existing `[clipX0, clipX1)` range alone reproduces the original's
      `setClip(x, y, 30, 24)` with no separate vertical clip needed).

      `BitmapFont` (M30) gains 0-9 (10 more hand-authored glyphs, 40
      total) for `HOTBAR_DIGIT_CHARS` -- same invented-shape status as
      every other glyph there, no original digit bitmap to recover
      either.

      `ComputeHotbarContext` is `computeHotbarContext()` transcribed
      directly: monster-targeted (combat hotbar) beats chest/NPC-in-
      sight (interact hotbar) beats neither (explore hotbar) -- a real
      if/else priority order, not an AND of all three conditions.
      `monsterTargeted` is passed as a plain parameter rather than
      folded into `PlayerState` as a 4th instance of M28/M29's own
      GameCanvas-statics precedent, because nothing in this port ever
      sets it yet: `GameCanvas.monsterTargeted` is only ever written by
      the combat attack-targeting flow, which isn't wired into the live
      tick loop -- adding a field nothing writes would just be dead
      state. Every real call site (`main.cpp`) passes `false` until that
      wiring lands. `chestInSight`, by contrast, IS added to
      `PlayerState` (unlike M30, which only ever needed
      `ChestInFront`'s one-off pointer inside the tick-gated move block
      to fire a `showMessage` call, `paintHotbar` itself runs every
      frame, not just on a movement tick, so this value has to be
      persisted rather than kept purely local).

      NOT ported here or anywhere yet: `paintActionFlashes()`
      (`monsterHitFlash`/`spellHitFlash`/`selfSpellFlash`, also
      `drawHotbarIcon`-based, but driven by the same unwired combat
      flow) and the `keyPressed()` dispatch that reads `hotbarContext`
      back (`attackRequested`/`interactRequested`/`campRequested` --
      same reason).

      Verified via the new `hotbar_renderer_smoke.exe`: real
      `icons.png`/`panel.png` dimensions confirmed against the archive;
      `ComputeHotbarContext`'s full priority-order truth table; each of
      the 3 contexts' panel background, all 4 icon positions (sampling
      each icon's own first opaque pixel within its 30px frame, not just
      "some sprite pixel"), and digit glyph placement (independently
      hand-traced from the new digit glyph rows, not read back from
      `hotbar_renderer.cpp`). All checks passed. Full clean rebuild zero
      warnings; all 31 smoke tests pass. Also rendered two real frames
      via a temporary diagnostic (a real created character standing in
      the real hub level) confirming context 0 (explore) and context 2
      (chest-in-sight) visually -- panel, all 4 icons, and legible digit
      prompts, with the expected single-icon swap between the two
      contexts' last slot -- before the diagnostic was removed.

- [x] **M32 -- monster targeting + the attack action + death/loot
      wiring** (this session). The first of `dispatchTickActions()`'s
      8-way priority-ordered action dispatch to actually get wired into
      the live tick loop (camp/interact/cast/cycle/options remain
      unwired -- each needs UI this port doesn't have yet: camp state,
      shop dialogue, a spell-selection overlay, an options menu).
      Attack outranks movement per tick, matching the original's own
      ordering, gated on a new `'A'` key mapped the same "poll the held
      key every tick, rely on the original's own internal cooldown to
      throttle it" way movement already established (rather than
      reproducing `keyPressed()`'s discrete per-keydown event
      semantics) -- and, like the original's own `key == 49` handler,
      only actually live while `hotbarContext == 1` (a monster
      targeted), computed once per tick from the PREVIOUS tick's
      `player.monsterTargeted`/`chestInSight`/`npcInSight`, exactly
      matching the original's own real timing (`hotbarContext` is a
      field `paintHotbar()` last wrote, not something `keyPressed()`
      recomputes inline).

      New `PlayerMovement::MonsterInFront` (`player/player_movement.h`/
      `.cpp`) mirrors `ChestInFront`/`NpcInFront`'s own
      `ComputeMoveTarget(1,...)` reuse for `Player.
      nearestAttackableMonster()` -- SIMPLIFIED but not lossy: since
      this port's `WorldRegistry` stores raw monster bytes directly
      (no separate "live `Monster` object" layer the original's own
      `GameCanvas.targetMonster` snapshots), it returns a pointer
      straight into the live record instead.

      New `combat/combat_tick.h`/`.cpp` (`CombatTick`, its own module
      for the same reason `combat/combat_resolution.h` is one: it needs
      BOTH `dawnstar_combat` and `dawnstar_render`, which don't depend
      on each other) ports `processAttack()`/`refreshTargetMonster()`/
      `resolveMonsterDeath()`. A genuinely equivalent (not merely
      simpler) consolidation: rather than caching a `targetMonster`
      across the tick boundary the way the original's own separate
      `refreshTargetMonster()` call does, `ProcessAttack` re-derives the
      front monster FRESH via `MonsterInFront` every time -- exactly
      equivalent because attack and movement are mutually exclusive per
      tick, so the player's front tile can't have changed since the
      last refresh whenever an attack fires. `RefreshAndResolveTargetMonster`
      combines `refreshTargetMonster()` + `resolveMonsterDeath()` into
      one call since the original always calls them back-to-back anyway.

      Monster death finally wires the loot-drop path this project has
      flagged as blocked since M15/M18/M22: `MonsterRuntime::OnDeath`'s
      `DeathDrop` is registered via `DungeonRuntime::AddDroppedItem` on
      the monster's OWN level (`target.dungeonLevel`) -- but
      `DungeonRuntime::RemoveMonster` is called on the PLAYER's current
      level, a real, faithfully-preserved oddity traced straight from
      `GameCanvas.resolveMonsterDeath()`'s own `ESGame.removeMonster(
      this.player.currentLevel, ...)` call (not `target.dungeonLevel`),
      which can silently search the wrong level's registry at a doorway
      tile -- harmless in practice, preserved rather than "fixed".
      Monster type 41's death still clears
      `specialEncounterResolved`/`roamingSpecialMonsterPresent`; type
      42's skips the loot roll entirely (its real end-of-game-UI
      transition isn't ported -- no menu system exists yet) but still
      runs every other cleanup step, matching the original's own
      fallthrough. The ailment-4 kill-heal bonus and the "Creature is
      dead!" popup are both wired too.

      `player.monsterTargeted` (folded into `PlayerState`, same
      reasoning as `chestInSight`/`npcInSight`) finally makes
      `HotbarRenderer::ComputeHotbarContext`'s context-1 (combat) branch
      live -- M31's own hotbar work was built and tested against a
      hardcoded `false` for exactly this. `paintActionFlashes()`'s
      `monsterHitFlash` case is also ported now (`HotbarRenderer::
      PaintActionFlashIcon`, reusing `drawHotbarIcon`'s own logic with
      caller-supplied random offsets via `LingoRandomInt`) --
      `spellHitFlash`/`selfSpellFlash` remain unported pending the
      spellcasting-wiring milestone.

      Verified via the new `combat_tick_smoke.exe` against the real
      37-level generated world: `MonsterInFront` against a real
      pre-placed monster spawn; `ProcessAttack`'s cooldown gating and
      its hp-invariant (`hit == (hpAfter < hpBefore)`, which holds
      regardless of the probabilistic hit roll's actual outcome --
      deliberately NOT forced to a specific result, since the
      underlying roll math was already verified in M14/M15/M16); and
      `RefreshAndResolveTargetMonster`'s full death-resolution path
      (registry removal, tile-bit clearing, the ailment-4 heal formula,
      the death popup, and both special monster-type branches). All
      checks passed. Full clean rebuild zero warnings; all 32 smoke
      tests pass. Also drove the real windowed pipeline end-to-end via
      a temporary diagnostic (a real generated level's real pre-placed
      monster, attacked with real combat math until it actually died)
      and rendered two real frames -- one mid-fight (context 1, the
      combat hotbar, correctly showing while `monsterTargeted`) and one
      right after the kill (context 0 again, with a real "CREATURE IS
      DEAD!" popup) -- before the diagnostic was removed.

- [x] **M33 -- the spellcasting action (cast + cycle)** (this session).
      The second of `dispatchTickActions()`'s priority-ordered actions
      to get wired into the live tick loop (camp/interact/options
      remain unwired -- each still needs UI this port doesn't have
      yet). Cast and cycle both outrank attack per tick, matching the
      original's own ordering exactly -- all of `Player.castOnSelf()`/
      `castOnMonster()`/`cycleSelectedSpell()`'s underlying math was
      already ported and verified back in M16; this milestone is
      purely the tick-orchestration wiring around it (mirroring what
      M32 did for the attack action).

      New `'S'` (cast) and `'C'` (cycle) keys. Cast is polled every
      tick like attack -- `GameCanvas.keyPressed()`'s own `key == 51`
      handler sets `castSpellRequested` unconditionally (no
      `hotbarContext` gate, unlike attack's own `key == 49`), but
      `processSpellCast()`'s own 500ms cooldown throttles repeated
      firing to the same cadence a held key would produce anyway --
      same reasoning M32 already established for attack. Cycle,
      unlike cast/attack, has NO internal cooldown -- every physical
      keydown cycles exactly once -- so it's the one action this port
      genuinely edge-detects: sampled at full frame rate (the same way
      the `'M'` zoom key already is, since the original's own
      `keyPressed()` fires immediately on a physical keydown too) into
      a pending flag, but -- unlike zoom's own immediate effect --
      only actually consumed once inside the tick-gated dispatch,
      matching `dispatchTickActions()`'s real once-per-tick
      consumption of `spellCycleRequested`.

      New `CombatTick::ProcessSpellCast`/`CycleSpell` (`combat/
      combat_tick.h`/`.cpp`) port `processSpellCast()`/
      `cycleSelectedSpell()` exactly, including the original's own
      real gate order (invalid spell id -> not enough Magicka -> the
      500ms cooldown -> offensive-vs-self dispatch) and a genuinely
      preserved quirk: `lastSpellCastTime` advances even when an
      offensive cast finds no monster targeted, because the original's
      own `this.lastSpellCastTime = now;` sits *outside* the
      `monsterTargeted` check, at the end of the same cooldown-gated
      branch that contains it -- attempting to cast at nothing still
      consumes the cooldown. An offensive cast re-derives the front
      monster fresh via `PlayerMovement::MonsterInFront` (the same
      "SIMPLIFIED, but not lossy" reasoning M32 already established:
      action dispatch is mutually exclusive per tick, so the front
      tile can't have moved since `player.monsterTargeted` was last
      refreshed this same tick).

      `paintActionFlashes()` is now fully ported: `spellHitFlash`
      (icon 8) and `selfSpellFlash` (icon 7) join M32's
      `monsterHitFlash` (icon 6), reusing `HotbarRenderer::
      PaintActionFlashIcon` as-is (already generic enough since M32 --
      no code changes needed there, only stale doc comments claiming
      the other two cases were still unported).

      Verified via the new `spellcast_tick_smoke.exe` against the real
      spellsin.dat `SpellDatabase` (picking whichever real offensive
      and real non-offensive spell the archive lists first, rather
      than hand-picked ids): the invalid-id/magicka/cooldown gate
      order and each gate's exact message text/priority (including
      `MessagePopup::Show`'s own negative-priority-becomes-10
      internal convention), the offensive-vs-self flash-flag dispatch,
      the preserved lastSpellCastTime-advances-with-no-target quirk,
      and `CycleSpell`'s own known-spells-mask-driven selection and
      messaging. All checks passed. Full clean rebuild zero warnings;
      all 34 smoke tests pass. Also rendered two real frames via a
      temporary diagnostic (a real generated level, a real monster
      spawn, real Magicka spend) confirming an offensive cast's red
      "spellHitFlash" burst next to the combat hotbar and a
      self-targeted cast's blue "selfSpellFlash" swirl alongside its
      own real message popup -- before the diagnostic was removed.

- [x] **M34 -- the interact action (chest looting)** (this session).
      The third of `dispatchTickActions()`'s priority-ordered actions
      to land, and it outranks all of cast/cycle/attack, matching the
      original's own ordering exactly (camp/options remain unwired --
      each still needs UI this port doesn't have yet).

      New `'I'` key, gated on `hotbarContext == 2` the same way
      `GameCanvas.keyPressed()`'s own `key == 57` handler is (`if
      (hotbarContext == 2) interactRequested = true;`). Unlike
      cast/attack, `processInteract()` has no internal cooldown of its
      own, so -- like M33's cycle key -- this is genuinely edge-
      detected at full frame rate rather than polled every tick.
      Reading `hotbarContext` at that same full frame rate (to apply
      its gate at the exact moment of the keydown, like the original
      really does) meant promoting `hotbarContext` itself from a
      tick-local recomputed fresh every tick (M32/M33) to a persistent
      `wWinMain` local only ever *reassigned* inside the tick-gated
      block -- a pure refactor, no behavior change, since every
      existing reader of it (`attackActive`'s own gate) still reads
      the exact same once-per-tick value it always did.

      New `InteractTick::ProcessInteract` (a new `src/interact/`
      module, `dawnstar_interact` -- its own small library for the same
      reason `dawnstar_combat` is one: it needs `dawnstar_player`
      (`PlayerInventory::AddItem`), `dawnstar_dungeon`
      (`DungeonRuntime::RemoveChest`/`AddDroppedItem`), AND
      `dawnstar_render` (the found-item/inventory-full popups) all at
      once, none of which may depend on each other) ports
      `processInteract()` in full. The `npcInSight >= 0` branch is a
      deliberate no-op stand-in -- `openNpcDialogue()` needs
      `Shop.dialogue()`'s actual line-selection logic (only
      `npcstrings.dat`'s raw text loads so far, M8) plus a whole
      `GenericInfoUI` screen, neither of which exists yet; same "the
      branch exists, but does nothing until its own UI milestone
      lands" shape as M32's monsterType-42 end-of-game-UI skip. The
      chest-loot half (`Player.pickUpDroppedItem()`) is fully ported,
      including two real preserved quirks: (1) the original's
      `result == -1` ("Chest locked!") branch is dead code --
      `pickUpDroppedItem()`'s own current body never actually returns
      -1, so there was nothing to reproduce there; (2) a chest record
      whose itemId low byte is 86 (`world/dungeon_generator.h`'s own
      "extended itemId" encoding, where the record's last byte holds
      the real id's high byte) is picked up as the literal byte 86 --
      `pickUpDroppedItem()` never resolves that high byte back into
      the real id, unlike `DungeonGenerator`'s own write side that
      encoded it. A real, silently-inert bug, confirmed by reading
      `Player.java` directly and ported exactly rather than "fixed".

      Verified via the new `interact_tick_smoke.exe` against the real
      37-level generated world (a real pre-placed chest spawn):
      chestInSight==false and npcInSight>=0 both confirmed as hard
      no-ops; a successful loot's inventory/popup/registry/tile-bit
      effects all checked against the real chest's own itemId; a
      full-inventory loot confirmed to still remove the chest from the
      registry while routing the item to a floor-dropped record
      instead (matching the original's own floor-drop branch exactly);
      and -- found by scanning the real generated world for actual
      low-byte-86 chests, the same way M24 first surfaced that
      encoding, rather than a synthetic one -- a real extended-itemId
      chest confirmed to add literal item 86, reproducing the
      preserved bug against real generation data, not a hand-built
      example. (Caught and fixed a real test bug of this milestone's
      own along the way: an early draft's scan iterated
      `world.chests[level]` with a range-based `for` while
      `ProcessInteract`'s own `RemoveChest` call erased entries from
      that same live map mid-iteration -- undefined behavior that
      segfaulted; fixed by collecting matching candidates into a
      separate list first, then dispatching the interact call
      afterward.) All checks passed. Full clean rebuild zero warnings;
      all 32 smoke tests pass. Also rendered two real frames via a
      temporary diagnostic (a real generated level, a real chest)
      confirming the interact hotbar (context 2) before looting and a
      real "IVORY CLASP" found-item popup with the hotbar correctly
      reverted to context 0 after -- before the diagnostic was removed.

- [x] **M35 -- the camp system** (this session). The fourth of
      `dispatchTickActions()`'s priority-ordered actions to land, and
      it outranks all of interact/cast/cycle/attack, matching the
      original's own ordering exactly (only options remains unwired --
      it still needs an options-menu UI this port doesn't have). Unlike
      M32-M34's actions, this one isn't a single dispatch call: it's a
      whole per-tick state machine (`GameCanvas.run()`'s own campState
      1/2/3 handling, sitting immediately before `dispatchTickActions()`
      itself) that can freeze the ENTIRE rest of a tick -- no attack,
      cast, cycle, interact, or movement -- for as long as the player is
      actually asleep.

      New `'Z'` key, gated on `hotbarContext == 0` the same way
      `GameCanvas.keyPressed()`'s own `key == 48` handler is, and
      genuinely edge-detected at full frame rate like M34's interact key
      (`enterCampState()` has no internal cooldown either). New
      `player/player_camp.h`/`.cpp` (`PlayerCamp::Rest`, `Player.rest()`)
      -- Player-only logic (no live `Monster` needed) that DOES need the
      live `WorldRegistry` for its roaming-special-monster cleanup, so
      it lives alongside `player_movement.h`'s own registry-touching
      methods rather than `player_combat_stats.h`. That cleanup turned
      out to be Player.java's rest() opening with the EXACT SAME guard/
      scan/removal block `PlayerMovement::CleanupRoamingMonsterIfPresent`
      already implements for `ComputeMoveTarget`/`ResetToHubPosition` (M23)
      -- so rather than a third copy, that method was promoted from
      private to public for `PlayerCamp::Rest` to call directly, its own
      doc comment updated to name the new caller.

      New `camp/camp_tick.h`/`.cpp` (`CampTick`, its own module for the
      same reason `combat/combat_tick.h`/`interact/interact_tick.h` are:
      it needs `dawnstar_player` (`PlayerCamp::Rest`), `dawnstar_dungeon`
      (`DungeonRuntime::TrySpawnMonsterNear`, for the monster that
      interrupts a disturbed camp), AND `dawnstar_render` (the "Cannot
      Camp!"/"Rest disturbed!"/"Rest complete!" popups) all at once)
      ports `enterCampState()` (`TryEnterCamp`) and `run()`'s own
      per-tick campState machine (`TickCampState`) exactly, including:
      the sequential (not if/else-if) `campState` overwrites
      `enterCampState()` itself uses -- default 1, then 2 if
      `safeCampingBuff`, then 3 if a 1-in-10 roll hits (gated on
      character level > 3 and `specialEncounterResolved` still false),
      then unconditionally 2 again if in the hub town, so a hub-town
      camp that also happens to roll into 3 still ends at 2, the hub
      check firing last; campState 3 ALWAYS resolves "disturbed"
      regardless of any roll (the interruption check itself is gated
      `campState != 3`, short-circuiting to skip the roll entirely for
      that case); and a genuinely two-different-counters finding traced
      directly from `Monster.java` (lines ~441 and ~461): `onDeath()`'s
      dropped-item spawnId uses `Item.nextSpawnId()` (this port's
      already-existing `nextDropSpawnId`), but `Monster.spawn()`'s own
      monster spawnId uses a SEPARATE `Monster.nextSpawnIdCounter` --
      so `main.cpp` grew a second, independent `nextMonsterSpawnId`
      counter for this milestone's new live monster-spawn call site
      (the first ever wired into this port; every prior monster spawn
      happened only during up-front world generation).

      `TryEnterCamp`'s own `monsterAttacking` parameter is always passed
      `false` by `main.cpp` -- `GameCanvas.monsterAttacking` is only
      ever set by `Dungeon.tickNearbyMonsters()`'s monster-AI tick loop,
      which isn't wired into this port yet (`CombatResolution::MonsterTick`
      has been fully ported and tested since M15, but nothing calls it
      from `main.cpp`) -- so the "Cannot Camp!" branch is real but
      currently unreachable, the same shape as M32/M34's own documented
      stand-ins. `GameCanvas.suppressMoveInput`'s real effect (skip ONLY
      the movement branch, for exactly the one tick a camp cycle
      resolves) is reproduced as `TickCampState`'s own
      `suppressMoveThisTick` out-parameter, since this port has no
      `pendingMoveDir` queue for the original's own
      `(pendingMoveDir != 0 || suppressMoveInput) && !suppressMoveInput`
      condition (which simplifies to "commit only if a move is pending
      AND not suppressed") to fold into.

      `PlayerState` grew `campState` (folded in, same GameCanvas-static
      reasoning as `chestInSight`/`npcInSight`/`monsterTargeted` -- the
      render step needs it every frame to choose between the camping
      screen and the normal game view). `campStartTime` stays a plain
      `main.cpp` local instead (same "GameCanvas field, not Player's
      own, nothing needs it across a frame boundary" reasoning as
      `lastAttackTimeMs`/`MessagePopupState`). `main.cpp`'s own
      persistent `hotbarContext` local (hoisted in M34) now also
      captures a real, previously-latent quirk for free: since it's
      only ever reassigned inside a tick where `TickCampState` returns
      `runTick == true`, it naturally freezes at its pre-camp value
      while actually camping -- exactly matching the original, where
      `paintHotbar()` (its only writer) never runs during
      `paintCampingScreen()` either.

      `paintCampingScreen()` itself is ported as a genuine SIMPLIFIED
      substitution, not a simplification of logic: a black screen plus
      "CAMPING" centered, using this port's own invented `BitmapFont`
      (see its class comment) in place of the original's own
      `BIG_MESSAGE_FONT` -- another MIDP built-in system font with no
      recoverable real glyph shapes or metrics, the same class of gap
      `SMALL_FONT` already was for M30.

      Verified via the new `camp_tick_smoke.exe` against the real
      37-level generated world: `TryEnterCamp`'s full `campState`
      decision table (including the "Cannot Camp!" short-circuit, the
      hub-town override beating a forced campState-3 roll, and the
      rare campState-3 roll itself, both forced via a probe `JavaRandom`
      seed whose OWN first call lands the exact 1-in-10 hit needed --
      seeded fresh right before the call under test, not reused from
      character creation, which would have already consumed calls off
      it); `TickCampState`'s full timer/interruption/resolution
      behavior for campState 1 (both the "not yet elapsed" and
      "elapsed, not interrupted, transitions to 2" and "elapsed,
      interrupted, resolves fully" cases), 2 (both "not yet elapsed"
      and "elapsed, full rest, resolves"), 3 (always resolves
      regardless of roll), and 0 (pure pass-through); and
      `PlayerCamp::Rest` hand-derived directly against Player.java's own
      arithmetic -- a partial (2/3) rest with ailment 8 active applies
      BOTH scalings in sequence (2/3 then 3/4 of what's left = exactly
      half the missing amount, not some single combined fraction), plus
      the unconditional level-exp-counter zeroing and buff-flag
      clearing. All checks passed. Full clean rebuild zero warnings;
      all 33 smoke tests pass. Also rendered two real frames via a
      temporary diagnostic (a real character with `safeCampingBuff` set
      for a clean campState-2 demo, driven through a fake advancing
      clock rather than a real 5-second wait) confirming the real
      "CAMPING" screen and, after resolution, the normal game view with
      a real "REST COMPLETE!" popup and HP visibly restored to max --
      before the diagnostic was removed.

- [x] **M36 -- the monster AI tick** (this session). Wires
      `Dungeon.tickNearbyMonsters()` itself into the live tick loop --
      not a `dispatchTickActions()` action at all, but `GameCanvas.run()`'s
      OWN per-tick monster-AI pass, called unconditionally the instant
      `runTick` is true (the very first thing inside that block, ahead
      of `hotbarContext`/dispatch), so monsters finally act whether or
      not the player does anything that tick. Every monster registered
      on the player's own current level within Manhattan distance 1-3
      either attacks (distance 1, via `CombatResolution::MonsterTick` --
      fully ported and tested since M15, but never called from
      `main.cpp` until now) or takes a chase-step toward the player
      (distance 2-3, via `MonsterRuntime::Chase`, at most once every 5
      calls per monster). New `CombatTick::TickNearbyMonsters`
      (`combat/combat_tick.h`/`.cpp` -- the same module, not a new one,
      since it already has exactly the right dependency shape: Player +
      Monster + the live registry + the popup this needs) folds in
      `run()`'s own two post-tick checks too: `player.minimapDirty` on
      any chase-step attempt, and a real "Creature attacks!" popup
      (priority 2) on any landed attack.

      SIMPLIFIED, but not lossy: iterates the live `WorldRegistry`'s own
      monster map directly (filtering candidates by distance from each
      entry's own position key, snapshotting the in-range keys into a
      separate list before touching any of them -- mutating a
      `std::unordered_map` while range-iterating it is undefined
      behavior, the same class of bug M34's own test caught and fixed
      for a different map) rather than the original's 7x7 TILE scan
      that only looks the registry up after finding a tile with bit 2
      set -- exactly equivalent, since every registered monster's own
      tile always carries that bit by construction.

      Closed a real gap `MonsterRuntime::Move`'s own doc comment has
      flagged since M15: it already updates a moved monster's tile bits
      but was never able to update a live registry KEY, since no
      registry existed yet. `TickNearbyMonsters` is the first live
      caller of `Chase`/`Move` in the actual tick loop, so it's now the
      one that re-keys a successfully-relocated monster's `WorldRegistry`
      entry (erase the old position, insert at the new one) -- the
      original's Java `Hashtable` needed the equivalent re-`put()` for
      the same reason (`store()`, which `Move()`/`Chase()` never called
      either, for the same "no registry yet" reason at the time).

      A second real, confirmed finding while tracing why `Chase()`'s own
      name is misleading: it never actually pathfinds toward a distant
      player at all in the usual sense -- it's really "attempt one step
      toward the target, preferring the larger-distance axis (ties
      broken randomly), falling back to the other axis if blocked, at
      most once every 5 calls" -- already correctly ported this way
      since M15, just newly annotated once a real live caller made the
      distinction concrete.

      A THIRD real finding, this one resolving a years-old-in-this-
      project mystery: `MonsterState::flag` (`rec[6]` in the packed
      record) was documented since M15 as a "collected/looted marker"
      of unconfirmed meaning, and M25's own `VisibleObjects::MarkLooted`
      had already traced it to really mean "this monster has EVER been
      visible" (permanent once set, confirmed by grepping every real
      write site). This milestone is what that finding was FOR:
      `GameCanvas.paintVisibleObjects()`'s own `monsterAttacking` --
      the flag gating "Cannot Camp!" and the "Creature attacks!"
      message -- turns out to just be `rec[6] != 0` for whatever's in
      any of the 13 `visibleObjects` slots. So despite its name, it
      really means "a monster that has EVER been sighted is somewhere
      in view right now", not "a monster is actively attacking" -- a
      real, surprisingly permissive quirk of the original game, ported
      exactly via the new `VisibleObjects::AnyMonsterAttacking`. Because
      M25's own data model already tracked the "seen" bit correctly,
      this needed no new data, just the derivation. `main.cpp`'s
      `monsterAttacking` local is now real (previously a hardcoded
      `false` at every `CampTick::TryEnterCamp` call site since M32) --
      it's computed at render time alongside `hotbarContext`, with the
      exact same one-tick lag (`paintVisibleObjects()`, like
      `paintHotbar()`, only ever runs as part of `paintGameView()`, so
      both freeze at their pre-camp value while actually camping).

      Also fixed a real, confirmed-stale doc comment found while
      scoping this milestone: `../src/Dungeon.java`'s own header claimed
      `tickNearbyMonsters()` (and the whole class) was "presently
      unreachable from GameCanvas/Player" because `ESGame.dungeons[]`
      was still typed with an old unrenamed class -- true when that
      note was written, but `Player.java`'s own header already recorded
      that gap being closed by a later rename pass (`ESGame.dungeons[]`
      is `Dungeon[]`, `currentDungeon()` returns `Dungeon`); `Dungeon.java`'s
      note just never got updated to match. Corrected in place.

      Verified via the new `monster_ai_tick_smoke.exe` against the real
      37-level generated world: a real pre-placed monster spawn's
      distance-1 attack driven repeatedly (exactly like the real tick
      loop would) until a hit actually lands, checking the
      outcome-independent invariant that matters (hp never increases
      from a monster attack, same M32 philosophy) rather than forcing a
      specific roll; a synthetic distance-2 "chaser" monster's
      guaranteed first-call step attempt (fresh `moveCooldown == 0`)
      confirmed to set `minimapDirty` and leave the registry
      self-consistent (found exactly once, keyed at wherever it now
      claims to be) whether or not the actual step landed; a
      distance-4+ monster confirmed completely untouched; and
      `AnyMonsterAttacking` checked directly against hand-built
      `visibleObjects` slots (unpopulated, monster-with-unset-flag,
      monster-with-set-flag, and a non-monster slot). All checks passed.
      Full clean rebuild zero warnings; all 34 smoke tests pass. Also
      drove a real character to a real landed hit against a real
      spawned monster via a temporary diagnostic (HP 45 -> 39), then
      confirmed the newly-real `monsterAttacking` correctly blocks a
      camp attempt with the real "Cannot Camp!" popup -- the exact
      branch that had been documented as unreachable since M32 --
      before the diagnostic was removed.

- [x] **M37 -- the generic `Screen` UI primitive** (this session). Ports
      `../src/Screen.java` (itself renamed from decompiled/g.java) --
      the one class the original implements EVERY non-3D-view UI screen
      through: main menu, options menu, NPC dialogue prompt lists,
      inventory/shop lists, and `GenericInfoUI`-style message popups, all
      via one `mode`-tagged class rather than a separate class per
      screen type. This is the UI primitive M31/M35's own doc comments
      have been pointing at as the still-missing piece blocking the
      options action and `openNpcDialogue` from being wired live.

      Deliberately a "logic + rendering first, wiring later" slice, the
      same shape as M9/M10's corridor-selection-vs-drawing split and
      M25/M26's visibleObjects-data-model-vs-sprite-drawing split:
      `ui/screen.h`/`.cpp` ports Screen's own self-contained data model
      (`setupList`/`setupMessage`/`setupPromptList`, both overloads),
      pixel rendering (`paint()`, all 4 modes plus the shared soft-key
      bar), and up/down list navigation (`handleKey()`'s game-action-1/6
      branches) -- everything Screen.java implements ITSELF, with no
      dependency on anything outside the class. Deliberately NOT
      ported: `ESGame`'s own navigation graph that CONSTRUCTS real
      Screens for the main menu/options/dialogue/shops and dispatches
      their Select/Cancel/Back commands into real game actions
      (`ESGame.commandAction()`, by far the largest switch in the whole
      decompiled source) -- that's a separate, larger future milestone
      this class exists to eventually be driven by. Needed no new
      library: `Screen` depends on nothing beyond what `dawnstar_render`
      already bundles (`Backbuffer`, M30's `BitmapFont`, and M35's
      `MessagePopup::WordWrap`, reused directly rather than
      re-implemented a second time), so `ui/screen.cpp` is folded
      straight into that library, the same "no separate library needed
      for a self-contained class that needs nothing new" precedent
      `bitmap_font.cpp` itself already set there.

      SIMPLIFIED, fields dropped entirely rather than ported (see
      `ui/screen.h`'s own class comment for the full reasoning): `game`/
      `canvas` (no `ESGame`/`GameCanvas` object exists for a Screen to
      hold a pointer to -- `width()`/`height()` just return this port's
      own fixed 176x208 constants directly); `listener` (no
      `CommandListener`/`ESGame.commandAction()` exists yet to route a
      press into -- `LeftSoftKeyCommand`/`RightSoftKeyCommand` are
      exposed publicly instead, for a future wiring milestone to read
      directly); `backTarget`/`returnDisplay`/`contextIndex`/
      `secondaryParam`/`rawTaggedText` (all four are ESGame's OWN
      navigation bookkeeping Screen itself never reads or writes back,
      per Screen.java's own field doc comments, so there's nothing yet
      for them to attach to). Text rendering uses the one single
      invented `BitmapFont` in place of Screen.java's own 4 distinct,
      unrecoverable MIDP `Font` objects (`TITLE_FONT`/`DEFAULT_TEXT_FONT`/
      `LARGE_TEXT_FONT`/`SOFT_KEY_FONT`) -- same "no real glyph shapes or
      metrics survive" status `SMALL_FONT` already had for M30, and the
      per-row line pitch (12px) reuses `message_popup.cpp`'s own
      already-invented pitch rather than inventing a second one.

      Two real, faithfully-preserved subtleties confirmed while
      transcribing `setupPromptList`'s own per-item word-wrap-splitting
      loop (the mechanism that lets a long list item's own wrapped
      continuation lines group under one logical item for up/down
      navigation): (1) its local `extraLines` (`wrapped.length`) is read
      at its ORIGINAL value exactly once (sizing the merged array and
      copying the wrapped lines in), then decremented exactly once as a
      side effect buried inside a `System.arraycopy` call's own argument
      list -- every later use in that same iteration (the itemGroupStart
      shift, the itemCount increase, the index advance) reads the
      DECREMENTED value instead. Both are correct and intentional (the
      decremented value is the real net "how many NEW rows a 1-item-for-
      N-lines split actually adds"), reproduced exactly rather than
      unified into one consistent variable. (2) `setItems()` (a small
      runtime-reconfiguration method, ported alongside the rest of the
      class per the usual "port the whole self-contained class" standard
      M13/M14/M25 established) never refreshes `itemCount`/`scrollBottom`
      to match a newly-set item list's real size -- only `scrollTop`
      resets to 0 -- so calling it with a different-length replacement
      genuinely leaves those two fields stale until something else (a
      fresh `setup*` call, or `setSelectedIndex`'s own window-adjustment)
      resets them. Not "fixed" here.

      Verified via the new `screen_smoke.exe` against real game data
      (no JVM ground truth, same reason as M6/M9/M11/M13-M36): a real
      5-item main-menu-shaped list and a real 10-item Options-shaped
      list (both from `../src/ESGame.java`'s own real string literals),
      confirming the single-command-always-right-softkey rule and the
      2-command Select/Cancel-vs-Back/Cancel resolution rule, plus
      pixel-level rendering (title bar, item text, the selected row's
      own highlight box, the softkey bar and its labels) cross-checked
      against `BitmapFont::DrawString` used as an independent oracle
      (rendering the same string onto a blank scratch buffer and
      diffing its own "on" pixels against `Screen::Paint`'s output at
      the expected offset, rather than hand-transcribing
      `bitmap_font.cpp`'s own glyph bit table a second time); a real
      `HelpText` body word-wrapped through `SetupMessage` and
      cross-checked against `MessagePopup::WordWrap` directly; a real
      `CharacterData::classNames` prompt list; a synthetic overlong
      prompt-list item (no real prompt-list item in this game is long
      enough to force the word-wrap-splitting branch, the same "real
      data doesn't hit this rare branch, so a synthetic one does"
      precedent M16/M30 already established) verified against an
      independently hand-derived `itemGroupStart`/merged-items
      expectation, including the highlight box correctly spanning every
      one of the split item's own wrapped rows; up/down navigation
      windowing on two real >10/>9-item lists, one through each of the
      two distinct navigation branches (plain `selectedIndex` for
      `SetupList`, `itemGroupStart`-aware for `SetupPromptList`) --
      catching a real, confirmed difference between them along the way
      (the `itemGroupStart` branch's own "does the NEXT item's start row
      still fit" check scrolls the window one step earlier than the
      plain branch's own "has the CURRENT selection outgrown the window"
      check, for lists that otherwise look equivalent); and all of
      `SetTitle`/`SetItems` (including its own stale-itemCount quirk
      above)/`SetSelectedIndex`/`SelectedItemText`/`FirstLine`/
      `SetTextColumn`/`SelectedIndexOrMinusOne`. All checks passed. Full
      clean rebuild zero warnings; all 35 smoke tests pass. Also
      rendered two real frames via a temporary diagnostic (the real
      main-menu-shaped list, and the real Options-shaped list after 2
      real `MoveSelectionDown()` calls) confirming a legible title bar,
      item list, correctly-tracking highlight box, and both softkey
      labels ("Back"/"Select") -- before the diagnostic was removed.

- [x] **M38 -- a real decompiler bug fix, then the real main menu +
      Help/Credits/quit-confirmation navigation** (this session). While
      scoping how `ESGame`'s own screen-wiring loop (M37's own "Milestones
      next" stub) might actually get built, found and fixed a real,
      significant decompiler/rename bug in `../src/ESGame.java` itself:
      `commandAction1()` (by far the largest method in the whole
      decompiled source, ~636 lines) and `handleNPCChoices()` dispatch
      EVERY real screen/command action by comparing `uic.mode` against
      dozens of distinct values (2, 7, 8-17, 20, 22, 27-41, 50-69,
      101-102, 200-206, 305, 353-360, 399, 410, 499, ...) -- but
      `Screen.mode` can only ever be 3/4/5/6 (`Screen.java`'s own
      `paint()` switch, confirmed against every real `new Screen(...)`
      call site in the file), never any of those. Every one of those
      "impossible for `mode`" values matches EXACTLY the `secondaryParam`
      argument passed (or later set via `setSecondaryParam`) at that
      Screen's own real construction site instead -- and `secondaryParam`
      was otherwise read NOWHERE ELSE in the entire codebase before this
      fix (`Screen.java`'s own field doc comment had called it a
      possible "debug/support reference code... not consumed by any
      rendering or input logic traced so far" -- it IS consumed, by
      these two methods, just mislabeled). Fixed by renaming all 50 real
      dispatch-comparison reads from `.mode` to `.secondaryParam` in both
      methods (confirmed nothing else in the file used `.mode` at all,
      so nothing legitimate was touched); re-verified with a full
      standalone `dawnstar/src/` compile (zero errors, same 6
      pre-existing warnings as every prior run, per `src/README.md`'s
      own compile-check procedure). `Screen.java`'s own `secondaryParam`
      field doc comment updated to match -- see both files' own doc
      comments for the full writeup. This is the same class of finding
      as M4's class/race swap and M12's operator-precedence bug: a real
      bug in the decompiled source itself, not something to silently
      route around while porting.

      This finding was essential, not incidental: building a menu
      dispatcher against the WRONG (`.mode`) semantics would have been
      completely broken, since `mainMenuUI`/`OptionsUI`/`helpUI` all
      share `mode == 3` -- selecting anything on any of the three would
      land in the exact same (wrong) branch. With the real semantics
      confirmed, `ui/menu_flow.h`/`.cpp` (`MenuFlow`, a new small,
      purpose-built navigation flow over M37's `Screen` -- NOT a port of
      `commandAction1`'s own giant machinery, which stays future work)
      reproduces the real main menu (`mainMenuUI`, secondaryParam 2),
      its own real Help topic list (`helpUI`, secondaryParam 203, built
      from M8's already-grouped `HelpText` titles/bodies), the shared
      "GenericInfoUI" info-message screen reused for both a selected
      Help topic's own body (secondaryParam 206, always returns to the
      topic list) and Credits (secondaryParam 204, always returns to
      Main Menu, using the real hardcoded credits string transcribed
      directly from `getCreditsString()` -- no data file backs it in the
      original either), and the "Are you sure?" quit confirmation
      (`newConfirmQuitUI`, secondaryParam 202). Wired into `main.cpp`:
      the app now shows this real menu FIRST (a new `inMenu` flag),
      replacing M20's own "always start a fixed character immediately"
      simplification -- "New Game" now constructs the character
      (still M20's own fixed class-0 stand-in; the original's own real
      multi-screen class-selection/name-entry flow, including a raw MIDP
      `TextField` form Screen itself never models, stays unported) only
      once actually selected. New `Window::Close()` (posts a real
      `WM_CLOSE`, the same message the OS's own close button sends) lets
      the quit confirmation actually close the window from inside the
      idle callback.

      A real, faithfully-preserved bug caught while transcribing
      `newConfirmQuitUI`'s own dispatch: it explicitly removes its OWN
      Cancel command (so there's no way to back out of "Are you sure?"
      at all), and its commandAction1 branch calls `exit()`
      UNCONDITIONALLY on Select -- never actually reading
      `selectedIndexOrMinusOne()`. So in the original game, picking
      "No" on the quit confirmation ALSO quits. Reproduced exactly (not
      "fixed" into checking which item was selected) and specifically
      tested with "No" actually selected, not just the untested default
      "Yes".

      Also a real, confirmed quirk `MenuFlow` had to model correctly
      rather than assume away: the original's own `Screen` instances
      are constructed ONCE and reused for every visit, never recreated
      -- so `helpUI`'s own `selectedIndex`/scroll state genuinely
      persists across a Help visit, a Cancel back to the main menu, and
      a later re-visit, matching real MIDP `Displayable` semantics. This
      milestone's own test caught its own wrong assumption here (that
      returning to the main menu resets its selection to index 0) and
      had to switch to forcing the selection back to a known index via
      repeated `OnUp()` calls first, rather than assuming a fresh state.

      Verified via the new `menu_flow_smoke.exe` against the real
      `HelpText` data (M8): the full navigation graph -- New Game (->
      `MenuFlowAction::StartNewGame`), Continue Game (a real, deferred
      no-op), Help -> a real topic -> its own real body -> back to the
      topic list (not Main Menu) -> Cancel back to Main Menu, Credits ->
      its own real body -> Main Menu (not the topic list, confirming the
      shared info screen's own two different hardcoded return targets
      are tracked correctly), and Exit -> the quit confirmation -> EITHER
      "Yes" or "No" actually exiting. All checks passed (after this
      milestone's own test-assumption fix above). Full clean rebuild
      zero warnings; all 36 smoke tests pass; the whole `dawnstar/src/`
      tree re-verified to still compile standalone with zero errors
      after the `secondaryParam` fix. Also rendered five real frames via
      a temporary diagnostic (the real main menu, the real Help topic
      list, a real topic's own body, the real credits text, and the real
      quit confirmation showing only a "Select" softkey with no Back/
      Cancel at all) confirming all five are legible and correct --
      before the diagnostic was removed.

- [x] **M39 -- the real in-game options menu** (this session). Wires
      `ESGame`'s own `OptionsUI` (secondaryParam 31, opened by
      `GameCanvas.openOptionsMenu()` -- the numeric-keypad '7' key,
      remapped to 'O' for a PC keyboard) via a new `ui/options_menu.h`/
      `.cpp` (`OptionsMenu`), the in-game counterpart to M38's own
      `MenuFlow`. A real, confirmed behavior this milestone had to
      reproduce correctly, caught by reading `GameCanvas.run()` itself
      before writing any wiring: whenever any real `Screen` (`activeScreen
      != null`) is showing, the original's own per-tick loop skips
      `dispatchTickActions()` ENTIRELY (not just rendering) -- opening
      this menu genuinely pauses movement/combat/camp/interact/casting,
      the same way M35's camping screen replaces `paintGameView()`
      outright, just one level higher. `main.cpp`'s own new
      `inOptionsMenu` flag is an early-return gate for exactly this
      reason (mirroring `inMenu`'s own shape), not merely an overlay; the
      'O' key itself is captured as a tick-gated pending flag (like
      interact/camp) so it still respects the original's real
      camp-outranks-interact-outranks-cast-outranks-cycle-outranks-attack-
      outranks-options-outranks-move dispatch priority, rather than
      preempting at full frame rate the way the 'M' zoom key does.

      Reproduces, from secondaryParam==31's own real dispatch: "Stats"
      (secondaryParam 32, `Player.buildCharacterSheet()` transcribed
      line-for-line -- name/class/level/HP/Magicka/Fatigue (via the
      already-verified `PlayerCombatStats::EffectiveStat`, M14) /active
      ailments (`HasAilment`)/gift points/all 8 attributes); "Clue Log"
      (secondaryParam 60/61, `ESGame.newClueLogUI()` transcribed exactly
      -- a real, intricate lookup this milestone had to trace carefully:
      a "Rumors" entry pooling any suspect's own revealed rumor steps via
      `Shop.RUMOR_STRING_OFFSET[traitorIndex]`, and 4 named-suspect
      entries each checking 6 topics x 3 "asked" flags against TWO
      parallel offset tables (`UNCONFIRMED_A`/`_B`) -- `_B` (the
      traitor's own admission) only when that suspect IS the player's
      real, hidden `traitorIndex` AND a second confirmation flag
      (`eventFlags[72+...]`) is set, with a `bump` value that resets only
      once per topic-row, not per flag -- all sourced from the real
      npcstrings.dat `ShopDialogue` (M8), which becomes real production
      code for the first time here (previously loaded only by its own
      M8 smoke test); "Help" (reopens the real M8 `HelpText` topic list,
      entered with `helpUI.backTarget = OptionsUI` -- SIMPLIFIED: a
      separate `Screen` instance from M38's own `MenuFlow::helpTopics_`,
      not literally the same shared object the original's single
      `ESGame.helpUI` field is, but provably unobservable in this port's
      own control flow, since nothing here ever lets a player return to
      `MenuFlow`'s own screens once a game has actually started); and
      "Quit Game" (`newConfirmQuitUI`, reusing the exact same real
      "either Yes or No exits" bug M38 already established).

      Deliberately DEFERRED as real, silent no-ops (same "Continue Game"
      precedent M38 established): "Inventory"/"Skills"/"Spells" (each
      needs its own summary-list screen); "Save Game"/"Load Game" (real
      file I/O plus `LoadingScreen`'s own background-thread machinery --
      `PlayerSave` only (de)serializes to/from an in-memory buffer so
      far); "Reveal Traitor" (secondaryParam 68/65/66's own multi-screen
      "who is the traitor?" mini-quiz, which on a correct guess calls
      `grantStarFrostItem()`/sets `newGamePlus`/`ambushTimer`, none of
      which are ported).

      Verified via the new `options_menu_smoke.exe`: the full navigation
      graph (Stats -> Options; each deferred action leaves Options
      showing; Clue Log -> a suspect entry -> Clue Log -> Options; Help
      -> a topic's body -> Help -> Options; Quit Game -> confirmation ->
      Cancel no-op -> "No" still exits) plus 3 independently-recomputed
      Clue Log scenarios (a named suspect via `UNCONFIRMED_A`; "Rumors"
      via `RUMOR_STRING_OFFSET`; the traitor's own admission via
      `UNCONFIRMED_B`) checked against the real npcstrings.dat content,
      using RUMOR_STRING_OFFSET/UNCONFIRMED_A/UNCONFIRMED_B tables
      transcribed AGAIN independently in the test file (not reused from
      options_menu.cpp) so the test isn't just checking its own
      implementation's arithmetic against itself -- the same standard
      M38's own MenuFlow test already set for `secondaryParam` dispatch.
      The real character sheet is checked the same way (an independently
      reconstructed expected string, not calling into options_menu.cpp's
      own internal `BuildCharacterSheet`). All checks passed (after this
      milestone's own test-authoring bug -- forgetting Clue Log's own
      `selectedIndex_` persists across visits just like M38's Main Menu,
      so a stale "Rumors" selection leaked into what was meant to be a
      fresh "Alhavara" scenario -- was caught and fixed). Full clean
      rebuild zero warnings; all 37 smoke tests pass; also rendered 6
      real frames via a temporary diagnostic (Options, Stats, Clue Log,
      a real clue entry, Help, and the quit confirmation) confirming all
      six are legible and correct -- before the diagnostic was removed.

- [x] **M40 -- the real class-selection/name-entry character-creation
      flow** (this session). Wires `ESGame`'s own `newGameUI`
      (secondaryParam 3, a class-selection prompt list), `characterMainUI`
      (secondaryParam 4, "You selected: <class>" -> See Class Info/Create
      Character), the shared "GenericInfoUI" reused for a class-info
      preview (5), the "Character Created!" prompt (6), the real
      name-length error (a MIDP `Alert` in the original -- folded into
      the same Info-screen-reuse pattern, since `Alert` has no real
      custom `paint()` to distinguish from a plain message + Ok either),
      and the 3-screen "Welcome"/"Introduction" chain (7/101/102) via a
      new `ui/character_creation_flow.h`/`.cpp` (`CharacterCreationFlow`)
      -- the same "small, purpose-built flow over Screen" shape M38/M39
      already established, not a port of `commandAction1`'s own giant
      machinery. `MenuFlow`'s own "New Game" now hands off here instead
      of immediately constructing M20's fixed "class 0, Traveler"
      stand-in -- a real, player-chosen/named character is used from
      here on.

      Also a new `ui/name_entry.h`/`.cpp` (`NameEntry`): the real
      `charNameTextForm`'s counterpart -- a raw MIDP `Form` + single
      `TextField(null, null, 10, 0)`, the ONLY real `TextField` anywhere
      in the whole decompiled source (confirmed by grep), so this is a
      small, purpose-built widget for that one real use, not a generic
      `Form`/`TextField` port. A real `Form` has NO custom `paint()` of
      its own at all (unlike `Screen`) -- its entire look was rendered
      by the phone's own MIDP implementation, so `NameEntry::Render()`
      is a deliberately invented presentation (reusing `Screen`'s own
      color palette for visual consistency), the same unrecoverable-
      system-UI status `graphics/bitmap_font.h`'s own invented glyphs
      already have. Accepts exactly what `BitmapFont` can render (space/
      '/-/! and A-Z/0-9), not the real `TextField`'s literal
      any-character constraint -- a practical, harmless restriction
      since no real string anywhere in this game's own data needs
      anything outside that set either.

      A real design point checked by reading `createNewGame()` (the
      background-thread body a real device ran while character-creation's
      own `createGameUI` `LoadingScreen` spun) before writing any of this:
      it does nothing but null out now-unneeded UI fields (memory
      pressure relief on a real MIDP device, irrelevant here) and
      re-load `Shop`'s dialogue table (already loaded once at this
      port's own startup, so redundant here too) before immediately
      showing the next screen -- no actual asynchronous work, unlike
      Save/Load's own real file I/O (still unported). So this milestone
      skips modeling that loading screen/background-thread machinery
      entirely, going straight from a confirmed name to "Welcome",
      functionally identical to what a modern runtime would do anyway.

      Also checked before writing any code: the original's own real
      character-construction TIMING. A live (but not yet `resetState`d)
      `Player` object exists from the moment a class is picked (for "See
      Class Info"'s own preview), but starting items / the hidden
      `traitorIndex` aren't rolled until `resetState(false)`/
      `grantStartingItems`, called only once, right before entering
      gameplay -- NOT at class-selection time. `CharacterCreationFlow`
      reproduces that exact shape with a dedicated, throwaway
      `previewRng_` for the "See Class Info" preview only (`Player.java`'s
      own `buildCreationSummary()`, now also ported as
      `PlayerCreation::BuildCreationSummary` -- real Player.java gameplay
      logic, so it lives in `player/player_creation.h`, not the UI file);
      the REAL, final `PlayerState` is only ever constructed once, in
      `main.cpp`, once `CharacterCreationAction::StartGame` actually
      fires -- with the player's own real chosen class and typed name,
      not M20/M38's fixed stand-in.

      Verified via the new `character_creation_flow_smoke.exe`: the full
      navigation graph (class select -> "You selected" -> See Class Info
      -> back -> Create Character -> name entry -> a too-short name's
      real error -> retry -> a valid name -> Welcome -> Introduction ->
      a second Introduction screen -> `StartGame`, plus Cancel from
      "You selected" back to class select and Cancel from class select
      requesting the main menu) using an independently-reconstructed
      `buildCreationSummary()` expectation (built from a SEPARATE preview
      character with a different rng seed than the flow's own internal
      one -- valid because the summary reads only deterministic,
      class-template-derived fields, not the rng-derived ones) and the
      real npcstrings.dat introduction text. Also checked `NameEntry`'s
      own real behavior directly: an unsupported character and lowercase
      input are silently ignored, typed text truncates at the real
      10-character max, Backspace on empty text is a no-op, and a
      too-short name's own error screen preserves the typed text (a real
      `TextField` is never cleared on that error) rather than assuming a
      reset. All checks passed. Full clean rebuild zero warnings; all 38
      smoke tests pass. Also rendered 9 real frames via a temporary
      diagnostic (class select, "You selected: Knight", the class-info
      preview, "Character Created!", the empty and typed name-entry
      widget, and all 3 Welcome/Introduction screens with real story
      text) confirming all nine are legible and correct -- before the
      diagnostic was removed.

- [x] **M41 -- the real Options-menu "Inventory"/"Skills"/"Spells"
      actions** (this session). Turns M39's three deferred no-ops into
      the real thing: `ESGame`'s own `InventoryUI`/`InventoryItemUI`
      (secondaryParam 33/34 -- the item list, and an item's own tooltip
      plus a dynamic Drop/[Equip-or-Unequip]/[Learn]/[Use] action list,
      built in the exact conditional order `newInventoryItemUI()`/its own
      dispatch use, so a selected index always maps back to the right
      action), `SkillsListUI` (secondaryParam 35/36 -- a rank>0 skill
      list, each entry's tooltip reusing the shared Info screen the same
      way Stats/Clue Log/Help already do), and `SpellsListUI`/
      `SpellInfoUI` (secondaryParam 37/38 -- a known-spell list, each
      entry's own tooltip plus a real "Ready Spell" prompt that sets
      `selectedSpellId`). All added directly to `ui/options_menu.h`/
      `.cpp` -- `OptionsMenu` grew 5 new `Screen` members (each REBUILT
      FRESH on every real (re)entry, unlike `options_`/`clueLog_`/
      `helpTopics_`, which are each a single persistent Screen instance
      the original itself never recreates -- see options_menu.h's own
      class comment on why that distinction actually matters here, the
      first time a *selectable* list's own stale `selectedIndex_` would
      otherwise leak across visits).

      Needed three small additions to already-existing modules rather
      than new ones: `PlayerInventory::ItemTooltip`/`DropInventoryItem`
      (`player/player_inventory.h`/`.cpp` -- `dropInventoryItem()` was
      deferred all the way from M18 pending a live dropped-item registry,
      which M22/M24's `DungeonRuntime`/`WorldRegistry` have supplied since;
      a real, faithfully-preserved quirk: dropping StarFrost, id 101,
      removes it from the inventory but never actually places it on the
      ground, see `DropInventoryItem`'s own doc comment) and
      `PlayerCombatStats::KnownSkillsSummary`/`NthKnownSkillIndex`/
      `SkillTooltip` (`player/player_combat_stats.h`/`.cpp` -- the Skills
      screen's own real logic; nothing was missing on the Spells side,
      `PlayerSpellcasting::KnownSpellsSummary`/`NthKnownSpellId`/
      `SpellTooltip` already existed from M16).

      A real architectural wrinkle, not a simplification: "Use" (an
      inventory item's 87-99 "gift" action) is the one thing
      `OptionsMenu::OnSelect` can't finish by itself -- `CombatResolution::
      UseItem` lives in `dawnstar_combat`, which already depends on
      `dawnstar_render` (for its own message-popup calls), so
      `dawnstar_render` (where `OptionsMenu` lives) linking back against
      `dawnstar_combat` would create a cycle. `OnSelect` stops short and
      returns a new `OptionsMenuAction::UseInventoryItem` instead;
      `main.cpp` (which already links both libraries) performs the real
      `UseItem` call itself (re-deriving the front monster fresh via
      `PlayerMovement::MonsterInFront`, same "SIMPLIFIED but not lossy"
      reasoning `combat/combat_tick.cpp`'s own `ProcessAttack`/
      `ProcessSpellCast` already established), then calls the new
      `OptionsMenu::FinishUseItem` to run the shared post-action tail --
      the same "return what happened, let main.cpp perform the actual
      real-world effect" shape `MenuFlowAction`/`CharacterCreationAction`
      already established, just with one extra round-trip. That tail
      itself preserves a real, easy-to-miss quirk: using item 87 ("Warp
      to Camp") from the Inventory screen sets `suppressStrafeAdjust`,
      which the original routes straight back to the GAME VIEW, not back
      to the Inventory list at all -- reproduced exactly via
      `FinishInventoryItemAction`'s own branch.

      Verified via the new `inventory_skills_spells_smoke.exe` against a
      real Sorcerer character (M11), real `ItemDatabase`/`SpellDatabase`
      data, and a real generated world: the full Inventory navigation
      graph (list -> an item's own tooltip+actions -> Cancel chain back
      to Options); a real Equip/Unequip round trip on whichever starting
      slot is actually gated (found by scanning, not assumed); a real
      Drop landing in the live `WorldRegistry` at the player's own tile,
      plus the StarFrost-never-lands-on-the-ground quirk; a real Learn
      round trip (an engineered category-12 scroll, gated on a real
      skill-rank check); both real Use outcomes (item 87's
      suppressStrafeAdjust routing to the game view, and item 96's
      "Safe Camping" NOT being consumed -- a quirk M18 already
      established); and the full Skills/Spells navigation graphs,
      including "Ready Spell" actually updating `selectedSpellId` and the
      freshly-rebuilt Spells list showing the real "R: " prefix
      afterward. Every expected value is independently re-derived from
      `../src/Player.java` directly (not reused from
      `player_inventory.cpp`/`player_combat_stats.cpp`/
      `options_menu.cpp`), the same standard M38-M40's own tests already
      hold to. M39's own `options_menu_smoke.cpp` was updated for
      `OnSelect`'s grown signature and to drop Inventory/Skills/Spells
      from its "still a deferred no-op" list (only Save Game/Load
      Game/Reveal Traitor remain there). All checks passed (both test
      files). Full clean rebuild zero warnings; all 39 smoke tests pass.
      Also rendered 7 real frames via a temporary diagnostic (Options,
      Inventory, an Item screen, Skills, Skill Info, Spells, and Spell
      Info) confirming all seven are legible and correct -- before the
      diagnostic was removed.

- [x] **M42 -- the real Options-menu "Save Game"/"Load Game" actions**
      (this session). Turns the two remaining M39 deferred no-ops into the
      real thing: `ESGame`'s own `saveGameState()`/`loadGameState()` and
      every helper those two call -- `writeMasterListsToRecordStore()`,
      `readMasterListRecords()`, `writeOtherStateInfoToBytes()`,
      `readOtherStateInfo()`, `maxWriteSize()`, `getRSNameNotInUse()`,
      `getLastGoodRSName()`, `cleanupRecordStores()`, `resumeGame()`,
      `openAndRepopulateDungeons()`, and `getGameAdvancementLevel()` --
      plus `LoadingScreen.java`'s own modes 8-11 progress bars that the
      original shows while they run.

      Two new modules. `save/game_save.h`/`.cpp` (a new `dawnstar_save`
      CMake library, linking `dawnstar_player` for M13's already-real
      `PlayerSave::ToBytes`/`FromBytes` and pulling `WorldRegistry`/
      `DungeonRuntime::RefreshTileFlags`/`MonsterRuntime`'s own byte codecs
      transitively; nothing links back against it, so no cycle) and
      `ui/loading_screen.h`/`.cpp` (added to the existing `dawnstar_render`,
      needing nothing beyond the `Backbuffer`/`BitmapFont` already there).

      THE RECORD-STORE SUBSTITUTION -- a port decision, not a
      simplification of anything recoverable: the original persists through
      MIDP's `javax.microedition.rms.RecordStore`, a phone-private database
      of named stores each holding an ordered list of opaque `byte[]`
      records, with `listRecordStores()`/`getLastModified()`/
      `deleteRecordStore()` and no filesystem at all. This port has no RMS,
      so one store == one plain file in a caller-supplied directory and one
      record == a 4-byte big-endian length prefix followed by that many raw
      bytes (`SerializeRecordStore`/`DeserializeRecordStore`, a format this
      port defines rather than recovers, kept private in an anonymous
      namespace). That substitutes only the unreachable container layer:
      the record ORDER (`maxWriteSize`'s own record first, then the 37
      `es_ML_<n>` monster-list records in ascending level order then the 37
      `es_CHEST_ML_<n>` chest-list records, 111 in all), each record's own
      CONTENT (the 38-byte other-state record's exact field order/width;
      `Monster.writeTo()`'s own 28-byte layout; M23's own chest and M24's
      own dropped-item codecs reused as-is, not re-derived), the single
      512-byte `Player.toBytes()` record, the `Util.randomInt(10000)`
      1..10000-inclusive naming re-roll, `getLastModified()`'s own
      newest-wins strict `>` tie-break, and `cleanupRecordStores()`'s own
      keep-only-the-newest single-save-slot behavior are all preserved.
      `Item.nextSpawnId`/`Monster.nextSpawnIdCounter` are saved and loaded
      faithfully, but nothing in this port advances them yet (the modules
      that would take their spawn id as an explicit parameter instead --
      see `OtherStateInfo`'s own comment), so that pair is real format
      fidelity whose live wiring arrives with whichever milestone ports
      those call sites.

      `OtherStateInfo` is the one place a still-unported class shows up:
      `Shop.java` itself has no counterpart here yet, so the 26 `Shop.*`
      statics the original persists ride along as one flat struct. That's a
      container decision, not a format simplification -- every one of the 26
      is written and read back in the exact order/width the original uses.
      Its `Reset()` reproduces `Shop.reset()`'s own post-condition (all 9
      `firstVisit` entries true, everything else zero/false) rather than
      value-initializing to all-false, and main.cpp calls it both at startup
      and after `CreateCharacter` -- because that function's own tail is
      `Player.resetState()`, whose last act is `Shop.reset()`
      (`../src/Player.java` line 2576).

      `LoadingScreen` ports ONLY modes 8-11, the plain "<action>... Please
      Wait" bars, which is the whole of what Save/Load actually use. Modes
      1/2 and everything that comes with them are DELIBERATELY not ported:
      `runSplashSequence()`'s own startup timing loop, `startThread()`/
      `stopThread()`/`run()`/`waitAtLeast()`, and `renderSplash()` -- that
      whole sequence is `ESGame`'s own boot flow, which this port doesn't
      reproduce (main.cpp goes straight to M38's own `MenuFlow`), and it
      needs four images this port never loads plus `ESGame.copyString`.
      It's also a standalone class rather than a `Screen` subclass as in the
      original: mode 8-11's own `renderProgress()` reads NOTHING from
      Screen's state -- no title, no items, no soft-key commands, no scroll
      position -- so inheriting Screen's ~20 fields would carry nothing but
      dead weight.

      Two real, easy-to-miss quirks reproduced exactly, both in the failure
      paths `ESGame.run()`'s own helperThreadState==5/6 else-branches show:
      a FAILED SAVE shows `GenericInfoUI` at secondaryParam 499, whose own
      Ok dispatch is an unconditional `this.exit()` -- pressing Ok on
      "Save Error" EXITS THE GAME, so `OptionsMenu::OnSelect` returns
      `OptionsMenuAction::Exit` rather than returning to whatever screen
      last opened the shared info Screen (which is what `Active::Info`
      would do, and what 499's own dispatch deliberately does NOT). And a
      FAILED LOAD's own `noSavedGameUI` message says "Press OK to return to
      main menu" while its secondaryParam==305 dispatch actually returns to
      the OPTIONS menu -- because secondaryParam==31's own case 6 set that
      `backTarget` to `OptionsUI` right before showing the LoadingScreen
      (the main-menu Load Game path, unported here, is the one that sets it
      to `mainMenuUI`, at `ESGame.java`'s own line 555). `OptionsMenu` grew
      exactly two new states for these: `Active::SaveError` reusing the SAME
      shared `info_` Screen Stats/Clue Log/Help already use (`ESGame` has
      exactly one `GenericInfoUI`), and `Active::NoSavedGame` on a new
      persistent `noSavedGame_` member -- persistent, not `Rebuild*`-style
      fresh, because `allocateAllUIs()` builds it once with a fixed message.

      Same ESGame-level-not-Screen-level split M41's `UseInventoryItem`
      established: `OnSelect` stops short and returns the new
      `OptionsMenuAction::SaveGame`/`LoadGame`, and main.cpp performs the
      real work itself -- driving `GameSave::SaveGameState`/`LoadGameState`
      (then `ResumeGame`, which the original calls from `run()` right AFTER
      `loadGameState()` returns true, not from inside it) while presenting
      the LoadingScreen at every reported percent. That synchronous
      render-and-present-per-percent callback is this port's stand-in for
      the original's own background `Thread` + `repaint()`/
      `serviceRepaints()` pair; the LoadingScreen swap itself is likewise
      main.cpp's job, which is why the Options list is still what's showing
      immediately after `OnSelect` returns.

      Verified via the new `game_save_smoke.exe` (88 checks across eight
      sections) against a real generated 37-level world (540 monsters, 180
      chests registered), a real Sorcerer character (M11), and real
      `dungeon.dat`/`npcstrings.dat` data: the 38-byte other-state record's
      exact layout re-encoded independently; `maxWriteSize()` over that real
      world; the full 111-record store layout and both master-list write
      loops; `saveGameState()`/`loadGameState()` end to end through real
      files, including the round-trip of all 540 monsters/180 chests, the
      single-save-slot cleanup, and every failure path (no store at all, a
      truncated file, a garbage file, a bad monster `dungeonLevel`);
      `getRSNameNotInUse()`'s own re-roll against a twin `JavaRandom`
      seeded identically; and the original's own hardcoded 1500-byte read
      buffer, measured against the real world's largest actual record (424
      bytes for a monster list) to confirm the original could load this
      save and to pin down what overflow would actually take (54+ monsters
      on one level). `LoadingScreen`'s own `renderProgress()` is checked
      pixel-exactly for all four modes x three percents, and the two new
      Options screens for their real titles, their mode-4 no-op Cancel, and
      their two divergent Ok behaviors. Every expected value is
      independently re-derived from `../src/ESGame.java`, `../src/Monster.
      java`, and `../src/LoadingScreen.java` directly -- with this file's
      own independent big-endian encoders, so the test can't merely be
      checking the implementation's arithmetic against itself -- the same
      standard M38-M41's own tests hold to. M39's own
      `options_menu_smoke.cpp` was updated to drop Save Game/Load Game from
      its "still a deferred no-op" list, leaving only "Reveal Traitor"
      there. All checks passed (both test files). Full clean rebuild zero
      warnings; all 40 smoke tests pass. Also rendered 7 real frames via a
      temporary diagnostic (all four LoadingScreen modes at several
      percents, plus both failure screens) and verified each
      programmatically -- the 2510210 background, the action line centered
      at y=30, "Please Wait" at y=45, the white 90x20 outline box at
      x=43..132/y=60..79, the blue bar inset 1px at y=61..78 with width
      exactly `percent * 88 / 100` under C++ integer division, and both
      message screens' own centered titles, 12px-pitch wrapped body lines
      and soft-key bars -- before the diagnostic was removed.

- [x] **M43 -- the real "Reveal Traitor" mini-quiz** (this session). The
      last deferred no-op in the Options menu is now real, and that list
      is EMPTY. `ui/options_menu.h`/`.cpp` grew the full
      `../src/ESGame.java` secondaryParam chain: 31's own case 8
      (`GenericInfoUI` 68, `setupMessage("Reveal Traitor",
      Shop.dialogue[9][66])`), 68's own Ok (any command in the original;
      only Ok is attached -- `newRevealUI()`'s mode-5 Yes/No prompt over
      `dialogue[9][67]`, with `removeCommand(cancelCommand)` so Cancel is
      a real no-op, the same "no way to back out" quirk as the quit
      confirmation), 65's own Select ("Yes" -> `newRevealWhomUI()`'s
      "Who is the Traitor?" over the 4 suspect names; "No" -> backTarget
      OptionsUI), 66's own Select (the guess itself), and 67's own Ok
      (`ambushTimer = 1`, `specialEncounterResolved = true`, back to
      `gameCanvas` -- again no command check; only Ok is attached). The
      66 result message is ALWAYS `dialogue[9][68]+"\n"+
      dialogue[9][69]+"\n"` plus a guess-dependent third line
      (`dialogue[9][70]` correct, `Util.replace`-substituted
      `dialogue[9][72]` wrong, naming the REAL traitor via
      `Shop.NAMES[5+traitorIndex]` -- the quiz tells you who it actually
      was).

      The real state changes: a correct guess sets `Player.newGamePlus`
      and calls `grantStarFrostItem()` -- ported as
      `PlayerInventory::GrantStarFrostItem` (`player/player_inventory.h`)
      alongside the other inventory slot management, since that's exactly
      what it is: set `starFrostBonusActive` (whose +4 `SkillValue` reader
      has existed since M14), take one spawn id from the live item
      counter, `AddItem(100 /* StarFrost */, spawnId, 0)`; on a full
      inventory, evict -- the FIRST id-87 slot wins outright
      (Player.java's own `break`, before any cheaper item is even
      considered), else the non-equipped slot with the lowest positive
      `Item.column(5,...)` sell price, then retry with the SAME spawnId.
      One real edge guarded defensively rather than ported literally:
      with every slot equipped/zero-priced Player.java's evictSlot stays
      -1 and `removeInventorySlot(-1)` would throw
      ArrayIndexOutOfBoundsException (unreachable in practice -- at most
      ~7 of 24 slots can be equipped); the port skips the removal instead
      and the retry add simply fails, same "C++ has no exceptions safety
      net" precedent as `PlayerMovement`'s no-neighbor-edge guard.
      `PlayerState` gained `newGamePlus`/`ambushTimer` (both transient:
      neither is in either save format, so a save/load cycle silently
      loses them -- and `starFrostBonusActive` too, while the StarFrost
      item itself survives in the serialized inventory).

      **A real, preserved quirk worth calling out:** either way --
      correct or wrong -- the 66 dispatch then runs
      `character.resetToHubPosition(false)` OUTSIDE its if/else, so even
      a WRONG guess yanks the player back to the hub and tells them who
      the real traitor was.

      **A doc correction, not a "fix":** CLASS_MAP.md had resolved the
      ambush system ("`Player.ambushTimer >= 0`, was `Q >= 0`") as DEAD
      code -- "nowhere in the entire codebase is ambushTimer ever
      assigned anything other than its -1 field initializer". That
      resolution was wrong even for the original: `ESGame.java`'s own
      secondaryParam==67 branch assigns `ambushTimer = 1`, and the
      M38-era decompile made the branch look unreachable only because the
      dispatch then read the mis-renamed `.mode`. With M38's
      `.mode`->`.secondaryParam` fix the branch is reachable as written,
      so M43 also corrects that CLASS_MAP.md bullet. The armed ambush
      still has no consumer in this port yet --
      `GameCanvas.tickPerSecond`'s whole once-per-second passive tick
      (regen/drain, effect countdowns, and the ambush spawner with its
      two `newGamePlus`-selected checkpoint schedules, culminating in the
      type-42 end-game monster at elapsed second 140) remains its own
      future milestone; M43 sets the fields and documents exactly what
      will read them.

      Wiring: `OptionsMenu::OnSelect` grew an `int16_t& nextItemSpawnId`
      parameter (main.cpp passes its own `nextDropSpawnId`,
      `Item.nextSpawnId()`'s stand-in, so the StarFrost grant draws from
      the same live counter combat's death drops already use -- the same
      grown-signature reasoning as M41's own additions); the
      `Util.replace` first-occurrence-only semantics got a small
      file-local helper in options_menu.cpp (its first real ported call
      site); and the 4 suspect names became one shared `kSuspectNames`
      table used by both the Clue Log and the quiz. The saved-copy
      spawn-id counters in `OtherStateInfo` remain un-synced with the
      live counters, unchanged from M42's documented state (that wiring
      still arrives with whichever milestone takes it up).

      Verified via the new `reveal_traitor_smoke.exe` (60+ checks across
      five sections) against a real generated 37-level world, a real
      character (M11), and real `npcstrings.dat`/`itemsin.dat` data: the
      full navigation graph (intro title+text, the Yes/No prompt with its
      removed-Cancel no-op, "No" -> Options, fresh-screen "Yes" -> the
      4-name list, Cancel -> Options) pixel-checked via the M37/M39
      BitmapFont oracle; a wrong guess against an independently
      re-derived expected message and state (no newGamePlus, no grant,
      no ambush arming -- yet the hub teleport still happens); a correct
      guess (newGamePlus, the immediate +4 via M14's own `SkillValue`,
      id-100 with `(spawnId<<16)+0` data at the first free slot, counter
      advanced, the `dialogue[9][70]` line, then the 67 tail arming
      `ambushTimer=1`/`specialEncounterResolved` and returning to the
      game); the transient-fields save/load quirk through a real
      `PlayerSave::ToBytes`/`FromBytes` round trip; and
      `GrantStarFrostItem`'s eviction math driven directly on scratch
      inventories, with every expected eviction re-derived from
      Player.java's own loop against the real sell prices (free-slot
      append, lowest-value eviction, the id-87 break, and the
      all-equipped guarded edge). M39's own test was updated to drop
      "Reveal Traitor" from its deferred-no-op section (it now verifies
      the entry/exit navigation only, deep checks in M43's own file).
      All checks passed (all four affected test files). Full clean
      rebuild zero warnings; all 41 smoke tests pass; `dawnstar_port.exe`
      launches and stays up.

- [x] **M44 -- run()'s timed tail: tickStatusCountdowns + tickPerSecond,
      the now-live ambush spawner, and the real Game Over chain** (this
      session). `passive/passive_tick.h`/`.cpp` (a new `dawnstar_passive`
      library, the same "own small module, no cycle" shape as
      combat/interact/camp) ports the whole timed tail of
      `../src/GameCanvas.java`'s run() loop (lines ~1407-1420), which the
      port until now had no counterpart of at all -- no per-second
      anything existed in main.cpp.

      `PassiveTick::TickStatusCountdowns` ports tickStatusCountdowns():
      the per-iteration millisecond countdown of the 3 timed ailment
      timers (trollThirst/glacierCurse/terrified, bits 3/4/6 -- M16's
      combat ailment infliction sets them to 30000ms), Terrified gated on
      `monsterAttacking`.

      `PassiveTick::TickPerSecond` ports tickPerSecond() block by block:
      Troll Thirst's UNCLAMPED per-second HP drain (2*maxHP/100, HP can
      go negative -- the death system's business, not this tick's);
      Glacier Curse's real quirk where Magicka regen of maxMagicka/10
      OVERFLOWS into a reset-to-0 plus an HP drain instead of clamping;
      the effectDurations[] per-second countdown, whose one special case
      (index 5, effect 6 "Safe Camping" expiring) removes the item-101
      StarFrost via the new `PlayerInventory::FindSlotOf` (Player.java's
      findInventorySlotOf); and the ambush spawner M43 armed -- both
      `newGamePlus`-selected checkpoint schedules, elapsed 140 always
      spawning the literal type-42 end-game monster, the exact
      `1+Util.randomInt(17)` X-then-Y retry-roll order, the "Enemy
      arrived!" popup, and the >5-monsters-per-level Game Over trigger.

      **Two real original-game findings, preserved as found rather than
      "fixed" and confirmed against the source:**
      - tickPerSecond's per-level scratch-cooldown block (lines
        1872-1888) is a genuine NO-OP: it decodes every record into a
        throwaway `Monster` copy and decrements THAT copy's cooldown
        bytes, never storing the mutated record back -- so no monster's
        cooldown is ever actually decremented by this tick (compare M36's
        real store-backed tick). Documented in passive_tick.cpp's own
        comment; nothing observable to port.
      - tickStatusCountdowns's expiry arms (`Util.setBit(3/4/6, ...)`)
        write the SAME bits their own `hasAilment(4)/(5)/(7)` branch
        conditions already require (Util.setBit is 0-based, hasAilment(n)
        reads bit n-1) -- idempotent no-ops, so the three timed ailments
        never expire on their own; the countdown's only real effect is
        zeroing the timers. The earlier "applies the matching debuff bit
        once each expires" reading (including the renamed source's own
        comment) was wrong; the `../src/GameCanvas.java` comment is
        corrected too, and the redundant bit-ORs are kept in the port
        exactly as written.
      Plus one more small one: `findInventorySlotOf(101)` only ever
      matches the EQUIPPED (negative-encoded) form, so an unequipped
      StarFrost survives the effect-6 expiry -- preserved (and now
      checked in the test).

      **The Game Over chain, real at last:** TickPerSecond's
      `PerSecondResult::EndOfGame` (a checkpoint spawn pushing the level
      past 5 monsters) makes main.cpp play ESGame's own
      `endOfGameUI = newGameOverUI()` + `setCurrentDisplay` -- the real
      mode-4 "Game Over" Screen with dialogue[9][73]'s <TAG> substituted
      by the real traitor's name, then (any command -- the original's
      200/201 dispatch has no command check) the "Exiting" Screen
      (GenericInfoUI 399, the concatenated `ESGame.copyString` notice,
      its only command swapped from Ok to Exit), then `exit()`.
      `util/text.h` is new, holding `ReplaceFirstTag` (Util.replace,
      promoted from options_menu.cpp's former file-local copy once two
      consumers existed) and `ESGame.copyString` (kCopyStringParts +
      the concatenation idiom) for this chain and the future boot
      splash. Like every other Screen display, the Game Over early-return
      genuinely PAUSES the whole tick loop -- matching the original's
      `activeScreen != null` branch, which stops tickPerSecond itself
      (the ambush clock freezes while the screen shows).

      main.cpp's loop grew the tail itself: `elapsed` computed against
      the previous tick's `now` (run()'s own prevNow idiom),
      TickStatusCountdowns every tick, then the `secondAccum`
      accumulation and -- past 1000ms -- TickPerSecond, OUTSIDE the
      camp gate exactly like the original's own position outside its
      `if (runTick)`, so the ambush clock (and the ailment countdowns)
      keep running while camping too.

      Verified via the new `passive_tick_smoke.exe` (40+ checks across
      six sections) against the real generated 37-level world and real
      data: the three timers' countdown/clamp/gate/idempotency; the two
      ailment drain blocks (including the negative-HP and
      overflow-resets-Magicka quirks); the effect countdown with both
      the equipped-StarFrost removal and the unequipped-survives quirk;
      the scratch no-op (a hand-built "live" record provably unchanged);
      and the ambush -- inactive-timer gate, the two schedules
      independently transcribed and proven different (elapsed 5
      non-NGP-only, elapsed 3 NGP-only), elapsed 140's literal type-42
      spawn, the popup, the mutually-exclusive EndOfGame branch, and the
      exact landing tile AND rolled monster type predicted by a
      twin-seeded replay of the original's own roll order (m22's oracle
      technique). Full clean rebuild zero warnings; all 42 smoke tests
      pass; `dawnstar_port.exe` launches and stays up.

- [x] **M45 -- NPC dialogue: `Shop.java`'s own static state + the real
      `dialogue()` line-selection logic** (this session). A new
      `dawnstar_npc` module (`port/src/npc/shop_interaction.h`/`.cpp`)
      ports `Shop.java` in full: `ShopState` (firstVisit[9]/
      questState1[4]/questState2[4]/interactionCount[4]/rewardsGiven[4]/
      showDeathGreeting -- the live counterpart of the exact same 26
      values M42's `OtherStateInfo` has carried as an inert flat
      container since before this class existed) and `ShopInteraction`
      (NAMES/SHOP_CATEGORY/SHOP_X/SHOP_Y/SHOP_STOCK/
      RUMOR_STRING_OFFSET, isNamedShop/isGenericPeddler/hubShopAt/
      questFlagsFor/clearQuestTurnInState/rumorFor, the single
      `dialogue()` dispatcher covering every action -- greet, buy, sell,
      the named shopkeepers' 2-stage quest-turn-in/reward/rumor chain,
      and Jakar's rumor-reveal/cure/camp-warp/heal branches -- plus the
      declared-but-no-confirmed-caller `isValidShopAction`/
      `shopActionCode` pair, ported anyway for a complete class, same
      precedent as M15/M16's own unused-field/quirk ports). This is the
      gap M8's `ShopDialogue` (raw npcstrings.dat text only) and M34's
      `InteractTick` (whose npcInSight branch was a documented no-op
      stand-in) were both left with.

      Wired for real: `InteractTick::ProcessInteract`'s npcInSight
      branch now calls `ShopInteraction::Dialogue(..., action=1, extra=0)`
      -- `GameCanvas.openNpcDialogue()`'s own call -- and returns the
      result for main.cpp to show as a blocking message screen (a new
      `inNpcDialogue`/`npcDialogueScreen`, the same
      early-return-pauses-the-tick-loop shape `inOptionsMenu`/
      `inGameOver` already established). NOT reproduced: the real game's
      own Ok dispatch on that popup always proceeds into
      `NPCChoicesUI[shopId]`, a full buy/sell/quest-turn-in/rumor-question
      list-menu this port has no counterpart for yet -- dismissing this
      port's popup just returns to the game instead, the same
      "the branch exists, but does less than the original until its own
      UI milestone lands" precedent M32's monsterType-42 end-of-game-UI
      skip already set. That whole interactive menu -- and wiring
      `ShopState` into `OtherStateInfo` so a save/load round-trip
      actually carries it -- is the still-open "shops" milestone
      `ShopInteraction`'s own actions 2-5/8/10-15 are now ready for.

      Small shared pieces promoted along the way, same
      reuse-over-duplication precedent M16/M17/M43 already set:
      `PlayerInventory::AddGold` (a one-line `gold += amount`, deferred
      since M18 as "nothing needs it yet" -- buy/sell both do now);
      `util/game_advancement.h`'s `GetGameAdvancementLevel` (Jakar's own
      greeting needs the same 0-5 advancement bucket `save/game_save.h`'s
      `GameSave::GetGameAdvancementLevel` already computed -- that method
      is now a one-line forwarder to the shared free function rather than
      a second copy of the formula); `util/text.h`'s `ReplaceFirstTag`
      grew the `values[]` overload (`Util.replace(source, tag, String[])`
      -- calls the single-value version once per entry in order,
      `rumorFor`'s "asked again" phrasing needs it for 3 substitutions in
      one template). `main.cpp`'s own pre-existing `kShopNames` array (a
      duplicate of `Shop.NAMES` that predates this milestone, M30's own
      shop-greeting popup) was deleted in favor of the new canonical
      `ShopInteraction::kNames`.

      **Three real findings, all preserved/ported exactly:**
      1. `Shop.java`'s own switch has NO `break` between the shopId
         5-8 case group and the shopId 4 case that follows it: with
         shopId 5-8 and action==8 specifically, the if/else-if chain's
         final `else if (action != 8) return "quack";` is false, so
         nothing returns and control falls straight into shop 4's own
         if/else chain, evaluated with the ORIGINAL shopId (5-8) still in
         scope. Traced by hand: every branch of shop 4's chain for
         action==8 (not 1/10/11/12/13) bottoms out at its own
         `return null` with no side effects, so this port returns
         `std::nullopt` directly for that case rather than literally
         reproducing the fallthrough -- provably equivalent, documented
         inline at both the header and the fallthrough site.
      2. `rumorFor(player, step)` indexes `player.skills[step][0]` --
         the SAME storage cell `PlayerCombatStats::GainSkillExp`/
         `SkillValue` use for skill `step`'s real combat rank -- to track
         how many times that rumor topic has been asked about. The
         first 6 named skills' rank and their associated rumor-ask-count
         are, in the original, literally one field. Ported exactly
         rather than given separate storage; checked directly in the new
         smoke test's own section F.
      3. The generic-peddler buy path (`action==14`) draws its spawn id
         (`Item.nextSpawnId()`, i.e. `++nextItemSpawnId` here) and the
         counter is already advanced BEFORE the code checks whether
         `addInventoryItem` actually succeeds -- so a failed purchase
         (a full pack) still burns a real spawn id, exactly like M43's
         already-documented `GrantStarFrostItem` precedent for the same
         counter.

      Verified via the new `shop_interaction_smoke.exe` (7 sections, A-G)
      against the real 37-level generated world, real `itemsin.dat`/
      `npcstrings.dat`/`charin.dat` data, and a real created character
      (M11): `ShopState::Reset()`'s post-condition; every static table
      and small helper (`IsNamedShop`/`IsGenericPeddler`/`HubShopAt`/
      `IsValidShopAction`/`ShopActionCode`, plus `QuestFlagsFor` against a
      synthetic 0xE4 byte exercising all 4 shops' own 2-bit windows);
      shops 0-3's buy (afford/unaffordable/pack-full, including the
      spawn-id-burn quirk) and sell (ordinary + the gift-item-blocked
      case, both found by scanning real `itemsin.dat` rather than
      hardcoded ids); shops 5-8's first-visit/subsequent-random-line
      greet (a twin `JavaRandom` predicting `ESGame.nextInt(3)`),
      action 2's quest-ask (a twin RNG independently re-deriving
      `Player.rollShopOutcome`'s own chance formula via the
      already-trusted `PlayerCombatStats::SkillValue`/`RollOutcome`, then
      checking the resulting `questState1`/`rewardsGiven` transition
      matches whichever of the 4 outcome branches that roll actually
      landed on) and its already-turned-in short-circuit, action 4's
      quest-item turn-in (found by scanning real items for a category-11
      item with a real nonzero `QuestFlagsFor` result for the shop under
      test), action 5's reward-gated rumor ask, and action 8's
      fallthrough-to-null; Jakar's first-visit intro (plain and
      death-greeting-prefixed), subsequent-visit advancement bump,
      cure/warp/heal (including the no-camp-mark vs. real-warp branches,
      the latter checked against real world levels), and the rumor-reveal
      chain (`eventFlags` progression, the traitorIndex-selected fragment
      substitution, and a twin-predicted pick roll) plus its
      everything-already-revealed "no new rumors" branch; and
      `InteractTick::ProcessInteract`'s own new wiring, checked to return
      exactly what a direct `Dialogue()` call would. All checks passed.
      Full clean rebuild zero warnings; all 43 smoke tests pass;
      `dawnstar_port.exe` launches and stays up (not visually re-verified
      talking to a real in-game NPC this session -- the ProcessInteract-
      vs-Dialogue equivalence check in section G is this milestone's
      wiring evidence instead).

- [x] **M46 -- the interactive NPC menus (`NPCChoicesUI` and every
      sub-screen)** (this session). `NpcMenu` (`port/src/ui/npc_menu.h`/
      `.cpp`, in `dawnstar_render`, which now links `dawnstar_npc` --
      no cycle, `dawnstar_npc` only depends on player/assets) ports the
      screen graph `ESGame.handleNPCChoices()`/`handleNPCAction()`/
      `setAidPointsForNPC()` and the NPC `secondaryParam` branches of
      `commandAction1()` drive: the M45 greeting popup's Ok now leads into
      the real per-shop choices screen (Buy/Sell for the 4 hub peddlers;
      Rumors/Cure/Warp/Recovery for Eustacia; Train/Give/Befriend/Threaten/
      Ask a question/Warp for the 4 named shopkeepers), and from there the
      Buy/Sell/Sell-confirm/Train/Give lists, the two-step "Ask about
      what/whom" question chain (ported exactly, including the eventFlags
      bookkeeping, the traitor-suspicion counter and its RNG draw's
      short-circuit position), Eustacia's warp-destination list, and the
      named shopkeepers' "Warp to camp" confirm. Every action is a call
      into M45's `ShopInteraction::Dialogue`, shown in the one shared info
      popup, so this milestone is pure orchestration on verified logic.
      Same shape as `OptionsMenu`: one `Screen` per real screen, an `Active`
      enum for `setCurrentDisplay`, caller-edge-detected keys, and
      `OnSelect`/`OnCancel` returning whether the flow left for the game.
      `main.cpp`'s M45 `inNpcDialogue` popup became `inNpcMenu`, and
      `npcInSight` shop 4 (which has no greeting line) now opens Eustacia's
      menu directly, as `GameCanvas.openNpcDialogue()` does.

      Small pieces added along the way: `PlayerMovement::WarpTo` (the
      hand-rolled position set + `refreshCorridorView()` of the warp-where
      dispatch); `ShopInteraction::kUnconfirmedA/B` promoted out of
      `options_menu.cpp` (now shared with the question chain); and the
      deferred M42/M45 sync -- `main.cpp` mirrors live `ShopState` and the
      two spawn-id counters into `OtherStateInfo` before every save and back
      out after every load, so a save/load round trip actually carries shop
      state now.

      Original behavior preserved rather than tidied: Sell/Give labels use
      `"E:"` (no space) where Inventory uses `"E: "`; screens with a
      `backTarget` (Train/Give/Question*/Warp*) return to the choices screen
      *without* refreshing the aid prompt, while Buy/Sell (none) refresh it;
      `NPCChoicesUI`/`NPCQuestionWhatUI` keep their selection between
      visits while every `newXxxUI()` list starts fresh; the "You have
      nothing to give me!" popup on the peddlers' Sell (param 53) re-opens
      an *empty* Sell list on Ok; the sell-confirm screen has no Cancel.

      Verified via the new `npc_menu_smoke.exe` (no JVM ground truth, same
      reason as M6/M9/M11/M13-M45) against the real 37-level world, real
      item/character/dialogue data and a real character: every action's
      resulting player/shop/RNG state compared against a twin
      `ShopInteraction::Dialogue` call; list labels, prompts and Ok/Cancel
      destinations checked for every screen; the question chain checked for
      the flag/aid arithmetic, the non-traitor UNCONFIRMED_A line, the
      already-asked repeat, the traitor's guaranteed admission at suspicion
      2 (UNCONFIRMED_B), and suspicion 3's 20% draw predicted from a copy
      of the live RNG; warp landing tile (one south of the shopkeeper,
      facing north) checked against `GeneratedLevel::specialShopX/Y`. Full
      rebuild zero warnings from the new code; all 44 smoke tests pass;
      `dawnstar_port.exe` launches and stays up (not driven through a live
      NPC conversation by hand this session).

- [x] **M47 -- chest/NPC sighting refresh after hub/camp warps.**
      `Player.resetToHubPosition()` and `warpToCampMark()` both end with
      `gameCanvas.refreshChestInSight()`/`refreshNpcInSight()`; the port had
      left them out (M18's "SIMPLIFIED" note), so after Eustacia's Warp,
      the "Warp to Camp" item, a camp-marker tile or death, `chestInSight`
      /`npcInSight` (hotbar context, interact target) went stale until the
      next step. Both functions now raise the transient
      `PlayerState::sightRefreshPending` (they cannot run the refresh
      themselves: it needs the message popup and non-const levels), and
      `main.cpp` runs one shared `refreshSightings()` lambda -- the same
      code the move path uses -- at the start of the next tick.
      `PlayerMovement::WarpTo` (the NPC-menu Warp, whose original hand-rolls
      the move) deliberately does not request it, matching the source.
      Verified in `use_item_smoke` (flag raised by both, not by `WarpTo`);
      the main.cpp wiring is not driven by hand.

- [x] **M48 -- the boot splash (`LoadingScreen` mode 2).**
      `ui/boot_splash.h`/`.cpp` ports `renderSplash()` and
      `runSplashSequence()`/`waitAtLeast()`, the piece M42 left out. The
      original runs it on its own Thread; here the whole timeline is a pure
      function of elapsed time (`BootSplash::PhaseAt`), quantised to the
      original's 500ms repaint step: 0-1s splash images, 1-4s images plus
      the progress bar, 4-6.5s the Vir2L/ZeniMax copyright card with the
      carrier logo, 6.5-8s images again, then the hand-off to the main menu
      (8s total; `waitAtLeast`'s do-while runs one iteration more than its
      argument suggests: 5x500ms and 3x500ms). Assets: `splashtop/
      splashbot.png` are loose files in the data root, `vir2lLogo/
      mformaLogo.png` are `imgfiles.lmp` entries. Assets load instantly on a
      PC, so the bar is simply at 100% once it appears and the
      `percent < 100` half of the hold condition never extends the 4s.
      `main.cpp` shows it before the main menu (`inSplash`). One deliberate
      addition: Return/Esc/Space skip it (the original has no skip key).
      Verified by `boot_splash_smoke`: the phase timeline is checked against
      a literal transcription of the two Java loops, and every phase's frame
      is compared pixel-for-pixel against independently placed
      Blit/FillRect/DrawString calls with the real assets. The on-screen
      wiring was only checked as "launches and stays up", not watched
      through by hand.

- [x] **M49 -- the level-up menu.**
      Until now `levelUpPending` (set by `GainSkillExp` once level-exp hits
      10) was written and never read: a character levelled up in the stats
      but never got its attribute picks. `ui/level_up_menu.h`/`.cpp` ports
      `ESGame.newLevelUpUI()` + the `secondaryParam 39` branch of
      `commandAction1()` + `Player.availableAttributeIncreases()`/
      `levelUp()`: three "Select an attribute to increase 3/2/1 point(s):"
      lists (no Cancel) offering the attributes whose bit is set in
      `attributeIncreaseFlags`, then attributes += 3/2/1,
      `recalcMaxStats()` (now `PlayerCreation::RecalcMaxStats`, public),
      flags cleared, `Shop.reset()` and level-exp -= 10. The list is
      rebuilt but not shrunk between picks, so one attribute can take all
      three (+6) -- kept. `main.cpp` opens it from the tick right after the
      target-monster refresh and before `tickVisibleObjects` (the original's
      spot), as an early-return `inLevelUp` block.
      Not reproduced: with no flag set the original passes `null` to
      `setupPromptList` and throws; the port's `Open()` returns false, so no
      menu (and no crash) -- reachable only via item 92's bare level-exp +1.
      Verified by `level_up_smoke`: flag-to-name mapping, the three prompts
      (compared pixel-for-pixel with an independently built Screen), the
      +3/+2/+1 and +6 cases, recalculated max stats, flags/exp/Shop state
      after, and an end-to-end run from the real `GainSkillExp`. The
      main.cpp hand-off was only checked as "launches and stays up".

- [x] **M50 -- death and respawn.** Until now HP hitting 0 did nothing at
      all: `GameCanvas.processIdleTick()`'s own `hp <= 0` check (now inline
      in `main.cpp`'s tick, right after `RefreshAndResolveTargetMonster`,
      its original spot) and `run()`'s own `deathState` 1/2/3 state machine
      (the new `death/death_tick.h`/`.cpp`, `DeathTick::TickDeathState`,
      the sibling of `camp/camp_tick.h`'s `TickCampState` sitting right
      after it in `main.cpp`'s own campState/deathState priority chain --
      `main.cpp` captures `campState` BEFORE calling `TickCampState` so
      `TickDeathState` is only ever consulted the tick campState found
      nothing to do, matching the original's exclusive if/else-if) were
      both unported. The chain: HP <= 0 sets `deathState = 2`; next tick
      flips it to 3 and clears the message popup outright (`paint()` now
      shows "You're Dead!" on black, `main.cpp`'s own new top-level branch,
      outranking the camping screen exactly like the original); once 5
      real seconds have passed, it resolves -- `normalizeForSummary`
      applied straight to the live stats (full heal, `coreStats[8]`
      zeroed), every UNEQUIPPED item dropped, `starFrostBonusActive`
      cleared, the new `PlayerMovement::ResetState(true, ...)` (a full
      transcription of `Player.resetState(respawning)`: ailment mask/3
      timers/`unconfirmedZ` cleared, repositioned to the hub's ALT spawn
      point via `ResetToHubPosition`, `effectDurations` zeroed, combat
      target + harm/armor/safe-camping buffs cleared), `Shop.
      showDeathGreeting = true`, `minimapDirty`, and the (now-current, i.e.
      hub) level's name shown as a popup -- `resetState(true)` already
      repositioned to the hub by the time `displayName()` is read, so this
      always says "Dawnstar", matching the original's own statement order,
      not the level actually died on. `PlayerMovement::ResetState`
      intentionally does not reproduce the `respawning == false` branch's
      `grantStartingItems()` call: no real caller here ever passes false
      (`PlayerCreation::CreateCharacter` already has its own inline
      equivalent). Also: `Player.commitMove()`'s own `Shop.
      showDeathGreeting = false` reset on every forward/backward step,
      left as a "Shop not ported yet" skip since M18 -- `PlayerMovement::
      Move` now raises the new one-shot `PlayerState::
      clearDeathGreetingPending` (the same reason/shape as M47's
      `sightRefreshPending`: `dawnstar_player` cannot reach `ShopState`
      without cycling back through `dawnstar_npc`), consumed by
      `main.cpp` next tick. One new small port-only addition along the
      way: `Dungeon.NAMES`/`displayName()` (all 37 level names) was
      never ported before this needed it -- now `DungeonRuntime::
      DisplayName(levelNumber)`.
      Verified by the new `death_respawn_smoke`: the alive/just-died/
      counting-down/resolved timeline: the resolve step's stat healing,
      inventory drop (equipped gear survives, unequipped doesn't), every
      `resetState` field, the alt-spawn position, `Shop.showDeathGreeting`/
      `minimapDirty`/the popup text, all against a real generated world and
      a real created character; `clearDeathGreetingPending` raised by a
      committed forward step and NOT by a turn. Full rebuild zero warnings;
      all 48 smoke tests pass (`launcher_smoke` too -- the prior session's
      failure there was a stale/path artifact, not a real regression, now
      confirmed passing clean); `dawnstar_port.exe` launches and stays up
      (not driven through an actual in-game death by hand this session).

## Milestones next

- [ ] The `Shop.SHOP_X/Y[5..8]` write-through noted in M6/M13/M46 -- internal
      plumbing with no visible effect.
- [ ] `GameCanvas.processIdleTick()`'s other half, `tickFatigueRegen`
      (gated on `!actionTakenThisTick`) -- noticed while porting M50;
      no `actionTakenThisTick`-equivalent exists in this port yet.
