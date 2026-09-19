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

- [x] **M11 -- NPC dialogue text (`npcstrings.dat`)** (this session).
      `ShopDialogue` (`port/src/assets/shop_dialogue.h`/`.cpp`, `../src/
      Shop.java`'s `loadDialogue()`/`load()`/`loadGroup()`): 8 fixed-size
      groups (`GROUP_SIZES = {20, 20, 20, 20, 5, 22, 5, 41}`, checked
      against the file's own per-group count, mismatch throws), groups
      0-6 mapping 1:1 onto the 7-NPC roster (0-3 the quest-turn-in
      shopkeepers, 4 Beneca, 5 Helga, 6 Varus -- see M8's `WardenState`),
      group 7 a separate 41-entry generic/rumor string pool. **Data only,
      same scoping dawnstar's own M8 used** -- the actual `dialogue()`
      dispatcher logic (quest-turn-in state machine, Beneca/Helga's
      charge/rest/heal economies) is deferred to a later milestone, since
      it needs several `Player` inventory methods
      (`removeInventorySlot`/`initializeItemCharge`/`isItemCharged`/
      `hasCampMark`/`warpToCampMark`/`rollShopOutcome`) this port hasn't
      ported yet -- `player_creation.cpp`'s own small inventory helpers
      (`AddInventoryItemRaw`/`EquipItem`/`UnequipSlot`) only cover what
      character creation itself needed, not general-purpose inventory
      management.

      Unlike dawnstar's own `ShopDialogue`, there's no "is it bundled
      inside an archive or not" special case to handle here at all --
      Stormhold has no archive indirection for ANY resource (established
      since M2), so `npcstrings.dat` loads through the exact same
      `AssetRoot::OpenFile()` every other table does.

      Verified by `shop_dialogue_smoke.exe` against the real
      `npcstrings.dat`: all 8 groups at their exact expected sizes, every
      line non-empty, and every line reads as in-character, prison-camp-
      themed NPC dialogue fitting Stormhold's own premise (Arantamo:
      "I see they have captured another pearl to cast before the
      swine...", Varus: "You are blessed to be part of my plans to rid
      the Empire of evil..."). A nice independent confirmation the group-
      to-shop mapping is right: group 7's very first line, "You can't
      take that. Your inventory is full.", is exactly the message
      `Shop.dialogue()`'s own Beneca branch (action 7, training-failed
      path) returns as `dialogue[7][0]` -- a generic system message, not
      a rumor, landing at exactly the index the code reads for that
      purpose.

- [x] **M12 -- general-purpose player inventory management** (this
      session). `PlayerInventory` (`port/src/player/player_inventory.h`/
      `.cpp`): `addInventoryItemRaw`/`removeInventorySlot`/`unequipSlot`/
      `equipItem`/`equipLastPickedUpItem`/`initializeItemCharge`/
      `isItemCharged`/`tryPickUpItem`/`dropInventoryItem`/`hasCampMark`/
      `markCampAndReturnToTown`/`warpToCampMark`. M9's `player_creation.cpp`
      used to keep small private duplicates of the add/equip pieces it
      needed (`AddInventoryItemRaw`/`EquipItem`/`UnequipSlot`) -- moved
      here and made public, with `player_creation.cpp` now calling into
      this one shared implementation instead. **Still deliberately NOT
      included:** `rollShopOutcome()` (needs `skillValue()`/
      `rollOutcome()`, combat-adjacent stat/RNG machinery -- a later
      combat milestone's job) and any actual world-registry wiring for
      `tryPickUpItem`/`dropInventoryItem` (no persistent per-level
      dropped-item registry exists yet, same "caller supplies/owns world
      state" gap M10 already flagged for the analogous case) -- this
      still doesn't unlock the FULL `Shop.dialogue()` dispatcher (that
      needs `rollShopOutcome` too), but does unlock its item-turn-in/
      charge-consuming branches.

      **A real, confirmed original-game quirk, found while porting
      `tryPickUpItem`/`dropInventoryItem` and preserved rather than
      fixed:** both methods pack/unpack a 16-bit "packed value" (most
      often `Item.nextSpawnId()`'s result) across 2 SIGNED Java bytes.
      Reconstructing via `(record[hi] << 8) + record[lo]` sign-extends
      whenever that value's high byte is >= 0x80 (i.e. the value is >=
      32768), producing a NEGATIVE reconstructed value instead of the
      original positive one -- confirmed real Java `byte` arithmetic, the
      exact same shape as `Player.collectChestItem()`'s own analogous
      chest-value unpacking. Reproduced exactly (`std::array<int8_t, 7>`
      records, not `uint8_t`) rather than "corrected" to always
      round-trip cleanly; `player_inventory_smoke.cpp` demonstrates the
      precise reconstructed value for a synthetic out-of-range input
      (40000 comes back as -25536, hand-verified).

      Verified by `player_inventory_smoke.exe` against real
      `CharacterData`/`ItemDatabase`: removing an equipped item clears
      its equip slot and compacts the array correctly, re-adding and
      re-equipping round-trips the sign flip, item-charge init correctly
      gates on equipment category (confirmed refused for a real
      category-11 item), pick-up/drop round-trips a normal item and
      confirms item id 109 never produces a record while still removing
      its slot, the sign-extension quirk reproduces exactly as
      hand-derived, and the camp bookmark correctly distinguishes
      `markCampAndReturnToTown`'s (12, 14) death/respawn point from
      character creation's (9, 10) -- and confirms `warpToCampMark`
      genuinely never touches `facing` at all, matching a direct reading
      of that method.

- [x] **M13 -- combat resolution primitives** (this session).
      `PlayerCombatStats` (`port/src/player/player_combat_stats.h`/
      `.cpp`): `skillValue`/`skillBonus`/`defenseSkillValue`/
      `baseEvasion`/`bestArmorSkillIndex`/`activeWeaponSkillIndex`/
      `defenseSkillIndex`/`attackPower`/`attackAccuracy`/`weaponDamage`/
      `armorValue`/`isEffectActive`/`clearEffect`/`hasAilment`/
      `effectiveStat`, plus the static `rollOutcome()` hit-tier roll and
      `rollShopOutcome()`. Deliberately NOT `attack(Monster)` itself, or
      `gainSkillExp`/the rank-up/level-up cascade `consumeLevelExp()`
      triggers (leveling is a separate concern nothing here depends on)
      -- `attack()` needs a live Monster target (`stat()`/`takeDamage()`/
      `store()`), and there is no Monster runtime port yet, only M3/M6's
      static `MonsterDatabase`/generation-time spawn list. Everything
      ported here is entirely self-contained in `PlayerState` +
      `CharacterData` + `ItemDatabase`, matching dawnstar's own equivalent
      milestone's scoping exactly. `RollShopOutcome` takes `Shop`'s
      `interactionCount[shopId]` as an explicit parameter rather than
      reading a ported `Shop` static-state object -- no live `Shop` state
      exists yet (M11 only ported its dialogue TEXT) -- same "caller
      supplies/owns world state" pattern M8/M10/M12 already established.
      This still doesn't unlock the FULL `Shop.dialogue()` dispatcher on
      its own (that also needs a real `Shop` state object to hold
      `interactionCount`/`questState1`/`questState2`/etc.), but the
      combat-side blocker M11/M12 both deferred is gone.

      Two real, confirmed findings carried through from the source
      itself, not fresh discoveries: `isEffectActive`'s `-2` duration
      code is conditional on `lastCombatTargetId != 0` ("has attacked
      something at least once"), **not** `giftPointsFound` despite a
      decompiled field reference that could plausibly have meant either
      -- `../src/Player.java`'s own header comment already flags this as
      resolved by reading the bytecode directly, not guessed; carried
      through here rather than re-litigated. And `weaponDamage()`/
      `armorValue()` both reuse `itemsin.dat`'s `questFlags` COLUMN as
      the equipped item's raw damage/armor magnitude (not a mistake --
      confirmed the same "column reuse" dawnstar's own `Item.java`
      documents for its analogous methods).

      Verified by `player_combat_stats_smoke.exe`: `RollOutcome` cross-
      checked against an INDEPENDENT reimplementation of `java.util.
      Random` (a fresh script, not reusing this port's own code) for 6
      seed/chance combinations -- the first attempt at that
      cross-check actually disagreed with this port's output, which
      turned out to be a bug in the *verification script* (Python's `%`
      floors toward negative infinity; Java's truncates toward zero --
      the same category of pitfall `JavaAbs`/`RandomInt1Based`'s own doc
      comments already warn about), not this port's `RollOutcome` --
      fixed the script's modulo to truncate like Java's, then all 6
      cases matched. Also verified: combat stats against a real created
      Barbarian character (`AttackPower`/`AttackAccuracy`/`WeaponDamage`/
      `ArmorValue`/`DefenseSkillIndex`/`BaseEvasion` all cross-checked
      against the loaded `ItemDatabase`'s real magnitude/category
      columns, not just internal self-consistency), the unequipped-
      character fallback defaults, `IsEffectActive`'s three duration
      codes, and `EffectiveStat`'s Regeneration-bonus clamp.

- [x] **M14 -- monster runtime + combat resolution** (this session).
      `MonsterRuntime` (`port/src/monster/monster_runtime.h`/`.cpp`, plus
      the plain-data `MonsterState` in `monster/monster_state.h`):
      `pickMonsterType`/`spawn`/`stat`/`rawStat`/`typeName`/`isUndead`/
      `takeDamage`/`toBytes`/`fromBytes`/`move`/`isStairwayTile`/
      `distanceTo`/`isWithinRange`/`isAdjacent`/`chase`/`onDeath` --
      everything in `../src/Monster.java` except `tick()` (needs a live
      Player) and `attack(Monster)`'s Monster-side reads, both of which
      live in a new sibling module instead: `CombatResolution`
      (`port/src/combat/combat_resolution.h`/`.cpp`)'s `PlayerAttack`/
      `MonsterTick`. Same three-module split (monster/player never depend
      on each other, only combat depends on both) dawnstar's own M15
      uses.

      `readFrom`/`writeTo` (Monster's SECOND, unpacked stream
      serialization -- presumably the save-game format) is deliberately
      NOT ported yet: this port has no `BinaryWriter` at all (only
      `assets/binary_reader.h`, read-only, for loading the shipped
      `.dat` tables) and no save-format milestone exists yet either --
      left for that future milestone rather than invented early.
      `PickMonsterType`/`Spawn` reuse M6's existing
      `DungeonGenerator::MonsterTypeForTierBucket` rather than
      duplicating the tier/rarity-bucket table a second time.
      `PlayerAttack`/`MonsterTick` both skip their `gainSkillExp()`/
      `defenseSkillIndex()`-exp-award call (no leveling system ported
      yet -- same M13-flagged cross-system-coupling deferral) and skip
      `target.store()`/`Dungeon.spawnAmbushMonsters(3)` (no live
      per-level Monster/registry exists yet) -- every omission flagged
      at its exact call site, same discipline as `player_movement.h`'s
      own deferred side effects.

      Two real, confirmed Stormhold-specific divergences from dawnstar's
      own Monster, found while transcribing `Monster.java` directly
      rather than assumed to carry over: **(1)** `Monster.move()` does
      NOT call `store()` itself in the real Stormhold source (unlike
      dawnstar's own `Monster.move`) -- a genuine engine difference, not
      a simplification this port chose. **(2)** `Monster.chase(Player)`
      gates itself on `isWithinRange` (manhattan distance <= 3)
      internally before touching `chaseCadence` at all; dawnstar's own
      `Monster.chase(x,y)` has no such gate and isn't even boolean-
      returning the way a first draft here assumed by analogy --
      Stormhold's real `chase()` is `void`, corrected once actually
      checked against dawnstar's own `chase(x,y)` return type by reading
      both source files side by side rather than assuming parity.
      **(3)** `Dungeon.isWalkable()` (what `Monster.move()` actually
      calls) tests a DIFFERENT bit set (1/2/8/32) than
      `PlayerMovement::IsWalkableTileBits` (1/2/32, no bit 8) --
      confirmed by reading `Dungeon.java` directly rather than assumed
      to match the player's own walkability rule.

      Verified by `monster_combat_smoke.exe` against real
      `MonsterDatabase`/`ItemDatabase`/`CharacterData`/
      `DungeonGeometry`-derived data (not just internal self-
      consistency): `Spawn`'s starting HP against the type's real RAW
      column-14 read, `Stat`/`RawStat` agreement across all 17 columns
      for a real monster type, `TakeDamage`'s clamp-at-0,
      `ToBytes`/`FromBytes`'s full round trip (including a negative HP
      and a negative spawnId), `Move`/`IsStairwayTile` against a REAL
      generated level 2 (a real monster spawn's tile, a real adjacent
      wall tile, and the real per-level stairway direction fields),
      `IsAdjacent`'s aiPhase-reset side effect, `Chase`'s range gate and
      its exact 1-in-5 step cadence traced call-by-call over a real open
      corridor, `OnDeath`'s guaranteed-vs-rolled drop and record layout,
      and `PlayerAttack`/`MonsterTick` run across 20 seeds each against a
      real created character + real monster type, checking the
      invariants that must hold regardless of roll outcome (HP only ever
      goes down, never below 0, `lastCombatTargetId` always set,
      `aiPhase` always ends at 1). `RollOutcome` itself was already
      independently verified in M13 -- not re-proven here, since both new
      entry points build on that same already-checked function rather
      than reimplementing the tier formula a second time.

- [x] **M15 -- leveling** (this session). `PlayerLeveling`
      (`port/src/player/player_leveling.h`/`.cpp`): `gainSkillExp`/
      `tryRankUpSkills`/`consumeLevelExp`/`pendingLevelUpAttributeNames`,
      plus `ApplyLevelUpAttributeChoices` -- a new name for logic that
      isn't a single named Java method at all, but `ESGame.java`'s own
      inline level-up-confirm UI handler (its 3-step "choose an
      attribute" flow, screenGroup 39, lines ~1046-1069): +3/+2/+1 to 3
      caller-chosen attribute indices (weighted by pick ORDER, matching
      the UI's 3 steps exactly), then `computeDerivedStats()` +
      `consumeLevelExp()`. The 3 attribute choices are a real player
      DECISION with no UI to source them from in this port (same "caller
      supplies the missing decision" pattern as M9's traitor-index roll/
      M13's `interactionCount`). This unblocks the one gap M13/M14 both
      had to flag and defer: `combat/combat_resolution.h`'s
      `PlayerAttack`/`MonsterTick` now both actually call
      `PlayerLeveling::GainSkillExp` on a strong hit/successful block,
      instead of a comment noting the skip.

      `PlayerCreation::ComputeDerivedStats` is a new public method,
      split out of `ApplyClassTemplate`'s previously-inlined copy of the
      same formula (confirmed the exact same real call --
      `Player.java`'s own `resetState()` and `ESGame.java`'s level-up
      handler both call the identical `computeDerivedStats()` method) --
      needed here since leveling has to re-run it after an attribute
      boost, not just once at character creation.

      NOT ported: `Shop.clearQuestTurnInState()`, a confirmed real
      cross-system coupling `consumeLevelExp()` triggers on every
      rank-up (clearing all 4 quest-shops' turn-in progress) -- no live
      `Shop` state exists yet (M11 only ported its dialogue TEXT), so
      `ConsumeLevelExp` skips it, flagged at the skip site. Also NOT
      ported: neither `Player.attack(Monster)` nor `Monster.tick()`
      calls `tryRankUpSkills()` itself in the real source (confirmed by
      reading both methods in full during M13/M14) -- rank-up checking
      is some OTHER caller's separate per-tick concern in the original
      (not yet recovered -- same class of still-stubbed-caller gap M14
      already flagged for `Monster.tick()` itself), so a caller driving
      a real combat loop needs to call `TryRankUpSkills`/eventually
      `ApplyLevelUpAttributeChoices` on its own, `CombatResolution`
      doesn't do it for them.

      One real, confirmed finding: `levelUpAttributeFlags` is only ever
      RESET at character creation and only ever OR'd into by
      `tryRankUpSkills()` -- there is no confirmed call site anywhere in
      `Player.java` (or `ESGame.java`'s level-up-confirm handler) that
      ever clears an individual bit after its attribute point has
      actually been spent. Confirmed by grepping every reference to the
      field, not assumed: bits accumulate across the character's whole
      life and `pendingLevelUpAttributeNames()` can keep returning a
      long-since-spent attribute's name indefinitely. Ported exactly --
      `ApplyLevelUpAttributeChoices` does not clear any flags either,
      matching this real behavior rather than "fixing" it.

      Verified by `player_leveling_smoke.exe` against a real created
      character: `GainSkillExp`'s negative-index guard; `TryRankUpSkills`
      ranking up multiple skills in one call (one exactly-at-threshold,
      one over-threshold with its excess exp correctly preserved rather
      than discarded, one under-threshold correctly untouched), the
      correct governing-attribute bits set, level-exp granted once per
      rank-up, and the character-level-up threshold/return value at
      exactly 10 level-exp; `ConsumeLevelExp`'s exact -10;
      `PendingLevelUpAttributeNames` against real `CharacterData::
      attributeNames`; `ApplyLevelUpAttributeChoices`'s +3/+2/+1
      weighting (with two overlapping attribute picks, to check each
      weight independently) and its `ComputeDerivedStats` recompute
      actually reflecting the new attribute values, not stale ones; and
      an integration check running `PlayerAttack`/`MonsterTick` across
      200 seeds each against a real created character + real monster
      type, confirming skill exp genuinely increases on at least one
      seed's strong-hit/successful-block outcome -- `RollOutcome` itself
      isn't re-verified here (already done in M13).

- [x] **M16 -- the live per-level registry** (this session).
      `WorldRegistry` + `DungeonRuntime`
      (`port/src/dungeon/dungeon_runtime.h`/`.cpp`, new `stormhold_dungeon`
      library): `StoreMonster`/`RemoveMonster`/`MonsterAt`/`StoreChest`/
      `RemoveChest`/`AddDroppedItem`/`RemoveDroppedItem`/
      `CountDroppedItemsAt`/`FirstDroppedItemAt`/`DroppedItemsAt`/
      `RefreshTileFlags`/`SpawnAmbushMonsters` -- `ESGame.monsters[]`/
      `chests[]`/`droppedItems[]` and every `Dungeon.java` method that
      manages them. Same 3-way milestone split as dawnstar's own
      precedent (its M22/M23/M24): this one scopes to the registry +
      management methods ONLY, matching dawnstar's M22 exactly --
      wiring `player/player_movement.h`/`player/player_inventory.h`/
      `combat/combat_resolution.h`'s own already-deferred call sites to
      actually populate/consume a `WorldRegistry` (dawnstar's M23), and
      registering M6's generation-time `GeneratedMonsterSpawn`/
      `GeneratedChestSpawn` output into one (dawnstar's M24), are both
      left for later milestones. `stormhold_dungeon` depends on
      `stormhold_monster`/`stormhold_world` only -- `stormhold_player`/
      `stormhold_combat` depend on neither of those back to this
      module, so there's no cycle.

      Confirmed Stormhold-specific divergence from dawnstar's own
      `WorldRegistry` (M14's finding, reconfirmed here by reading
      `Dungeon.java` directly): `WorldRegistry::monsters` is
      **spawnId-keyed** (`unordered_map<int16_t, ...>`), not
      position-keyed like dawnstar's -- matching Stormhold's real
      `Monster.store()`. `chests` stays position-keyed (`PackTileKey`,
      a plain-int stand-in for `Util.posKey`'s string), and
      `droppedItems` stays an unordered per-level list, matching
      dawnstar's shapes for those two. `MonsterAt` has to linear-scan
      the whole spawnId-keyed registry by position -- confirmed the
      real `Dungeon.monsterAt(x,y)` does exactly this too, it has no
      position index for monsters either.

      **A real, necessary data-model addition, not scope creep:**
      `GeneratedLevel` gained a new `rooms` field
      (`GeneratedRoomRect`, `world/dungeon_generator.h`) -- `Dungeon.
      spawnAmbushMonsters(count)` needs real room bounding boxes to
      pick a random position from (Dungeon.java's own do-while loop:
      pick a random room, then a random point in ITS bounding box, not
      any random walkable tile on the whole level), and M6 had never
      exposed `PopulateLevel`'s own internal room list on
      `GeneratedLevel` before now -- only its two spawn-list SUMMARIES
      (`monsters`/`chests`). `BuildHubLevel` leaves `rooms` empty (no
      RNG, no room list at all, hand-carved). `SpawnAmbushMonsters`
      itself reproduces the original's RNG draw order per retry attempt
      exactly (room index, then monster type, then x, then y --
      re-rolled on EVERY rejected/non-walkable attempt, not just the
      final one, since that's load-bearing for later RNG-stream
      determinism); the one confirmed simplification is that
      `spawnIdCounter` only advances once per ACTUALLY-placed monster,
      not once per rejected attempt too (the real game's `nextSpawnId()`
      burns a spawnId on every attempt) -- deliberate, since spawnId's
      specific numeric value has no confirmed observable effect
      anywhere in the source, only ever used as a registry key.

      `RemoveMonster` throws `std::runtime_error` on an unregistered
      spawnId rather than reproducing a crash: the real
      `ESGame.killMonster()` reads `record[4]`/`record[5]` BEFORE its
      own `if (record != null)` null-check (reconfirmed directly, a
      real latent `NullPointerException` for exactly this case, first
      flagged in M14) -- not reproducible as a genuine crash in C++
      without deliberately dereferencing something invalid, so this
      surfaces the same "should never happen, but the original doesn't
      guard it either" condition loudly instead, same discipline
      `player/player_movement.h`'s `ComputeMoveTarget` already
      established for its own no-neighbor edge case. `RefreshTileFlags`
      SIMPLIFIED: skips the hub-town `Shop.wardenPresent` bit-32 refresh
      the original also performs there -- no live Shop/Warden state is
      wired to a `WorldRegistry` yet (M8's `WardenState` stays its own
      standalone, caller-supplied object).

      Verified by `dungeon_runtime_smoke.exe`: monster store/remove/
      lookup (including the wall-tile guard on `MonsterAt` and the
      confirmed-throw on an unregistered spawnId), chest store/remove
      (including its wall-tile guard blocking removal, and a safe no-op
      on a second removal), dropped-item add/remove/count/first/all
      with two distinct records sharing one tile (the presence bit stays
      set until the LAST one is removed), `RefreshTileFlags` rebuilding
      all 3 presence bits from registries alone on an otherwise-blank
      level, and `SpawnAmbushMonsters` against a REAL generated level 2
      (confirming its room list is non-empty, exactly 3 new registry
      entries appear, every spawned monster's tile carries the
      monster-presence bit and falls inside SOME real room's bounding
      box, and `spawnIdCounter` advances by exactly 3).

- [x] **M17 -- wiring the WorldRegistry into player_movement/
      player_inventory/combat_resolution** (this session). Dawnstar's own
      M23 equivalent -- the wiring half of the 3-way split M16 (dawnstar's
      M22) deliberately deferred. No new module: `player/player_movement.h`,
      `player/player_inventory.h`, and `combat/combat_resolution.h` all
      gained `WorldRegistry&`/`GeneratedLevel&` parameters and now call
      into M16's `DungeonRuntime` for real, replacing 4 separate
      "SIMPLIFIED, no live registry yet" skip points flagged back at
      M10/M12/M14:
      - `PlayerMovement::CommitMove` now runs `Player.commitMove()`'s
        dropped-item auto-loot block for real (single-item and
        multi-item-on-one-tile cases, the locked-item early return, and
        `giftPointsFound` accumulation), plus `autoMarkCampOnTile()` (a
        bit-8 tile auto-triggers `PlayerInventory::MarkCampAndReturnToTown`
        then immediately clears `justMarkedCamp` back to false, matching
        the original's own sequence).
      - `PlayerInventory::DropInventoryItem` now calls `DungeonRuntime::
        AddDroppedItem` itself (`GeneratedLevel&`/`WorldRegistry&` added
        as explicit params, standing in for the original's own
        `this.currentDungeon()` session lookup) instead of just handing
        the caller a record to insert somewhere.
      - `CombatResolution::PlayerAttack` now calls `DungeonRuntime::
        StoreMonster` for real (`target.store()`); `CombatResolution::
        MonsterTick`'s ailment-2 ("swarm curse") branch now calls
        `DungeonRuntime::SpawnAmbushMonsters` for real instead of a no-op.

      **A real, confirmed bit-test asymmetry, found while transcribing
      `commitMove()`'s dropped-item block character-by-character rather
      than assuming its "one item here" and "several items here" branches
      share one rule:** both branches gate whether a picked-up gift-
      category item's subtype adds to `giftPointsFound` on `record[6]`'s
      bit 2 ("was this item already possessed before" -- set by
      `Player.dropInventoryItem()`, clear on a fresh monster-death/chest
      drop) -- but the SINGLE-item branch tests `(record[6] & 2) == 0`
      (grants points only for a NEVER-possessed item) while the
      MULTIPLE-items-on-one-tile branch tests `(record[6] & 2) != 0` (the
      OPPOSITE: grants points only for an ALREADY-possessed item).
      Reads like a copy-paste inversion bug in the original, not
      something this port introduced -- preserved exactly rather than
      unified into one consistent rule, with `m17_world_wiring_smoke.cpp`
      demonstrating both branches' opposite behavior directly against the
      same kind of item.

      **Two related side effects `Player.commitMove()`'s dropped-item
      block also performs, deliberately left SKIPPED (not silently
      dropped -- flagged at their exact omission point):**
      `ESGame.getGameAdvancementLevel()`/`checkOpenAndPopulateDungeons()`
      -- the progressive zone-opening system a gift-point gain can
      trigger -- has no live `ESGame` session object to open zones on in
      this port; `p.giftPointsFound` itself still accumulates correctly
      (pure `PlayerState` arithmetic), only the dungeon-opening side
      effect is dropped. And `Shop.wardenPresent`'s on-any-step clear /
      the level-37-entry forced-respawn of the type-41 "roaming" monster
      both stay unwired despite `WorldRegistry`/`WardenState` individually
      now existing -- wiring a `WardenState&` through `CommitMove` (and
      the level-37 special case) is scoped OUT of this milestone on
      purpose, matching what docs/PORT_ROADMAP.md's own "what's next"
      already committed M17 to, rather than silently expanding scope.

      **`CombatResolution::MonsterTick`'s new `ambushRng` parameter is
      deliberately a SEPARATE `JavaRandom&` from `globalRng`:** the real
      `Dungeon.spawnAmbushMonsters()` draws from that `Dungeon` INSTANCE's
      own persisted `this.rng` (the same per-level generator stream
      `Dungeon.generate()` seeded once, that keeps advancing across the
      level's whole live lifetime), never `ESGame`'s shared roll RNG
      `globalRng` stands in for here -- this port has no persisted
      per-level RNG object at all (`GeneratedLevel` doesn't carry one),
      so the caller supplies whichever stream stands in for that level's
      own, same "caller supplies/owns world state" pattern as everywhere
      else in this port.

      Verified by a new `world_wiring_smoke.exe` (single-item gift pickup
      granting `giftPointsFound`, the same but already-possessed
      correctly granting NOTHING, a locked item setting
      `pendingLockedItemFlag` and skipping pickup with an early `return
      true`, the confirmed multi-item bit-test asymmetry demonstrated
      directly against two real category-11 items with distinct
      subtypes, a non-gift item picking up cleanly with zero
      `giftPointsFound` change, the auto-camp-mark side effect capturing
      the JUST-STEPPED-ONTO tile as the bookmark, and a full drop-then-
      walk-away-then-walk-back-and-re-pick-up round trip against a real
      created character) plus new coverage added directly to the
      existing `m14_monster_combat_smoke.cpp` (`PlayerAttack`'s
      `target.store()` now actually registers the post-damage record on
      any connecting hit across 20 seeds; `MonsterTick` against a real
      monster type whose RAW column-11 is confirmed 2 ("swarm curse"),
      confirming `SpawnAmbushMonsters` fires and registers exactly 3 new
      monsters on at least one of 20 seeds) -- `RollOutcome`/
      `SpawnAmbushMonsters`'s own RNG-order/room-bounding-box correctness
      aren't re-verified here, already covered by M13/M16.

- [x] **M18 -- registering the live registry from world generation itself**
      (this session). Dawnstar's own M24 equivalent -- the last piece of
      the 3-way registry split M16 (dawnstar's M22)/M17 (dawnstar's M23)
      worked through. Since M6, `GeneratedLevel::monsters`/`::chests`
      have held the room-monster/chest spawns `DungeonGenerator::
      PopulateLevel`/`PlaceChests` compute as plain output data -- a
      deliberate placeholder, since no live registry existed yet to put
      them in. In the original, `Dungeon.populate()`'s
      `spawnRoomMonsters()`/`placeChests()` register directly into
      `ESGame.monsters[]`/`chests[]` (`Monster.spawn(...).store()`,
      `Dungeon.storeChest(record)`) as part of generation itself -- so
      until this milestone, a `WorldRegistry` would have stayed
      permanently empty for any level this port ever generates, only
      ever gaining whatever M17's own dynamic wiring added later at
      runtime. New `DungeonRuntime::RegisterGeneratedSpawns(level, world,
      monsterDb)` (`dungeon/dungeon_runtime.h`/`.cpp`) closes that gap:
      converts each `GeneratedMonsterSpawn` into a full 28-byte
      `Monster.toBytes()` record via the already-existing
      `MonsterRuntime::Spawn`, and each `GeneratedChestSpawn` into the
      real 8-byte chest record layout, keyed exactly like every other
      registry entry (spawnId for monsters, position for chests -- M14/
      M16's own confirmed keying). Deliberately does NOT touch
      `level.tiles` -- `DungeonGenerator` already sets the monster (2)/
      chest (16) presence bits itself while building `level.tiles`, so
      this only adds the missing registry side, matching the real
      `Monster.store()`/`Dungeon.storeChest()`'s own actual behavior
      (neither sets a genuinely NEW bit here either). Unlike dawnstar's
      own M24, no `GeneratedMonsterSpawn` data-model change was needed --
      Stormhold's own M6 already gave it a `spawnId` field from the
      start (a per-level-local counter, `roomIndex + 1`, same reasoning
      `GeneratedChestSpawn::spawnId` already used), so this milestone is
      purely new registration logic, not a data-model catch-up.
      `RegisterGeneratedSpawns` couldn't live inside `world/
      dungeon_generator.h` itself: `stormhold_world` is a dependency OF
      `stormhold_dungeon`, so the reverse would cycle -- same constraint
      M16's own class comment already documents for this whole module's
      placement.

      **A real, confirmed and unavoidable simplification, caught while
      transcribing `Dungeon.placeChests()`'s literal record-building
      code (not needed since M6 only ever stored a semantic `itemId`
      summary, never the literal packed bytes):** the real record's byte
      3 packs a random `[0,2]` "tier bits" value into its top 2 bits
      alongside `tier` in its low 6 -- M6's own `PlaceChests` already
      draws that random value for RNG-stream-order fidelity but discards
      the RESULT (no confirmed reader anywhere in `../../../src/`), so
      `GeneratedChestSpawn` has nowhere to carry it forward. This
      milestone's reconstructed record therefore always has those top 2
      bits as 0, unlike what the real record would contain in memory --
      `tier` itself (the low 6 bits) still matches exactly, and nothing
      in this port ever reads the top 2 bits back either way, so this
      has no observable effect on anything the port models.

      **A second, independently-confirmed subtlety, found by reading
      `Dungeon.placeChests()`'s byte[4]/byte[7] split character-by-
      character rather than assuming it mirrors `Item.rollLoot()`'s own
      packing rule:** `rollLoot()` decides internally whether to return
      an extended (2-byte-packed) item id based on the rolled rarity
      COLUMN being 1 (`assets/item_database.h`'s own `RollLoot` doc
      comment); `placeChests()`, entirely separately, re-splits whatever
      int it got back into the record's low/high storage bytes based on
      a completely different, purely literal check -- `if (low == 86)`
      -- unrelated to why that value was produced. Both checks are
      transcribed exactly as their own separate things, not folded into
      one; `m18_registered_spawns_smoke.cpp` demonstrates the literal
      `low==86` gate directly (22 of the 180 real generated chests hit
      it) rather than assuming it lines up with the rarity-column rule.

      Verified by a new `registered_spawns_smoke.exe` (no JVM ground
      truth, same reason as M6): the hub town (no room-monster/chest
      generation at all) registers nothing; every one of the 36 standard
      levels' registered monster/chest counts match their generated
      counts exactly (540 monsters, 180 chests total); every generated
      spawn's registry entry round-trips back to the exact same type/hp/
      position/spawnId/dungeonLevel (monsters) or position/tier/itemId/
      spawnId bytes (chests, including the confirmed always-zero byte 2
      and the extended-id branch, actually exercised by 22 of the 180
      real chests) with the presence tile bit already set from
      generation; exactly one guaranteed-gift chest per level (36
      total); and, as an integration check going beyond M16/M17's own
      tests (which only ever exercised dynamically-spawned/synthetic
      monsters), `DungeonRuntime::RemoveMonster` correctly removes a
      REAL monster this milestone registered from actual world
      generation. All checks passed on the first attempt; full clean
      rebuild stayed at zero warnings; all 17 smoke tests pass.

## What's next

M19 onward: likely a `WardenState`-into-movement wiring pass (`Shop.
wardenPresent`'s on-any-step clear, deliberately left out of M17) and
the level-37-entry forced-respawn special case player_movement.h's
CommitMove still flags as skipped; then the player save format
(Monster's own `readFrom`/`writeTo` included, plus whatever caller
actually drives `tryRankUpSkills()`/`Monster.tick()`/`onDeath()` in the
original -- none of their real callers are recovered yet, see
`GameCanvas.java`'s remaining stubbed methods); and eventually starting
on the rendering/UI side (`GameCanvas`'s ~15 still-stubbed pixel methods)
now that a real, wired-together game-logic core exists to render --
following dawnstar's own later milestones roughly but expecting further
Stormhold-specific divergences the way
M3/M6/M7/M8/M9/M10/M12/M13/M14/M16/M17/M18 already found.
