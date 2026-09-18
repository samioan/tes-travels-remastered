# Port Roadmap

Tracks `port/`, the actual PC port -- as opposed to [`ROADMAP.md`](ROADMAP.md),
which tracks the reverse-engineering work that feeds it (now fully done: all
13 classes renamed/compile-checked in [`../src/`](../src/), all asset formats
confirmed in [`ASSET_FORMATS.md`](ASSET_FORMATS.md)). Follows the sibling
`dawnstar` project's own precedent (`../../dawnstar/docs/PORT_ROADMAP.md`,
which in turn follows `shadowkey-decomp`'s): milestone by milestone, each one
landing a real, runnable/testable slice rather than a big-bang rewrite,
verified with a smoke-test executable per milestone rather than just
read-through.

## Decisions carried through every milestone

- **Behavioral reimplementation, not byte-exact recompilation.** `../src/`'s
  renamed Java is the reference for *what* every system does; the port is
  ordinary modern C++ that reproduces that behavior, not a mechanical
  transliteration of MIDP-era Java idioms into C++ syntax. Same rule as
  dawnstar's own port.
- **C++17, CMake + Ninja, MSVC**, matching `dawnstar`/`shadowkey-decomp`'s
  toolchain and this repo's existing `port/` scaffold.
- **Raw Win32 + GDI (`StretchDIBits`), no SDL2/D3D/GL** -- same rationale as
  dawnstar: the original is a software-rendered MIDP `Canvas`, so a Win32
  window blitting a manually computed backbuffer reproduces that
  architecture directly with zero external dependencies.
- **176x208 virtual canvas.** Same reasoning as dawnstar's own port decision
  (`../../dawnstar/docs/PORT_ROADMAP.md`): `GameCanvas` reads
  `getWidth()`/`getHeight()` at runtime rather than assuming a fixed
  hardware size (`../src/GameCanvas.java` lines 253-278), and this jar's own
  `MIDlet-1` manifest line names `icon3650.png` -- the Nokia 3650, whose
  display is 176x208. **This is a port decision, not a confirmed original
  constant**, same caveat as dawnstar's.
- **Assets stay out of the repo.** The port reads `itemsin.dat`/
  `spellsin.dat`/etc. and (later) `.cus`/`.png` files directly from a
  configurable root path at runtime (defaulting to `../extracted/`, this
  project's existing `tools/extract_jar.py` output) -- nothing copyrighted
  is copied into git.
- **No archive indirection, unlike dawnstar.** Stormhold has no
  `datfiles.lmp`/`imgfiles.lmp` -- every resource is its own top-level jar
  entry (`ASSET_FORMATS.md`'s header note). So there's no `DatArchive`
  equivalent to port; `AssetRoot` (`src/assets/asset_root.h`) is a plain
  root-directory-relative file opener instead, structurally simpler than
  dawnstar's linear-scan archive reader.
- **No scripting engine to port.** Same as dawnstar -- `ESGame`/`GameCanvas`/
  `Player`/`Monster`/`Dungeon` *are* the game logic, directly, in Java. No
  SimKin-equivalent, no "embed an interpreter" milestone.
- **Shared "ngame" engine code is duplicated for now, not factored out.**
  `../docs/ROADMAP.md` originally suggested factoring shared engine code
  into one location "once both are understood" -- both now are, but
  dawnstar's port is a mature, 45+-milestone codebase already shipping a
  launcher/release pipeline. Retrofitting a cross-project shared library
  underneath it is a real, risky refactor of working code, not a
  reverse-engineering task -- deliberately deferred rather than done as a
  side effect of starting Stormhold's own port. In the meantime, small
  engine-level pieces that turned out byte-for-byte identical (`GameClock`,
  `Backbuffer`, `Window`, `BinaryReader` -- all confirmed identical cadence/
  format between the two games' decompiled sources) are copied rather than
  shared; revisit consolidation once Stormhold's port has enough of its own
  milestones to show which pieces actually *stay* identical under real
  gameplay-logic weight, the same way dawnstar's own split between
  `player/`/`monster/`/`combat/` only became obvious once M14/M15 needed it.

## Milestones done

- [x] **M0 -- scaffold** (pre-existing). CMake/MSVC/Ninja project proves the
      toolchain: opens a blank Win32 window. No tick loop, no backbuffer, no
      game logic.

- [x] **M1 -- real tick loop + backbuffer + present** (this session). A
      `GameClock` running the real 250ms/4Hz cadence
      (`GameCanvas.run()`'s "steady 250ms tick", `../src/GameCanvas.java`
      around line 505 -- confirmed identical to dawnstar's own `GameCanvas.
      run()` cadence, see that method's own header comment), a 176x208
      `Backbuffer`, and `StretchDIBits` presentation scaled to the window's
      client area -- structurally identical to dawnstar's M1 (same engine,
      same architecture). Still no game logic: proves the loop/presentation
      architecture only. Verified by building and launching
      `stormhold_port.exe` (stays up, presents a solid-color frame every
      tick, closes cleanly).

      `Backbuffer` here has no `Blit()`/`DecodedImage` yet, unlike
      dawnstar's own M1-era backbuffer had grown by M10 -- no PNG decoder
      exists yet in this port, so there's nothing to composite beyond flat
      fills this milestone.

- [x] **M2 -- asset foundations** (this session). `BinaryReader`
      (big-endian primitives + `readUTF`-style length-prefixed strings,
      matching `Util.java`/`Item.java`/`Spell.java`'s `DataInputStream`
      usage -- byte-for-byte identical to dawnstar's own `BinaryReader`,
      confirmed by diffing the two) and `AssetRoot` (the direct-file
      resource opener described above), then the first two real data
      loaders on top of it: `ItemDatabase` (`itemsin.dat`+
      `droppeditemsin.dat`, `../src/Item.java`) and `SpellDatabase`
      (`spellsin.dat`, `../src/Spell.java`). Data-only for now --
      RNG-driven methods (`Item.java`'s `randomGiftItemOfSubtype()`/
      `rollLoot()`) are deferred to whichever later milestone actually
      needs dungeon generation, matching how dawnstar's own M2 deferred its
      equivalents to M6.

      Verified by `asset_smoke.exe` against the real `extracted/`: 109
      items across 17 categories, a 43-row loot table, and 25 spells, all
      with sane-looking names/prices/stats (`Miner Pick`/`Woodman's Axe`/
      `Battle Axe`, `Frenzy`/`Shield`/`Deft Security`) -- the same 3 spell
      names dawnstar's own M2 smoke test happened to print first, a nice
      independent confirmation that `spellsin.dat`'s early rows are shared
      content between the two games, not a coincidence of this port's own
      code.

- [x] **M3 -- monster type database + dungeon geometry** (this session).
      `MonsterDatabase` (`monstersin.dat`, `../src/Monster.java`'s
      `loadTypes()` -- u32 count/names/17-byte stat rows, identical layout
      to dawnstar's own `monstersin.dat`) and `DungeonGeometry`
      (`geomin.dat`'s 37 six-byte rows, `../src/ESGame.java`'s
      `loadDungeonGeometryRows()`/`../src/Dungeon.java`'s `neighbors[]`/
      `stairsUpDir`/`stairsDownDir`). Deliberately just the per-*type*
      monster database and static level connectivity, not per-instance
      Monster spawn/AI/combat or dungeon generation -- gameplay logic for
      later milestones, same scoping dawnstar's own M3 used.

      **`DungeonGeomRow`'s doc comment departs from dawnstar's own M3
      finding, on purpose:** dawnstar's `geomin.dat` stairways sit at a
      *fixed* per-direction tile coordinate; Stormhold's `Dungeon.
      generate()` instead calls `carveStairwell(direction)` to place each
      stairwell procedurally as part of level generation (confirmed in
      phase 2, `../docs/ASSET_FORMATS.md`) -- so `stairsUpDir`/
      `stairsDownDir` here are direction codes with no fixed coordinate to
      hardcode alongside them. Also unlike dawnstar: Stormhold has no
      separate `DungeonGenerator` class at all (`../src/Dungeon.java`'s own
      header comment) -- the room-carving/generation algorithm is fused
      directly onto `Dungeon`, confirmed by reading `decompiled/i.java`
      directly. Neither difference blocked this milestone (data loading
      only), but both matter for whichever later milestone ports
      generation itself.

      Verified by `monster_dungeon_smoke.exe` against the real
      `extracted/`: 41 monster types with sane names/stats (`Weak
      Prisoner`/`Prisoner`/`Ruffian`, increasing HP/attack/loot-row as
      expected for early low-tier types), and 37 geometry rows whose
      connectivity is topologically sane -- level 1 (the hub town)
      connects N/E/S/W to levels 2/11/20/29 with no stairs (`stairsUp=-1`/
      `stairsDown=-1`), the exact same hub connectivity dawnstar's own M3
      smoke test found for its hub level, a strong independent
      cross-check that both games share this world layout. Also: 41
      monster types with type 41 being the last one lines up with `Dungeon.
      java`'s own header comment that level 37's last room gets a "forced
      monster type 41" scripted encounter.

- [x] **M4 -- character data (class/race/skill templates)** (this
      session). `CharacterData` (`charin.dat`, `../src/Player.java`'s
      `loadCharacterData()` -- stat labels, attribute names, class names,
      race names, skill names, per-skill governing attribute, and the big
      per-class stat template table). Unlike dawnstar's own M4, no rename
      fix was needed here -- phase 1's read-through already carried
      dawnstar's class/race naming fix over correctly (`../docs/
      ASSET_FORMATS.md`'s `charin.dat` section says so directly), so this
      milestone is confirmation against real data rather than a fresh
      find.

      Verified by `character_data_smoke.exe` against the real
      `extracted/`: the exact same 7 classes (Barbarian/Battlemage/
      Knight/Nightblade/Rogue/Sorcerer/Spellsword) and 6 races (Redguard/
      Nord/Breton/High Elf/Wood Elf/Dark Elf) as dawnstar, correctly in
      the class/race slots (not swapped) -- and, as a nice independent
      confirmation, `classTemplates[class][1]` (the race each class comes
      with) resolves to the same archetypal pairing dawnstar found:
      Sorcerer -> High Elf (index 3), the classic pure-caster combo.

- [x] **M5 -- bit-exact `java.util.Random`** (this session). `JavaRandom`
      (`port/src/util/java_random.h`, header-only): the 48-bit LCG
      (`NextInt`/`next(32)`) plus `RandomInt1Based`/`RandomInt0Based`,
      matching every real caller in `../src/` (`Dungeon.java`,
      `Item.java`, `Monster.java`, `ESGame.java`, `Shop.java` -- only ever
      the no-arg `nextInt()` plus the codebase's own `Math.abs(x % bound)`
      wrapping, never `Random`'s more involved `nextInt(bound)`).
      Foundational rather than optional: `Dungeon.generate()` seeds one of
      these **deterministically per level** (`new Random(seed)`, seed =
      `levelNumber * 5000` -- confirmed in `../src/Dungeon.java`'s own
      header comment, *not* dawnstar's `levelNumber * 8000`), so a level's
      room layout/monster spawns/loot are a pure function of the level
      number, same as dawnstar.

      **A real naming-convention divergence from dawnstar, confirmed
      directly against both games' sources rather than assumed
      symmetric:** dawnstar's own `JavaRandom` calls the *1-based* formula
      (`1 + abs(nextInt() % bound)`) `LingoRandomInt` and the *0-based*
      one `RandomIntBelow`/`nextInt`. Stormhold's decompiled/renamed
      source has the names **swapped**: `ESGame.nextInt(bound)` and
      `ESGame.randomInt(Random, bound)`/`Util.randomInt(...)` are the
      1-based formula, while `ESGame.lingoRandomInt(bound)` is the
      0-based one (`../src/ESGame.java` lines 2241-2250) -- and
      `Dungeon.java`/`Item.java` mostly skip named helpers entirely,
      inlining `Math.abs(this.rng.nextInt() % bound)` directly at each
      call site. `java_random.h`'s `RandomInt1Based`/`RandomInt0Based`
      names sidestep both games' inconsistent naming rather than
      reusing either one, with each function's doc comment mapping to
      its real Java call sites and flagging the swap explicitly so a
      future reader doesn't assume dawnstar's naming carries over.

      Verified the strongest way available, same standard as dawnstar's
      own M5: captured real `java.util.Random(seed).nextInt()` sequences
      from an actual JVM (java 1.8.0_503, `javac --release 8`) for 5
      seeds, including `10000` = level 2's real generator seed
      (`2 * 5000`), and checked `JavaRandom` reproduces them **bit-for-
      bit** -- `java_random_smoke.exe` matched on the first try. The
      other 4 seeds (0/42/-1/4294967295) produced the exact same output
      dawnstar's own M5 capture did, confirming `java.util.Random`'s
      algorithm is identical between the two games as expected -- only
      the seed formula and call-site naming actually differ. Also ported
      `Math.abs(int)`'s `Integer.MIN_VALUE` quirk explicitly (`JavaAbs`)
      rather than calling `std::abs`, same rationale as dawnstar's own M5.

- [x] **M6 -- procedural dungeon generation** (this session).
      `DungeonGenerator` (`port/src/world/dungeon_generator.h`/`.cpp`):
      the full room-carving/corridor-connection/monster-placement/
      chest-placement pipeline transcribed from `../src/Dungeon.java`
      (named `DungeonGenerator` here purely for cross-reference symmetry
      with dawnstar's port -- the real source has no separate class at
      all, per M3's finding), plus `Item.java`'s two loot-roll methods
      added onto `ItemDatabase` (`RollLoot`/`RandomGiftItemOfSubtype`,
      M2's data-only struct's first added logic methods) and
      `MonsterDatabase::RawStat()` (the unmasked signed-byte read
      `Monster`'s real constructor uses for starting HP, distinct from
      `Stat()`'s `& 0xFF`).

      **Correction to M3's own note, caught while transcribing this
      milestone:** M3 said Stormhold's stairways are placed
      "procedurally" as opposed to dawnstar's "fixed per-direction tile
      coordinate" -- reading `carveStairwell(dir)` end to end shows that's
      not quite right. It's still a fixed coordinate per compass
      direction (N->(17,4), S->(17,30), W->(4,17), E->(30,17), same
      values dawnstar's own `CarveStairwayCorridor` implementation
      actually carves at, despite that game's `DungeonGeomRow.h` doc
      comment describing slightly different numbers) -- the real
      difference between the two games here is just *how* that mapping
      is expressed (a small dispatch function vs. a data-shaped doc
      comment), not the underlying behavior. Noted here rather than
      silently reworded, same policy as the phase-2 `.cus` correction.

      **A second, more consequential architecture difference confirmed
      by finishing this transcription:** Stormhold has no "special
      shopkeeper room" mechanic inside procedurally-generated levels at
      all, unlike dawnstar (which places 4 NPCs at generation-chosen
      positions on levels 3/12/21/30 and writes them back into `Shop.
      SHOP_X[5..8]`/`SHOP_Y[5..8]`). `Shop.SHOP_X`/`SHOP_Y` here are
      fixed 7-entry arrays (6 hub-town shops + 1 conditional Warden), and
      `Dungeon.generate()`'s "mark one room's center" step has no
      level-number-gated branch -- it always marks bit 8 (the same
      "purpose not confirmed" no-spawn marker from M3/phase 1), never bit
      32. `GeneratedLevel` accordingly has no `specialShopX`/`specialShopY`
      fields at all here.

      **A real, easy-to-miss RNG-order subtlety, caught by reading
      `rollRoomRect()` character-by-character rather than assuming
      uniformity with every other random draw in the file:**
      `rollRoomRect()`'s 6 draws all use `Math.abs(rng.nextInt()) %
      bound` (**abs, then mod**), while every other random draw in
      `Dungeon.java` uses `Math.abs(rng.nextInt() % bound)` (**mod, then
      abs**) -- two different formulas that usually agree but aren't
      identical (e.g. around `Integer.MIN_VALUE`). Kept as a local
      `AbsThenMod()` helper in `dungeon_generator.cpp` rather than folded
      into `java_random.h`'s shared `RandomInt0Based`/`RandomInt1Based`,
      since nothing else in this codebase uses that order. Confirmed
      dawnstar's own port independently caught the identical pattern in
      its own `DungeonGenerator.java` (its `AbsThenMod` helper is used in
      exactly the same one place, its own `RandomRoomRect`).

      **A confirmed-dead field, found and documented rather than
      "fixed":** `Dungeon.placeChests()` writes a per-chest byte meant to
      flag the guaranteed-gift chest (`record[2] = first ? 1 : 0`), but
      `first` is already flipped to `false` earlier in the *same*
      iteration, before that write executes -- so the byte is always `0`
      for every chest, including the real gift one. Confirmed dead: the
      only consumer (`Player.collectChestItem()`) never reads byte 2 at
      all. `GeneratedChestSpawn::guaranteedGift` models the real game
      logic (chest 0 really does get the gift-subtype roll) rather than
      this always-zero on-disk byte, documented in the struct's own
      comment.

      Verified via `dungeon_generator_smoke.exe` against the real
      37-level geometry/item/monster data (no JVM ground truth possible,
      same reason as dawnstar's own M6 -- `ESGame`'s `RegisteredMIDlet`
      base class's static initializer throws on real MIDP stub jars the
      instant anything touching `ESGame` actually *runs*): the hub level
      plus 2/3/12/15/21/30/37 -- all checks passed on the first attempt:
      exactly 15 rooms/monster spawns and 5 chests (exactly 1
      guaranteed-gift) per level, every monster/chest position
      walkable/in-bounds with the right tile bit set, every chest's
      rolled item id valid, exactly one bit-8 marker tile per level and
      zero bit-32 tiles from generation (confirming the no-special-room
      architecture finding above), the hub town's 6 shop tiles correctly
      marked, level 37's last room forced to monster type 41, and no
      undocumented tile bits anywhere.

- [x] **M7 -- `RawImage` indexed-color sprite decoder** (this session).
      `RawImage::Load` (`port/src/assets/raw_image.h`/`.cpp`): the
      from-scratch `.cus` sprite format transcribed from `../src/RawImage.
      java`'s `load()` (no dawnstar analog, see `../docs/ASSET_FORMATS.md`'s
      ".cus files" section -- itself phase 2's own correction of the
      original "per-bodypart 3D mesh" naming-pattern guess). Decodes
      straight to the same ARGB4444-ish `uint16_t` pixel format the Java
      keeps (alpha nibble `0xF000` set/cleared per pixel at load time, no
      separate alpha mask) rather than converting to `Backbuffer`'s RGB565
      -- compositing (alpha test + format conversion, i.e. a real `Blit()`)
      is deliberately deferred to whichever later milestone actually draws
      a frame with sprites in it, same scoping note `Backbuffer`'s own
      header comment already flagged back at M1.

      **A real dead-field/dead-code pair, found and documented rather than
      silently reproduced or "fixed":** `RawImage.load()` stores a
      `widthAgain` field (a second copy of `width`) that's never read
      anywhere else in `../src/` -- not ported. Separately, its palette
      buffer (`paletteScratch`) is a **static** 256-entry array reused
      across every image load, so any pixel index `>= colorCount` for a
      given image would silently resolve to a *stale entry left over from
      whichever `.cus` file loaded before it* -- load-order-dependent
      behavior no well-formed asset should ever trigger. The port doesn't
      reproduce that static-reuse nondeterminism; an out-of-range index is
      a hard `std::runtime_error` instead, verified never to fire against
      any real file (see below). Also: `RawImage.load()`'s own "colorCount
      > 255" check is dead code -- `colorCount` is read as `in.read() &
      0xFF`, which can never exceed 255 in the first place -- noted, not
      reimplemented as unreachable logic.

      Verified by `raw_image_smoke.exe` against **all 37 real `.cus`
      files** in `extracted/` (the complete roster `../docs/
      ASSET_FORMATS.md` lists): every one decodes cleanly, width*height
      always matches the decoded pixel count, every one of them actually
      sets `hasTransparency=true` and has a plausible count of transparent
      pixels (ranging from 15 on the smallest sprite, `bagsmall.cus`
      12x10, up to 4297 on the largest, `overseerbodyf3lc.cus` 105x158),
      the alpha-nibble invariant holds pixel-for-pixel on every image, and
      -- the strongest check here -- the out-of-range-palette-index guard
      above never fired once across all 37 files, confirming real assets
      never actually depend on the Java static-scratch staleness quirk.

- [x] **M8 -- Warden visits/leaves world event** (this session).
      `WardenState` (`port/src/world/warden.h`/`.cpp`): `Shop.java`'s
      Warden world-event state machine -- `shouldWardenVisit()`'s
      escalating 13/26/39 elapsed-counter thresholds gating
      `wardenArrives()`/`wardenLeaves()`'s tile-bit-32 flip at Varus's
      fixed hub position (`Shop.SHOP_X[6]`/`SHOP_Y[6]` = (9,9)). Deliberately
      scoped to just this mechanic, not the rest of `Shop.java`'s
      `dialogue()` dispatcher (the full 7-NPC roster, quest-turn-in
      economy, Beneca/Helga's bespoke branches) -- that needs a live
      Player/inventory/dungeon-mutation model this port doesn't have yet,
      a later milestone's job (see "What's next" below).

      **A real, confirmed bug, preserved rather than fixed, same
      discipline as M6's dead chest-record byte:** `wardenLeaves()` reads
      the tile byte to clear bit 32 from `ESGame.dungeons[1]` (level
      INDEX 1 = level NUMBER 2, an actual procedurally-generated dungeon)
      at Varus's position, then writes the result into `ESGame.dungeons[0]`
      (the HUB)'s tile at that same position -- instead of
      reading/clearing/writing the hub's own tile the way `wardenArrives()`
      correctly does. Near-certainly a copy-paste index bug
      (`dungeons[1]` should read `dungeons[0]`): "the Warden leaving"
      doesn't restore the hub tile's real prior value at all, it
      overwrites it with whatever level 2 generated at the same (x, y)
      that tick, with bit 32 cleared. `WardenState::Leave()` takes both
      `GeneratedLevel&` arguments explicitly (hub and level 2) and
      reproduces this exactly, with the header comment flagging it plainly
      so a future reader doesn't assume it's a typo in the port itself.

      Verified by `warden_smoke.exe` against real `extracted/` data (hub +
      level 2, both from M6's `DungeonGenerator`): Varus's hub tile starts
      with bit 32 clear (consistent with M6's own finding that
      `BuildHubLevel` only marks the other 6 shops), the 13/26/39
      threshold escalation is exact (checked one elapsedCounter below and
      at each threshold), `Arrive()` sets bit 32 and blocks re-triggering
      while already present, `Leave()` reproduces the dungeons[1]/
      dungeons[0] bug exactly (asserted against level 2's real tile value,
      not just "bit 32 is now clear"), and the visit cap after
      `visitCount==3` matches the original's missing 4th branch (checked
      with an elapsedCounter of 100000).

- [x] **M9 -- player character creation** (this session). `PlayerState`
      (`port/src/player/player_state.h`) and `PlayerCreation`
      (`port/src/player/player_creation.h`/`.cpp`) port the real
      character-creation pipeline out of `../src/Player.java`:
      `applyClassTemplate()` (attributes/skills/derived stats/starting
      known-spell mask) fused with `resetState(classIndex, false)`'s "new
      character" branch (hub-town spawn position, ailment/timer reset,
      then `grantStartingItems()` -> `addInventoryItem()`/`equipItem()`)
      -- the real game calls these from two separate UI steps
      (class-select, then confirm), fused here since this port doesn't
      model the UI screens between them, only the resulting character
      data. Same fusion dawnstar's own M11 `PlayerCreation` uses for its
      equivalent two-step call. Also added `ItemDatabase::
      IsEquipmentCategory()` (M2's data-only struct's third round of
      added logic methods, same pattern as M6's `RollLoot`/
      `RandomGiftItemOfSubtype` and M7's decode-time alpha logic).

      **A real, notable divergence from dawnstar, confirmed by reading the
      whole creation pipeline end to end:** Stormhold's character creation
      involves NO randomness at all -- no `Util.randomInt`/`ESGame.
      *Random*` call appears anywhere in `applyClassTemplate()`,
      `resetState(classIndex, false)`, `setHubSpawnPosition()`, or
      `grantStartingItems()`. Two starting items and a fixed class-driven
      stat/skill/spell template, deterministically, every time -- unlike
      dawnstar's own character creation, which rolls a hidden "traitor
      index" via `Util.randomInt(4)` at this exact step. Consistent with
      `Shop.java`'s own header note that Stormhold may have no
      hidden-traitor subplot at all (see M8's entry): this milestone
      confirms the creation path at least has no such roll. Also unlike
      dawnstar: Stormhold has no `gold`/currency concept anywhere in
      `Player.java` at all (`Shop.java`'s own header note: no buy/sell
      action branch exists anywhere in that class either) -- the one
      place `Player.java`'s debug-summary method prints something
      labeled "gold" turns out, on reading the whole method, to just be
      printing `inventoryCount` under a stale/copy-pasted label, not a
      real currency value. And, checked directly against real
      `charin.dat` (not assumed from dawnstar's own data): **every one of
      the 7 classes** has a nonzero starting rank in all 5 spell-tier
      skills (Alteration/Conjuration/Destruction/Illusion/Restoration),
      so every class -- not just caster archetypes -- ends up with the
      exact same `knownSpellsMask` (`0x108421`) and `selectedSpellId`
      (`1`) straight out of character creation, a real content
      difference from dawnstar's own class-differentiated spell access.

      **A real phase-1 renaming bug, found and FIXED in `../src/
      Player.java` directly (not preserved -- see that file's own
      updated header comment on `equipItem()`), unlike this project's
      usual "document, don't fix" policy for bugs confirmed to be in the
      original game itself:** `equipItem()`/`unequipMatchingCategory()`
      used to read `Item.column(1, id)` (itemsin.dat's *category*
      column, values 1-10) and use the result directly as an index into
      `equippedItems` (`byte[7]`, valid indices 0-6) -- real
      `itemsin.dat` data has items in every category 1-10 (checked
      directly), so equipping any Boots/Gloves/Helmet/Shield item
      (categories 7-10) would throw `ArrayIndexOutOfBoundsException` in
      a real JVM, clearly not how a shipped game behaves. Re-checked
      against `decompiled/j.java` (the raw, unrenamed decompiler output)
      directly: the real bytecode call at both sites is the
      SINGLE-argument `a.a(var3)` -- `Item.equipSlotOf(id)`, the
      *dedicated* `equipSlot` column, confirmed 0-6 (fits `equippedItems`
      exactly, and matches every other `equippedItems[n]` read throughout
      the file). A genuine phase-1 transcription slip in THIS project's
      own renaming pass, not a bug in the original game -- corrected at
      the source, then the full renamed tree re-compiled clean against
      the MIDP stub jars (`javac`, zero errors) to confirm the fix
      didn't break anything else.

      Verified against real `CharacterData`/`ItemDatabase` for all 7
      classes via `player_creation_smoke.exe`: every attribute/skill/
      derived-stat field cross-checked directly against the loaded
      `classTemplates` row (not just internal self-consistency), hub-town
      spawn position, the all-classes-identical spell mask/selectedSpellId
      confirmed against an independent `charin.dat` parse, and starting
      items granted + auto-equipped into two always-distinct equip slots
      (weapon vs. armor) for every class -- confirmed directly against
      real `itemsin.dat` category/equipSlot data, not assumed.

- [x] **M10 -- player movement** (this session). `PlayerMovement`
      (`port/src/player/player_movement.h`/`.cpp`): `computeMoveTarget()`
      (facing-relative position math + cross-level boundary stitching,
      including the hub-town (19x19) <-> standard-level (35x35)
      recentering), `isWalkableTileBits()`, `commitMove()`, and `move()`'s
      turn/step/turn-back strafe wrapper. **Deliberately scoped to just
      position/facing/fatigue-cost mechanics** -- `commitMove()` also has
      several side effects this port can't model yet (dropped-item
      pickup, `Shop.wardenPresent`'s on-any-step clear, the
      level-37-entry forced-monster-respawn), all needing persistent
      per-instance world state (a real `ESGame.dungeons[]`/
      `ESGame.monsters[]`-equivalent session object) this port doesn't
      have. `PlayerMovement::LevelLookup` (a `GeneratedLevel&`-returning
      callback) is the caller-supplies-what's-needed pattern M6/M8 already
      established, not a persistent world registry.

      **A real, confirmed dead-code finding, caught by reading
      `commitMove()`'s exact control flow rather than assuming symmetry
      between `enteredNewLevelZone` and `leftLevelZone`:** both flags are
      computed the same way (`oldWalkable`/`newWalkable` on the old and
      new tile), but `commitMove()` already returns `false` earlier in the
      SAME method whenever the target tile isn't walkable -- so by the
      time `newWalkable` is computed, it's unconditionally `true`. That
      makes `leftLevelZone` (`oldWalkable && !newWalkable`) permanently
      dead: it can never actually become `true` via `commitMove()`, even
      though the original computes it as if it could go either way.
      `enteredNewLevelZone` has no such issue (`!oldWalkable && true`
      genuinely depends on `oldWalkable`). Preserved faithfully (not
      hand-simplified), with `m10_player_movement_smoke.cpp` directly
      constructing the one scenario that comes closest (an unwalkable
      starting tile) to prove the claim rather than just asserting it.

      **Corrected two real M6-era modeling gaps, found while porting this
      milestone (both needed to make `commitMove()`'s tile/level-state
      reads meaningful, not movement-specific findings per se):**
      `GeneratedLevel`'s M6-era `visited` field (always `false`,
      unused by any milestone until now) was actually modeling
      `Dungeon.populated` -- confirmed the real semantics directly:
      the hub's constructor sets `populated = true` unconditionally, but
      a STANDARD level's `populated` is set only by `ESGame`'s external,
      not-yet-ported progressive zone-opening system, never by
      `generate()` itself. Renamed to `populated` (matching Dungeon.
      java's own field name) and set `true` on both generators' output,
      since this port has no other path to a generated level at all --
      "was generated by this port's generator" is the closest available
      stand-in until a real zone-opening milestone exists. Separately,
      `Player.java`'s own header comment revealed `Dungeon.unconfirmedH`
      (previously flagged there as an unconfirmed, seemingly-dead field)
      actually DOES have a write site: `commitMove()` sets it `true` on
      every successful move into a level -- it's `Dungeon.java`'s own
      dawnstar-equivalent "visited" flag (has the PLAYER been here), a
      real, DISTINCT concept from `populated`. Added as a genuinely new
      `GeneratedLevel::visited` field (separate from `populated`), set by
      `CommitMove`.

      **A real, minor divergence from dawnstar, confirmed by reading
      `move()` directly:** dawnstar's own strafe wrapper guards the
      turn-back step with a one-shot `suppressStrafeAdjust` flag;
      Stormhold's has no such field or guard at all -- the turn-back
      always runs unconditionally. Both games share the same
      return-value-overwrite shape otherwise (a strafe move's return
      value reflects the final turn-back's success, not the actual
      sideways step's).

      **Left as an unresolved gap for a later milestone, not fixed
      here:** `BuildHubLevel()` (M6) always marks exactly the 6
      always-present shop tiles, but the real hub constructor's own tile
      marking is actually conditional on `Shop.wardenPresent` (marks a
      7th tile, Varus's own position, when the Warden is present) --
      `BuildHubLevel` doesn't take a `wardenPresent` parameter at all.
      Not fixed now since wiring it up needs a live `Shop`/session object
      this port doesn't have yet, and doesn't affect movement's own
      correctness (Varus's tile just stays unmarked, matching the
      "Warden never present" baseline this port's tests already exercise).

      Verified by `player_movement_smoke.exe`: exact turning-sequence
      arithmetic, exact position deltas for all 4 facings, exact
      hand-derived hub<->standard recentering coordinates (independently
      cross-checked: the derived X=17 landing coordinate matches M6's own
      fixed stairwell X-coordinate, a nice confirmation the recentering
      formula really does line up both levels' edges the way the game
      intends), standard<->standard crossings with no recentering,
      walkability gating, the dead-`leftLevelZone` finding demonstrated
      directly, the no-neighbor guard actually throwing when triggered,
      fatigue cost (including the ailment-bit-0 3x multiplier and
      clamping at 0), strafe displacement, and a real-data integration
      walk from the actual hub spawn position across real generated
      neighbor levels with zero exceptions.

## What's next

M11 onward: the rest of `Shop.java`'s dialogue() dispatcher (quest-turn-in
shops 0-3, Beneca, Helga -- now that there's a real `PlayerState`/
inventory/movement model to hang it off of), combat, and the player save
format, following dawnstar's own later milestones roughly but expecting
further Stormhold-specific divergences the way M3/M6/M7/M8/M9/M10 already
found.
