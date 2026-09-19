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

- [x] **M19 -- the last two CommitMove side effects: Warden clearing +
      level-37 boss respawn** (this session). `player/player_movement.h`'s
      `CommitMove` gains `monsterDb`/`warden` parameters and now wires the
      two side effects M17 deliberately scoped out (flagged there as
      needing `WorldRegistry`/`WardenState` individually, but not yet
      threaded through):
      - Entering level 37 from anywhere else
        (`p.pendingLevel == 37 && p.currentLevel != 37`) scans every
        monster M18's `RegisterGeneratedSpawns` already put into level
        37's registry and fully heals the one with `typeIndex == 41`
        (the forced "roaming" monster from M6's own last-room special
        case) back to the type's own max-HP column, then re-stores it --
        a "level 37's boss is always at full health when you arrive"
        mechanic. Moving WITHIN level 37 (not entering it) never
        re-triggers this, matching the original's own `currentLevel !=
        37` guard exactly.
      - A successful STEP (not a turn) clears `WardenState::present`
        straight to `false` if it was set.

      **A real, confirmed inconsistency, found by reading `Player.java`'s
      own `commitMove()` character-by-character rather than assuming it
      calls `Shop.wardenLeaves()` (M8's own `WardenState::Leave`) the way
      a first read might expect:** it doesn't. This is a direct,
      unconditional `Shop.wardenPresent = false;` -- no `wardenLeaves()`
      call, no tile mutation at all, and no `GeneratedLevel&` for the hub
      even needed at this call site. That means taking a single step
      ANYWHERE in the game while the Warden happens to be visiting
      silently makes `warden.present` read `false` again, while the
      hub's own tile bit 32 at Varus's position (set by `WardenState::
      Arrive`) stays SET until whatever caller actually drives
      `WardenState::Leave` on its own schedule eventually runs -- that
      caller still isn't recovered (same class of gap as `Monster.
      tick()`'s own missing driver, `tryRankUpSkills()`'s, and now
      `Monster.onDeath()`'s, none of which have a confirmed call site in
      `../src/` yet). Reads like the original genuinely intends
      `wardenPresent` as a lightweight "has the player acted since he
      arrived" gate, distinct from the tile's own visible state -- ported
      exactly as two independent mechanisms, not unified or "fixed" into
      one consistent story.

      `(byte) m.stat(14)`'s masked-then-narrowed HP reset is ported as
      `static_cast<int8_t>(MonsterRuntime::Stat(m, monsterDb, 14))`
      rather than jumping straight to the equivalent (and already-
      established) `RawStat` shortcut, to mirror the original's exact
      call -- both are proven bit-identical (masking to `uint8_t` then
      narrowing back to `int8_t` always reproduces the same underlying
      byte `RawStat` reads directly), noted in the method's own doc
      comment rather than silently substituted.

      All existing `CommitMove`/`Move` callers across
      `m10_player_movement_smoke.cpp`/`m17_world_wiring_smoke.cpp` were
      updated to pass a `MonsterDatabase`/`WardenState` (none of their
      own scenarios touch level 37 or the Warden, so a default-
      constructed pair changes nothing about what they were already
      checking).

      Verified by a new `warden_and_boss_respawn_smoke.exe`: a damaged
      type-41 monster on level 37 gets healed back to its real
      `monstersin.dat` max-HP column on entry from another level while an
      also-damaged non-type-41 monster on the same level stays untouched;
      moving within level 37 itself never re-triggers the heal; a
      successful step clears `warden.present` while leaving
      `WardenState::visitCount` (only touched by `Arrive`/`Leave`)
      untouched and never even looking up a hub-level object; and a TURN
      (dir 3/4) leaves `warden.present` alone entirely, confirming the
      real `isStep`-only gate. All checks passed on the first attempt;
      full clean rebuild stayed at zero warnings; all 18 smoke tests
      pass.

- [x] **M20 -- the player save format** (this session). New
      `assets/binary_writer.h` (a `BinaryReader`-mirroring big-endian
      writer, wrapping `std::ostream&` to match `BinaryReader`'s own
      `std::istream&` convention -- the first writer this port has had
      at all). `player/player_save.h`/`.cpp`'s `PlayerSave::ToBytes`/
      `FromBytes` port `Player.toBytes(true)`/`fromBytes(data, true)` --
      Player.java's own header comment calls this "the complete
      in-progress save" actually used by save/load, as opposed to
      `toBytes(false)`/`fromBytes(data, false)`'s lightweight
      "character summary" format (most likely a high-score/leaderboard
      record). `monster/monster_runtime.h`'s `ReadFrom`/`WriteTo`
      (`Monster.readFrom`/`writeTo`, deferred since M14 pending exactly
      this `BinaryWriter`) land alongside it, closing that gap too.

      **Deliberately NOT ported: the `full=false` lightweight format**
      -- same deferral dawnstar's own M12 `PlayerSave` already made for
      its own equivalent, for a related reason: it needs
      `applyClassTemplate()`+`resetState(classIndex, false)`'s "new
      character" reconstruction run BEFORE the serialized fields layer
      on top of it (overwriting only SOME of what that reset just set --
      inventory/equipment/position are left at whatever the fresh reset
      produced, never re-read by this format at all). Unlike dawnstar's
      own case (complicated by a hidden "traitor index" RNG roll baked
      into creation), Stormhold's own creation pipeline has NO
      randomness at all (M9's own finding) and already exists as a
      directly-reusable building block (`player/player_creation.h`'s
      `CreateCharacter`) -- so this is a scope choice, not a blocker,
      left for whichever later milestone actually needs a leaderboard-
      style summary record rather than the real save/load path this
      milestone unblocks.

      **A real, confirmed divergence from dawnstar's own equivalent
      finding, caught by reading `Monster.readFrom`/`writeTo` and
      `toBytes`/`fromBytesShared` side by side rather than assuming
      dawnstar's own "two genuinely different serializations" result
      carries over:** it doesn't. Stormhold's `readFrom`/`writeTo`
      encode the EXACT SAME 28 fields in the EXACT SAME order as
      `toBytes()`/`fromBytes*()` -- just written through
      `DataInputStream`/`DataOutputStream` primitive calls instead of
      manual bit-shifting into a `byte[]`. `ReadFrom`/`WriteTo` are
      still implemented as their own direct field-by-field stream calls
      (not delegating to `ToBytes`/`FromBytes` internally) for
      line-for-line fidelity with `Monster.java`'s own two separate
      methods, but `m20_save_format_smoke.cpp` demonstrates the
      byte-for-byte identity directly rather than just asserting it in
      a comment. The 64-bit `unconfirmedTimestamp` is composed from two
      `WriteU32`/`ReadU32` halves rather than adding a dedicated
      `WriteS64`/`ReadS64` to `BinaryWriter`/`BinaryReader` -- matching
      dawnstar's own port's identical choice for the same field, and
      keeping `BinaryWriter`'s surface area to exactly what this port
      actually needs.

      `PlayerState` gained the save format's last two previously-unported
      fields (`unconfirmedIntField`, `unconfirmedFlag2`) -- both already
      flagged in `Player.java`'s own header comment as having "no
      confirmed meaningful read/write site beyond (de)serialization,"
      carried through here unchanged rather than guessed at. Confirmed,
      by reading `toBytes(true)`/`fromBytes(data, true)` in full, that
      every OTHER `PlayerState` field this port already carries
      (`pendingLevel`/`pendingTileX`/`pendingTileY`/`pendingFacing`,
      `prevTileX`/`prevTileY`, `crossingLevelBoundary`,
      `enteredNewLevelZone`/`leftLevelZone`, `justMarkedCamp`,
      `pendingLockedItemFlag`) genuinely is NOT part of the real save
      format -- all per-tick/per-turn transient scratch state recomputed
      fresh on the next move, matching the original exactly, not an
      oversight in this milestone.

      Still deliberately left unwired, same as M19's own "what's next"
      already flagged: whatever caller actually drives
      `tryRankUpSkills()`/`Monster.tick()`/`Monster.onDeath()` in the
      original isn't recovered in `../src/` (confirmed by grepping every
      one of those three call names again this session) -- no save-
      format work changes that; not invented here either.

      Verified by a new `save_format_smoke.exe`: `BinaryWriter`/
      `BinaryReader` round-trip every primitive including sign-extension
      edge cases (a negative byte/short/large-negative int, a large
      unsigned 32-bit value); `MonsterRuntime::WriteTo`'s output is
      asserted BYTE-FOR-BYTE identical to `ToBytes()`'s own packed array
      for a monster exercising every sign-boundary case (negative
      spawnId/HP, a large 64-bit timestamp), then `ReadFrom` round-trips
      all of it back; and `PlayerSave::ToBytes`/`FromBytes` round-trip a
      real created character (M9) with every full-save field mutated to
      a varied, non-default (including several negative) value, checked
      field-by-field, while separately confirming every deliberately-
      excluded transient field comes back at `PlayerState`'s own default
      rather than accidentally round-tripping. All checks passed on the
      first attempt; full clean rebuild stayed at zero warnings; all 19
      smoke tests pass.

- [x] **M21 -- corridor wall-segment selection logic** (this session).
      Starts the rendering side dawnstar's own port precedent puts here
      (its M9), but a real, consequential discovery changed this
      milestone's actual shape: unlike dawnstar, whose `GameCanvas.java`
      phase-1 pass was already a COMPLETE transcription before Phase 3
      rendering work ever started, `../src/GameCanvas.java`'s own header
      comment confirms it was only ever a "PARTIAL PASS" -- its ~15
      private pixel-rendering methods were left as signature-only stubs
      (`throw new UnsupportedOperationException(...)`), never actually
      read from `decompiled/e.java`. There was nothing to port yet. This
      milestone did the missing phase-1 work for exactly ONE of those
      stubs -- `paintWalls()` (was `e.java`'s `j(Graphics)`) -- confirmed
      by structure (`wallSegmentTable`, `Player.corridorView` via the
      newly-confirmed `Dungeon.viewGridAt`, `Player.hasAilment(3)`/`(4)`)
      to be dawnstar's own `paintCorridorWalls()` equivalent, then ported
      its SELECTION logic to C++ as data only, matching dawnstar's own M9
      scope exactly ("deliberately stops short of drawing actual
      pixels"). New `dungeon/dungeon_runtime.h` methods `TileAt`
      (`Dungeon.tileAt(x,y)`'s cross-level-boundary tile lookup, reusing
      the same recentering math `player/player_movement.h`'s
      `ComputeMoveTarget` already established, via its own independently-
      declared `LevelLookup` caller-supplied callback -- `stormhold_dungeon`
      can't depend on `stormhold_player` to share that type directly) and
      `SampleCorridorView`/`ViewGridAt` (`Dungeon.sampleCorridorView`/
      `viewGridAt`, the corridor renderer's own 9x5 visibility-sample
      grid and its relative-offset reader); new `render/
      corridor_render_plan.h`/`.cpp` (`stormhold_render`, a new library)
      port `paintWalls()`'s wall-segment selection loop itself as
      `CorridorRenderPlan::Plan()`, plus its two direct helpers
      `drawWallSegment()`/`resolveWallFrame()` (now real Java in
      `GameCanvas.java` too, `resolveWallFrame` byte-for-byte identical
      to dawnstar's own).

      **Two real, confirmed architectural simplifications vs. dawnstar's
      own `paintCorridorWalls()`, found by reading `paintWalls()`/
      `drawWallSegment()` in full rather than assumed to mirror
      dawnstar's shape:** only ONE wall bit is ever tested (bit 1, plain
      wall) -- there is no dawnstar-style bit-64 "gate/edge" branch
      anywhere in this method at all; and there is no per-dungeon-number
      texture switch either -- `floorTexture`/`wallTexture` are each a
      SINGLE shared `Image` (matching this class's own field
      declarations exactly), not dawnstar's 5 separate ice/plain/gate
      Image fields. `drawWallSegment()`'s own mirroring trick for
      "frame > 7" uses Nokia `DirectGraphics`' `TRANS_MIRROR` flag on the
      SAME spritesheet rather than a second stored mirrored image, and
      has no `wallDrawnNear`/`wallDrawnMid` dedup state at all (dawnstar's
      version has both). Whether `floorTexture`/`wallTexture` load from a
      `.cus` file (M7's `RawImage` format) or a plain MIDP-native `Image`
      resource is left an open question for whichever milestone actually
      wires image loading -- none of M7's own 37 confirmed `.cus` files
      read as a wall/floor texture by name.

      **A second real finding, incidental to identifying `paintWalls()`
      correctly:** at least one OTHER still-stubbed method's existing
      "(was e.java's X(Graphics))" placeholder comment turns out to be
      WRONG -- it was apparently assigned by call-order/signature
      guesswork during the original partial pass, never verified against
      real content. `paintFloor()`'s claimed `b(Graphics)` body actually
      reads `player.ad`-shaped data (looks like `paintObjects()`'s real
      counterpart instead), and `paintMessagePopup()`'s claimed
      `e(Graphics)` body reads/writes the monster-hit/spell-hit/self-
      spell flash flags (`unconfirmed_S`/`_ao`/`_am`), not a message
      popup at all. Flagged directly in `GameCanvas.java`'s own header
      comment and `CLASS_MAP.md` -- NOT corrected here. Re-identifying
      every remaining stub's real decompiled counterpart is a separate,
      larger follow-up phase-1 pass, not part of this milestone's scope.

      **Deliberately does NOT wire `SampleCorridorView`/`corridorView`
      into `PlayerState`/`CommitMove` at all** -- `PlayerState` has no
      `corridorView` field yet, so `CorridorRenderPlan::Plan()` takes an
      already-sampled `CorridorViewGrid` as a caller-supplied input
      instead, same "caller supplies/owns world state" pattern this port
      uses everywhere. Wiring a live, always-current
      `PlayerState::corridorView` (and `refreshVisibleObjectSlots()`'s
      own eventual port, which reads the identical grid) is left for
      whichever later milestone actually needs one -- likely the real
      pixel renderer itself.

      Verified by `javac` re-confirming the whole `../src/` tree still
      compiles clean (zero errors, only the same expected `new
      Integer(int)` deprecation warnings already documented) after the
      `GameCanvas.java` edit, and by a new `corridor_render_plan_smoke.exe`:
      `ViewGridAt`'s depth<4-vs->=4 indexing hand-checked; `TileAt` checked
      directly within a level, with no neighbor in each of the 4
      directions (returns wall), and crossing a REAL hub<->standard
      boundary with the exact same recentering `player_movement_smoke`'s
      own M10 test already confirmed for player movement; a confirmed,
      preserved asymmetry in `SampleCorridorView` (one grid slot is a
      literal constant 0 for east/west facing but a real tile read for
      north/south facing) demonstrated directly; a synthetic wall placed
      one tile ahead correctly producing matching wall segments from both
      the forward AND mirrored scans; `Plan()`'s ailment-3/4 floor-flag
      priority (3 beats 4) exercised for all 4 combinations; and an
      integration pass sampling all 4 facings against a REAL M6-generated
      level 2 with zero exceptions and every produced frame/x value in
      its valid range. All checks passed on the first attempt; all 20
      smoke tests pass.

- [x] **M22 -- the rest of `GameCanvas`'s stubbed paint methods** (this
      session). Pure phase-1 Java transcription, no C++ this time (the
      pixel compositor M21 deferred still doesn't exist, so there's
      nothing yet to port these TO) -- every remaining `paint*` stub
      (`paintFloor`/`paintObjects`/`paintHud`/`paintUnknown_l`/
      `paintMessagePopup`/`paintUnknown_b`/`paintHotbar1`/`paintHotbar2`)
      plus the non-paint helpers they directly depend on
      (`isNpcDialogueDue()`, needed by the new `paintHud()`) is now real,
      read directly from `decompiled/e.java` end to end.

      **Confirmed and fixed THREE real mapping bugs the M21 pass had
      flagged or missed, rather than just documenting them this time:**
      the old `paintFloor()` (`b(Graphics)`) never painted a floor --
      `paintWalls()` already does that itself -- its real body renders
      visible chests (8-byte)/dropped items (7-byte, M17/M18's own
      confirmed record lengths) on corridor tiles, renamed
      `paintObjects()`. The old `paintObjects()` (`a(Graphics)`) didn't
      render objects -- its real body draws the HP/Magicka/Fatigue
      status-bar triad via the newly-needed `Player.effectiveStat()`/
      `coreStats`, renamed `paintStatusBars()`. The old
      `paintMessagePopup()` (`e(Graphics)`) was actually the monster-hit/
      spell-hit/self-spell-hit flash overlay, renamed
      `paintFlashOverlays()`; the REAL message popup was sitting under
      the previous pass's `paintUnknown_l()` (`l(Graphics)`, the
      `messageLines`/`unconfirmed_ad`-gated rounded box), which now
      takes over the `paintMessagePopup()` name. `paintHotbar1()`/
      `paintHotbar2()` (`k(Graphics)`/`h(Graphics)`) turned out to be the
      two minimap zoom levels (`minimapTileGrid`/`visibleTileGrid`), not
      a hotbar at all -- renamed `paintMinimapZoomedOut()`/
      `paintMinimapNormal()`. Their dispatch in `paintGameView()` also
      had a genuine bug carried over from the very first partial pass:
      it gated on `hotbarActionSet` (decompiled `aq`, the numeric-hotkey
      selector) instead of `hotbarContext` (decompiled `f`, the field
      actually cycled by the `*` key and the one `e.java`'s own
      dispatcher actually tests) -- fixed directly, not just flagged.
      `paintHud()` (`d(Graphics)`) and `paintUnknown_b()` (`b(Graphics,
      int)`) turned out to already be correctly named; only their
      bodies were missing.

      New helper methods needed to make the real bodies callable, kept
      one-to-one with `e.java`'s own private methods rather than
      collapsed together (same discipline M21's `drawWallSegment`/
      `resolveWallFrame` split already established):
      `renderObjectAt`/`renderObjectNear`/`renderObjectMid`/
      `renderObjectFar`/`hasCrystalGlow`/`drawRawImageFull` (the objects
      family); `renderMonsterSpriteForSlot`/`monsterNearZoneRow`/
      `monsterMidZoneRow`/`monsterFarZoneRow`/`renderMonsterNearSprite`/
      `renderMonsterOrIconSprite`/`renderMonsterMidZoneSprite`/
      `drawMonsterZoneFrame`/`renderMonsterFarZoneSprite`/
      `renderWardenCompassIcon`/`drawRawImageFrame` (the monster/Warden-
      icon family, also reused directly by `paintUnknown_b()`'s NPC/
      shop-portrait icons and by `paintMonsters()`'s own Warden-String
      branch, which reuses `renderMonsterFarZoneSprite`/
      `renderMonsterMidZoneSprite` with the LITERAL row constants 32/31
      -- the exact same rows the level-37 type-41 "roaming" monster (M19)
      resolves to, confirmed by `e.java`'s own literal call sites, not a
      row-lookup call); and `drawMinimapGrid` (the minimap family).

      **A byproduct worth tracking, not acted on here:** `paintMonsters()`
      gates each visible-object-slot render on the record's own
      `byte[6] != 0` -- a live Monster record's `unconfirmedFlag`, per
      the 28-byte layout M14/M20 already confirmed -- suggesting that
      flag may really mean something like "alive/renderable" rather than
      the minor miscellaneous bit its current name implies. Not renamed
      (would ripple through M14/M17/M18/M20's own code); flagged in
      `GameCanvas.java`'s header comment for a future pass.

      **Left deliberately unresolved, same discipline as the wall/
      monster/object sprite-metadata tables already flagged unconfirmed:**
      `hasCrystalGlow()`'s real in-game meaning (a glowing/special
      dropped item?); the exact semantics of `unconfirmedTable_ae`/`_a`/
      `_J`'s individual columns (only their control-flow role is
      confirmed, not their content); and two brand-new, previously
      unknown methods this pass's reading turned up but did NOT
      transcribe (out of scope -- nothing calls into them from any paint
      method) -- `void q()`/`void p()`, which populate
      `minimapTileGrid`/`visibleTileGrid` from `Dungeon`, called from
      somewhere in the still-untranscribed tick logic, not from painting
      itself.

      Verified by `javac` re-confirming the whole `../src/` tree still
      compiles clean (zero errors, only the same expected `new
      Integer(int)` deprecation warnings already documented) after the
      full `GameCanvas.java` rewrite. No C++ changes this milestone (the
      compositor these would feed doesn't exist yet, per M21's own
      framing), so the existing 20 smoke tests are unaffected and were
      not re-run.

- [x] **M23 -- `Backbuffer::Blit()`, the real alpha-test/clip/mirror
      compositor** (this session). Now that M22 gave every `GameCanvas`
      paint method a real Java body to port from, this implements the
      ONE primitive both of its RawImage-drawing idioms
      (`drawRawImageFull()`/`drawRawImageFrame()`) reduce to once
      `Graphics.setClip()` is modeled as an extra column-range argument
      instead of real stateful clipping: draw a whole `RawImage` at
      `(x, y)`, optionally mirrored horizontally first (the
      `DirectGraphics` manipulation flag 8192/`TRANS_MIRROR`
      `drawWallSegment()`/`renderWardenCompassIcon()` both use),
      alpha-tested, clipped to both the backbuffer itself and an
      additional `[clipX0, clipX1)` column range. `drawRawImageFrame()`'s
      own frame-slicing idiom (draw the WHOLE spritesheet shifted left by
      `frame * frameWidth`, then let an external clip crop it to one
      frame-wide column) is reproduced exactly, not specially-cased --
      the caller just passes `x - frame*frameWidth` as `Blit()`'s `x` and
      `[x, x+frameWidth)` as the clip range.

      New `graphics/backbuffer.h` free functions `IsOpaquePixel()`/
      `Argb4444ToRgb565()`: `RawImage`'s own decoded pixel format (M7) is
      Nokia UI API's `TYPE_USHORT_4444_ARGB` (the literal `4444`
      `GameCanvas`'s `drawPixels()` calls pass as their own `format`
      argument) -- top nibble is a binary alpha TEST bit (real MIDP
      hardware this old has no partial alpha blending, matching
      dawnstar's own `Blit()` precedent exactly), the low 12 bits R/G/B
      nibbles, each widened to a full 8-bit channel before repacking to
      RGB565.

      Verified with a new `backbuffer_blit_smoke.exe`: synthetic
      known-value ARGB4444 pixels convert to the exact expected RGB565
      output and the alpha test skips exactly the transparent one;
      `drawRawImageFrame()`'s own "shift then clip" idiom demonstrated
      directly against a synthetic 3-frame spritesheet (only the
      requested frame's columns land, its neighbors on both sides stay
      untouched); mirroring reverses column order on a 3-pixel synthetic
      image; five blits straddling every edge/corner of the backbuffer
      (including two placed 1000px off-screen entirely) neither crash
      nor corrupt in-bounds pixels; and a real M7-confirmed `.cus`
      sprite (`chestnearclosed.cus`, 80x68, 4430 opaque / 1010
      transparent pixels) blitted onto a filled backbuffer has EVERY
      pixel match `Blit()`'s own `IsOpaquePixel()`/`Argb4444ToRgb565()`
      rules exactly, cross-checked pixel-by-pixel against the source
      `RawImage` directly. One test bug caught and fixed during
      verification (an out-of-bounds check asserted about a corner none
      of its own blit calls actually touched); no bugs in `Blit()`
      itself. All checks passed after that fix; full clean rebuild
      stayed at zero `/W4` warnings; all 21 smoke tests pass.

      **Deliberately does NOT wire this into an actual render pass yet**
      -- there's still no `PlayerState::corridorView` field, no asset
      loading for `floorTexture`/`wallTexture`/`monsterImages`/etc, and
      no live `GameCanvas`-equivalent render function calling
      `CorridorRenderPlan::Plan()` (M21) or any of M22's Java render
      methods. `Blit()` is the composited-pixel PRIMITIVE those future
      pieces will call into, same "build the primitive, verify it in
      isolation, wire the pipeline later" shape M21 already used for
      `CorridorRenderPlan` itself.

- [x] **M24 -- `DecodedImage`, the plain-PNG decoder** (this session).
      Resolves phase-3 M21's own open question -- whether
      `floorTexture`/`wallTexture` load from a `.cus` file (M7's
      from-scratch `RawImage` format) or a plain MIDP-native `Image`
      resource -- by grepping `ESGame.java`'s own asset-loading call
      sites directly: `GameCanvas.floorTexture = this.createImage(
      "floor3.png")`/`GameCanvas.wallTexture = this.createImage(
      "newwallsnok.png")`, both real, plain `.png` files (confirmed
      present in `extracted/`), same for `effectImages`/`hotbarIcons`.
      Vendors `stb_image` (copied verbatim, same pinned commit, from the
      sibling `dawnstar` project's own identical M10 vendoring -- see
      `third_party/stb/PROVENANCE.md`) rather than writing a PNG/DEFLATE
      decoder from scratch, same reasoning dawnstar and
      `shadowkey-decomp` already used it for. New
      `assets/decoded_image.h`/`.cpp` (`DecodedImage::Load(AssetRoot,
      name)`, RGBA8, structurally copied from dawnstar's own
      `DecodedImage` but reading through `AssetRoot`'s plain directory
      instead of an archive) and a new `Backbuffer::Blit(DecodedImage,
      ...)` overload -- the plain-PNG counterpart of M23's `RawImage`
      one, same signature shape (position, `[clipX0,clipX1)` column
      clip, `mirrorX`), same binary alpha test (`A == 0` transparent,
      anything else opaque -- no partial blending, matching M23's own
      `RawImage` `Blit()` and dawnstar's identical precedent).

      **Two real cross-checks against the actual files, not just
      synthetic data:** `floor3.png` decodes to exactly 36px wide,
      matching `paintWalls()`'s own `col * 36` floor-tiling loop (M21)
      exactly; `newwallsnok.png` decodes to exactly 144px wide -- 8 real
      18px-wide frames, confirming `drawWallSegment()`'s own "frame > 7
      mirrors frame-8" logic (M21/M22) means 8 PHYSICAL frames covering
      a logical 0-15 frame range via mirroring, not 16 separately stored
      frames. `GameCanvas.java`'s own `paintWalls()` doc comment updated
      to mark the open question resolved.

      Verified with a new `decoded_image_smoke.exe`: synthetic
      known-value RGBA8 pixels convert/alpha-test identically to M23's
      `RawImage` version; the same "shift then clip" frame-slicing idiom
      and mirroring behave identically too; and all 4 real `.png` files
      GameCanvas actually loads (`floor3.png`/`newwallsnok.png`/
      `blood1.png`/`icon_camp.png`) decode to sane, non-empty buffers,
      with `icon_camp.png` blitted onto a filled backbuffer matching
      `DecodedImage`'s own `R()`/`G()`/`B()`/`A()` accessors pixel-by-
      pixel (246 opaque / 474 transparent pixels, zero mismatches). All
      checks passed on the first attempt; full clean rebuild stayed at
      zero `/W4` warnings (the vendored `stb_image` library itself
      compiles at `/W3`, same carve-out dawnstar's own vendoring uses);
      all 22 smoke tests pass.

      **Still deliberately NOT wired into an actual render pass** -- same
      "primitive first, pipeline later" framing M21/M23 already used.
      `chestImages`/`bagImages`/`crystalImages`/`monsterImages` (all
      `RawImage`, M23's territory) and now `floorTexture`/`wallTexture`/
      `effectImages`/`hotbarIcons` (all `DecodedImage`, this milestone's)
      can BOTH now actually be decoded and blitted -- what's still
      missing is loading them into a single live asset bundle, a real
      `PlayerState::corridorView` field, and a render function that
      calls `CorridorRenderPlan::Plan()` (M21) + both `Blit()` overloads
      together.

- [x] **M25 -- `GameRenderer`, the first real end-to-end pixel render**
      (this session). `PlayerState::corridorView` (`byte[9][5]` in the
      original -- matched structurally, not by including
      `dungeon/dungeon_runtime.h`'s heavier `CorridorViewGrid` alias
      directly, to keep `player_state.h` lightweight) is now a real
      field, and `PlayerMovement::RefreshCorridorView` (`Player.
      refreshCorridorView()`) wires it through `CommitMove` at all THREE
      of `Player.commitMove()`'s own call sites: both locked-dropped-item
      early returns (Player.java lines 860/894) and the final
      end-of-method call (line 914) -- in the exact same relative order
      as the original (refreshCorridorView() runs BEFORE
      autoMarkCampOnTile(), matching the original's own statement order).

      New `render/corridor_assets.h` (`CorridorAssets::Load` -- just
      `floorTexture`/`wallTexture` via `DecodedImage::Load`, deliberately
      NOT a general "GameAssets" bundle also covering `monsterImages`/
      `chestImages`/`bagImages`/`crystalImages`/`effectImages`/
      `hotbarIcons` -- those feed paint methods needing far more live
      state this port doesn't have yet, see below) and `render/
      game_renderer.h`/`.cpp` (`GameRenderer::RenderCorridorView`):
      `GameCanvas.paintWalls()` transcribed directly against
      `CorridorRenderPlan::Plan()`'s own output (M21) -- the ailment-4
      dark-red fallback fill (color literal `10485760` = `0xA00000` =
      RGB(160,0,0)), the ailment-3 "draw nothing" skip, the 5x tiled
      floor, and each `WallDrawCall` blitted via `Backbuffer::Blit()`
      (M23/M24) with the exact same shifted-then-clipped positioning
      `drawWallSegment()` itself uses (frames 0-7 unmirrored at
      `x - frame*18`, frames 8-15 mirrored at `x - (frame-8)*18`, both
      clipped to `[x, x+18)`).

      **Real, deliberately unwired gap, documented rather than silently
      dropped:** `Player.java`'s other THREE `refreshCorridorView()` call
      sites (character creation's `resetState()`, `markCampAndReturnToTown()`,
      `warpToCampMark()` -- lines 339/2493/2501) aren't wired, since none
      of their port-side equivalents (`PlayerCreation::CreateCharacter`,
      `PlayerInventory::MarkCampAndReturnToTown`/`WarpToCampMark`) take a
      `LevelLookup` today; threading one through each is a small, separate
      follow-up, not a blocker for this milestone's actual goal. Harmless
      in practice for every existing caller (none renders off
      `corridorView` between one of those calls and the player's next
      `CommitMove`, which does refresh it).

      Verified with a new `game_renderer_smoke.exe`: `CommitMove` leaves
      `p.corridorView` exactly matching a fresh, independently-computed
      `SampleCorridorView` call at the landed position/facing (both for a
      plain step and for the locked-item early return); synthetic
      known-value `DecodedImage`s confirm `RenderCorridorView`'s floor-
      tiling/fallback-fill/ailment-3-skip branches pixel-exactly; and a
      real integration pass against `floor3.png`/`newwallsnok.png` and a
      real M6-generated level (all 4 facings from the level's own
      stairway-corridor point, same reference point M21's own test uses)
      independently recomputes each wall segment's expected source column
      (shift + mirror math) and compares it directly against
      `DecodedImage`'s own `R()`/`G()`/`B()`/`A()` accessors -- 36,608
      real floor pixels and 105,565 real opaque wall pixels checked, zero
      mismatches, both plain (frames 0-7) and mirrored (frames 8-15)
      wall frames actually exercised by the real generated level. All 23
      smoke tests pass; full clean rebuild stayed at zero `/W4` warnings.

      **Still deliberately NOT a complete render pass:** every OTHER
      paint* method M22 transcribed (`paintObjects`/`paintMonsters`/
      `paintStatusBars`/`paintHud`/`paintMinimapZoomedOut`/
      `paintMinimapNormal`/`paintMessagePopup`/`paintFlashOverlays`/
      `paintUnknown_b`) needs far more live state this port doesn't wire
      up yet -- `Player.visibleObjects`, a populated `WorldRegistry`-backed
      monster/chest/dropped-item cache actually feeding a frame, hotbar/
      dialogue state, HP/Magicka/Fatigue bars -- and there's still no
      live game loop calling any of this from `main.cpp` at all. Same
      "primitive first, pipeline later" discipline M21/M23/M24 already
      established, just now applied to one whole vertical slice (corridor
      floor+walls, selection logic through real pixels) instead of a
      single primitive. (`paintStatusBars()` turned out to need none of
      that live state after all -- see M26, immediately below.)

- [x] **M26 -- `StatusBarPlan` + `GameRenderer::RenderStatusBars`,
      `paintStatusBars()`** (this session). The cheapest remaining paint
      method, exactly as this entry's own previous "what's next" note
      predicted: entirely self-contained in `PlayerState` +
      `CharacterData` (`PlayerCombatStats::EffectiveStat`'s own
      requirement, from M13) -- no new asset loading, no world state, no
      `main.cpp` game loop needed to exercise it meaningfully.

      New `render/status_bar_plan.h`/`.cpp` (`StatusBarPlan::Plan`),
      split from pixel drawing the same way `CorridorRenderPlan` (M21)
      already is: three widths (`stat * 38 / max`) for the HP
      (`coreStats[2]`/`[3]`), Magicka (`[4]`/`[5]`), and Fatigue
      (`[6]`/`[7]`) bars. `GameRenderer::RenderStatusBars` draws them as
      three 40x7 yellow (`0xFFFF00`) track rects with a 38x5 colored fill
      inset by (1,1) -- red HP, green Magicka, blue Fatigue.

      **A real, confirmed asymmetry preserved, not simplified away:**
      only `fatigueWidth` is clamped to 40 in the original -- `hpWidth`/
      `magickaWidth` have no clamp at all. `EffectiveStat`'s own internal
      clamp to the matching max stat means none of the three should
      naturally exceed 38 in practice, so this reads like a defensive
      check the original evidently considered necessary for Fatigue
      specifically (current transiently exceeding max?) but not the
      other two -- not something this port introduced or "fixed" for
      consistency.

      `stormhold_render` now links `stormhold_player` for
      `PlayerCombatStats::EffectiveStat` (no cycle: `stormhold_player`
      doesn't depend on `stormhold_render`).

      Verified with a new `status_bar_smoke.exe`: hand-picked
      `PlayerState` values confirm the exact width formula (including
      integer-division truncation); a real M9-created fresh character
      renders all three bars completely full (current == max at
      creation, confirmed by M9 itself); the fatigueWidth-only clamp
      asymmetry is exercised directly (`coreStats[6]=200`,
      `coreStats[7]=10` -> 760 clamped to 40, while the identical
      HP/Magicka setup stays unclamped at 760); and a pixel-exact
      `RenderStatusBars` check on a synthetic `Backbuffer` confirms every
      track/fill boundary column, a zero-width fill leaving its whole
      track visible, and every row outside the three bars staying
      untouched. All 24 smoke tests pass; full clean rebuild stayed at
      zero `/W4` warnings.

- [x] **M27 -- `VisibleObjects`, the 13-slot corridor-view object cache
      (data model only)** (this session). Unblocks `paintObjects()`/
      `paintMonsters()` (M22) the same way M21 unblocked `paintWalls()`:
      mirrors that same selection-logic-vs-drawing split (this milestone
      is the SELECTION half, no pixels), and mirrors dawnstar's own
      identical M25 milestone for its equivalent system.

      New `player/visible_objects.h`/`.cpp` (`VisibleObjects` class) ports
      `Player.refreshVisibleObjectSlots()`/`refreshVisibleObjects()`/
      `resolveVisibleObjectSlot()`/`placeVisibleObject()`/
      `facingAxisDistance()`. `VisibleSlotKind`/`VisibleSlot` (new,
      `player_state.h`) replace Java's `SLOT_EMPTY`/`SLOT_BLOCKED`/
      `SLOT_SHADOWED` Integer-reference-identity sentinels with a tagged
      enum, same treatment dawnstar's own `VisibleSlot`/`VisibleSlotKind`
      already got; `visibleObjects` itself moves from a Java `static`
      field onto `PlayerState` (harmless single-player simplification,
      same as every other really-static-but-per-player field here). New
      `DungeonRuntime::RelativeViewOffset` (pure facing-relative
      coordinate math, needs no Dungeon/level state despite living on
      `Dungeon` in the original) feeds `ResolveSlot`'s own position
      cascade, the same way `ViewGridAt` (M21) feeds `RefreshSlots`'
      wall-occlusion cascade.

      **Two real, confirmed findings from reading this pipeline in
      full, both preserved/fixed rather than glossed over:**
      - `resolveVisibleObjectSlot()`'s own header comment (an earlier
        pass's doc-comment guess, not anything from the decompiled
        bytecode) had its `kind==4`/`else` branch labels SWAPPED --
        said "4=dropped-item"/"else=chest", but the real callers are
        `placeVisibleObjectIfSlotFree(2, ...)` for dropped items
        (landing in the `else` branch) and `(4, ...)` for chests
        (landing in the `kind==4` branch). Functionally inert (both
        branches read the same `data[0]`/`data[1]` offsets, and both
        record kinds are confirmed `[0]/[1]=x/y`), but fixed at the
        source in `../src/Player.java` anyway, same discipline M22's
        own real mapping-bug fixes used.
      - `placeVisibleObjectIfSlotFree()` computes `facingAxisDistance()`
        and appears to branch on it (`kind != 4 && dist != 1`) -- but
        BOTH branches of that if/else call `placeVisibleObject()` and
        return `true` identically, so neither the distance nor the
        branch has any effect. `refreshVisibleObjects(includeWarden)`'s
        own parameter is equally dead (the Warden placement reads
        `Shop.wardenPresent` directly, never `includeWarden`; nothing
        else uses it either). Both exposed/kept for signature fidelity
        anyway (`VisibleObjects::FacingAxisDistance`/`Refresh`'s own
        `includeWarden` parameter), documented rather than dropped, same
        discipline M10's dead `leftLevelZone` and M17/M18's asymmetric
        bit tests already used.
      - **Also confirms, rather than just flags, M22's own "byproduct
        worth tracking":** `placeVisibleObject()`'s monster case sets
        `unconfirmedFlag` PERMANENTLY true (`data[6] = 1`, an overwrite)
        the first tick a monster is ever placed into a visible slot, and
        nothing anywhere in `../src/` ever clears it. So it means "has
        the player ever seen this monster", not "alive/renderable" --
        exactly dawnstar's own confirmed finding for its equivalent
        field. `GameCanvas.java`'s own header comment updated to mark
        this confirmed, not just flagged.

      **Still no confirmed caller anywhere in `../src/`:** unlike this
      system's own Player.java class comment ("rebuilt every move"),
      `refreshVisibleObjects()` has no call site in the currently-
      transcribed source at all, and `PlayerMovement::CommitMove`
      doesn't call this port's `Refresh()` either -- same class of gap
      as the still-untranscribed tick-loop helpers.

      Verified with a new `visible_objects_smoke.exe`: `FacingAxisDistance`'s
      exact per-facing formula; `ResolveSlot`'s position/occlusion-guard
      cascade at hand-derived canonical positions; `RefreshSlots`' own
      wall-occlusion cascade, INCLUDING a case specifically proving the
      LITERAL sequential statement order (not a fixed-point loop) --
      slot 8 ends up Shadowed purely as a second-order effect of slot 5's
      earlier cascade, via a later check reading slot 9's freshly-written
      Shadowed state; `Refresh()`'s own world-wiring (a dropped item/
      chest/monster each landing in the expected slot, the monster's
      `unconfirmedFlag` overwrite and dropped item's bit-0 OR both
      persisted back into the registry, and the chest's record
      confirmed untouched); Warden placement on level 1; and
      `includeWarden=true` vs. `false` producing identical results
      end to end. All 25 smoke tests pass; full clean rebuild stayed at
      zero `/W4` warnings.

- [x] **M28 -- `VisibleObjectRenderer`, the actual sprite drawing off
      M27's `visibleObjects`** (this session). `paintObjects()`/
      `paintMonsters()` themselves, the "selection logic done, now draw
      it" pairing this milestone completes (M21->M25, now M27->M28).

      New `assets/monster_image_set.h`/`.cpp` (`MonsterImageSet::Load`):
      a genuinely new, previously-unported data file,
      `monsterfilenamesin.dat` (a no-count-prefix `<UTF x 5 x 7>` grid,
      `ESGame.loadMonsterImageFileNames()`), feeding
      `runMonsterImageLoader()`'s own chunk/count/imageless-type logic to
      load `GameCanvas.monsterImages`' 33 `.cus` files by their real,
      per-type filenames (5 of the 33 slots -- types 4/11/18/23/30 --
      deliberately stay unloaded, modeled as `std::nullopt`, matching
      `isImagelessMonsterType()`). New `render/visible_object_assets.h`
      (`VisibleObjectAssets::Load`) bundles that plus `bagImages`/
      `crystalImages`/`chestImages` (confirmed filenames, all `RawImage`,
      M23's compositor). New `render/visible_object_renderer.h`/`.cpp`
      (`VisibleObjectRenderer::RenderObjects`/`RenderMonsters`) ports
      `renderObjectAt`/`renderObjectNear`/`Mid`/`Far`/`hasCrystalGlow`
      and `renderMonsterSpriteForSlot`/`monsterNearZoneRow`/`MidZoneRow`/
      `FarZoneRow`/`renderMonsterNearSprite`/`renderMonsterOrIconSprite`/
      `renderMonsterMidZoneSprite`/`drawMonsterZoneFrame`/
      `renderMonsterFarZoneSprite`/`renderWardenCompassIcon` directly
      against `Backbuffer::Blit()` (M23/M24) -- `drawRawImageFrame()`'s
      own shift-then-clip/mirror idiom needs no reinterpretation, it's
      exactly `Blit()`'s own established shape.

      **A real, reachable, previously-undiscovered original-game crash
      bug, found by literally counting a table's rows against its own
      header comment's claim:** `unconfirmedTable_a`'s comment said "41
      rows x 2 cols" -- counted directly, it actually has only 31.
      `renderMonsterOrIconSprite()` indexes it at `[typeIndex-1]` for
      the WHOLE `monsterNearZoneRow()`-resolves->=0 range, which includes
      typeIndex 26-40, not just 26-31 (all this 31-row table can cover).
      typeIndex 41 is intercepted by its own earlier `if` branch and
      never reaches the table, but typeIndex 32-40 are ordinary,
      CONFIRMED-spawnable monster types (`world/dungeon_generator.cpp`'s
      own `kMonsterTypeByTier` references types up to 40 at real deep
      dungeon tiers) with no such interception -- so any of those 9
      types walking into the player's near-view slot (1, directly ahead)
      makes the real game evaluate a 31-entry array at index 31-39:
      `ArrayIndexOutOfBoundsException` on real hardware. Unlike M10's
      `leftLevelZone` or M19's unreachable neighbor-throw, this one is
      NOT dead code -- reachability was independently confirmed against
      the real generation table, not assumed. Preserved as a genuine
      crash condition in the port (`VisibleObjectRenderer` throws
      `std::runtime_error` rather than reading out of bounds or silently
      clamping -- undefined behavior would be a strictly WORSE port-side
      outcome than the original's own clean exception, not a faithful
      one), not "fixed" by inventing data for the missing 9 rows.
      `../src/GameCanvas.java`'s own header comment on that field
      corrected and expanded with the full writeup.

      Also confirmed real, dead code (unlike the bug above): the near-
      zone `crystalGlow` branch `renderObjectNear()` takes is never
      actually reachable (every real caller passes `false`, and unlike
      `renderObjectMid`/`Far`, `renderObjectNear` never calls
      `hasCrystalGlow()` itself either) -- a dropped item with the
      crystal-glow bit set renders as a normal bag sprite at the near
      slot, never `crystalImages[0]`. Preserved exactly (already flagged
      by M22's own doc comment, reconfirmed here while porting).

      Verified with a new `visible_object_renderer_smoke.exe`: exact
      zone/slot screen positions for chests/dropped items/crystal-glow
      items via synthetic images; the confirmed-dead near-zone crystal
      branch (renders green, not blue, even with the bit set); the
      near-zone primary/secondary-overlay sprite draw INCLUDING real
      frame-slicing (typeIndex 3's nonzero `secondaryFrame`); the
      confirmed-reachable typeIndex-32-40 crash (throws) vs. typeIndex 41
      (doesn't, intercepted earlier); Warden placeholder rendering in
      the far/mid zones without setting the monster-drawn flag; and a
      real integration check against `chestfarclosed.cus` loaded through
      `VisibleObjectAssets::Load` (300 real opaque pixels, zero
      mismatches). All 26 smoke tests pass; full clean rebuild stayed at
      zero `/W4` warnings.

      **Still no live game loop wiring any of this together** -- same
      gap M25/M27 already flagged, `main.cpp` still just presents a
      blank frame.

## What's next

M29 onward: `paintHud()` needs `hotbarIcons` (`DecodedImage`, M24's
compositor, real filenames not yet grepped from `ESGame.java`) plus
`resolveHudIconSet()`'s own `unconfirmed_aa`/`_m`/`_R`/`_W` flags and
`isNpcDialogueDue()` (a `Shop`/`targetMonster` dependency) -- none of
which have a port-side home yet, likely a small new "HUD/UI state"
struct alongside `PlayerState`, not a `PlayerState` field itself (these
are `GameCanvas`'s own static fields in the original, not `Player`'s);
`paintMinimap*()` need the still-untranscribed `q()`/`p()` minimap-
populate methods M22 found but didn't transcribe. Beyond that: an actual
live game loop in `main.cpp` calling `GameRenderer`/
`VisibleObjectRenderer`/`VisibleObjects::Refresh`/whatever M29+ adds
every tick instead of presenting a blank frame; the still-untranscribed
tick-loop helpers (`showMessage`/`tickStatusCountdowns`/`tickPerSecond`/
`rollCampInterrupted`/`tickMovementAndAI`/`setSomeFlag`); and the
still-unrecovered `tryRankUpSkills()`/`Monster.tick()`/`Monster.
onDeath()` callers (flagged again this session, unchanged since
M14/M15), which may well turn out to live in exactly those same
tick-loop helpers. Following dawnstar's own later milestones roughly but
expecting further Stormhold-specific divergences the way
M3/M6/M7/M8/M9/M10/M12/M13/M14/M16/M17/M18/M19/M20/M21/M22 already
found.
