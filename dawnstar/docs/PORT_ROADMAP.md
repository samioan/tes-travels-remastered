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

## Milestones next

- [ ] **M16 and beyond (not yet planned in detail):** `Player`'s
      spellcasting; the lightweight "character summary" save format M12
      deferred; object/monster/chest/NPC sprites and the HUD/minimap
      (`GameCanvas.paintGameView()`'s other calls, now that the base
      corridor view renders, the player can move through it, and combat
      resolves); and finally `ESGame`'s own screen-wiring loop tying it
      all together. Each gets its own milestone once the shape of "how
      much fits in one slice" is clearer -- following `shadowkey-decomp`'s
      pattern of not over-planning milestones far in advance of actually
      reaching them.
