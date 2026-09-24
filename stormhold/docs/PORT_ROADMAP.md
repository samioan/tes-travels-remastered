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

- [x] **M29 -- `HudState`/`IsAdjacentToVarus`/`IsNpcDialogueDue`/
      `ResolveHudIconSet`, `paintHud()`'s own SELECTION logic (data
      only)** (this session). Same "selection logic first, pixels later"
      split M21->M25 and M27->M28 already used -- deferred here
      specifically because `paintHud()` itself needs TWO primitives this
      port doesn't have at all yet: a filled rounded-rect and character-
      glyph text rendering (`hotbarKeyGlyphs`). Rather than build those
      under time pressure just to unblock one milestone, this milestone
      ports everything ELSE `paintHud()` depends on that doesn't need
      pixels.

      New `render/hud_state.h` (header-only): `HudState` (the 4
      `GameCanvas`-level UI flags `resolveHudIconSet()` reads --
      deliberately NOT on `PlayerState`, matching the class-boundary
      distinction M25's own `corridorView` comment already draws, just
      the other direction), `TargetMonsterInfo` (the minimal shape of
      `GameCanvas.targetMonster` `IsNpcDialogueDue()` itself needs --
      position + typeIndex, `std::nullopt` for `null`; **no confirmed
      setter exists anywhere in `../src/` yet**, same class of gap as
      the still-untranscribed tick-loop helpers -- the caller supplies
      whatever it currently believes this to be, same pattern
      `CorridorRenderPlan`'s own already-sampled view uses),
      `IsAdjacentToVarus` (`Shop.isAdjacentToVarus`, reusing
      `WardenState::kShopX`/`kShopY` (M8) rather than standing up a
      whole `Shop` module for one distance check), `IsNpcDialogueDue`,
      and `ResolveHudIconSet`.

      **A real, stale-comment discrepancy noticed while reading
      `Shop.isAdjacentToVarus()` directly, flagged rather than "fixed"
      (out of scope for this milestone -- `Shop.java` itself wasn't
      touched):** that method's own header comment mentions a
      "confirmed-required state (`player.j == 1`)" the method doesn't
      actually check anywhere in its real body (just the level/distance
      test) -- looks like a leftover from an earlier, less certain pass,
      not a missing condition this port should reproduce.

      Verified with a new `hud_state_smoke.exe`: every cardinal-adjacent/
      diagonal/same-tile/off-level case for `IsAdjacentToVarus`; both of
      `IsNpcDialogueDue`'s branches (Varus short-circuits true
      regardless of any target monster; the level-37/typeIndex-41/
      adjacency conditions for the target-monster branch, including the
      wrong-type, wrong-level, and non-adjacent negatives); and all 4 of
      `ResolveHudIconSet`'s branches including the priority order between
      them. All 27 smoke tests pass; full clean rebuild stayed at zero
      `/W4` warnings.

- [x] **M30 -- `graphics/bitmap_font.h`/`Backbuffer::FillRoundRect`, this
      port's first text rendering, wired into the message popup**
      (this session). Closes the "paint methods need text, no primitive
      exists yet" gap M29 deliberately deferred.

      **The same genuine architecture choice dawnstar hit at its own
      M30, put to the user again rather than silently reused just
      because dawnstar already answered it once:** MIDP's `Font`/
      `Graphics.drawChar`/`drawString` have no real recoverable asset
      (a system font is platform/device-dependent, never bundled game
      data). Asked the user directly; they again chose the hand-rolled
      bitmap-font route (over drawing text via GDI directly), keeping
      every future text-touching milestone testable via the same
      pixel-level `Backbuffer` assertions everything else already uses.

      `BitmapFont` (`graphics/bitmap_font.h`/`.cpp`) reuses dawnstar's
      own identical hand-authored 30-glyph alphabet (space/`'`/`-`/`!`/
      A-Z, 4x7 pixels each, case-folded, `kAdvance`=5px) verbatim --
      both ports share the same unrecoverable-MIDP-font gap, so
      inventing a second, arbitrarily different shape for the same
      class of problem would just be gratuitous divergence. Confirmed
      sufficient for this milestone's own real text (every `MSG_*`/
      `npcNameLines` character in `../../../src/GameCanvas.java` is
      space/`'`/A-Z, by direct reading rather than assumption) --
      digits (`hotbarKeyGlyphs`, needed by `paintHud()`) are NOT yet
      defined, deferred to whichever milestone actually wires
      `paintHud()`'s own pixels.

      `Backbuffer::FillRoundRect` ported identically to dawnstar's own
      (a straightforward reimplementation of the documented MIDP
      primitive -- both real call sites' `arcWidth`/`arcHeight` are
      plain constants, no operator-precedence trap to preserve).

      New `render/message_popup.h`/`.cpp` (`MessagePopupState`/
      `MessagePopup::Show`/`Tick`/`Paint`) ports `GameCanvas.
      showMessage()`/`paintMessagePopup()` plus `run()`'s own per-tick
      3000ms auto-hide timeout. **`showMessage()` itself was a
      `throw new UnsupportedOperationException` stub in `../src/
      GameCanvas.java` until this milestone** -- filled in from
      decompiled/`e.java`'s real `a(String[],int)` body (a simple
      priority-gate: a message only replaces whatever's showing if
      strictly higher priority, unless negative -- "always show" --
      which stores as priority 10). Also renamed
      `unconfirmed_X`->`messagePriority` now that its role is fully
      confirmed, matching dawnstar's own field name.

      **NOT ported here, unlike dawnstar's own M30:** `wordWrap()`/
      `wrapToTwoLines()`. Every real `showMessage()` call site in
      `GameCanvas.java` passes a pre-baked `MSG_*`/`npcNameLines`
      `String[2]` constant directly -- confirmed by reading every real
      call site, none ever wraps dynamic text through this popup, unlike
      dawnstar's own shop-greeting-name popup which needed it for
      exactly that reason. (`UIScreen.java` has its own, unrelated,
      `wordWrap()` for a different screen entirely -- out of scope.) So
      there was nothing real here to wrap, a genuine, confirmed scope
      reduction rather than a shortcut.

      Verified with a new `message_popup_smoke.exe`: `BitmapFont`
      glyph-shape/case-folding/unsupported-character checks;
      `FillRoundRect` corner-cutting/zero-arc/non-positive-size checks;
      `Show`/`Tick`'s exact priority-gate and timeout arithmetic; and
      `Paint`'s visible-vs-hidden gating and pixel placement against
      real `MSG_REST_COMPLETE` content ("Rest"/"complete!", exercising
      case-folding and punctuation together). All 28 smoke tests pass;
      full clean rebuild stayed at zero `/W4` warnings.

      **Still not wired into any live tick loop** -- `Show`/`Tick` have
      no real caller yet (every real `showMessage()` call site lives in
      `run()`'s still-untranscribed tick-loop helpers), same gap
      M25/M27/M29 already flagged.

- [x] **M31 -- `GameRenderer::RenderHud`, `paintHud()`'s own actual
      pixel drawing** (this session). Both primitives it needed
      (`Backbuffer::FillRoundRect`, `BitmapFont`) now existed as of
      M30 -- this milestone adds the two things still missing (digit
      glyphs, the icon asset bundle) and wires the actual draw.

      `BitmapFont` gains `0`-`9` (10 more hand-authored glyphs, 40
      total) for `hotbarKeyGlyphs` (`'1'`/`'3'`/`'5'`/`'7'`/`'9'`/`'0'`)
      -- identical shapes to dawnstar's own digit glyphs (its own M31),
      same "reuse rather than invent a second arbitrary shape"
      reasoning M30 already used for the letters.

      New `render/hotbar_assets.h` (`HotbarAssets::Load`) -- the 6
      `hotbarIcons` plain-PNG files (`icon_attack`/`icon_cast`/
      `icon_change`/`icon_option`/`icon_action`/`icon_camp.png`), real
      filenames/order confirmed via `ESGame.java`'s own asset-loading
      call site. Deliberately its own small bundle, not folded into
      `CorridorAssets` -- same "named for exactly what it holds" split
      `CorridorAssets` itself already draws.

      `GameRenderer::RenderHud` transcribes `paintHud()`
      (`../../../src/GameCanvas.java` lines 1024-1062) directly: the
      black background fill, the rounded panel (same 0xC79967 color
      `MessagePopup`'s own background uses), then -- gated on
      `iconSet` (a plain parameter, M29's `ResolveHudIconSet` computed
      by the caller, same "caller supplies/owns state" pattern this
      port uses throughout) -- 4 icons plus 4 digit glyphs at fixed
      positions, whichever 4 indices the real `iconSet` (0/1/2) branch
      selects. An out-of-range `iconSet` draws just the two background
      fills, matching the original's own if/else-if chain with no
      final else. `hotbarActionSet` (read back by `keyPressed()`'s
      numeric-hotkey dispatch in the original) is NOT modeled -- input
      handling remains out of scope, same gap M29 already flagged.

      Verified with a new `hud_renderer_smoke.exe`: the new digit
      glyph shapes; each of the 3 `iconSet` branches' exact (icon,
      glyph) index selection, cross-checked against `paintHud()`'s own
      literal index lists directly, not read back from
      `game_renderer.cpp`; the background fills still drawing for an
      out-of-range `iconSet`; and a real-asset integration check (every
      opaque pixel of real `icon_attack.png` once blitted, independently
      recomputed and compared against `DecodedImage`'s own accessors).
      All 29 smoke tests pass; full clean rebuild stayed at zero `/W4`
      warnings.

      **Still not wired into any live tick loop** -- same gap every
      rendering milestone so far has flagged; `main.cpp` still just
      presents a blank frame.

- [x] **M32 -- `DungeonRuntime::SampleSquareView`, the minimap's own
      populate step** (this session). The "selection logic" half of
      `paintMinimap*()`'s remaining gap -- pixels (`drawMinimapGrid()`,
      already fully transcribed in `../src/GameCanvas.java` since M22)
      deliberately deferred to a follow-on milestone, same "selection
      first, pixels later" split M21/M27/M29 already used.

      Turned out most of the reverse-engineering legwork for this was
      **already done**: `Dungeon.tileAt()`/`sampleView()`/
      `sampleSquareView7()`/`sampleSquareView17()` were all fully
      transcribed in `../src/Dungeon.java` already (M21's own
      `tileAt()`, plus `sampleView()` itself, found already complete
      while reading `Dungeon.java` directly for this milestone -- not
      new work). What was actually still missing, confirmed against
      decompiled `e.java`'s own `q()`/`p()`: `GameCanvas` never called
      any of it. Filled in `populateMinimapGrid()`/`populateVisibleGrid()`
      (renamed from `q()`/`p()`) as thin wrappers -- same "confirmed
      and transcribed" treatment M30 gave `showMessage()`. Both real
      call sites live inside still-untranscribed tick-loop helpers, so
      -- same as `showMessage()` before M30's own live wiring existed
      -- these have no reachable caller yet either.

      New `DungeonRuntime::SampleSquareView` (`dungeon/dungeon_runtime.h`/
      `.cpp`) is the C++ side of `sampleView()`: wall/special-bit
      sampling (via the already-ported `TileAt`, so it sees across
      level boundaries near an edge) into a `size`x`size` grid (7 or 17,
      one shared implementation), then an overlay pass -- live monster/
      chest/dropped-item positions from `WorldRegistry` on every real
      dungeon level, or (**preserved exactly as found, not "corrected"**)
      hub-town NPC positions gated on `Shop.questRewardClaimable[i]` --
      the SAME surprising gate `Shop.java` already documents for its
      one-time quest-reward-collection branch, not a general visibility
      flag (M27's own header comment already flagged this exact pattern
      once, for `resolveVisibleObjectSlot()` -- same original quirk,
      independently reachable a second time here).

      This port has no live Shop/quest-economy model at all yet (see
      `world/warden.h`'s own class comment -- deliberately scoped to
      just the Warden visit mechanic). Rather than invent one or skip
      the hub-town branch's overlay silently, the new `HubMinimapMarkers`
      struct (`questRewardClaimable[7]`, `wardenPresent`) is a plain
      caller-supplied parameter -- same "caller supplies/owns state"
      pattern as `render/hud_state.h`'s `TargetMonsterInfo` (M29) --
      defaulting to `nullptr`, which skips the marker overlay entirely
      (wall-bit sampling still works) rather than guessing. `Shop.
      SHOP_X`/`SHOP_Y` themselves ARE hardcoded (`kShopMarkerX`/
      `kShopMarkerY`) -- plain static data, not live state, safe to
      reuse directly (index 6 matches `WardenState::kShopX`/`kShopY`
      exactly, cross-checked).

      Verified with a new `minimap_sample_smoke.exe`: wall/special-bit
      sampling cross-checked against `TileAt` directly (including the
      "both bits set -- wall bit wins" precedence case); monster/chest/
      dropped-item overlay bits for BOTH the facing==1/3 and facing==2/4
      index-math branches, each independently re-derived from
      decompiled/`i.java`'s own literal formulas; the dropped-item
      visibility gate and monster `unconfirmedFlag` gate; and the
      hub-town branch's marker overlay, Warden-slot skip-unless-present
      gate, and `nullptr` no-crash fallback. All 30 smoke tests pass;
      full clean rebuild stayed at zero `/W4` warnings. `javac`
      recompiled clean (8 expected warnings only) after the
      `GameCanvas.java` fill-in.

- [x] **M33 -- `GameRenderer::RenderMinimapZoomedOut`/
      `RenderMinimapNormal`, `paintMinimap*()`'s own actual pixel
      drawing** (this session). The "pixels" half of M32's own deferred
      split -- as expected, a short follow-on: both primitives it
      needed (`BitmapFont`, `Backbuffer::FillRect`) already existed.

      Transcribes `paintMinimapZoomedOut()`/`paintMinimapNormal()`
      directly (`../../../src/GameCanvas.java` lines 1229-1250): the
      compass glyph (M31's `BitmapFont`, white, at each method's own
      fixed position -- `compassGlyphs[facing]`, a plain char array
      already covered entirely by existing letters/`'0'`, no new glyphs
      needed), a black backdrop rect, then the grid itself via a shared
      `DrawMinimapGrid` helper (mirroring `drawMinimapGrid()`'s own
      shared-helper structure exactly) fed by M32's `SquareViewGrid`.

      `DrawMinimapGrid` reproduces the original's own if/else-if color
      precedence exactly (1=black/wall, 0=white/floor, else bit
      2=red/bit 4=blue/bit 8=purple -- checked in that order, so a cell
      with both bit 2 and bit 4 set draws red, never blue), plus the
      unconditional green dead-center player marker painted OVER
      whatever the cell's own color was. The purple color (bit 8) is
      decoded directly from `drawMinimapGrid()`'s own literal
      `13369599` (=0xCC00FF) rather than trusting that method's own
      header comment's "cyan-ish" guess from an earlier, less certain
      pass -- not corrected there, since the real RGB bytes are what
      matters, not the color-name guess.

      Verified with a new `minimap_renderer_smoke.exe`: the compass
      glyph and backdrop for both zoom levels, cross-checked against
      each method's own literal geometry directly; every one of
      `DrawMinimapGrid`'s per-byte-value color branches, including the
      bit2-vs-bit4 precedence case; and the dead-center marker
      overriding an otherwise-wall cell. All 31 smoke tests pass; full
      clean rebuild stayed at zero `/W4` warnings.

      **Still not wired into any live tick loop** -- same gap every
      rendering milestone so far has flagged; `main.cpp` still just
      presents a blank frame. With this milestone, EVERY `paint*`
      method `GameCanvas.java` transcribes real pixel logic for now has
      a real C++ counterpart somewhere in this port (`GameRenderer`
      here; `VisibleObjectRenderer` for `paintObjects`/`paintMonsters`,
      M28; `MessagePopup` for `paintMessagePopup`, M30) EXCEPT
      `paintFlashOverlays`/`paintUnknown_b`, both still gated on live
      tick-loop state this port doesn't model yet -- the dominant
      remaining gap is "wire it all into an actual running loop," not
      "port more paint methods."

- [x] **M34 -- wire the real pipeline into the actual windowed
      `stormhold_port.exe`** (this session). Until now `main.cpp` was
      still M1's placeholder: a solid-color `Backbuffer::Fill` and an
      empty tick, with every real milestone (M2-M33) only ever
      exercised through console smoke tests. Same scope/shape as
      dawnstar's own M20, same milestone-number role (its own "wire the
      pipeline in" milestone) even though the two projects' own numbers
      have diverged by now.

      Loads the real extracted assets, builds the real 37-level world
      (M6's `DungeonGenerator::BuildHubLevel`/`PopulateLevel` +
      M18's `RegisterGeneratedSpawns`), creates a real class-0 character
      (M9's `PlayerCreation` -- there's no character-creation UI yet,
      so the class is a fixed stand-in; its own `setHubSpawnPosition`
      fusion already places the player at the hub's real (9,10)
      facing 1, no separate spawn logic needed here). On every
      `GameClock` tick (M1's real 250ms cadence), reads arrow-key state
      (`GetAsyncKeyState`, polled once per tick so a held key advances
      once per tick rather than as fast as the message pump spins)
      into `PlayerMovement::Move` (turn/step only, no strafe -- M17/M19/
      M25's own side effects, including `RefreshCorridorView`, come
      along for free), refreshes `VisibleObjects` unconditionally every
      tick (M27 -- nothing else yet mutates monster/chest/dropped-item
      state between ticks, so this is harmless even though the real
      game only refreshes it after an actual move), and renders through
      every already-verified pixel pipeline this port has --
      `GameRenderer::RenderCorridorView` (M25), `VisibleObjectRenderer::
      RenderObjects`/`RenderMonsters` (M28), `GameRenderer::
      RenderStatusBars` (M26), `GameRenderer::RenderHud` (M31, iconSet
      via M29's `ResolveHudIconSet` against a fresh, all-false
      `HudState`), and `GameRenderer::RenderMinimapZoomedOut`/
      `RenderMinimapNormal` (M33, fed by M32's `SampleSquareView`,
      toggled with a new 'M' key binding) -- then presents through the
      existing GDI `Window::Present`. `CMakeLists.txt`'s `stormhold_port`
      target now links `stormhold_render`/`stormhold_player`/
      `stormhold_dungeon` (previously linked against neither).

      No new gameplay logic was ported here -- this is pure wiring of
      already-verified pieces. Deliberately NOT wired: `paintFlashOverlays()`/
      `paintUnknown_b()` (both still gated on live tick-loop state),
      the message popup (`MessagePopup::Show` has no reachable real
      call site yet -- every one lives inside a still-untranscribed
      tick-loop helper), and the tick-loop helpers themselves
      (`tickStatusCountdowns`/`tickPerSecond`/`rollCampInterrupted`/
      `tickMovementAndAI`/`setSomeFlag`) -- so there is no monster AI,
      no status-effect ticking, and no combat input yet; only
      turning/stepping and the minimap zoom toggle.

      Verified by actually running `stormhold_port.exe` and capturing
      its real window content directly (`PrintWindow`, not a plain
      screen-region copy -- this session's sandboxed desktop doesn't
      reliably composite the window onto the capturable screen surface,
      so a raw `CopyFromScreen` grab came back black/wrong even though
      the process was alive and responding; `PrintWindow` asks the
      window to render into a supplied DC directly and got the real
      content). The captured frame shows the real hub-town corridor
      view (real floor/wall textures), the real HP/Magicka/Fatigue bars,
      the real hotbar icon set 0 (`icon_cast`/`icon_change`/
      `icon_option`/`icon_camp`, key glyphs `3`/`5`/`7`/`0`, matching
      `ResolveHudIconSet`'s own all-false-flags branch exactly), and the
      zoomed-out minimap showing a real wall/floor pattern around the
      spawn point with the player's own green center marker. Simulating
      a real physical UP-arrow key press (`SendInput`, not `PostMessage`
      -- `GetAsyncKeyState` reads actual hardware input state, which
      only `SendInput` affects regardless of window focus) for 1.2s and
      re-capturing shows a completely different corridor frame (walking
      forward into a real corridor, brick wall textures on both sides)
      and a correspondingly different minimap pattern -- confirming
      real keyboard input actually reaches `PlayerMovement::Move` and a
      newly rendered frame results, not a static placeholder. All 31
      smoke tests still pass; full clean rebuild stayed at zero `/W4`
      warnings.

- [x] **M35 -- `PlayerCombatStats::TickStatusCountdowns`, the first
      tick-loop helper** (this session). `GameCanvas.
      tickStatusCountdowns()` (was decompiled/e.java's `c(long)`) was a
      throw-stub in `../src/GameCanvas.java` until now -- filled in and
      ported, the same "confirmed and transcribed" treatment M30 gave
      `showMessage()`.

      Per-tick countdowns for exactly 3 ailments (4/"vampirism", 5/
      "mana burn", 7/"terrified" -- `PlayerState::vampirismTimer`/
      `manaBurnTimer`/`terrifiedTimer`, already present on `PlayerState`
      since an earlier milestone even though nothing used them yet):
      while `Player.hasAilment(id)` -- **NOT** `Player.isEffectActive
      (id)`, confirmed by reading decompiled/`j.java`'s own `k(int)`
      (reads `ailmentMask` directly) vs. `t(int)` (reads
      `effectDurations[]`) directly, a real risk of confusing the two
      given their near-identical shape -- the matching timer counts
      down by `deltaMs`; once it drops below 0, it clamps to 0 and the
      ailment bit clears.

      **A real, surprising coupling, confirmed by reading this exact
      method and preserved rather than smoothed over:** ailment 7's own
      timer additionally requires `unconfirmed_A` -- the SAME flag
      `paintMonsters()` (M22, ported as `VisibleObjectRenderer::
      RenderMonsters`, M28) sets true only when it actually draws a
      real monster sprite that frame, reset false at the top of that
      method every call, never set by a Warden render. So ailment 7's
      countdown only progresses on a tick where a monster was ALSO just
      rendered -- a real dependency between this port's paint and tick
      passes. Modeled as a plain `monsterRenderedThisFrame` parameter
      (caller-supplied, same pattern this port uses throughout) rather
      than inventing mutable static state for it.

      Wired into `main.cpp`'s tick loop, ahead of that tick's own
      render pass, fed by whichever `RenderMonsters` call the
      *previous* tick made (matching the original's own "tick runs
      before this frame's repaint" ordering exactly -- confirmed, not
      assumed). Also replaced `RenderCorridorView`'s own hardcoded
      `ailment3Active`/`ailment4Active` = false,false with real
      `HasAilment(3)`/`HasAilment(4)` reads -- currently a no-op either
      way (nothing yet sets any ailment bit, `Monster.tick()` itself
      still has no wired caller), but no longer a placeholder.

      Verified with a new `status_countdowns_smoke.exe`: the ailment
      gate itself (untouched without the bit set); the countdown-then-
      clamp-and-clear transition for all 3 ailments independently; and
      ailment 7's own extra `monsterRenderedThisFrame` gate in both
      states. All 32 smoke tests pass; full clean rebuild stayed at
      zero `/W4` warnings. `javac` recompiled clean (8 expected
      warnings only) after the `GameCanvas.java` fill-in. Re-verified
      the real windowed exe still launches and runs after the `main.cpp`
      changes.

- [x] **M36 -- `PlayerCombatStats::TickPerSecond`, the second tick-loop
      helper** (this session). `GameCanvas.tickPerSecond()` (was
      decompiled/e.java's `l()`) was a throw-stub in
      `../src/GameCanvas.java` until now -- filled in and ported.
      **Also fixed a stale header comment on this exact method** that
      claimed it included "the Warden-visit gate" -- there is no Shop/
      Warden reference anywhere in its real body at all; that looks
      like a guess made before the method was ever actually read.

      Three independent per-real-second mechanics, byte-for-byte:
      1. EVERY currently counting-down `effectDurations[]` slot (all
         25, not just the 3 millisecond-timer ailments M35's
         `TickStatusCountdowns` separately tracks) decrements by 1.
         When slot 5 (effect id 6) reaches exactly zero THIS call, item
         109 ("daedric weapon", per the original's own debug println)
         is located by a new `PlayerInventory::FindEquippedSlotForItem`
         (`Player.findEquippedSlotForItem`, already named/transcribed
         in Java but not yet ported) and removed outright via the
         existing `RemoveInventorySlot` -- **not** the drop-into-the-
         world path `DropInventoryItem` uses; the weapon simply
         vanishes when its own temporary effect wears off.
      2. `HasAilment(4)` ("vampirism") drains 2% of maxHP from HP.
      3. `HasAilment(5)` ("mana burn") regenerates 10% of maxMagicka
         into Magicka; the moment Magicka reaches or exceeds its own
         max, it resets to EXACTLY ZERO (not clamped to max) and HP
         takes a 10%-of-maxMagicka hit instead -- a real, punishing
         overflow-and-burn mechanic, not a clamp bug.

      **A confirmed but provably inert 4th piece, transcribed into Java
      for a complete record but deliberately NOT reproduced in the C++
      port:** the original also loops over every monster registered on
      the player's current level, decrementing a byte pair on a
      throwaway DECODED COPY of each record -- `Monster.
      fromBytesShared()` (decompiled/d.java's own static `a(byte[])`)
      copies bytes into a shared scratch instance rather than aliasing
      the registry's own stored array, and the loop never calls
      `store()` afterward. Confirmed by reading `fromBytesShared()`'s
      own byte-by-byte copy directly (not assumed from the missing
      `store()` call alone) to have zero observable effect anywhere --
      same "document, don't mechanically port, a provably dead branch"
      treatment M10's own dead chest-record byte already got.

      **A real latent crash this port had to guard against rather than
      silently corrupt memory on:** if effect 6 expires while item 109
      is nowhere in inventory, `FindEquippedSlotForItem` returns -1 and
      `RemoveInventorySlot(-1, ...)` -- the real decompiled `y(int)` has
      NO lower-bound guard either (only an upper one), so the original
      would throw `ArrayIndexOutOfBoundsException` reading `this.H[-1]`.
      `RemoveInventorySlot` now throws `std::runtime_error` for a
      negative slot explicitly, same discipline as M18's `RemoveMonster`/
      M28's `unconfirmedTable_a` OOB guard -- silent out-of-bounds
      `operator[]` access would be strictly worse than the original's
      own clean crash.

      Wired into `main.cpp`'s tick loop with its own `secondAccumulatorMs`
      gate, matching `run()`'s own "accumulate deltaMs, fire once past
      1000ms" pattern exactly (against `GameClock`'s fixed 250ms
      interval rather than a real wall-clock delta -- equivalent here,
      since this port's own ticks are already fixed-interval).

      Verified with a new `tick_per_second_smoke.exe`: the decay loop's
      ordinary-slot vs. slot-5 branches; the daedric-weapon removal
      (including the compaction it leaves behind); the negative-slot
      crash-guard; both ailment branches (drain, and regen-then-
      overflow-burn) independently; and a real-data check that item 109
      actually exists in `itemsin.dat`. All 33 smoke tests pass; full
      clean rebuild stayed at zero `/W4` warnings. `javac` recompiled
      clean (8 expected warnings only). Re-verified the real windowed
      exe still launches and runs after the `main.cpp` changes.

- [x] **M37 -- `CombatResolution::TickMonstersOnLevel`, real monster AI
      -- finally closes the M14/M15 "Monster.tick()/chase() have no
      wired caller" gap** (this session). `GameCanvas.tickMonsterAI()`
      (was decompiled/e.java's `b(long)`) was a throw-stub, mislabeled
      `tickStatusCountdowns_b`, in `../src/GameCanvas.java` until now --
      renamed and filled in. Every low-level piece it needed
      (`Monster.isAdjacent`/`chase`/`tick`/`store`, `DungeonRuntime::
      StoreMonster`) already existed from earlier milestones; this
      milestone was purely the orchestration loop that was still
      missing.

      For every monster registered on the player's own current level:
      not adjacent -> `Chase()` one step (`IsAdjacent`'s own non-
      adjacent call ALSO resets `aiPhase` to 0, a confirmed side
      effect); adjacent -> an 800ms wind-up (`aiPhase` 0->1 starts the
      timer, `aiPhase` 1 past 800ms resolves the FIRST real attack via
      `MonsterTick` AND returns true -- the caller's cue to show the
      "Creature attacks!" popup, confirmed reproduced ONLY on this
      first wind-up-to-attack transition -- and any LATER 800ms-elapsed
      tick resolves a repeat attack with no further popup). Every
      branch stores the monster back -- unlike `tickPerSecond()`'s own
      confirmed dead-write loop (M36) over this exact same registry,
      this one really does persist.

      **Also renamed 2 more of this file's own earlier wrong stub
      guesses** while confirming what actually calls `tickMonsterAI`:
      `setSomeFlag` -> `refreshVisibleObjectsAndMinimap` (it doesn't set
      any flag at all -- it's `Player.refreshVisibleObjects()`, already
      fully transcribed, plus a minimap-grid refresh gated on which zoom
      level is actually showing), and `rollCampInterrupted` filled in
      for real (`Util.randomInt(10) == 1`, trivial). `tickMovementAndAI`/
      the real per-tick action dispatcher (`e(long)`) remain honest
      "not yet transcribed" stubs -- both fan out into several more
      interconnected methods (`a()`/`f()`/`g(long)`/`h(long)`/`n()`/
      `d(long)`/`m()`), too large a web to responsibly finish in this
      same pass.

      A real C++-specific hazard this port had to guard against that
      Java's own legacy `Hashtable`/`Enumeration` merely tolerates:
      `MonsterTick`'s own ailment-2 ("swarm curse") branch can insert
      NEW monsters into the SAME per-level registry this loop is
      enumerating (`DungeonRuntime::SpawnAmbushMonsters`) -- a real,
      confirmed original quirk, not introduced here, but a mid-loop
      insert into a live `std::unordered_map` while iterating it is
      genuine undefined behavior, strictly worse than Java's own
      unspecified-but-non-crashing behavior for the same case.
      `TickMonstersOnLevel` iterates a snapshot of spawnIds taken before
      the loop starts instead -- any ambush-spawned monster simply
      waits for the next tick's own fresh snapshot.

      Wired into `main.cpp`'s tick loop ahead of `VisibleObjects::
      Refresh` (matching `run()`'s own real relative order for this
      piece specifically; the exact ordering against M35/M36's own
      helpers is a documented simplification, not exact) -- and, since
      this is also the first real reachable call site for `MessagePopup`
      (M30), the "Creature attacks!" popup this method's own return
      value triggers is now actually shown and painted for real.

      Verified with a new `monster_ai_tick_smoke.exe`: the non-adjacent
      chase-and-reset path, all 3 phases of the adjacent wind-up (idle,
      winding-up, attack-with-message, repeat-attack-without-message),
      and a real integration run (20 ticks against a real generated
      level's real monster population) confirming no crash. **Caught a
      real bug in the test itself, not the port** -- an early draft's
      hand-built `MonsterState` left `typeIndex` at its default (0),
      which crashed `MonsterTick`'s own stat lookups
      (`typeStats[-1]`, wrapping to a huge `size_t`) hard enough to hang
      the whole process behind a blocked Windows crash dialog rather
      than cleanly failing -- fixed by giving every test monster a real
      `typeIndex`. All 34 smoke tests pass; full clean rebuild stayed
      at zero `/W4` warnings. `javac` recompiled clean (8 expected
      warnings only). Re-verified the real windowed exe still launches
      and runs.

- [x] **M38 -- `PlayerMovement::MonsterInFront` + live targetMonster
      refresh/death handling** (this session). `GameCanvas.
      refreshTargetMonster()`/`resolveTargetMonsterDeath()` (were
      decompiled/e.java's `a()`/`m()`) and `Player.monsterInFront()`
      (was decompiled/j.java's `n()`) are all new, fully transcribed
      this session -- none had a GameCanvas.java-side stub at all
      before now (found purely by reading the decompiled source
      directly while investigating `tickMovementAndAI`'s own real
      identity, decompiled/e.java's `e(long)`, confirmed as the per-tick
      ACTION dispatcher these 4 unconditional calls -- along with 2
      more not transcribed this session -- live inside).

      `Player.monsterInFront()`: the monster at the forward-facing
      look-ahead tile, via `computeMoveTarget(1)` -- but UNLIKE
      `commitMove()`'s own unguarded call to it (a real, if believed-
      unreachable, crash risk on a no-neighbor edge, preserved exactly
      there), gracefully returns null instead when that happens,
      matching the original's own explicit `pendingLevel <= 0` check.
      Ported as `PlayerMovement::MonsterInFront`, which reproduces that
      same graceful path by catching the exception `ComputeMoveTarget`
      already throws for exactly that case, rather than duplicating its
      entire boundary-stitching body just to avoid throwing in the
      first place.

      `refreshTargetMonster()`: refreshes `targetMonster` from
      `monsterInFront()` every call; when found, sets `unconfirmed_aa`
      true (`resolveHudIconSet()`'s own icon-set-1 gate, M29) and shows
      a 2-line "Found `<Name>`" popup, splitting the monster's own
      `typeName()` at its first space. `resolveTargetMonsterDeath()`:
      once `targetMonster`'s HP drops to 0 or below, rolls its death-
      drop (guaranteed for the level-37 type-41 "roaming" monster, M19),
      removes it from the registry (`ESGame.killMonster()`, already
      `DungeonRuntime::RemoveMonster`), heals the player 30% of maxHP
      under `hasAilment(4)` ("vampirism"), shows "Creature is dead!",
      then clears `targetMonster`/`unconfirmed_aa`. **Finally closes the
      other half of the M14/M15 gap** M37 didn't: `Monster.onDeath()`
      now has a real, confirmed caller too.

      Wired directly into `main.cpp`'s tick loop even though neither
      Java method has a reachable caller in the real game yet either
      (their own real caller, `tickMovementAndAI`/`e(long)`, remains a
      stub) -- same "wire the confirmed mechanic ahead of its still-
      stubbed original dispatcher" precedent M35-M37 already used. Also
      the second real call site for `MessagePopup`, and gives `render/
      hud_state.h`'s `HudState`/`TargetMonsterInfo` (M29) their own
      first live values instead of a fixed all-false/`nullopt` stand-in.
      `resolveTargetMonsterDeath`'s own trigger is currently
      unreachable in practice -- nothing yet lets the player actually
      damage `targetMonster` (no combat input exists) -- but wired
      anyway, same precedent.

      Verified with a new `target_monster_smoke.exe`: a real monster
      directly ahead is found (with `ComputeMoveTarget`'s own pending-
      field side effects confirmed too); nothing ahead resolves to
      `nullopt`; and the no-neighbor edge case resolves to `nullopt`
      gracefully rather than propagating the exception
      `ComputeMoveTarget` itself still throws for it. All 35 smoke
      tests pass; full clean rebuild stayed at zero `/W4` warnings.
      `javac` recompiled clean (8 expected warnings only). Re-verified
      the real windowed exe still launches and runs.

- [x] **M39 -- `CombatResolution::ResolveAttackInput`, real player
      attack input** (this session). `GameCanvas.resolveAttackInput()`
      (was decompiled/e.java's `d(long)`) is new this session -- no
      GameCanvas.java-side stub existed before now, found by the same
      `tickMovementAndAI`/`e(long)` dispatch-branch reconnaissance M38's
      own `a()`/`m()` came from. The player finally has a way to
      actually swing at `targetMonster`, not just target it.

      Once at least 500ms have passed since the player's own last
      attack (`lastAttackTimeMs`, a new instance field -- was
      decompiled/e.java's `B`) AND `targetMonster` is set, calls
      `Player.attack(targetMonster)` -- already fully ported since
      M14/M17 -- stamps `lastAttackTimeMs`, and sets `unconfirmed_S`
      (`paintFlashOverlays()`'s own monster-hit flash trigger, M22).
      Clears `unconfirmed_av` (the attack-request flag `keyPressed()`'s
      own '1' key already sets, gated there on `hotbarActionSet == 1`)
      either way, whether or not an attack actually connected -- a key
      press that arrives mid-cooldown is simply dropped, not queued.

      **Not reproduced in the C++ port:** the original's own `at` flag,
      set here (and in a couple of still-untranscribed sibling dispatch
      branches) to gate a per-tick passive-regen call inside
      `tickMovementAndAI`/`e(long)`'s own body -- that whole consumer
      isn't ported, so `at` would have no observable effect either way.

      Wired into `main.cpp`'s tick loop the same way M35-M38 wired
      their own confirmed-but-not-yet-dispatched mechanics, bound to a
      new SPACE key (polled the same way as the arrow keys) rather than
      reproducing the original's own `hotbarActionSet`-gated numeric-key
      binding, since `hotbarActionSet` itself remains out of scope
      (input handling in general, same gap M29/M31 already flagged).
      `ResolveAttackInput`'s own internal 500ms cooldown means holding
      SPACE auto-repeats attacks at that rate, the same way holding an
      arrow key auto-repeats movement once per tick.

      Verified with a new `attack_input_smoke.exe`: the no-target gate,
      the 500ms cooldown gate (including the exact `>=` boundary, not
      `>`), `attackRequested` clearing either way, and a real
      successful attack actually reaching `PlayerAttack` (cross-checked
      against `player.lastCombatTargetId`, the same invariant M14's own
      `PlayerAttack` test already relies on). All 36 smoke tests pass;
      full clean rebuild stayed at zero `/W4` warnings. `javac`
      recompiled clean (8 expected warnings only). Re-verified the real
      windowed exe still launches and runs.

- [x] **M40 -- `MenuFlow`, the Main Menu / new-game character-creation
      flow** (this session). Until now `main.cpp` hardcoded a class-0
      "Traveler" character and jumped straight into the live tick loop
      on launch (M34's own explicitly-flagged placeholder) -- this
      milestone closes that gap, the last one this "What's next"
      section had been carrying since M34: `main.cpp` no longer
      hardcodes a class.

      New `ui/menu_flow.h`/`.cpp` (`stormhold_render`): a small
      state machine standing in for `src/UIScreen.java` (its own
      mode-driven paint/keyPressed dispatch) fused with
      `src/ESGame.java`'s `commandAction()` screenGroups 2 (main menu),
      3-6 (class select/confirm/info/character-created), 7/101
      (welcome/intro/into-gameplay), and 305 (no-saved-game) -- the
      same class of "fuse several real call sites into one port-only
      flow" simplification `player/player_creation.h`'s own
      `CreateCharacter` already documents for its own two-step fusion.
      Real flow, walked end to end against real `charin.dat`/
      `npcstrings.dat` data: Main Menu (New Game/Continue Game/
      Credits/Exit) -> class select (all 7 real classes) -> class
      confirm ("You selected: <class>", See Class Info/Create
      Character) -> class info (new `PlayerCreation::
      CharacterSummaryShort`, `Player.characterSummaryShort()`'s
      counterpart -- race/class, HP/Magicka/Fatigue, all 8 attributes,
      every skill with a nonzero rank) -> character created -> enter a
      name (rejects under 3 letters, matching the real `TextField`
      validation exactly) -> welcome -> intro (real
      `ShopDialogue.groups[7][3]` body text) -> hands off into the
      exact same tick/render pipeline M34-M39 already built.

      **Deliberately not modeled:** Help (`loadHelpTopicBodies()`'s own
      Java transcription is itself incomplete past topic index 4 -- a
      real, pre-existing gap, not one this milestone introduces) and
      Continue Game's real load path (this port has no persisted
      save file at all yet -- `PlayerSave`, M20, only round-trips a
      `PlayerState` in memory, no `WorldRegistry`/master-list save
      format exists) -- selecting "Continue Game" always takes the
      real game's own "no saved game" branch, which is simply the
      truth for every run of this port today. Also: the real
      `classInfoUI`'s own commandAction branch (screenGroup 5) only
      reacts to `cmdBack`, but the only command ever added to that
      screen is `cmdOk` -- reading the whole dispatcher shows pressing
      its one softkey does nothing at all in the original (likely a
      genuine original dead end, not knowingly reproduced); this port's
      own Confirm/Cancel both just return to ClassConfirm from there.

      New-glyph addition to `graphics/bitmap_font.h`/`.cpp`: period and
      comma (appended, not inserted, so no earlier glyph's index
      shifts), needed for the real dialogue/message body text this
      milestone is the first to actually display at length.

      Name entry is typed via `GetAsyncKeyState('A'-'Z'/'0'-'9'/
      Backspace)`, edge-triggered the same one-key-per-press way all of
      this milestone's own menu navigation is -- no `WM_CHAR` plumbing
      added to `platform/win32/window.h` for this (a new
      `Window::RequestClose()` was added instead, for the Main Menu's
      own "Exit" item, since nothing else needed the window to close
      itself from inside its own idle callback before now). Message
      screens (class info/character-created/no-saved-game/credits/
      welcome/intro) repurpose Up/Down as a simple line-scroll instead
      of list selection -- confirmed necessary for real: the Intro
      body text alone word-wraps to more lines than fit on screen at
      once.

      Also folded in here, opportunistically, since the same transition
      point needed touching anyway: the very first live-game frame now
      calls `PlayerMovement::RefreshCorridorView` once right after the
      real character replaces the old hardcoded stand-in, so the
      corridor view is real from the first rendered frame instead of
      blank until the player's first keypress (a real, if minor, gap
      M34 never flagged explicitly).

      Verified with a new `menu_flow_smoke.exe`: the whole flow walked
      programmatically against real asset data (Main Menu clamping,
      Continue Game's no-saved-game branch, Credits round-tripping,
      class select/confirm/info, the too-short-name rejection then a
      successful 3-letter name, Welcome -> Intro -> Finished, Cancel()
      at every screen that has one, and Exit's own `exitRequested`
      flag) -- 37 smoke tests now pass in total. Full clean rebuild
      stayed at zero `/W4` warnings. No `.java` files changed this
      milestone (the real Java-side flow was already fully
      transcribed; this was pure port-side wiring), so no `javac`
      recompile was needed. Manually walked the ENTIRE flow in the real
      windowed exe via `PrintWindow`/`keybd_event` (Main Menu -> class
      select -> Nightblade -> class confirm -> class info -> character
      created -> name "AB" rejected, "ABC" accepted -> welcome -> intro
      (scrolled) -> live game, corridor rendering immediately) and
      confirmed every screen's text and the final hand-off render
      correctly.

- [x] **M41 -- the rest of `GameCanvas`'s per-tick action dispatcher
      (pure Java transcription, closing out decompiled/e.java)** (this
      session). Same shape as M22 ("pure phase-1 Java transcription, no
      C++ this time") applied to the LAST remaining unread part of
      `decompiled/e.java` (roughly lines 1325-1823): the real per-tick
      action dispatcher (`e(long)`, was this file's own
      "tickStatusCountdowns_e" placeholder) and every one of its dispatch
      targets that weren't already confirmed by M35-M39 --
      `a(long)`/`c()`/`f()`/`f(long)`/`g(long)`/`h()`/`h(long)`/`k()`/
      `n()`/`a(long,long)`. Only `void d(int)` (the NPC-dialogue-screen
      trigger `f(long)` calls into) is deliberately left as an explicit
      new stub -- see its own header comment -- everything else in the
      file's remaining stub inventory is now real: `grep -rl
      UnsupportedOperationException src/*.java` finds nothing outside
      `GameCanvas.java`, and within it only `talkToNpc()` (new, was
      `void d(int)`) still throws.

      Resolving every remaining single-letter call site required the
      same field-position cross-referencing this project has used since
      M3, applied to THREE more classes this session: `Player`/`j.java`
      (confirmed `i`=`crossingLevelBoundary`, `Q`=`justMarkedCamp`,
      `s`/`L`/`I`=`increaseHarmBuff`/`increaseArmorBuff`/
      `safeCampingBuff` by BEHAVIOR, not just position -- the
      `computeMoveTarget()`/`resetState()` call sites these letters
      appear in independently confirm each), `Shop`/`k.java` (confirmed
      `k.a()`=`wardenLeaves()`, `k.d`=`wardenPresent`, `k.f`=
      `wardenVisitCount` by reading `wardenLeaves()`'s own body directly
      -- "WARDEN LEAVES!!", the exact dungeons[1]/dungeons[0] bug M8
      already ported), and `ESGame.java` (confirmed `aq`=`npcHelloUI`,
      `R[]`=`npcChoicesUI[]`, `t`=`inventoryUI`, `aP`=
      `unconfirmedScreenAP`, `F()`=`newEndOfGameUI()` by counting
      `UIScreen`-typed field declarations in original order and matching
      position 1:1 against the already-renamed file -- unlike the other
      12 classes, `ESGame.java` kept its real name through decompilation,
      so its own decompiled source doubles as the position key directly).

      **The session's single most consequential finding: the confirmed
      trigger for Stormhold's own end-of-game/victory sequence, found
      while transcribing `n()` (now `resolveMovementSideEffects()`).**
      `Player.pendingLockedItemFlag` (M17's own name for decompiled `g`,
      set when `commitMove()`'s dropped-item block picks up a record
      with bit 2 (`0x4`) set on byte 6) coming back true after a move
      doesn't just mean "this item is locked" the way M17's own naming
      implied -- `GameCanvas.n()` responds to it by showing
      `ESGame.newEndOfGameUI()` (already a real, fully-transcribed
      method since `ESGame.java`'s own pass) and disabling auto-repaint.
      This also resolves `ESGame.java`'s own "`unconfirmedScreenAP`...
      no confirmed assignment site found" comment from that file's own
      pass -- the assignment site was always in `GameCanvas.java`, which
      that earlier pass never read. `Player.pendingLockedItemFlag`
      itself is NOT renamed this session (would ripple through
      `player_movement.h`/`dungeon_generator.cpp`'s own C++ callers) --
      flagged in `resolveMovementSideEffects()`'s own header comment
      instead, same "correction, not silent rename" policy M6's
      stairway note and M19's warden-clearing note already established.

      **A second real, previously-unrecovered caller, found transcribing
      `c()` (now `refreshNpcNameplateAndWardenLeave()`):**
      `WardenState::Leave`'s own missing driver (M8's own class comment:
      "that caller still isn't recovered" -- still open as of M19).
      When the tile directly ahead isn't a shop/NPC tile, and
      `isNpcDialogueDue()` is false, and the Warden is present, and the
      player's own `wardenLoreStep` has caught up to
      `Shop.wardenVisitCount` (heard everything this visit has to
      offer), `Shop.wardenLeaves()` finally gets called for real --
      distinct from M19's own confirmed finding (a successful STEP
      unconditionally clears `Shop.wardenPresent` straight to `false`,
      no tile mutation) -- that one is `Player.commitMove()`'s own
      direct assignment; this is the separate, tile-mutating trigger.

      **A third, smaller confirmed-dead-branch finding, in
      `resolveInteractInput()` (was `f(long)`):** its chest-interaction
      3-way switch has an `== -1` branch (would show "Chest locked!")
      that's UNREACHABLE -- `Player.collectChestItem()` (already ported
      since M12) only ever returns `0` or `1`, never `-1`. Preserved
      anyway, same "confirmed dead, not deleted" discipline as M10's
      `leftLevelZone`/M19's unreachable neighbor-throw.

      **Not transcribed, deliberately: `void d(int)`** (renamed
      `talkToNpc()`, a new explicit stub) -- `resolveInteractInput()`'s
      own NPC-talk branch. Its real body is straightforward (calls the
      already-ported `Shop.dialogue()`, then wires the result into
      `ESGame.npcHelloUI`/`npcChoicesUI[]`, both confirmed this session),
      but its `"<TAG>"` template-substitution VALUE reads a `UIScreen`
      field (decompiled `.N`, an `int`) this session didn't cross-
      reference against `UIScreen.java`'s own field list with enough
      confidence to transcribe responsibly -- left as a real stub rather
      than guessed, same standard M29 already held `IsNpcDialogueDue`'s
      dependencies to.

      No C++ changes this milestone (same framing M22 used: nothing yet
      to port these TO -- `main.cpp` still drives movement/attack
      directly via `PlayerMovement::Move`/`CombatResolution::
      ResolveAttackInput`, bypassing this whole dispatcher entirely, and
      wiring camp/rest/spell-casting/inventory-open/NPC-dialogue into the
      C++ port is real follow-on work of its own). Verified by `javac`
      recompiling the whole renamed tree clean (zero errors, same 3
      `[options]`-obsolete-source warnings the baseline already had) --
      the existing 37 smoke tests are unaffected (no C++ touched) and
      were not re-run.

- [x] **M42 -- the camp/rest system, wired into the live C++ port**
      (this session). The first of M41's newly-transcribed dispatch
      pieces to actually reach `main.cpp`'s tick loop: new
      `player/camp_state.h`/`.cpp` (`CampState`/`Camping`, `stormhold_
      player`) ports `GameCanvas.startCampOrRest()` (M41) plus run()'s
      own campState==1/2 handling (real Java since long before M41/M42
      existed, just never had a C++ home or a trigger) -- `Camping::
      Start`/`RollInterrupted`/`Tick`. Two new supporting methods it
      drives: `PlayerCombatStats::ApplyRestRecovery` (`Player.
      applyRestRecovery(fullyRested)` -- partial/full stat recovery,
      buff clearing, the item-96/ailment-cure rolls) and `DungeonRuntime
      ::SpawnAmbushMonsterNearPlayer` (`Dungeon.
      spawnAmbushMonsterNearPlayer(Player)` -- a genuinely DIFFERENT
      "ambush spawn" method from M16's own `SpawnAmbushMonsters`,
      confirmed by reading it separately rather than assumed to share
      that one's shape: exactly one monster, 5 fixed adjacent-tile
      candidates, no-op in the hub town).

      **A real, confirmed RNG-fidelity difference between the two
      "ambush spawn" methods, caught by reading `spawnAmbushMonsterNearPlayer`
      character-by-character rather than assuming it mirrors
      `spawnAmbushMonsters(count)`'s own already-ported shape:**
      `Monster.spawn(this)` rolls the monster type AND burns a spawnId
      UNCONDITIONALLY, before the original's own for-loop even tries its
      first candidate tile -- unlike M16's `SpawnAmbushMonsters`, which
      only advances its spawnId counter once per actually-placed
      monster (a confirmed, deliberate simplification there, since
      spawnId's numeric value has no observable effect). This method
      reproduces the real, unconditional burn exactly -- `m42_camp_state_
      smoke.cpp` demonstrates it directly (every attempt against a real
      generated level advances `spawnIdCounter` by exactly 1, success or
      not).

      Wired into `main.cpp`'s tick loop: a new 'C' key (same pragmatic
      "no hotbar system exists" stand-in M39 already used for attack)
      triggers `Camping::Start`, gated on `monsterRenderedLastFrame`
      ("Cannot Camp!" when a monster is visible, matching
      `tickPlayerAction`'s own `unconfirmed_A` gate). `Camping::Tick`
      runs every tick regardless; while it returns `StillWaiting`,
      movement/attack/target-monster-refresh/monster-AI are all skipped
      for that tick (matching run()'s own `shouldRunTick==false` gate)
      -- `TickStatusCountdowns`/`TickPerSecond`/`VisibleObjects::Refresh`
      /rendering all keep running unconditionally every tick regardless,
      same as the original's own tail-of-loop calls.

      **Deliberately NOT rendered as its own screen:** `GameCanvas.
      paintCampScreen()` (already real Java, transcribed long before
      this milestone) has no C++ counterpart yet -- the corridor view
      just keeps rendering underneath while camping, same "primitive/
      logic first, pixels later" discipline M21/M27/M29 already
      established. Also not wired: the death/respawn sequence
      `tickDeathAndRegen` (M41) itself would trigger -- `main.cpp` still
      has no live HP-reaching-0 handling at all, a pre-existing gap this
      milestone didn't touch.

      Verified with a new `camp_state_smoke.exe`: `Camping::Start`'s
      3 branches (default/safeCampingBuff/hub-town); both `RollInterrupted`
      outcomes reachable across 200 seeds; `Camping::Tick`'s full
      state==0/1/2 machine (including exact partial-vs-full recovery
      arithmetic on the disturbed/complete transitions, seed-searched to
      exercise BOTH the disturbed and not-disturbed branches
      deterministically); `ApplyRestRecovery`'s ailment-8 compounding
      (2/3 * 3/4 = 1/2) and buff-clearing; `SpawnAmbushMonsterNearPlayer`'s
      hub-town no-op and a real placement success against a real
      generated level 2. All 38 smoke tests pass; full clean rebuild
      stayed at zero `/W4` warnings. Manually launched the real windowed
      exe and confirmed it starts and stays up -- a full interactive
      keybd_event walkthrough of the camp/rest flow itself (like M40's
      own menu walkthrough) was NOT done this session, left as a
      lighter-weight verification gap worth closing in a follow-up.

- [x] **M43 -- chest detection/interaction + the Warden-leaving trigger,
      wired into the live port** (this session). The second of M41's
      newly-transcribed dispatch pieces to reach `main.cpp`'s tick loop.
      Two new methods, both mirroring `PlayerMovement::MonsterInFront`'s
      own established shape (M38): `PlayerMovement::ChestAheadOfPlayer`
      (`Player.chestAheadOfPlayer()`, was decompiled/j.java's `h()` --
      NOT the already-ported `h(long)`/`resolveSpellCastInput`, a
      distinct overload) and `PlayerInventory::CollectChestItem`
      (`Player.collectChestItem()`) -- port `GameCanvas.checkChestAhead()`'s
      "Chest" popup and `resolveInteractInput()`'s chest-opening half
      (both M41).

      **Deliberately NOT wired: `resolveInteractInput()`'s NPC-talk half**
      (`Player.shopAheadOfPlayer()>=0 -> talkToNpc()`) and
      `refreshNpcNameplateAndWardenLeave()`'s NPC-nameplate half (the
      `bit-32-set` branch) -- both need `Shop.questShopAt()`, which reads
      a live `questRewardClaimable[7]` array this port has no Shop
      quest-economy model to back (same gap M8's `WardenState`/M11's
      `ShopDialogue` class comments already flag), and `talkToNpc()`
      itself is still an explicit stub (M41). Flagged at their own
      omission points in `main.cpp` rather than silently dropped.

      **The OTHER half of `refreshNpcNameplateAndWardenLeave()` DOES get
      wired here, though** -- the Warden-leaving trigger (M41's own
      finding: `WardenState::Leave`'s missing caller, open since M8),
      needing none of the Shop quest-economy state the nameplate half
      does. Reuses `DungeonRuntime::ViewGridAt` (M21) against the
      player's own live `corridorView` to test the look-ahead tile's bit
      32, `render/hud_state.h`'s `IsNpcDialogueDue` (M29), and
      `WardenState::Leave` (M8, its own confirmed dungeons[1]/
      dungeons[0] bug preserved exactly, untouched by this milestone).

      Bound to a new 'F' key for the interact action (`ChestAheadOfPlayer`
      is polled every tick regardless, matching `checkChestAhead()`'s own
      unconditional per-tick check). `CollectChestItem` takes its
      `record` parameter BY VALUE rather than by reference -- the
      original's own `record[2] = 2` write is a confirmed dead byte (M6),
      and the record is removed from the registry immediately after
      either way, so there's no aliasing behavior worth preserving.

      Verified with a new `chest_interaction_smoke.exe`: `ChestAheadOfPlayer`
      finding a real stored chest directly ahead, resolving to `nullopt`
      with nothing ahead, and the same no-neighbor edge case
      `MonsterInFront` already handles gracefully; `CollectChestItem`'s
      with-space path (item added, chest removed from both the tile bit
      and the registry) cross-checked against a REAL category-11 item
      from `itemsin.dat` (giftPointsFound increases by exactly that
      item's own subtype column, not assumed); and the without-space path
      (auto-drop onto the same tile, chest still removed). 40 smoke tests
      now pass in total; full clean rebuild stayed at zero `/W4`
      warnings. Manually launched the real windowed exe and confirmed it
      starts and stays up -- same lighter-weight verification level M42
      used, not a full interactive walkthrough.

- [x] **M44 -- `talkToNpc()`, closing out `decompiled/e.java` completely
      (Java only)** (this session). Same bounded "pure Java transcription,
      no C++" scope M41/M22 already established. Resolved the one piece
      M41 left unfinished: the decompiled `.N` field on `h`/`UIScreen`,
      position- AND type-confirmed this session (a 27-field, byte-for-
      byte type-sequence match against `UIScreen.java`'s own declaration
      order, from `Q`/`mode` through `A`/`highlightAllLines`) as
      `UIScreen.contextIndex` -- a SECOND confirmed write site for that
      reusable per-screen scratch int (ESGame.java's dispatcher already
      uses it for shopId/level-up-attribute-index), which also means
      `contextIndex`'s own "declared, never seen read/written again"
      comment was already stale before this session even found its own
      site; corrected in `UIScreen.java` alongside this method.

      `talkToNpc()` calls `Shop.dialogue(player, npcId, 1, 0)` (already
      fully ported since an earlier session) and, on a non-null result,
      wires it into `ESGame.npcHelloUI` (decompiled `aq`, position-
      confirmed AND behaviorally confirmed -- both are constructed
      identically via `new UIScreen(this, 4, 8)` /
      `setupMessage("NPC name here", "NPC text here", true)`):
      `setTitle`/`setMessageBody`/`nextScreen = npcChoicesUI[npcId]`
      (decompiled `R[]`, also position-confirmed)/`contextIndex = npcId`.
      Then re-reads that same next screen and substitutes its
      `tagTemplate`'s `<TAG>` placeholder (`Util.replace`) with one of
      Shop's per-NPC point totals: `rewardsGiven[npcId]` for quest shops
      0-3 (`Shop.isQuestShop`), `benecaPoints` for npc 4, `helgaPoints`
      for npc 5 -- Varus (6) hits none of these branches. A null
      `dialogue()` result (Beneca(4)/Helga(5) only) falls back to
      re-showing their own existing choices screen with the same
      substitution and a hardcoded "has nothing more to say" println.

      **A real, confirmed dead read, preserved rather than cleaned up:**
      the re-read screen's own `messageBody()` is called and its result
      immediately discarded, overwritten by the very next statement
      without ever being read back -- same "assigned but never used"
      shape as M43's `record[2] = 2` dead byte, just at method-call
      granularity instead of a struct field.

      `GameCanvas.java` now has zero remaining `UnsupportedOperationException`
      stubs for anything outside the ~15 pixel-paint helper methods the
      file's own header comment already scopes out (`paintFlashOverlays()`/
      `paintUnknown_b()` chief among the still-unread ones) -- every
      non-paint dispatch method is transcribed. Verified via a clean
      `javac --release 8` recompile against the MIDP stub jars (3
      warnings, all pre-existing/unrelated, 0 errors).

      **Deliberately NOT wired into the C++ port this session**, same
      reasoning M43 already gave for deferring this exact method: no
      Shop-economy C++ model exists at all yet (`ShopDialogue` in
      `port/src/assets/shop_dialogue.h` is a pure string-table loader, no
      `questRewardClaimable`/`benecaPoints`/`rewardsGiven`/`questState1`/
      `questState2` live state), and `Shop.dialogue()`'s own real body is
      a large per-shopId/per-action switch -- porting it responsibly is a
      milestone of its own, not a quick follow-on to a Java-only pass.

- [x] **M45 -- `paintFlashOverlays()`'s C++ body, wired into the live
      port** (this session). The short follow-on the previous "what's
      next" flagged: the Java side was already fully transcribed (M22),
      so this milestone is pure C++ -- no decompiled-source
      investigation needed. New `render/flash_overlay_assets.h`
      (`FlashOverlayAssets`, the same small-bundle-loader shape
      `HotbarAssets` already established) loads the 3 real plain PNGs
      (`blood1.png`/`monsterspell.png`/`selfspell.png`, confirmed
      filenames via `ESGame.java`'s own asset-loading call site) through
      `DecodedImage`/`Backbuffer::Blit` (both M24). New `render/
      flash_overlay.h`/`.cpp` (`FlashOverlayState` + `FlashOverlay::
      Paint`, the same state-struct-plus-static-class shape
      `MessagePopupState`/`MessagePopup` already established, M30): the
      3 independent one-shot flash overlays, each self-clearing its own
      trigger flag once drawn, at a small random jittered position via
      `RandomInt1Based` (`Util.randomInt()`'s own 1-based convention).

      **A deliberate, documented divergence from the real game's own
      shared static RNG:** the original draws these jitter offsets from
      `ESGame`'s single process-wide `Random rng` -- the same stream
      combat/dungeon generation etc. all draw from -- purely for cosmetic
      screen-space jitter with zero gameplay effect. This port already
      keeps combat/ambush spawning on dedicated `JavaRandom` instances
      rather than one shared stream (M16/M37/M42's own "confirmed
      different RNG fidelity" findings), so `main.cpp` reuses its own
      `combatRng` here rather than adding a 3rd RNG stream or wiring in
      a genuinely shared one -- consistent with that existing split, not
      a new divergence.

      Wired into `main.cpp`'s tick loop: `CombatResolution::
      ResolveAttackInput`'s own return value (M39, previously discarded)
      now sets `FlashOverlayState::hit` on a landed attack --
      `paintFlashOverlays()`'s own `unconfirmed_S` trigger, real since
      M39 but with no observable effect anywhere in this port until now.
      `Paint()` is called directly after `MessagePopup::Paint`, matching
      `paintGameView()`'s own real call order. `spellHitMonster`/
      `spellHitSelf` (`unconfirmed_ao`/`unconfirmed_am`) paint for real
      here too but stay permanently unreachable until spell casting
      itself is wired -- no invented trigger site, same "wire what's
      reachable, document what isn't" discipline M41/M43 already used.

      Verified with a new `flash_overlay_smoke.exe`: real asset decode
      dimensions; a true no-op with every flag false; each of the 3
      flags independently, with the exact jitter offset re-derived
      INDEPENDENTLY via a second `JavaRandom` fed the same seed (not
      read back from `flash_overlay.cpp`'s own output) and checked
      pixel-exact against the real loaded PNG's own opaque pixels at
      that computed offset, not just "some pixel changed somewhere"; all
      3 flags set at once, all 3 clearing after one `Paint()` call. 41
      smoke tests now pass in total; full clean rebuild stayed at zero
      `/W4` warnings. Manually launched the real windowed exe and
      confirmed it starts and stays up.

- [x] **M46 -- spell casting/cycling, wired into the live port** (this
      session). New `combat/spell_casting.h`/`.cpp` (`stormhold_combat`,
      alongside `combat_resolution.h` since `CastOnMonster` needs a live
      `MonsterState` + `WorldRegistry`, and its own case-14 branch calls
      `CombatResolution::PlayerAttack` directly): `SpellCasting::
      CastOnSelf`/`CastOnMonster` (`Player.castOnSelf(int)`/
      `castOnMonster(int, Monster)`, both fully confirmed Java since M13
      but never ported to C++ before now) and `CycleSelectedSpell`
      (`Player.cycleSelectedSpell()`), plus the GameCanvas-level dispatch
      pair `ResolveSpellCastInput`/`ResolveSpellCycleInput`
      (`resolveSpellCastInput(long)`/`resolveSpellCycleInput(long)`, M41's
      own `unconfirmed_ap`/`unconfirmed_U` dispatch branches, decompiled/
      e.java's `h(long)`/`g(long)`) -- M41's whole per-tick dispatch web is
      now fully wired except `openInventory()`. Also extended
      `player/player_combat_stats.h` with `ActiveAilmentCount`/
      `CureRandomAilment` (`Player.activeAilmentCount()`/
      `cureRandomAilment()`, a genuinely different mechanic from
      `ApplyRestRecovery`'s own per-bit 25%-chance loop, not a duplicate of
      it).

      **Real key-binding finding:** unlike attack/camp/interact (all
      pragmatic stand-in keys, SPACE/'C'/'F', since their real hotbar keys
      are gated behind a `hotbarActionSet` this port doesn't model), spell
      cast/cycle's real key codes ('3'/'5') are confirmed genuinely
      UNCONDITIONAL in `keyPressed()` -- no `hotbarActionSet` gate at all.
      So `main.cpp` binds directly to the real keys this time, edge-
      triggered (`KeyEdge`) like camp/interact, not level-triggered like
      attack's own SPACE.

      **Two real, deliberately-preserved findings from reading
      `castOnMonster`'s whole switch side by side, confirmed by dumping
      the real `spellsin.dat` rows while scoping this milestone (25 spells,
      names/school/cost/power all read directly, not assumed):**
      1. Only 5 of its 9 `scratch[]`-mutating cases (4, 11, 12, 13, 15)
         call `target.store()` -- cases 10, 16, 18, 19 mutate
         `target.scratch[]`/`player.effectDurations[]` but never persist
         back to the registry. A real asymmetry, preserved exactly (see
         `combat/spell_casting.h`'s own class comment) and directly proven
         by a live test: casting spell 4 (which stores) really updates the
         `WorldRegistry`'s own copy; casting spell 16 (which doesn't)
         leaves the registry's copy untouched even though the local
         `target` mutated.
      2. Spell 17 ("Sanctuary") has `school=1` in the real data, not 2 --
         so `Spell.isOffensive(17)` is always false and `castOnMonster`'s
         own case 17 (a defensive self-buff, oddly written into the
         *offensive* spell method) is confirmed UNREACHABLE through any
         real dispatch. Transcribed anyway, not deleted, same "port a
         confirmed-dead branch faithfully" treatment M40's own
         classInfoUI dead-branch finding already established.

      Also confirmed and preserved: `castOnMonster`'s own local
      `Spell.power` read is never actually used anywhere in its switch
      (every damage case derives its magnitude from skill values/
      `targetOffense` instead, unlike `castOnSelf` where `power` IS used)
      -- a genuine dead read, elided (not declared at all) rather than
      left to trip this port's `/W4` bar, same treatment given
      `Player.nthKnownSpellId`'s own dead local (computes `spellId = i+1`
      but returns `i`) in `CycleSelectedSpell`'s private helper. And: a
      real, surprising control-flow finding -- `lastSpellCastTimeMs`
      restamps even when an offensive spell finds no target (`NoMonster`),
      since the original's own restamp sits OUTSIDE the offensive/self
      dispatch; preserved exactly, not "fixed" into only restamping on an
      actual cast.

      `ResolveSpellCastInput` returns a `Result` enum (`NotRequested`/
      `InvalidSpell`/`NotEnoughMagicka`/`NoMonster`/`OnCooldown`/
      `CastOnMonster`/`CastOnSelf`) rather than a bare bool, same
      "game logic returns a signal, the caller shows the message" pattern
      `CampTickResult`/`ResolveAttackInput` already established --
      `main.cpp` shows `MSG_NOT_ENOUGH_MAGICKA`/`MSG_NO_MONSTER` and sets
      `FlashOverlayState::spellHitMonster`/`spellHitSelf` accordingly,
      finally giving those two M45 flags a real trigger site.
      `ResolveSpellCycleInput`'s own "newly selected spell" message reuses
      `main.cpp`'s existing `ItemFoundMessageLines` helper rather than
      duplicating a 3rd copy of the same word-split algorithm -- confirmed
      identical output to `resolveSpellCycleInput()`'s own inlined 2nd
      copy of `itemFoundMessageLines()`'s split, unlike the original's own
      two separate near-identical blocks.

      Verified with a new `spell_casting_smoke.exe`, built on real
      `spellsin.dat` data (dumped first to hand-pick spell ids whose
      effect is provably tier-independent, so every check is deterministic
      regardless of RNG seed -- no seed-hunting needed): every
      `ResolveSpellCastInput` branch (not-enough-Magicka, invalid spell,
      the 500ms cooldown's exact `>=` boundary, no-monster-but-still-
      restamps, a deterministic self cast, a deterministic
      ailment-cure-with-count-1 cast, a deterministic offensive cast with
      a live `store()` check), the store()-asymmetry proof above, and
      `CycleSelectedSpell`/`ResolveSpellCycleInput`'s wraparound/single-
      spell/no-spells-known behavior. 42 smoke tests now pass in total;
      full clean rebuild stayed at zero `/W4` warnings (after eliding the
      2 confirmed-dead locals above, which otherwise tripped it). Manually
      launched the real windowed exe and confirmed it starts and stays up.

- [x] **M47 -- death/respawn sequence, wired into the live port** (this
      session). New `player/death_sequence.h`/`.cpp` (`stormhold_player`,
      alongside `camp_state.h` -- same "GameCanvas session field, not on
      Player.java itself" placement): `DeathSequence::TickDeathAndRegen`
      (`GameCanvas.tickDeathAndRegen()`) and `DeathSequence::Tick` (run()'s
      own already-Java-transcribed `facing != 1` death/respawn state
      machine, finally with a real driver -- the same "primitive ported
      long ago, wired now" shape M42 gave the camp state machine).
      HP reaching 0 (via `CombatResolution::TickMonstersOnLevel`, M37) now
      actually does something instead of silently going negative forever.
      Also new: `PlayerCombatStats::TickFatigueRegen`
      (`Player.tickFatigueRegen()`), `PlayerCreation::NormalizeToMaxStats`/
      `RespawnAfterDeath` (`Player.normalizeToMaxStats()`/
      `resetState(classIndex, true)`'s "respawn" branch, the ONE branch
      M9's own `CreateCharacter` didn't port -- that milestone only wired
      `resetState(classIndex, false)`'s "new character" branch),
      `PlayerInventory::IsSlotEquipped` (`Player.isSlotEquipped()`,
      confirmed sole caller: exactly this respawn handling), and a brand
      new asset loader, `assets/dungeon_names.h`/`.cpp`
      (`Dungeon.loadNames()`/`displayNames()`, `dungnamesin.dat` -- never
      touched by any earlier milestone) backing the real respawn-location
      message.

      **`PlayerState::facing` doubles as the death-sequence's own state
      field, confirmed directly from the original, not a port-side
      invention:** 1=alive/normal, 2=just died this tick, 3=waiting out
      the 5s respawn window -- the exact same field `player/
      player_movement.h`'s own `CommitMove`/`ComputeMoveTarget` drive for
      ordinary movement. `DeathState` (this milestone's own new struct)
      only carries the death timestamp (`GameCanvas.unconfirmed_s`) --
      structurally the same split `player/camp_state.h`'s own `CampState`
      already established for `campRollAt` vs. `campState`.

      **A real, confirmed structural finding:** in the original, run()'s
      own `campState==1`/`campState==2`/`facing != 1` dispatch is ONE
      shared else-if chain, so a player can never be simultaneously
      camping and dead. This port's own `Camping::Tick` and
      `DeathSequence::Tick` are two independent calls in `main.cpp`
      instead (ANDed together into one `shouldRunTick`) -- a structural,
      not behavioral, divergence: reaching HP<=0 needs
      `CombatResolution::TickMonstersOnLevel` to actually deal damage,
      which itself only runs once camp/death gating has already cleared
      for the tick, so the two states still can't overlap in real play.

      **Two real findings carried through from `Player.java`'s own header
      comments, not fresh discoveries, both preserved rather than
      "cleaned up":** `resetState(classIndex, true)`'s own trailing
      `this.facing = 1` is confirmed REDUNDANT (its own
      `setHubSpawnPosition(true)` call already sets `facing = 1` as part
      of the hub-position write) -- kept anyway, matching the original's
      own identical redundant write. And the respawn-location message
      selection (`enteredNewLevelZone` -> "Warden's Camp",
      `leftLevelZone` -> "Outer Camp", else the current level's own
      display name) reads TWO flags that `RespawnAfterDeath`/
      `resetState(true)` never clears -- confirmed by reading
      `resetState()` directly, neither field is in its own reset list --
      so this can genuinely read STALE state from whatever the player's
      last real move was before dying, not a bug on this port's side.
      `leftLevelZone`'s own branch is additionally confirmed permanently
      UNREACHABLE (M10's own finding), transcribed anyway, same "port a
      confirmed-dead branch faithfully" treatment M40/M46 already gave
      their own dead branches.

      Wired into `main.cpp`'s tick loop right alongside `Camping::Tick`:
      `DeathSequence::Tick` runs every tick (same position `Camping::Tick`
      already occupies), gating `shouldRunTick` the same way; a
      `Respawned` result shows the real respawn-location message via the
      new `DungeonNames` asset; `DeathSequence::TickDeathAndRegen` runs at
      the end of the `shouldRunTick` block (after `TickMonstersOnLevel`),
      and a `true` (just-died) return clears `targetMonster`/
      `hudState.unconfirmedAa` the same way `GameCanvas.unconfirmed_aa`'s
      own reset does -- this port's own render/HUD-state boundary, kept
      out of `death_sequence.h` itself, same "game logic returns a
      signal, caller owns its own render/HUD state" pattern
      `combat/spell_casting.h`'s own `Result` enum already established.

      **NOT modeled, same already-documented gap M46's own
      `ResolveSpellCastInput` header comment flags for its own read of the
      same field:** `unconfirmed_at` (an "an action was already resolved
      this tick" gate the original puts on `tickFatigueRegen`'s own call
      site) -- fatigue regen here always runs unconditionally instead. Also
      not modeled: the `facing==2->3` transition's own message-popup clear
      (`unconfirmed_ad = false; messagePriority = 0`) -- already a
      confirmed, documented gap from `render/message_popup.h`'s own
      `Tick()` comment (M30), if under a slightly mislabeled "camp-state
      transition" description there -- corrected here: it's this death-
      sequence transition, not a camp one.

      Verified with a new `death_sequence_smoke.exe`: `TickFatigueRegen`'s
      exact gain formula and max-clamp; `TickDeathAndRegen`'s alive/dead
      (including a negative, not just zero, HP overshoot) branches;
      `Tick`'s Alive/Waiting/Respawned branches including the exact
      `facing==2->3` transition tick and the `<=5000` vs. `>5000` boundary;
      a full integration respawn (a real `PlayerCreation::CreateCharacter`
      character, with an added un-equipped item alongside its 2 equipped
      starting items, real accumulated gift-points/rumor/camp-mark
      progress, and a real ailment/effect/combat-scratch state) confirming
      full stat restoration, the (12, 14) hub landing point, PRESERVED
      gift/rumor/camp-mark progress (unlike fresh character creation),
      CLEARED ailment/effect/combat-scratch state, and that only the
      un-equipped item gets stripped; and `RespawnMessageLines`'s all 3
      branches against real `dungnamesin.dat` data (including confirming
      the hub's own display name is non-empty). 43 smoke tests now pass in
      total; full clean rebuild stayed at zero `/W4` warnings. Manually
      launched the real windowed exe and confirmed it starts and stays up.

- [x] **M48 -- movement message-popup layer, wired into the live port**
      (this session). New `player/movement_messages.h`/`.cpp`
      (`stormhold_player`, alongside `death_sequence.h`): `MovementMessages
      ::Resolve` (`GameCanvas.resolveMovementSideEffects()`, confirmed
      real and flagged as unwired by M47's own "what's next" note) --
      real movement finally shows the level-crossing message (reusing
      M47's `DungeonNames`) and the "Found <item>!"/"Several items!"
      message off the same before/after `inventoryCount` comparison M17
      already wires through `DungeonRuntime`.

      **`MovementMessageResult` is genuinely unlike every other
      Result-shaped enum this port already uses (`CampTickResult`/
      `DeathTickResult`/`combat/spell_casting.h`'s `Result`):** confirmed
      by reading `resolveMovementSideEffects()` directly, the original
      checks its crossing-message condition and its `itemsFound` count as
      TWO INDEPENDENT `if`-statements, not one if/else-if -- a single move
      can genuinely produce BOTH messages at once (nothing about
      `CommitMove`'s own tile-bit checks makes level-crossing and an
      on-tile item pickup mutually exclusive), with the original's own
      second `showMessage()` call simply overwriting the first's popup.
      `MovementMessageResult` models this with two independently-optional
      fields rather than one enum, and the new smoke test proves both
      firing together directly rather than just documenting it.

      **A real, confirmed-by-grep original quirk, preserved rather than
      "fixed" with an invented reset:** `Player.pendingLockedItemFlag` is
      never set back to `false` anywhere in the whole original codebase
      (both its write sites are inside `CommitMove`'s own dropped-item
      block, only ever setting it `true`) -- so once a player ever picks
      up one locked item, EVERY subsequent move reports
      `lockedItemEndOfGame` forever, permanently shadowing the
      crossing-message branch from then on. In the real game this is
      likely inconsequential (the flag's real effect is triggering
      `ESGame.newEndOfGameUI()`, and normal play doesn't continue past
      that) -- this port doesn't model the end-of-game screen at all yet,
      so the practical effect here is a silent, harmless no-op on every
      move after the first locked pickup, not a crash or a wrong message.

      Wired into `main.cpp`'s tick loop right after whichever of the 4
      existing `PlayerMovement::Move` calls ran, gated on a held movement
      key (matching the original's own `pendingMoveDir != 0` gate, this
      port's simpler direct-dispatch equivalent -- see M34's own header
      comment on why this file never buffers a `pendingMoveDir`/
      `strafeFlag` pair the way `resolveMovementSideEffects()`'s own
      declaration comment notes the original does). `strafeFlag`'s own
      clear (the original's `if (this.strafeFlag) this.strafeFlag =
      false;`) is N/A here for the same reason -- this file has no
      buffered strafe state to clear in the first place, not a new gap.

      Verified with a new `movement_messages_smoke.exe`:
      `pendingLockedItemFlag` correctly taking precedence over (and
      suppressing) an otherwise-would-fire crossing message; the full
      3-way crossing-message choice against real `dungnamesin.dat` data
      (including the confirmed-dead `leftLevelZone` branch, preserved);
      `itemsFound`'s None/One/Several branches against real
      `ItemDatabase` names (including the negative/equipped-id
      `Math.abs()` case); and a direct proof that a crossing message and
      an items-found message both populate from one call. 44 smoke tests
      now pass in total; full clean rebuild stayed at zero `/W4` warnings.
      Manually launched the real windowed exe and confirmed it starts and
      stays up.

- [x] **M49 -- `WorldRegistry` save/load format** (this session). New
      `dungeon/world_save.h`/`.cpp` (`stormhold_dungeon`, alongside
      `dungeon_runtime.h`): `WorldSave::ToBytes`/`FromBytes`
      (`ESGame.writeAllLevelRegistries()`/`readPerLevelRecords()`), the
      `monsters[]`/`chests[]`/`droppedItems[]` half of the real
      "RecordStore-based save format" (`ESGame.java`'s own header
      comment) -- the piece M47/M48's own "what's next" note flagged as
      still missing after M20's `PlayerSave`. `MonsterRuntime::ReadFrom`/
      `WriteTo` and `BinaryReader`/`BinaryWriter` (both already added at
      M20) do all the real per-record work; this milestone is the
      "count, then N records, per level" framing loop around them.

      **Deliberately NOT ported here, two confirmed, separate gaps:**
      `writeMasterLists()`/`readMasterLists()` (`Item.nextSpawnId`/
      `Monster.nextSpawnIdCounter` plus most of `Shop`'s own static
      per-shop quest-economy state) -- blocked on the not-yet-built
      Shop-economy C++ model, since most of what it writes
      (`questRewardClaimable`/`interactionCount`/`rewardsGiven`/
      `questState1`/`questState2`/`benecaPoints`/`helgaPoints`) has no
      live port-side state to read from at all yet. And the
      RecordStore-equivalent file I/O itself (multi-save-slot naming/
      discovery/deletion) -- this port needs its own plain-file save
      mechanism, not a MIDP `RecordStore`, a later milestone's job
      together with wiring `main.cpp`'s Main Menu "Continue Game" item
      (still always takes the no-saved-game branch, M40) to any of this.

      **A real, confirmed dead-code finding, NOT mechanically re-ported:**
      `ESGame.java` declares TWO near-identical methods for reading this
      data -- `readPerLevelRecords` (confirmed, by reading
      `loadGameState()` directly, the ACTUAL one it calls) and
      `readAllLevelRegistries` (grepped: declared, with no caller
      anywhere in the whole file -- `loadGameState()`'s own header
      comment even NAMES `readAllLevelRegistries` as what it calls, but
      its real body calls `readPerLevelRecords` instead, a stale/
      copy-pasted comment, not a second real call site). `FromBytes`
      ports `readPerLevelRecords`'s real logic only -- hand-duplicating
      an entire dead METHOD with zero callers would just be dead weight,
      a real difference from this port's usual "preserve a dead BRANCH
      inside a live method" treatment (M40/M46/M47/M48's own findings).

      **NOT modeled: tile-flag resync**, matching the original exactly --
      neither `writeAllLevelRegistries` nor `readPerLevelRecords` touches
      `Dungeon`'s own tile bits at all. The real load path's own next
      step, `Dungeon.refreshTileFlagsFromRegistries()`, is ALREADY ported
      (`DungeonRuntime::RefreshTileFlags`, M16 -- its own doc comment
      already names "after a save/load" as its intended use) -- the
      caller is expected to run that, once per level, right after
      `FromBytes`, same "caller supplies/owns world state" pattern this
      whole port already uses throughout.

      Not wired into `main.cpp` -- no file I/O layer exists yet to call
      `ToBytes`/`FromBytes` from (see the gaps above).

      Verified with a new `world_save_smoke.exe`: a fully empty
      `WorldRegistry` round-trips to all-empty; the hub (index 0) is
      proven to be genuinely SKIPPED by the format for monsters/chests
      but INCLUDED for dropped items (by manually stuffing a record into
      index 0 and confirming which ones survive the round trip, not just
      asserting the loop bounds); and real spawned-monster/chest/
      dropped-item data (including a negative scratch byte, a real
      64-bit timestamp, and negative dropped-item record bytes) round-
      trips byte-exact, with every untouched level confirmed to stay
      empty. 45 smoke tests now pass in total; full clean rebuild stayed
      at zero `/W4` warnings.

- [x] **M50 -- `GameSave` file I/O + a real "Continue Game"** (this
      session). New `player/game_save.h`/`.cpp` (`stormhold_player`,
      alongside `player_save.h`): `GameSave::Exists`/`Save`/`Load`
      (`ESGame.saveGameState()`/`loadGameState()`) -- the plain-file
      framing M49's own "what's next" note flagged as ready once
      `WorldSave` existed. `PlayerSave::ToBytes`/`FromBytes` (M20) and
      `WorldSave::ToBytes`/`FromBytes` (M49) already do all the real
      field-level work; `Save`/`Load` just write/read a 4-byte
      player-blob length followed by the player blob then the world blob
      (the rest of the file, read to EOF) to one fixed path.

      **A confirmed simplification, not a guess:** read together,
      `generateUniqueSaveName`/`findMostRecentSaveName`/`deleteOtherSaves`
      can never actually leave more than one `RecordStore` on disk at a
      time -- `deleteOtherSaves` prunes every OTHER store at the end of
      every successful save, so the random-suffix/enumerate/prune dance
      is pure MIDP name-collision plumbing around a single real save
      slot, not an actual multi-save-slot feature (nothing else in
      `ESGame.java` ever lists or offers a choice between save names to
      the player either). `GameSave` skips reproducing that dance and
      just uses one fixed path -- same observable behavior (exactly one
      save persists, always the most recent), same "behavioral
      reimplementation, not byte-exact" call this project's own
      "Decisions carried through every milestone" section already
      commits to.

      **Wired into the live port:** `main.cpp` computes a real
      `<userDir>/savegame.dat` path (empty when `STORMHOLD_USER_DIR` is
      unset, the same "no launcher, no persisted state" convention the
      log file already uses -- `GameSave::Exists`/`Save`/`Load` all
      degrade to harmless no-ops on an empty path). `MenuFlow::Confirm`'s
      MainMenu "Continue Game" item (`ui/menu_flow.h`/`.cpp`) now calls
      `GameSave::Exists(savePath)` and only falls to `NoSavedGame` when
      that's false -- otherwise it jumps straight to `Finished` with a
      new `MenuFlowState::loadRequested` flag set, and `main.cpp`'s own
      hand-off block (the same place a New Game's `draft` becomes the
      live `player`) does the real `GameSave::Load` call, since only
      `main.cpp` holds the live `WorldRegistry`/level array `MenuFlow`
      itself never touches. `Confirm`'s new `savePath` parameter defaults
      to an empty string, so every pre-M50 call site (M40's own smoke
      test included) keeps compiling unchanged and keeps taking
      `NoSavedGame` exactly as before.

      **A genuine port-only correctness fix, not a Java transcription:**
      this port eagerly builds/registers EVERY level up front
      (`BuildWorld`, `main.cpp`) rather than the original's own lazy
      per-level `populate()` -- so unlike a REAL
      `refreshTileFlagsFromRegistries()` call (always a level's very
      FIRST population, onto all-zero transient tile bits),
      `DungeonRuntime::RefreshTileFlags` after a `GameSave::Load` would
      OR the loaded registries' bits on top of whatever the throwaway
      freshly-generated world already left set -- a chest already looted
      before that save was written would wrongly stay flagged forever.
      `RefreshTileFlags` itself is untouched (still a faithful,
      purely-additive port of the real method -- M16's own smoke test
      still only ever calls it against an all-zero level, same as the
      original always does); `main.cpp`'s own new, port-only
      `ClearTransientTileFlags` clears exactly the 3 bits
      `RefreshTileFlags` ever sets (2/4/16) first, so the load path gets
      the same "OR onto zero" result the original always has.

      **Deliberately NOT ported here, unchanged from M49's own gap:**
      `writeMasterLists()`/`readMasterLists()`, still blocked on the
      not-yet-built Shop-economy C++ model. **Deliberately not wired:** a
      real Save trigger -- the original's own Save/Load menu items live
      inside an in-game pause/stats screen (Stats/Inventory/Skills/
      Spells/Save/Load/Help) this port hasn't built at all, a bigger,
      separate lift already in the same bucket as `openInventory` (M41's
      own gap) -- unlike Camp/Attack/spell-cast's own pragmatic single-key
      stand-ins (M39/M42/M46), Save's real trigger is a menu-item click
      with no already-confirmed real KEY CODE to fall back to, so
      inventing one here would be a port-only addition, not a behavioral
      reimplementation of anything real. "Continue Game" is therefore
      real and tested but still practically unreachable in a fresh run
      today (nothing yet writes `savegame.dat`) -- the same
      "data/wiring ready, one more piece still missing" shape M49 itself
      left behind, narrowed to exactly the Save-trigger UI.

      Verified with a new `game_save_smoke.exe`: `Exists`/empty-path
      no-ops; a real `PlayerState` + `WorldRegistry` (a spawned monster
      and a chest) round-tripping through an actual temp file exactly;
      `Load` failing cleanly (not throwing) on a missing file; and
      `MenuFlow::Confirm`'s three-way "Continue Game" behavior (no
      savePath argument, a savePath with no file, and a savePath with a
      real saved file) all landing on the right screen. 46 smoke tests
      now pass in total; full clean rebuild stayed at zero `/W4`
      warnings. Manually launched the real windowed exe and confirmed it
      starts and stays up.

- [x] **M51 -- the Warden finally arrives** (this session). Pure wiring,
      no new files: `main.cpp`'s `shouldRunTick` block now calls
      `WardenState::ShouldVisit(player.giftPointsFound)`/`::Arrive(hub)`
      (both already ported at M8, matching `run()`'s own `if
      (Shop.shouldWardenVisit(this.player.giftPointsFound))
      Shop.wardenArrives();`) -- the one missing driver for an
      already-fully-built-but-inert pipeline. `player/visible_objects.h`'s
      `Refresh` (M27) already reads `warden.present` directly (its
      `includeWarden` parameter is a confirmed dead no-op) and
      `render/visible_object_renderer.h`'s `RenderWardenCompassIcon`
      (M28) already exists, but `warden.present` could never actually
      become true before now -- so neither the compass icon nor the
      Warden's own visible-object slot could ever render, and the
      ALREADY-WIRED `warden.Leave` call (M43) could never fire either,
      since nothing had ever set `present` true to begin with. All three
      go fully live with this one addition. Same "wire an
      already-Java-transcribed/already-ported mechanic for real"
      pattern M42/M47/M48 already established.

      **A finding, turned up while scoping this milestone, that changed
      its shape -- later CORRECTED at M53, see that entry:** the original
      plan was to also build a minimal `ShopState` (`questRewardClaimable
      []`) to finally supply `dungeon_runtime.h`'s own `HubMinimapMarkers`
      (M32, always passed as `nullptr` today, so the hub-town minimap
      never shows NPC markers) -- but `Shop.questRewardClaimable`'s ONLY
      assignment site in the entire codebase is inside `Shop.reset()`,
      which this session's own grep for textual `Shop.reset()`/`reset()`
      call sites found ZERO of anywhere in `../src/`. **That grep missed
      `Shop.java`'s own trailing `static { reset(); }` initializer block
      -- a real call site the JVM runs automatically the first time the
      `Shop` class loads, which happens for real early in the real game's
      own startup. `Shop.reset()` DOES run in the real game; this array
      is NOT permanently null.** (M53's own entry documents the
      correction and how it was found.) The scope decision this session
      actually made -- not building a live `ShopState` this milestone,
      leaving `hubMarkers` at `nullptr` -- turned out to be the right
      call anyway, just for the ordinary "not built yet" reason, not the
      dead-code reason originally written here. `WardenState`'s own
      `wardenVisitCount`/`wardenPresent` were never affected either way --
      both are primitives with real inline default values at their Java
      field declarations, not dependent on `reset()` at all.

      Verified against the existing suite (no new isolable logic to add
      a smoke test for, same as M41's own pure-wiring milestone -- the
      2-line change calls only already-tested M8 methods with the
      correct field). 46 smoke tests still pass; full clean rebuild
      stayed at zero `/W4` warnings. Manually launched the real windowed
      exe and confirmed it starts and stays up.

- [x] **M52 -- dungeon zone gating (`GameAdvancement`)** (this session).
      New `world/game_advancement.h`/`.cpp` ports `ESGame.
      getGameAdvancementLevel()`/`checkOpenAndPopulateDungeons()` -- the
      progressive "open this zone's dungeon levels once giftPointsFound
      crosses a threshold" system `world/dungeon_generator.h`'s own
      `GeneratedLevel::populated` header comment had flagged as
      unmodeled since M6/M10 (that field was an always-true placeholder
      the whole time, since nothing had ever flipped it any other way).
      `GameAdvancement::Level(giftPoints)` buckets into the real 0-8 zone
      index; `GameAdvancement::OpenZone(zone, levels)` flips `populated`
      true for exactly that zone's own level list (ESGame's own
      `zoneLevels` table, transcribed verbatim). `DungeonGenerator::
      PopulateLevel` now starts standard levels at `populated=false`
      (matching `Dungeon`'s own real field default); `BuildHubLevel`
      still starts the hub at `true` (its own real constructor does
      this unconditionally). `player/player_movement.h`'s `CommitMove`
      finally gets a real `if (!target.populated) return false;` gate,
      matching `Player.commitMove()` line for line (src/Player.java:
      809-811) -- and calls `GameAdvancement::OpenZone` at its own two
      gift-point-increment branches (both previously stamped "SKIPPED,
      no live ESGame session object exists yet"); `player/
      player_inventory.h`'s `CollectChestItem` gets the same treatment
      (a new `levels` parameter) at its own gift-point branch. `main.cpp`
      opens zone 0 right after a New Game's `draft` becomes the live
      `player` (`ESGame.createNewGame()`'s own `for (i = 0; i <= zone;
      i++) checkOpenAndPopulateDungeons(i);` loop, kept as a loop for
      line-for-line fidelity even though `zone` is always 0 for a fresh
      character).

      **A second real, confirmed dead-code finding, turned up the same
      "grep every call site" way M51's own `Shop.reset()` finding was,**
      that reshaped this milestone's own Load-path behavior: `ESGame.
      openAndPopulateAllUpTo()` (the "open every zone up to the loaded
      player's own advancement" catch-up pair, meant to run right after
      `loadGameState()`) is ONLY ever called from `ESGame.
      enterCurrentZone()` -- which itself has ZERO callers anywhere in
      `../src/*.java`. (There's a confusingly same-prefixed
      `enterCurrentZoneStatic()`, called once from ESGame's own static
      initializer, but it's an unrelated method that only calls
      `buildHubTileTemplate()` -- easy to conflate by name alone, so
      called out explicitly in `game_advancement.h`'s own header
      comment.) That means **in the real shipped game, loading a save
      never re-opens any dungeon zone.** Since even a same-level STEP's
      own `target` in `commitMove()` is `dungeons[currentLevel-1]` (the
      level the player already occupies), a fresh-launch Continue Game
      resumed past zone 0 finds that level's own `populated` bit back at
      its freshly-booted default (false) -- and the player cannot move
      AT ALL, not even in place. A genuine softlock in the original
      engine. Building a load-time zone catch-up into `GameSave::Load`
      anyway would be STRICTLY MORE correct than the shipped game -- the
      same "behavioral gain, not reimplementation" trap M51's own note
      already warns future work away from -- so `GameAdvancement::
      OpenUpTo`/`enterCurrentZone`'s equivalent was deliberately NOT
      built at all, and `GameSave::Load`'s own hand-off in `main.cpp`
      calls nothing from this new class, faithfully reproducing the
      softlock. Documented prominently at 3 levels (`game_advancement.h`'s
      own class comment, `CommitMove`'s own declaration comment, and the
      `main.cpp` call site itself) rather than buried in one place, given
      how easy this is to trip over by accident in a later milestone.

      **Verified safe against every existing smoke test that exercises
      cross-level movement against real `PopulateLevel`/`BuildHubLevel`
      output** (`m10_player_movement_smoke.cpp`'s own `TestAgainstRealData`
      only asserts zero exceptions, never exact success counts -- traced
      directly, not assumed). Two existing test files needed real edits
      because `CommitMove`/`CollectChestItem` now call `GameAdvancement::
      OpenZone` for real whenever a test's own gift item actually crosses
      a threshold, and `OpenZone` touches EVERY level in the opened
      zone's own list, not just the one level a hand-built test cache
      happened to contain: `m17_world_wiring_smoke.cpp` (7 call sites)
      and `m43_chest_interaction_smoke.cpp` (1 of its 3 `CollectChestItem`
      call sites needed a real all-levels lookup; the other 2 never cross
      a threshold at all) both switched their hand-built `LevelLookup`s
      from a plain `cache.at(n)` to a lazy "create a fresh open level on
      first ask" fallback -- exactly the shape `m10`'s own
      `TestAgainstRealData` already established.

      Verified with a new `game_advancement_smoke.exe`: `Level`'s own
      0-8 bucket boundaries against every real threshold; `OpenZone`
      opening exactly one zone's levels and leaving every other zone
      untouched (additive across repeated calls); `OpenZone(9, ...)`
      throwing `std::out_of_range` (there are only 9 zones); `CommitMove`
      correctly blocking both a same-level step and a cross-level step
      into an unpopulated level, then succeeding once `GameAdvancement::
      OpenZone` opens it; and `DungeonGenerator`'s own real
      `PopulateLevel`/`BuildHubLevel` populated defaults against real
      asset geometry. 47 smoke tests now pass in total (46 carried over
      + this milestone's own); full clean rebuild stayed at zero `/W4`
      warnings. Manually launched the real windowed exe and confirmed it
      starts and stays up.

## M53: ShopState -- the Shop-economy data model's own first slice

**Scope.** A live C++ counterpart for `Shop.java`'s per-NPC quest-economy
state (new `world/shop_state.h`, header-only, `stormhold_world`):
`ShopState` (`questRewardClaimable[7]`/`firstVisit[7]`/`questState1[4]`/
`questState2[4]`/`interactionCount[4]`/`rewardsGiven[4]`/
`unconfirmedCooldownH[4]`/`benecaPoints`/`helgaPoints`/
`showSpecialGreeting` -- deliberately excludes `wardenVisitCount`/
`wardenPresent`, which already live on `WardenState`, M8/M51) plus a new
`Shop` class holding the confirmed-real, purely-functional lookup helpers
that don't need a live Player/Item-inventory/dialogue model at all:
`IsQuestShop`/`QuestShopAt`/`QuestFlagsFor` (the last transcribed against
real `itemsin.dat` `questFlags` bytes, already loaded since M4). Same
"primitive first, wiring later" shape `WardenState` itself used (a
primitive since M8, wired for real 43 milestones later by M51).

**A finding this milestone made, then had to CORRECT within the same
session -- left in as a methodology lesson, not scrubbed.** Tracing
`Shop.clearQuestTurnInState()` (the confirmed real gap
`player/player_leveling.h` has flagged unwired since M13/M14) to its own
field writes reconnected it to M51's own `Shop.reset()` finding, and the
first draft of this milestone concluded `Shop.reset()` has zero callers
anywhere (re-running the same grep M51 used), so every one of the seven
array fields above is a permanently-null Java array, and that
`writeMasterLists()`/`readMasterLists()` (Save/Load) and
`Player.consumeLevelExp()`'s own `clearQuestTurnInState()` call (completing
a level-up) all unconditionally throw `NullPointerException` in the real
game. **That grep -- like M51's own before it -- was checking for textual
`Shop.reset()`/`reset()` call sites only, and missed `Shop.java`'s own
trailing `static { reset(); }` initializer block** (`src/Shop.java:672-
674`), a real call site the JVM guarantees runs automatically the first
time the `Shop` class loads, which happens for real early in the game's
own startup (`ESGame.runAppload()` calls `Shop.loadDialogue()`, a static
method reference that alone forces class init, at the latest).
**`Shop.reset()` DOES run in the real game; none of the "always crashes"
claims above are true.** (M52's own `enterCurrentZoneStatic()` caveat, in
`world/game_advancement.h`, already knew to check for exactly this kind
of implicit class-load call site when it shipped earlier this same
session -- this milestone's own first draft simply didn't apply that same
check to `Shop.reset()`, and neither had M51 before it.) `world/
shop_state.h`, `player/player_leveling.h`, `dungeon/world_save.h`, and
this file's own M51 entry above were all corrected in place once this
was caught, rather than left standing. One real, smaller fact survives
the correction: a static initializer fires at most once per class-load
(once per app launch), so quest-economy state does NOT reset between a
death-restart and the next character within the same running app
instance -- noted in `shop_state.h`'s own comment, not modeled (this
port has no live app-lifetime `Shop` instance yet to carry that state
between restarts either way).

With the finding corrected, `ShopState`'s own fields now default-
construct to the REAL state `Shop.reset()` produces (`questRewardClaimable`
/`firstVisit` true for all 7, everything else 0/false) -- a genuine
faithful default, not a placeholder. `Shop.clearQuestTurnInState()` and
`writeMasterLists()`/`readMasterLists()`'s own missing format half stay
unwired for now, but as an ordinary, not-yet-done port completeness gap
(this struct now exists to wire them TO), not a bug-preservation
question -- left for a later milestone.

**Verification.** New `shop_state_smoke.exe`: `IsQuestShop` against all 7
shops' real categories; `QuestShopAt` against a fresh, default-constructed
`ShopState` (finds every real shop position immediately, including Varus
at shop 6, confirming the lookup itself isn't quest-shop-gated, only
flag-gated; misses a non-shop position; misses once a flag is explicitly
cleared); `QuestFlagsFor` against every real item in `itemsin.dat` (each
shop 0-3's own extracted 2-bit field is in range, and OR-ing all four
back together reconstructs the item's own raw `questFlags` byte exactly,
confirming the extraction shape without needing to guess a specific
expected value; shop 4+ falls through to 0, matching the original's own
final `else` branch). 50 smoke tests now pass in total (49 carried over +
this milestone's own); full clean rebuild stayed at zero `/W4` warnings.
Manually launched the real windowed exe and confirmed it starts and
stays up.

## M54: `Shop.clearQuestTurnInState()` wired into `ConsumeLevelExp`

**Scope.** New `Shop::ClearQuestTurnInState(ShopState&)` (`world/
shop_state.h`): clears `questState1[i]`/`questState2[i]` for shops 0-3,
the real per-level-up quest-turn-in reset. `PlayerLeveling::
ConsumeLevelExp`/`ApplyLevelUpAttributeChoices` (`player/player_leveling.h`
/`.cpp`) both now take a `ShopState&` and call it, matching
`Player.consumeLevelExp()`'s own real statement order exactly (clear
first, then spend the 10 level-exp). Closes the gap M13/M14's own "what's
next" notes first flagged and M53 built `ShopState` specifically to be
able to close. Currently inert in the live port either way -- neither
`ConsumeLevelExp` nor `ApplyLevelUpAttributeChoices` has a call site in
`main.cpp`'s own tick loop yet (the real level-up-confirm flow needs a
3-step attribute-choice UI screen this port doesn't have) -- same "wire
the primitive correctly, live UI wiring is a later milestone" shape M53
itself used for `ShopState` as a whole.

**Verification.** `m15_player_leveling_smoke.cpp`'s existing
`TestConsumeLevelExpAndPendingNames`/`TestApplyLevelUpAttributeChoices`
both updated: a `ShopState` with deliberately dirtied `questState1`/
`questState2` is passed through, and both tests now assert those fields
land back at all-zero after the call, confirming the wiring actually
fires (not just "still zero from a fresh `ShopState`"). 50 smoke tests
pass; full clean rebuild stayed at zero `/W4` warnings. Manually launched
the real windowed exe and confirmed it starts and stays up.

## M55: `ShopState`/`WardenState` join the save format

**Scope.** `writeMasterLists()`/`readMasterLists()`'s own missing half of
the save format (M49's own "what's next" note) is now covered, as far as
this port currently models it: new `Shop::WriteTo`/`ReadFrom`
(`world/shop_state.h`) and `WardenState::WriteTo`/`ReadFrom`
(`world/warden.h`) serialize every field either struct owns, via a new
`BinaryReader::ReadBool()` (mirroring the existing `BinaryWriter::
WriteBool`). `GameSave::Save`/`Load` (`player/game_save.h`/`.cpp`) both
now take a `ShopState&`/`WardenState&` and write/read them right after
the world blob. Wiring this into the file needed one real format change:
the world blob now gets its own 4-byte length prefix (previously read to
EOF, which only worked because it was always the last thing in the
file) -- documented in `game_save.h`'s own header comment as an
unremarkable dev-only format bump, not a compatibility concern. `main.cpp`
gained a new session-local `ShopState shop;` alongside its existing
`WardenState warden;`, both now populated for real by `GameSave::Load`'s
own call site instead of staying at their default-constructed state
forever.

**Deliberately still NOT covered:** `Item.nextSpawnId`/
`Monster.nextSpawnIdCounter`, the other half of the original's own
`writeMasterLists()`/`readMasterLists()` record -- this port has never
modeled either as the single persistent global counter the original
uses (every spawn site already uses its own local counter instead, a
confirmed divergence made peace with since M6/M9); unifying them into
one save-able counter is a bigger, separate lift. This record's own byte
LAYOUT is this port's own (shop fields in `ShopState`'s own declaration
order, then warden fields), not the original's interleaved one -- already
a "behavioral reimplementation, not byte-exact" divergence the moment the
two spawn-id counters are skipped, so reproducing the original's exact
byte positions here would be misleading, not faithful.

**Verification.** `m53_shop_state_smoke.cpp` and `m8_warden_smoke.cpp`
each gained a focused `WriteTo`/`ReadFrom` round-trip test (a
deliberately non-default `ShopState`/`WardenState` through the exact
`BinaryWriter`/`BinaryReader` pair `GameSave` composes). `m50_game_save_
smoke.cpp`'s own `TestRoundTrip` extended with a real, non-default
`ShopState`/`WardenState` alongside its existing player/world data,
asserting every field of both round-trips through a full `GameSave::
Save`/`Load` cycle; its other 3 tests' call sites updated for the new
signature. 48 smoke tests pass; full clean rebuild stayed at zero `/W4`
warnings. Manually launched the real windowed exe and confirmed it
starts and stays up.

- [x] **M56 -- `ShopInteraction`, shops 0-3's quest-turn-in dialogue
      dispatch** (this session). New `player/shop_interaction.h`/`.cpp`
      (`stormhold_player`, not `world/shop_state.h`'s own `Shop` class in
      `stormhold_world` -- this needs `PlayerState` directly, and
      `stormhold_world` cannot depend on `stormhold_player`, the
      dependency runs the other way): `ShopInteraction::QuestShopDialogue`
      ports `Shop.dialogue()`'s shopId-0-3 case (actions 1-6: greet,
      first/second quest-ask, quest-item/cooldown-item turn-in, rumor
      request, reward claim) verbatim, plus `IsValidShopAction`/
      `ShopActionCode` (confirmed shops-0-3-ONLY by reading `Shop.java`
      directly -- the original's own switch has no case at all for shopId
      4-6) and `RumorFor`. First real slice of the dispatcher the roadmap
      flagged after M55 as "worth its own multi-part treatment" --
      deliberately split the same "one coherent slice at a time" way
      M53/M54/M55 already split `ShopState` itself, rather than attempted
      whole (dawnstar's own equivalent milestone, M45, did theirs in one
      pass, but that port also has buy/sell and 9 NPCs to Stormhold's
      none/7 -- a smaller lift here justifies smaller slices instead).
      Deliberately NOT covered: shops 4 (Beneca)/5 (Helga)/6 (Varus) --
      each a bespoke single-NPC branch sharing no pattern with this one or
      each other (`Shop.java`'s own header comment already says so) --
      left for later milestones.

      **A real, confirmed original quirk, ported exactly rather than given
      separate storage:** `Shop.rumorFor(player, step)`'s per-step
      ask-count (`player.skills[step][0]`) is the SAME storage cell that
      skill's own real combat rank lives in (`PlayerCombatStats::
      SkillValue`/`GainSkillExp`, M13/M15) -- not a dedicated field. Also
      confirmed directly from `Shop.java`'s own category-15 branch: the
      "reduce the greeting cooldown" item's magnitude reuses
      `itemsin.dat`'s `questFlags` column as a RAW (non-bit-extracted)
      value, the same "column reuse for an unrelated purpose" shape M13's
      own `weaponDamage()`/`armorValue()` finding and M53's own
      `QuestFlagsFor` both already established for that column.

      Verified by a new `shop_interaction_smoke.exe` against real
      `itemsin.dat`/`npcstrings.dat`/`charin.dat` data and a real created
      character (M9): `IsValidShopAction`/`ShopActionCode`'s shops-0-3-only
      matched pair; `RumorFor`'s first-ask vs. repeat-ask template
      substitution and its shared skill-rank storage; the greet branch's
      first-visit/cooldown/fatigued/random-line cases (the random line
      cross-checked against a twin `JavaRandom` predicting
      `lingoRandomInt(3)`'s own pick); both quest-ask actions' full
      outcome-dependent state-transition matrix across 10+ seeds each,
      cross-checked against a twin call into the already-trusted
      `PlayerCombatStats::RollShopOutcome` (not re-deriving the roll
      formula, M13 already did that) to predict the exact outcome before
      asserting on it; the category-15 cooldown-reducer and category-11
      quest-item turn-ins, both found by scanning real `itemsin.dat` rather
      than hardcoded ids (same precedent M43's own item-scan approach set);
      the reward-gated rumor request; the reward-claim action's
      `questRewardClaimable` clear and hub tile bit-32 clear; and the
      no-matching-action `std::nullopt` fallthrough. 49 smoke tests pass;
      full clean rebuild stayed at zero `/W4` warnings. Manually launched
      the real windowed exe and confirmed it starts and stays up.

- [x] **M57 -- `ShopInteraction::BenecaDialogue`, shop 4** (this session).
      Second slice of the `Shop.dialogue()` dispatcher (see M56's own
      entry for the "one NPC-group at a time" plan): greet (first-visit
      line, then `std::nullopt` on every repeat -- Beneca has no
      cooldown/fatigue/random-line branches at all, unlike shops 0-3's own
      greet), the item-donation economy (action 4: an eligible item
      -- category not 13/15/17 -- earns one `benecaPoints` and is
      consumed; an ineligible one is refused and kept), and the
      training-reward redemption (action 7: 3 points buys back a
      caller-chosen item, gated on `benecaPoints / 3 > 0`).

      **A real naming trap in `Shop.java` itself, caught by reading
      `Player.addInventoryItemRaw(int itemId, int packedValue, int
      charge)`'s own real signature rather than trusting the local
      variable's name:** action 7's own local is named `slot` (copy-pasted
      from action 4's branch just above it, where `slot` genuinely IS an
      inventory slot index) but is actually passed as
      `addInventoryItemRaw`'s FIRST parameter -- `itemId`, not a slot at
      all. So action 7's own `extra` argument is really the ITEM ID being
      requested as a training reward, matching the plain
      `addInventoryItemRaw(itemId, packedValue, extra)` call shape used
      everywhere else in `Player.java`. `BenecaDialogue`'s own parameter is
      named `itemId`, not `slot`, to not carry the original's misleading
      name forward -- flagged prominently in the method's own header
      comment given how easy this is to misport by pattern-matching
      against the sibling branch instead of checking the real callee.

      `spawnIdCounter` (`Item.nextSpawnId()`'s global counter) is threaded
      through the same `int16_t&`, caller-supplies-it,
      pre-incremented-with `++` way `combat/combat_resolution.h`/
      `dungeon/dungeon_runtime.h` already established -- not reproduced as
      a ported static. Confirmed the training-reward branch burns a spawn
      id even when the pack is full and the item isn't actually granted
      (`Item.nextSpawnId()` runs before `addInventoryItemRaw`'s own
      capacity check), the same "counter advances before the outcome is
      known" shape this project's own M43 `GrantStarFrostItem` finding
      already documented. Also confirmed (independently, by M11's own
      smoke test before this milestone existed) that the training-failure
      message is a real cross-group return: `dialogue[7][0]` (the GENERIC
      pool), not `dialogue[4][...]` -- reproduced exactly as
      `text.groups[7][0]`, not narrowed to Beneca's own group.

      Verified by extending `shop_interaction_smoke.exe` (same executable
      as M56, `src/tests/m56_shop_interaction_smoke.cpp`) against real
      `itemsin.dat` data: the first-visit/repeat-visit greet pair; a real
      donation-eligible item (found by scanning for `category != 13/15/17`)
      earning exactly one point and being removed, and a real
      category-13/15/17 item being refused and kept; the
      insufficient-points training-reward refusal spending nothing; a
      successful training reward spending exactly 3 points, burning
      exactly one spawn id, and actually landing the item in inventory;
      and a full-pack training reward returning the cross-group failure
      line while still burning the spawn id and spending no points. 49
      smoke tests pass (same total as M56 -- this extended the existing
      executable rather than adding a new one); full clean rebuild stayed
      at zero `/W4` warnings. Manually launched the real windowed exe and
      confirmed it starts and stays up.

- [x] **M58 -- `ShopInteraction::HelgaDialogue`, shop 5** (this session).
      Third slice of the `Shop.dialogue()` dispatcher (see M56's own entry
      for the "one NPC-group at a time" plan) -- by far the busiest single
      NPC branch in the original: greet/rumor-reveal (first-visit resets
      `rumorRevealStep` to 0 and returns two concatenated lines; a
      subsequent visit reveals the next rumor fragment once
      `GameAdvancement::Level(giftPointsFound)`, M52, has advanced past
      `rumorRevealStep`, else nothing new), a repeat-rumor query (action
      13), a category-13 "quality" item turn-in (`itemSubtypeAtSlot` > 3
      pays 5 points, else 3), item-charge initialization (action 8), a
      one-shot safe-camping buff (action 9), an ailment cure (action 10),
      a camp-mark warp (action 11), and a full HP/Magicka heal (action
      12) -- 8 distinct actions, no shared pattern with `BenecaDialogue`
      or `QuestShopDialogue` at all, confirmed by reading the whole branch
      directly. `showSpecialGreeting` (`ShopState`, M53 -- "no assignment
      site found in this file, producer lives elsewhere") gets its own
      confirmed CONSUMER here for the first time: it prefixes the greet
      branch's return value with `dialogue[5][21]` exactly once, in all
      three of the greet branch's own sub-cases (first visit, a new rumor
      reveal, and "nothing new to reveal"), then clears -- still no
      confirmed producer anywhere in this port, same "consumer exists,
      producer doesn't yet" situation `GeneratedLevel::visited`'s own M10
      note already carried for a different field.

      **A real double-gate, easy to flatten into a single call by
      mistake, documented rather than silently simplified:** action 8's
      item-charge branch checks `Item.isEquipmentCategory(itemId) &&
      !player.isItemCharged(slot)` BEFORE calling `initializeItemCharge`
      -- `PlayerInventory::InitializeItemCharge` (M12) already re-checks
      the equipment-category half internally, and would happily re-stamp
      an ALREADY-charged item's charge back to the same value (3) if this
      method called it based on its own return value alone, silently
      masking the "already charged" case (`dialogue[5][14]`) behind the
      "success" case (`dialogue[5][13]`). `HelgaDialogue` replicates both
      original conditions explicitly instead of trusting
      `InitializeItemCharge`'s own internal gate to be sufficient.
      `Player.itemSubtypeAtSlot(slot)` (action 4's own "quality tier"
      read) is a one-line `Item.column(2, itemId)` in the original --
      inlined directly as `items.subtype[itemId - 1]` rather than given
      its own wrapper method, matching how `QuestShopDialogue`/
      `BenecaDialogue` already read `items.category`/`items.questFlags`
      directly for equally one-line original reads.

      Verified by extending `shop_interaction_smoke.exe` (same executable
      as M56/M57) against real `itemsin.dat` data and a real created
      character: the plain and special-greeting-prefixed first-visit
      pair; the advancement-reveal and no-new-advancement pairs (each
      with and without a pending special greeting); the repeat-rumor
      query's own no-state-change guarantee; category-13 turn-ins for a
      real high-quality (subtype > 3) and a real low-quality (subtype <=
      3) item, both found by scanning real `itemsin.dat`, plus a real
      non-category-13 item's refusal (kept, not removed); the item-charge
      double-gate (a real equipment item charging successfully, spending
      exactly 7 points and setting the charge flag, THEN being refused on
      a second attempt without spending any more points -- the exact
      scenario the double-gate finding above is about -- and separately a
      real non-equipment item's refusal, found by scanning for
      `!IsEquipmentCategory`); the safe-camping buff's one-shot gate (an
      already-buffed refusal spending nothing, vs. a fresh success
      spending exactly 2 points); the ailment cure clearing `ailmentMask`
      entirely and spending exactly 1 point; the camp-mark warp's
      no-camp-mark refusal; the full-heal action's curHP/curMagicka
      assignment; and the no-matching-action `std::nullopt` fallthrough.
      49 smoke tests pass (same total as M56/M57); full clean rebuild
      stayed at zero `/W4` warnings. Manually launched the real windowed
      exe and confirmed it starts and stays up.

- [x] **M59 -- `ShopInteraction::VarusDialogue`, shop 6** (this session).
      Fourth and FINAL slice of the `Shop.dialogue()` dispatcher (see
      M56's own entry for the "one NPC-group at a time" plan) -- all 7
      NPCs' own dialogue logic is now ported. Unlike every other shop,
      Varus's own branch ignores `action`/`extra` entirely: it's a pure
      state machine over `WardenState::visitCount` (M8/M51) and
      `player.wardenLoreStep`, revealing one lore line per Warden visit
      (visitCount 1/2/3 each reveal exactly one new line, gated so a
      repeat ask at the same visitCount/loreStep pair returns
      `std::nullopt` instead of re-revealing).

      **A real, confirmed dead branch, preserved rather than removed --
      same documentation discipline as M6's dead chest-record byte and
      M10's dead `leftLevelZone`:** the original's own `dialogue()` has a
      4th case for `wardenVisitCount == 4`, but `WardenState::ShouldVisit`
      (`world/warden.h`) only ever checks 3 escalating thresholds (13/26/
      39) -- M8's own smoke test already confirmed `visitCount` caps at 3
      in the real game (`checked with an elapsedCounter of 100000`). The
      `visitCount == 4` branch can therefore never actually fire through
      any real code path; ported anyway rather than dropped, and exercised
      directly in the new smoke test (constructing that `WardenState` by
      hand, since nothing else in this port can ever produce one) to prove
      the port's own behavior matches the original exactly even though
      the original's own game logic can never reach it.

      Verified by extending `shop_interaction_smoke.exe` (same executable
      as M56/M57/M58): `visitCount == 0` always `std::nullopt` regardless
      of `loreStep`; each of visitCount 1/2/3 revealing its own line
      exactly once, with a repeat ask at the same state returning
      `std::nullopt`; an "already caught up" loreStep (ahead of what a
      given visitCount would reveal) also returning `std::nullopt` rather
      than re-revealing; and the confirmed-unreachable `visitCount == 4`
      branch behaving exactly as ported. 49 smoke tests pass (same total
      as M56/M57/M58); full clean rebuild stayed at zero `/W4` warnings.
      Manually launched the real windowed exe and confirmed it starts and
      stays up.

- [x] **M60 -- the NPC greeting flow, wired live** (this session). Closes
      the two gaps M43's own "what's next" note left open once M56-M59's
      `ShopInteraction`/`Shop` dispatch existed to close them: `GameCanvas.
      resolveInteractInput()`'s NPC-talk half and
      `refreshNpcNameplateAndWardenLeave()`'s NPC-nameplate half. New
      `PlayerMovement::ShopAheadOfPlayer` (`Player.shopAheadOfPlayer()`):
      the SAME `ComputeMoveTarget(1)` look-ahead `MonsterInFront`/
      `ChestAheadOfPlayer` already use, gated on `pendingLevel == 1` (every
      NPC stands on the hub town only) then `Shop::QuestShopAt` (M53/M56).
      New `Shop::kNames[7]` (`Shop.NAMES[]`, never ported before now). New
      `ui/npc_dialogue.h`'s `NpcDialogue` -- a port-only blocking screen
      standing in for `npcHelloUI`: title (the NPC's name) + word-wrapped
      body text (a small, deliberate duplicate of `ui/menu_flow.cpp`'s own
      file-local `WordWrap`, which isn't exported and belongs to a
      genuinely separate pre-game state machine), dismissed on Enter.
      `main.cpp`'s tick loop computes `shopAhead` ONCE per tick (reused for
      BOTH the nameplate popup and the interact-key dispatch, a provably-
      equivalent consolidation of the original's own two separate real
      call sites -- see `ShopAheadOfPlayer`'s own declaration comment), and
      dispatches the interact key ('F', shared with chest-opening, NPC-talk
      taking priority same as the original's own if/else-if order) to
      whichever of M56-M59's four dialogue methods owns the found shopId,
      always with action=1/extra=0 -- exactly what `talkToNpc()` itself
      always calls. While the dialogue screen is open, movement/attack/
      spellcast/monster-AI all skip the tick, the same `shouldRunTick`-style
      gate `Camping`/`DeathSequence` already established.

      **Deliberately, substantially NOT modeled, flagged prominently rather
      than silently simplified:** the real `npcChoicesUI[npcId]` follow-up
      screen -- the INTERACTIVE menu a player reaches after the greeting
      (Train/Give/Befriend/Threaten/Kill for the quest shops, or Beneca/
      Helga's own item-donation/point-spending choices), `ESGame.java`'s
      own screenGroups 9-14/20/22/27/350, each with their own item-
      selection sub-screens and result popups. Reading that dispatch web
      directly (while scoping this milestone) confirmed it's a
      substantially bigger lift than this one -- likely dawnstar's own
      M46 (`NpcMenu` and every sub-screen) in size -- so this milestone
      only wires the GREETING (`talkToNpc()`'s own single hardcoded
      `action=1` call), matching the "one coherent slice at a time"
      discipline M56-M59's own quartet already used for the dialogue TEXT
      side of this same gap. Also not reproduced: `talkToNpc()`'s own
      null-result fallback for npcId 4/5 (re-showing THEIR OWN choices
      screen with a "has nothing more to say" message + point-total
      substitution) -- needs that same deferred choices-screen text; a
      null result here just doesn't open the dialogue screen at all, the
      same practical outcome shopId 0-3/6 already have (no fallback branch
      for those in the original either).

      Verified by a new `npc_dialogue_smoke.exe`: `ShopAheadOfPlayer`
      against a real `BuildHubLevel` output for all 7 real shop positions
      (each resolving to its own real shopId), a cleared
      `questRewardClaimable` flag making the same position resolve to -1,
      a non-hub `currentLevel` always resolving to -1, and an empty tile
      resolving to -1; `Shop::kNames` against the real 7 names; `NpcDialogue
      ::Show`/`Dismiss`'s own state transitions; and an end-to-end
      integration check mirroring `main.cpp`'s own exact by-shopId
      dispatch for all 7 real NPCs against a freshly created character,
      confirming `ShopAheadOfPlayer` finds the right id AND dispatching
      through it produces a real first-visit greeting line that actually
      opens `NpcDialogue` with that NPC's own real name as the title. 50
      smoke tests pass; full clean rebuild stayed at zero `/W4` warnings.
      Manually launched the real windowed exe and confirmed it starts and
      stays up. **Not independently re-verified this session:** walking up
      to a real NPC in a live play session and visually confirming the
      dialogue screen renders/word-wraps/dismisses correctly -- that would
      need navigating the Main Menu/character-creation flow and the hub
      town's own corridor layout via live keyboard input, which wasn't
      attempted; every state-mutating code path IS exercised against real
      asset data by the smoke test above, just not the live rendering
      itself.

- [x] **M61 -- `PlayerInventory`'s inventory-ACTION logic (`itemTooltip`/
      `canEquipOrUnequip`/`isScrollCategory`/`canLearnSpellFromScroll`/
      `learnSpellFromScroll`/`canUseItem`/`useItem`)** (this session). The
      last un-ported pieces of `Player.java`'s general inventory logic --
      everything `ESGame.java`'s own `newInventoryItemUI()` (the real
      Drop/Equip-or-Unequip/Learn/Use per-slot action menu) reads before
      showing each option, plus the two actions ("Learn"/"Use") themselves.
      Deliberately NOT the inventory screens themselves (`newInventoryUI`/
      `newInventoryItemUI`, or `openInventory`'s own dispatch out of
      `tickPlayerAction()`) -- that's its own separate, bigger UI-wiring
      lift (`docs/PORT_ROADMAP.md`'s own "What's next" already flagged
      `openInventory` as M41's last unwired branch), same "logic first,
      wiring later" split M56 (`ShopInteraction`)/M60 (its live wiring)
      already used. All the underlying primitives this logic needed
      (`AddInventoryItemRaw`/`RemoveInventorySlot`/`EquipItem`/
      `UnequipSlot`/`IsSlotEquipped`/`MarkCampAndReturnToTown`/
      `WarpToCampMark`/`HasCampMark`, `PlayerCombatStats::
      CureRandomAilment`/`SkillValue`, `DungeonRuntime::StoreMonster`,
      `MonsterRuntime::Stat`) already existed from M9/M12/M17/M36/M46/M47 --
      this milestone only needed to add the 7 new methods themselves.

      **Two real, confirmed findings, both preserved rather than
      "cleaned up":**
      - `canUseItem(slot)` and `isScrollCategory(slot)` are BYTE-FOR-BYTE
        the exact same category-13-or-15 check under two different names
        -- confirmed by reading both methods side by side, not a
        transcription slip on this port's side. Verified in the smoke test
        by sweeping every real item in `itemsin.dat` and confirming the two
        ported predicates never disagree.
      - `useItem()`'s case 91 (no confirmed item name -- its own
        `specialEffectText` flavor-text entry, giftIdx 4, is blank, the
        ONLY blank entry in the whole 87-99 table) adds `3 *
        coreStats[5]` (max MAGICKA, the same stat case 90 restores current
        Magicka to) to `coreStats[6]` (current FATIGUE), uncapped -- NOT
        `3 * coreStats[7]` (max Fatigue), despite sitting immediately after
        the Magicka-restore case in the switch, which would suggest a
        Fatigue-restore item. Confirmed by reading `useItem()`'s own case
        91 line directly; the blank flavor text raises the possibility
        this is unused/debug content in the original rather than a live
        player-facing item, but it's ported exactly as found either way.
      - Also confirmed (not a finding needing preservation, just worth
        recording): `ESGame.java` has exactly ONE call site for
        `useItem(slot, target)`, and it always passes `target = null`.
        There is no confirmed path anywhere in `../../../src/` that ever
        gives `useItem()` a real monster to target, making the id 97/98/99
        instant-kill scroll branches dead code in the real game's own
        confirmed call graph, not just an unwired gap in this port. Ported
        faithfully anyway (`UseItem` takes a nullable `MonsterState*`, and
        the real `MonsterDatabase&`/`WorldRegistry&` machinery those three
        branches need), on the chance a caller this pass missed exists, or
        a future milestone (the still-unwired `npcChoicesUI`/inventory
        screens) adds one.

      `CanEquipOrUnequip` also confirmed as a DISTINCT predicate from
      `ItemDatabase::IsEquipmentCategory` (equipment categories 1-10 only)
      -- the real `canEquipOrUnequip()` additionally allows category 17
      (the same category `ItemTooltip`'s own case 17 branch, "Weapon
      value: 20+bonus", already handles), confirmed by reading
      `canEquipOrUnequip()`'s own switch directly rather than assumed to
      match the existing predicate's range. `ItemTooltip` reuses the same
      real "questFlags column doubles as weapon/armor magnitude" quirk
      `player_combat_stats.cpp`'s `WeaponDamage`/`ArmorValue` already
      documented (M13) -- not a new discovery here, just the same
      `items.questFlags[id-1]` read applied to a second real call site.
      `Item.specialEffectText[13][2]`'s fixed flavor text has no loaded
      asset-table equivalent (it's hardcoded in the original source, never
      read from `itemsin.dat` -- confirmed by `docs/ASSET_FORMATS.md`'s own
      note on item ids 87-99) -- kept as a literal `kSpecialEffectText`
      table in `player_inventory.cpp`, matching the original's own
      hardcoded static field exactly.

      Verified by a new `inventory_actions_smoke.exe`: the confirmed
      `CanUseItem`/`IsScrollCategory` duplicate swept across every real
      item in `itemsin.dat`; `CanEquipOrUnequip`'s 1-10-or-17 range swept
      the same way; `CanLearnSpellFromScroll`'s skill-rank-prerequisite
      gate (both refused-at-zero-rank and allowed-at-nonzero-rank) plus
      `LearnSpellFromScroll`'s `knownSpellsMask` bit-set and slot-consume,
      against a real category-12 item; every one of the 13 real ids 87-99
      individually (camp mark/warp branching on `HasCampMark`, the random-
      ailment cure, HP/Magicka/both restoratives, the confirmed id-91
      quirk, the level-exp increment, the harm/armor/safe-camping buffs
      including safe-camping's own non-consuming exception, and the three
      instant-kill scrolls' real `target=nullptr` shape all just consuming
      their slot and doing nothing else); the id-97/98/99 instant-kill
      gate exercised separately against a REAL `MonsterState`/
      `MonsterDatabase` (both a monster type that passes the gate, killed
      and stored into the registry, and one that fails it, left completely
      untouched) -- a logic check on the ported switch itself, not a claim
      that this path is reachable in the real game (see the dead-call-site
      finding above); and `ItemTooltip` against a real starting weapon, a
      real starting armor piece, a real category-12 spell scroll, and real
      item id 87's own confirmed `specialEffectText[0]` line. 51 smoke
      tests pass; full clean rebuild stayed at zero `/W4` warnings.
      Manually launched the real windowed exe and confirmed it starts and
      stays up (this milestone changes no live-gameplay-reachable code
      path at all -- nothing calls any of these 7 new methods yet -- so
      this launch check is a basic regression guard, not a feature check).

- [x] **M62 -- the inventory screens, wired live (`newInventoryUI`/
      `newInventoryItemUI`/`openInventory`)** (this session). Closes the
      LAST branch of M41's own dispatch web: new `ui/inventory_ui.h`/`.cpp`
      (`InventoryUiState`/`InventoryUi`, `stormhold_render`) is a two-
      screen state machine standing in for `ESGame.java`'s `newInventoryUI()`
      (screenGroup 5/33, the item list) and `newInventoryItemUI()`
      (screenGroup 5/34, the per-slot Drop/Equip-or-Unequip/Learn/Use
      action menu) plus `GameCanvas.openInventory()`'s own hotkey dispatch,
      the same "port-only screen state machine" shape `ui/npc_dialogue.h`
      (M60) already used, built entirely on M61's already-ported
      `PlayerInventory` action logic. Wired into `main.cpp`'s live tick
      loop on a new 'I' hotkey stand-in (same pragmatic class as 'C'/'F'
      from M39/M41 -- this port still has no real hotbarActionSet-driven
      numeral-key system, an already-documented M29/M31 gap); Up/Down move
      the cursor, Enter is "Ok", Escape is "Back", all edge-triggered and
      checked outside the `shouldRunTick` gate the same way M60's own NPC-
      dialogue dismiss key already is, since the whole point is driving
      the screen that gate blocks live gameplay behind. `shouldRunTick`
      itself now also excludes `inventoryUi.active`, matching
      `npcHelloUI`/`inventoryUI` both fully taking over
      `Display.setCurrent()` in the original rather than overlaying the
      corridor view.

      One new `PlayerInventory` method needed discovering along the way:
      **`IsEquippedSlot` (`Player.isEquippedSlot`) -- a real, confirmed,
      easy-to-conflate DISTINCT method from the already-ported
      `IsSlotEquipped` (M47, `Player.isSlotEquipped`).** Both look almost
      identical and both return "is this slot's item currently equipped",
      but they gate on different columns: `isSlotEquipped` (M47) checks
      `ItemDatabase::IsEquippable` (the equipSlot column), while
      `isEquippedSlot` (M62, needed by `newInventoryItemUI`'s own Equip/
      Unequip label picker) checks `ItemDatabase::IsEquipmentCategory`
      (categories 1-10). Confirmed by reading `ESGame.java`'s own call site
      directly (`this.player.isEquippedSlot(slot)`) rather than assuming
      the already-ported method was the right one -- an easy mistake to
      make given the near-identical names. Since `EquipItem` itself also
      gates on `IsEquipmentCategory` (1-10 only, not 17), a category-17
      item can never actually become negative in the first place, so
      `IsEquippedSlot` always returns false for one -- a **second real,
      confirmed dead/misleading menu entry**: `newInventoryItemUI` would
      show "Equip" (never "Unequip") for a category-17 item regardless of
      its true state, AND pressing it is a complete no-op, even though
      `CanEquipOrUnequip` (M61) says the option should be offered at all.
      Not reachable with real `itemsin.dat` data either way -- M61's own
      `TestCanEquipOrUnequip` already found no real category-17 item
      exists in the extracted assets. `UnequipSlot` was refactored to call
      `IsEquippedSlot` internally instead of re-inlining the same check,
      matching `Player.unequipSlot()`'s own real call structure exactly
      (a small correctness improvement, not a behavior change).

      **The big finding this milestone turned up, and a deliberate,
      clearly-labeled port-only exception -- this project's first (see
      the "what's next" section's own prior note that no such precedent
      existed yet):** the real `GameCanvas.openInventory()` does NOT call
      `newInventoryUI()` at all. It only does
      `this.game.showScreen(this.game.inventoryUI)` -- showing whatever's
      CACHED in that field. Exhaustive grep of every `inventoryUI =`
      assignment in `ESGame.java` found exactly 5: the in-game pause
      screen's own "Inventory" menu item (screenGroup 31 case 1, itself
      not built anywhere in this port -- see this section's own prior
      "what's next" note) and the 4 action-menu branches that refresh it
      after Drop/Equip/Learn/Use. `openInventory()`'s hotkey path is NOT
      one of the 5. So in the real game, pressing the inventory hotkey
      BEFORE ever opening the pause screen's Inventory tab at least once
      that session leaves `game.inventoryUI == null`, and
      `ESGame.showScreen(null)` falls through every one of its own
      `instanceof` branches -- pausing the game thread and disabling
      auto-repaint with no screen shown and no path back. A real softlock,
      reachable from a completely ordinary fresh New Game, on par with
      M52's own confirmed Continue-Game-past-zone-0 softlock. Reproducing
      it faithfully in this port would require first building the whole
      not-yet-existing pause/stats menu AND its own "populates a cache the
      hotkey path skips" quirk on top of it -- out of scope for this
      milestone, the same "deliberately not reproduced, needs machinery
      this port doesn't have" call `ui/npc_dialogue.h`'s own class comment
      already made for its un-built `npcChoicesUI` fallback. Instead,
      `InventoryUi::Open` always builds the list fresh -- which is also
      what 4 of the real game's own 5 `inventoryUI =` call sites already
      do (only the buggy hotkey entry point skips it), so this is the
      DOMINANT real behavior being reproduced, not an invented one. Fully
      documented in `ui/inventory_ui.h`'s own class comment for whoever
      eventually builds the pause menu and has to decide whether to revisit
      this call, the same "conscious decision point, not a silent default"
      framing this section's own M52 heads-up already uses.

      Also confirmed (not a finding needing preservation, just worth
      recording): `Player.endOfGameTriggered` -- already flagged as
      declared-but-never-set by the original transcription pass itself
      (`Player.java`'s own comment on the field) and by M61's own `UseItem`
      doc comment -- gates a real branch in `ESGame.java`'s screenGroup 34
      "Use" handler (close the whole screen back to live gameplay instead
      of refreshing the list). Since the field is confirmed dead,
      `InventoryUi::Confirm` always takes the other (live, reachable)
      branch: rebuild the list and stay on it. Not a fork this port needs
      to model, just documented here for anyone auditing the dispatch.

      Verified by a new `inventory_ui_smoke.exe`: `Open`/`Cancel`'s own
      state transitions (including List-screen Cancel closing the whole
      view, the deliberate port-only mapping for the real game's
      not-yet-built `optionsUI` fallback); `MoveSelection`'s clamp-not-wrap
      behavior at both ends and its no-op on an inactive/empty state;
      selecting a slot off the List screen building an action menu whose
      Drop/Equip-or-Unequip/Learn/Use membership is cross-checked directly
      against `CanEquipOrUnequip`/`CanLearnSpellFromScroll`/`CanUseItem`
      for a real starting item, plus an out-of-range slot selection being a
      no-op; Drop actually consuming the slot and registering a real
      dropped-item record via `DungeonRuntime::AddDroppedItem`; a full
      Equip-then-Unequip round trip on a real starting item, checking both
      the resulting `IsEquippedSlot` state AND that the label flips from
      "Equip" to "Unequip" in between; Learn against a real category-12
      item (skill-rank gate, `knownSpellsMask` bit-set, slot-consume); Use
      against real item id 92 (confirming `Confirm`'s dispatch actually
      reaches `UseItem` with `target=nullptr`, not re-deriving each of the
      87-99 cases' own effects -- M61's own test already covers those in
      isolation); Cancel from the ItemAction screen returning to the List
      screen without closing the whole view; and both screens' own
      `Render` at least running end to end without crashing against real
      asset data. 52 smoke tests pass; full clean rebuild stayed at zero
      `/W4` warnings. Manually launched the real windowed exe and confirmed
      it starts and stays up. **Not independently re-verified this
      session** (same disclosed gap M60's own entry already has): actually
      pressing 'I' in a live play session and walking through Drop/Equip/
      Learn/Use via real keyboard input -- that would need navigating the
      Main Menu/character-creation flow first, which wasn't attempted;
      every state-mutating code path IS exercised against real asset data
      by the smoke test above, just not the live rendering/input itself.

- [x] **M63 -- the in-game pause/Options menu, wired live (`optionsUI`:
      Stats/Inventory/Skills/Spells/Save Game/Load Game), plus a real Save
      trigger** (this session). New `ui/pause_menu.h`/`.cpp`
      (`PauseMenuState`/`PauseMenu`, `stormhold_render`) is a multi-screen
      state machine covering `ESGame.java`'s `optionsUI` (screenGroup 31,
      the 8-item list) and everything it opens onto: `newStatsUI`
      (screenGroup 32, via a new `PlayerCombatStats::CharacterSheetText`),
      `newSkillsListUI`/`newSkillInfoUI` (screenGroups 35/36, via two new
      `PlayerLeveling` methods, `SkillSummaryList`/`NthLearnedSkillIndex`/
      `SkillTooltip`), and `newSpellsListUI`/`newSpellInfoUI` (screenGroups
      37/38, via three new `SpellCasting` methods --
      `KnownSpellsSummary`/`SpellTooltip`, plus `NthKnownSpellId` itself
      PROMOTED from a file-local helper inside `combat/spell_casting.cpp`
      to a public static method, since `ui/pause_menu.cpp` needed it too
      and it already carries real game-rule content worth a single source
      of truth, unlike the trivial palette/WordWrap helpers this port's UI
      files otherwise duplicate on purpose). "Inventory" (item 1) hands
      off directly to the already-live M62 `InventoryUi` rather than
      duplicating it. "Save Game"/"Load Game" (items 4/5) finally give
      M50's `GameSave` its first live call site: both run synchronously,
      no progress screen, the same simplification `player/game_save.h`'s
      own class comment already committed to; a successful Load reports a
      new `PauseMenuAction::GameLoaded` so `main.cpp` can run the exact
      same tile-flag-refresh + `RefreshCorridorView` fixup the "Continue
      Game" main-menu path already runs, since `PauseMenu` itself has no
      access to `levels`/`levelLookup` (same "caller supplies/owns state"
      pattern as everywhere else in this port). Wired on a new 'P' hotkey
      stand-in (same pragmatic class as 'C'/'F'/'I'); Up/Down/Enter/Escape,
      all edge-triggered outside the `shouldRunTick` gate, exactly matching
      M62's own inventory-key wiring shape; `shouldRunTick` now also
      excludes `pauseMenu.active`.

      **The big finding this milestone turned up, bigger than any single
      screen's own content, and a second deliberate, clearly-labeled
      port-only exception in the same category M62's own `openInventory`
      softlock call established:** the real in-game pause menu is not
      merely under-wired -- it is **completely unreachable** in the
      original game. `ESGame` declares `cmdMenu` (a `Command("Menu", ...)`)
      clearly meant to open it, but grep confirms that `Command` is never
      once passed to `addCommand` anywhere in `ESGame.java` OR
      `GameCanvas.java` (`GameCanvas` never calls `addCommand` at all), and
      `keyPressed()`'s own numeral-hotkey dispatch has no branch for it
      either. The ONLY confirmed code path that ever calls
      `showScreen(optionsUI)` is the Stats screen's OWN "Ok" button
      (screenGroup 32) -- which requires already being on Stats, which is
      only ever reached FROM the Options menu. A closed loop with no real
      entry point: Stats/Skills/Spells/Save Game/Load Game are all
      confirmed unreachable during real gameplay in the shipped original,
      almost certainly a missing one-line `gameCanvas.addCommand(cmdMenu)`
      call, not a deliberately cut feature (the screens are all fully
      built and functional, and `GameSave` has existed since M50 with
      nothing calling it). Rather than reproduce total unreachability --
      which would make this entire milestone permanently dead code,
      including the only real Save trigger this port will ever have -- the
      new 'P' hotkey opens it directly, following the same "faithful
      reproduction would require machinery this port doesn't have" logic
      M62's own exception used, even though this is a different flavor of
      it: M62 declined to reproduce a real, harmful bug; here there is no
      real behavior to reproduce at all, so making it reachable is closer
      to restoring a one-line-missing wire-up than overriding a confirmed
      design -- still flagged explicitly either way, per this project's own
      "conscious decision point, not a silent default" discipline.

      **A second, real, CONFIRMED bug, preserved exactly:** `optionsItems`
      (the menu's own 8-entry label array, ending in "Quit Game" at index
      7) is one short of screenGroup 31's own `switch (choice)`, which
      has a 9th case (`case 8`, a leftover debug-form opener) --  meaning
      "Quit Game" itself actually runs `case 7`'s body, which shows
      `newCreditsUI(...)`, the SAME developer-credits screen the main
      menu's own "Credits" item uses, built from `ESGame.creditsText()` (a
      fuller dev-team/legal string than `ui/menu_flow.cpp`'s own
      `kCreditsText`, which comes from a different real source,
      `UIScreen.creditsLines`). Selecting "Quit Game" from the pause menu
      **never quits the game** -- it shows the credits screen instead
      (`nextScreen = optionsUI`, matching `newCreditsUI`'s `backTarget`
      parameter always being whatever screen was active when it's called),
      and the real "quit"/debug case (index 8) can never be reached at all
      with only 8 real menu items. `PauseMenu` reproduces this exactly --
      "Quit Game" shows `PauseScreen::Credits` (with `ESGame.creditsText()`'s
      own real text, including its own "Vir2L Studos" typo, preserved
      verbatim, not corrected) -- rather than "fixed" to actually quit.

      **Deliberately NOT wired this milestone, a clean, explicitly-flagged
      boundary (same shape as M60's own un-built `npcChoicesUI` follow-up
      menu):** "Help" (item 6) is a confirmed no-op. `helpTopicTitles`/
      `helpTopicBodies` hold 12 topic slots each, but the Java
      TRANSCRIPTION itself only fills in bodies for topics 0-4 -- a real
      gap in this project's own decompilation work (already flagged in the
      prior "what's next" section), not something this port could resolve
      either way. Also mapped to a port-only fallback, same "machinery
      doesn't exist yet" grounds: the real `noSavedGameUI` (Load Game, no
      save file) targets `mainMenuUI` and `saveErrorUI` (a save I/O
      failure) sets no `nextScreen` at all (a likely real, never-confirmed
      softlock) -- this port has no "return from live gameplay to the main
      menu" transition at all (`ui/menu_flow.h`'s state machine only runs
      BEFORE `gameStarted`), so both port-only map back to Options instead.

      Verified by a new `pause_menu_smoke.exe`: `Open`/`Cancel` from
      Options (Cancel closes the whole menu back to gameplay, matching the
      real screenGroup 31 `cmdBack` branch); `MoveSelection` clamping on
      the 8-item Options list and no-op on inactive/message-only screens;
      Stats round-tripping through `CharacterSheetText` and back; "Inventory"
      correctly closing `PauseMenu` and opening `InventoryUi`; Skills ->
      SkillInfo resolving the same index `NthLearnedSkillIndex` gives, with
      a real non-empty tooltip, and Cancel/Confirm both returning the right
      way; Spells -> SpellInfo -> "Ready Spell" actually setting
      `selectedSpellId` (checked via the "R: " prefix appearing in
      `KnownSpellsSummary` afterward) with the right 0-based/1-based
      conversion; Help staying a no-op on Options; "Quit Game" showing
      Credits and both Confirm/Cancel returning to Options from there; a
      full Save-then-Load round trip against a real temp file (`GameSave::
      Exists`/`Save`/`Load` all exercised for real, including the
      NoSavedGame path when no file exists yet, and confirming a successful
      Load reports `PauseMenuAction::GameLoaded`); and `Render` running
      every one of the 9 `PauseScreen` values at least once without
      crashing against real asset data. 53 smoke tests pass; full clean
      rebuild stayed at zero `/W4` warnings. Manually launched the real
      windowed exe and confirmed it starts and stays up. **Not
      independently re-verified this session** (same disclosed gap M60's
      and M62's own entries already have): actually pressing 'P' in a live
      play session and navigating Stats/Skills/Spells/Save/Load via real
      keyboard input wasn't attempted; every state-mutating code path IS
      exercised against real asset data by the smoke test above, including
      a real file-system Save/Load round trip, just not the live
      rendering/input itself.

- [x] **M64 -- the NPC choices menu, wired live (shops 0-3: Train/Give/
      Befriend/Threaten/Kill)** (this session). Part 1 of the multi-part
      treatment M60's own "what's next" note predicted this would need:
      new `ui/npc_choices_menu.h`/`.cpp` (`NpcChoicesMenuState`/
      `NpcChoicesMenu`, `stormhold_render`) covers `dispatchNpcChoice`'s
      own `case 0/1/2/3` group (`ESGame.java`'s screenGroups 9-12/20/22) --
      the 4 quest shops' shared Choices/TrainWhat/GiveWhat/Result state
      machine, built entirely on M56's already-ported
      `ShopInteraction::QuestShopDialogue`/`IsValidShopAction`/
      `ShopActionCode`. Beneca (shop 4)/Helga (shop 5) are their own
      bespoke menus, deliberately deferred to a follow-up milestone, same
      "one coherent slice at a time" split M56-M59's own quartet already
      used for the dialogue TEXT side of this exact gap.

      **The big finding this milestone turned up, and this project's
      THIRD deliberate, clearly-labeled port-only exception in the same
      category M62's `openInventory` softlock and M63's whole-pause-menu
      findings both established:** the real path from the M60 greeting
      screen into ANY NPC's choices menu is completely broken.
      `talkToNpc()` sets `npcHelloUI.nextScreen = npcChoicesUI[npcId]`,
      but `npcHelloUI`'s own screenGroup (8) is explicitly carved out of
      `commandAction()`'s big dispatch chain, landing in the outermost
      catch-all instead: `else if (cmd == UIScreen.cmdOk) {
      showScreen(activeScreen.backTarget); }` -- `backTarget`, a
      genuinely SEPARATE `UIScreen` field from `nextScreen` (confirmed by
      reading both field declarations directly), which defaults to `null`
      and is never once set for `npcHelloUI` anywhere in `ESGame.java`.
      So pressing "Ok" on ANY NPC's greeting prints "ERROR: next is
      null!" and calls `showScreen(null)` -- a confirmed softlock (pauses
      the game thread, shows nothing, no path back), per the same
      `showScreen(Object)` body `ui/inventory_ui.h`'s own class comment
      already documented for M62's finding. **In the real shipped game,
      the entire `npcChoicesUI` system -- for every one of the 7 NPCs,
      not just this milestone's own shops 0-3 -- is unreachable.** Rather
      than reproduce that, `main.cpp`'s own Enter-key dismiss handling for
      the M60 greeting screen now opens `NpcChoicesMenu` directly for
      shops 0-3 (honoring `nextScreen`'s own clearly-intended target
      instead of the broken `backTarget` read); shops 4/5/6 still just
      dismiss to gameplay, since their own menus aren't built by any
      milestone yet.

      **A second, real, separate crash bug, confirmed while reading the
      very same array, NOT applicable to this milestone's shops-0-3 scope
      and not something any future milestone needs to build against
      either:** `npcChoicesUI = new UIScreen[6]` (valid indices 0-5 only)
      -- but Varus is shopId 6, and `Shop.dialogue(player, 6, 1, 0)`
      returns a real non-null greeting once `wardenVisitCount >= 1`
      (`ShopInteraction::VarusDialogue`'s own state machine, M59), so
      `talkToNpc(6)` would throw `ArrayIndexOutOfBoundsException` the
      first time a player talks to Varus after any real Warden visit.
      Varus has no real choices-menu CONTENT to reproduce even if this
      port built a 7th slot (his own dialogue ignores `action`/`extra`
      entirely), so this is pure documentation, not a "what's next" item.

      **Two more minor, real, cheap-to-preserve findings, both confirmed
      by reading `UIScreen.setItemText`/`setTitle`/`setupMessage`/
      `setupPromptList` directly:** every per-action RESULT popup writes
      `Shop.NAMES[shopId]` into the same single `StringItem`
      `setMessageBody(result)` immediately overwrites right after --
      dead, invisible -- while the screen's real TITLE (a separate
      Form-level property, untouched after construction) stays whatever
      `npcResponsePopup`'s own `setupMessage("NPC name here", ...)`
      call set it to; reproduced exactly (Train/Befriend/Threaten/Kill/
      successful-Give results are titled the literal placeholder "NPC
      name here", and the one early-exit "nothing to give" case --
      reusing the pre-built `npcResponseUI` rather than a fresh screen --
      is titled ITS OWN leftover placeholder, "Oracle"). Separately, the
      Choices menu itself is built ONCE at startup with the literal
      placeholder title "Name" (`npcChoicesUI[i].setupPromptList("Name",
      ...)`), never patched afterward -- unlike `trainWhatMenu`/
      `giveWhatMenu`, which correctly pass the real `Shop.NAMES[shopId]`
      since they're rebuilt fresh on every open. Also reproduced exactly:
      `giveWhatMenu`'s own equipped-item prefix is `"E:"` (no space),
      confirmed to genuinely differ from `newInventoryUI`'s own `"E: "`
      (M62, WITH a space) by reading both methods side by side -- not
      normalized to match.

      **Noted, not modeled, an input-layer question this port has never
      attempted to answer for ANY screen:** `setupList`/`setupPromptList`
      add `cmdSelect`/`cmdCancel` (confirmed distinct `Command` objects
      from `cmdOk`/`cmdBack`), while most of `commandAction()`'s own
      screenGroup branches check `cmdOk`/`cmdBack` specifically -- whether
      that's a real bug or resolves through MIDP's own `List`/
      `ChoiceGroup` implicit-command semantics isn't something this
      project's transcribed Java source alone can settle. Doesn't change
      anything here either way: this port has never modeled literal
      `Command`-object identity for any menu, back to `ui/menu_flow.cpp`'s
      own M40 debut (every screen already collapses down to this port's
      own uniform Enter="confirm"/Escape="cancel" convention).

      Verified by a new `npc_choices_menu_smoke.exe`: `Open`/`Cancel` from
      Choices (closes the whole menu, matching the real, UN-broken
      `npcChoicesUI[shopId].nextScreen = gameCanvas`); `MoveSelection`
      clamping on the 5-item Choices list and no-op on an inactive state;
      a full Train round trip for all 4 quest shops (confirming each
      shop allows exactly 3 skills, matching `ShopActionCode`'s own
      `kCodes` table, and that a real, non-empty rumor line comes back);
      Give with a real starting inventory (list length matches
      `inventoryCount`, a real response line comes back) AND with an
      empty inventory (the exact literal "You have nothing to give me!"
      message, titled "Oracle"); Befriend/Threaten/Kill each producing
      real non-empty lines, plus Kill's own tile-bit-32 clear verified
      directly against a synthetic hub level; Cancel's own real-vs-port-
      only split (TrainWhat/GiveWhat close the whole menu, matching
      `nextScreen=gameCanvas`; Result returns to Choices, the deliberate
      port-only mapping); and `Render` running all 4 screens at least once
      without crashing against real asset data. 54 smoke tests pass; full
      clean rebuild stayed at zero `/W4` warnings. Manually launched the
      real windowed exe and confirmed it starts and stays up. **Not
      independently re-verified this session** (same disclosed gap M60's/
      M62's/M63's own entries already have): actually walking up to a
      real quest-shop NPC in a live play session and navigating
      Train/Give/Befriend/Threaten/Kill via real keyboard input wasn't
      attempted; every state-mutating code path IS exercised against real
      asset data by the smoke test above, just not the live rendering/
      input itself.

- [x] **M65 -- Beneca's own choices menu, wired live (shop 4: Give Item/
      Take Crystal)** (this session). Part 2 of the multi-part treatment
      M64's own "what's next" note scoped: `ui/npc_choices_menu.h`/`.cpp`
      (already `stormhold_render`, no new files) extends `NpcChoicesScreen`
      with a `TakeWhat` screen and teaches `Choices`/`GiveWhat`/`Cancel`
      to branch on `shopId <= 3` vs. `shopId == 4`, built entirely on M57's
      already-ported `ShopInteraction::BenecaDialogue`. Helga (shop 5)
      remains deliberately deferred -- bigger still, with her own
      Enchant-item sub-screen on top of everything Beneca needed.

      **Two real, confirmed findings from Beneca's own construction,
      genuinely DIFFERENT from the shops-0-3 findings M64 already
      documented, not just repeats of them:**
      - Beneca's OWN choices menu does NOT have M64's "Name" placeholder-
        title bug -- `npcChoicesUI[4].setupPromptList("Beneca", ...)`
        passes her real name directly (unlike the shops-0-3 LOOP's own
        generic `"Name"` literal), confirmed by reading her separate
        construction site. This port's own Choices-screen title is now
        `shopId <= 3 ? "Name" : Shop::kNames[shopId]`, a real per-shop
        difference, not an inconsistency in this port's own rendering.
      - `takeWhatMenu` (Beneca's "Take Crystal" list, screenGroup 27) sets
        `nextScreen = null`, unlike `trainWhatMenu`/`giveWhatMenu` (both
        `nextScreen = gameCanvas`) -- so its own Cancel press skips the
        top-level generic `cmdBack`-shortcut entirely and falls through to
        screenGroup 27's OWN explicit `cmdBack` handler, which shows
        `npcChoicesUI[shopId]` (refreshing its gift-count label first).
        **Cancel on "Take Crystal" genuinely, correctly returns to the
        Choices menu, while Cancel on "Give Item"/"Train What?" genuinely,
        correctly closes the whole thing straight to gameplay** -- a real
        behavioral asymmetry between sub-screens, confirmed by reading
        each one's own `nextScreen` assignment side by side, reproduced
        exactly (`NpcChoicesMenu::Cancel`'s own `TakeWhat` case is the
        only sub-screen that returns to Choices instead of closing).

      Also wired: half of `talkToNpc()`'s own null-result fallback M60's
      class comment left open -- `Shop.dialogue(player, 4, 1, 0)` returns
      `null` on every visit after the first (`BenecaDialogue`'s own
      action==1 branch), and the real game re-shows `npcChoicesUI[4]`
      directly in that case rather than opening an empty greeting.
      `main.cpp`'s own interact-key dispatch now does the same for shop 4
      specifically (Helga's identical-shaped fallback stays unwired, same
      as her whole menu). This is a plain, previously-just-unbuilt gap,
      not part of the M64 softlock finding -- closed because the machinery
      it needs (this file) finally exists.

      `BenecaDialogue`'s action 7 (Take Crystal) reuses `giveWhatMenu`'s
      shared screen for Give Item but needs its own new `TakeWhat` list
      (`Item.specialItemNames()`, ids 87-99, inlined as a one-line
      `items.name[86+i]` read, same "no wrapper for a one-line real read"
      precedent `player/shop_interaction.h`'s own class comment already
      uses) -- confirmed the naming trap `BenecaDialogue`'s own doc
      comment already flagged applies here too: the confirmed item id
      (`selectedIndex + 87`) is passed as `extra`, NOT a slot.

      Verified by a new `beneca_choices_menu_smoke.exe` (kept separate
      from M64's own test file, covering only what's new): Beneca's own
      2-item Choices list; Give Item sharing `QuestShopDialogue`'s own
      empty-inventory early exit AND a real successful give
      (`benecaPoints` increments, matching `BenecaDialogue`'s own action
      4); a full Take Crystal round trip (13-item list, `benecaPoints`
      spent by exactly 3, the granted item's id confirmed to be
      `selectedIndex+87` not a slot) AND the insufficient-points case
      (`benecaPoints < 3`); the confirmed Cancel asymmetry (TakeWhat back
      to Choices, GiveWhat/Choices itself closing the whole menu); the
      real "greeting is null after the first visit" premise the
      `main.cpp`-only fallback wiring depends on; and `Render` running all
      4 of Beneca's own screens without crashing. 55 smoke tests pass
      total; full clean rebuild stayed at zero `/W4` warnings. Manually
      launched the real windowed exe and confirmed it starts and stays up.
      **Not independently re-verified this session** (same disclosed gap
      M60's/M62's/M63's/M64's own entries already have): actually walking
      up to Beneca in a live play session and navigating Give Item/Take
      Crystal via real keyboard input wasn't attempted; every state-
      mutating code path IS exercised against real asset data by the
      smoke test above, just not the live rendering/input itself.

- [x] **M66 -- Helga's own choices menu, wired live (shop 5: Rumors/Give
      Crystal/Enchant/Bless/Cure/Warp/Recovery)** (this session). Part 3,
      the last and biggest, of the multi-part treatment M60's own "what's
      next" note first predicted this system would need: `ui/npc_choices_
      menu.h`/`.cpp` (still no new files) extends `NpcChoicesScreen` with
      an `EnchantWhat` screen and teaches `Choices`/`GiveWhat`/`Cancel` to
      branch on `shopId == 5` too, built entirely on M58's already-ported
      `ShopInteraction::HelgaDialogue`. With this, EVERY shop's own
      `npcChoicesUI` follow-up menu is now built.

      **This milestone's own big finding, and a genuine CORRECTION to
      M64's own work, both turned up by reading `commandAction()`'s full
      dispatch chain once more for Helga's own Bless/Cure/Warp/Recovery
      quartet:** of the 11 distinct RESULT-popup screenGroups this whole
      3-part system produces (Train/Give/Befriend/Threaten/Kill/Beneca's
      Take-Crystal/Helga's Enchant/Bless/Cure/Warp/Recovery), 9 are
      confirmed genuine dead ends (explicitly excluded from every branch
      of the dispatch chain that could reach them, same root cause M64's
      own entry already documented) -- but **2 are NOT**. Kill's own
      result (screenGroup 26) and Warp's own result (screenGroup 41) each
      happen to reuse a screenGroup NUMBER that a real, WORKING handler
      elsewhere in the dispatch chain already claims for an unrelated
      purpose -- 26 is a bare, unconditional `showScreen(gameCanvas)`, and
      41 is the real camp-mark-confirmation screen's own `if (cmd ==
      cmdOk) { player.justMarkedCamp = false; showScreen(gameCanvas); }`.
      Near-certainly accidental id reuse, not intentional design, but the
      practical effect is real: **pressing Ok on Kill's or Warp's own
      result screen genuinely, confirmedly closes straight back to
      gameplay in the original, unlike every other result in this whole
      system.** M64 had NOT caught this -- it treated Kill the same as
      every other (correctly) confirmed-dead result, routing it back to
      the Choices menu. **Fixed here:** `NpcChoicesMenuState::resultCloses`
      (new) now tracks the distinction; Kill and Warp both set it true
      (closing the whole menu on Confirm/Cancel), every other result
      leaves it false (the pre-existing port-only "return to Choices"
      mapping). Warp's own real side effect (`player.justMarkedCamp =
      false`) is reproduced too, applied at Result-creation time rather
      than dismissal time -- observably identical, since nothing reads
      that flag in between.

      **A second, related finding, the exact same root-cause bug breaking
      a THIRD screen:** Helga's own "Rumors" popup (`rumorsUI`, choice 0,
      screenGroup 360) sets `rumorsUI.nextScreen = npcChoicesUI[5]` -- but
      screenGroup 360 is excluded from the dispatch chain in EXACTLY the
      same way screenGroup 8 (`npcHelloUI`, M64's own original finding)
      is, landing in the identical `backTarget`-reading catch-all, which
      is ALSO never set for `rumorsUI`. So "Rumors" softlocks too, for the
      identical reason -- reproduced the same way (opens as a Result
      screen, port-only returns to Choices, honoring `nextScreen`'s real
      intended target rather than the broken dispatch). Also confirmed
      while reading `showRumors()` directly: its own `setItemText(0,
      Shop.NAMES[5])` has the SAME dead-write-clobbered-by-setMessageBody
      pattern M64 already documented for every other result popup, but
      against a THIRD distinct leftover placeholder title this system
      produces -- "Rumors" (`rumorsUI`'s own pre-built title), not "NPC
      name here" or "Oracle". Confirmed with a real title check.

      Also confirmed while building Enchant (`enchantWhatMenu`,
      screenGroup 350): its own item list is the player's FULL inventory,
      UNPREFIXED (no `IsEquippedSlot` check at all, unlike `giveWhatMenu`)
      -- so the selected row is the slot index directly, same as GiveWhat,
      new `BuildEnchantWhatList`. `HelgaDialogue`'s own action 12
      (Recovery) is the one Helga action with NO `helgaPoints` gate at all
      (confirmed by reading `Shop.java`'s own action==12 branch directly
      -- no `if (helgaPoints < n)` check, unlike Bless/Cure/Warp/Enchant),
      verified directly: a full heal succeeds even at 0 points.

      Finally wires Helga's own half of `talkToNpc()`'s null-result
      fallback (`main.cpp`), completing what M65 started for Beneca:
      `HelgaDialogue`'s own action==1 branch returns null once there's
      nothing new to say, and the real game re-shows `npcChoicesUI[5]`
      directly in that case; this port now does the same for both NPCs.

      Verified by a new `helga_choices_menu_smoke.exe` (56 smoke tests
      total, kept separate from M64/M65's own test files): Helga's own
      7-item Choices list; the M64 Kill-fix verified directly (`Confirm`
      after Kill now closes the whole menu) alongside a contrast check
      that Befriend's own result is still correctly a dead end; Rumors'
      real title/body/port-only-return-to-Choices behavior; Give Crystal
      sharing `HelgaDialogue`'s own action 4 (a real `helgaPoints`
      increment); a full Enchant round trip (real equipment-category item,
      `helgaPoints` spent by exactly 7, the item actually charged) plus
      Cancel from EnchantWhat closing the whole menu (NOT Beneca's
      TakeWhat-style exception); Bless/Cure producing real non-empty
      dead-end results; Recovery's own confirmed no-gate full heal; and a
      full Warp round trip (`justMarkedCamp` cleared, `helgaPoints` spent
      by exactly 1, `resultCloses` confirmed true, Confirm closing the
      whole menu). Full clean rebuild stayed at zero `/W4` warnings.
      Manually launched the real windowed exe and confirmed it starts and
      stays up. **Not independently re-verified this session** (same
      disclosed gap M60's/M62's/M63's/M64's/M65's own entries already
      have): actually walking up to Helga in a live play session and
      navigating all 7 of her own choices via real keyboard input wasn't
      attempted; every state-mutating code path IS exercised against real
      asset data by the smoke test above, just not the live rendering/
      input itself.

- [x] **M67 -- `paintUnknown_b()` wired live: the NPC/shop-portrait icon
      and Warden compass icon overhead the look-ahead tile** (this
      session). Closes the confidence-only gap a prior session's research
      (see this file's own "resolved, then wired" note above) identified
      but didn't build: `VisibleObjectRenderer::RenderUnknownB` (new,
      `render/visible_object_renderer.{h,cpp}`) is the C++ counterpart of
      `GameCanvas.paintUnknown_b()`, switching on the SAME value
      `PlayerMovement::ShopAheadOfPlayer` already computes (confirmed
      identical to `Player.questShopAtPendingTile()`, not a coincidence --
      see that resolution's own writeup). Wired into `main.cpp`'s render
      pass between `RenderObjects`/`RenderMonsters`, matching
      `paintGameView()`'s own call order exactly; recomputes `shopAhead`
      fresh at paint time (matching the original's own paint-time-fresh
      call, not a cache) rather than reusing the tick-time local of the
      same name declared in an outer scope.

      **A second real, reachable original-game crash found and preserved
      while building this:** case 6 (Varus/the Warden)'s own
      `tier = min(wardenVisitCount, 3) - 1` indexes
      `kUnconfirmedTableO[tier]` -- and Varus (shop 6) is a fixed,
      always-present NPC independent of the WANDERING Warden's own visit
      count, so `stat==6` is reachable from the very start of a New Game,
      before the Warden has ever visited (`wardenVisitCount==0`), giving
      `tier==-1`. Java's own out-of-bounds access there throws a real,
      catchable `ArrayIndexOutOfBoundsException`; a raw negative C++
      array index is undefined behavior, not a safe throw, so
      `RenderUnknownB` now explicitly guards `tier < 0` and throws
      `std::runtime_error` instead -- same "preserve the crash, don't
      paper over it" discipline `RenderMonsterOrIconSprite`'s own
      typeIndex-32..40 check (M28) already established, just newly
      REQUIRED here (not just faithful) because of the C++/Java UB gap.

      Verified by new `paint_unknown_b_smoke.exe` (5 checks): stat 0/1
      route to distinct typeIndexes (proven by which image slot lit up,
      not just position -- their table rows happen to share baseX/baseY);
      stat 0's literal columnOverride argument threads through to
      `RenderMonsterOrIconSprite` correctly (a transparent frame 0 vs. an
      opaque frame 1, distinguishing "override applied" from "ignored");
      the Warden compass icon at a valid tier; the new `wardenVisitCount
      == 0` crash-preservation throw; and an out-of-range `stat` being a
      true no-op (untouched backbuffer), matching the original switch
      falling through with nothing after it. Full clean rebuild stayed at
      zero `/W4` warnings on every project file (two pre-existing,
      unrelated `D9025 overriding '/W4' with '/W3'` command-line notices
      from a vendored/legacy target, not from any file this milestone
      touched). Manually launched the real windowed exe and confirmed it
      starts and stays up. **Not independently re-verified this session**
      (same disclosed gap every recent milestone's own entry already
      has): walking up to Varus/a quest shop in a live play session to
      watch the icon actually render wasn't attempted; the dispatch and
      crash-preservation logic IS exercised directly by the smoke test
      above, just not the live rendering/input path end to end.

- [x] **M68 -- fixed a real, reachable crash on every new game: `RenderUnknownB`
      (M67) and the NPC-nameplate popup (M60) were both missing a gate the
      original always requires alongside the quest-shop-tile check.** Found
      by actually doing the "not independently re-verified" live check
      M67's own entry above flagged as skipped -- driving the real windowed
      exe through character creation via Win32 `SendKeys` immediately threw
      the `tier < 0` guard M67 added, on the very first frame of every new
      game, every time.

      Root cause: `decompiled/e.java`'s `paint(Graphics)` only ever calls
      `paintUnknown_b()` from behind `if (W) { ... }` -- a SEPARATE
      condition from `stat = player.questShopAtPendingTile()`'s own tile
      check, set once per tick by `c()`: `W = f.a((byte)32, var1.a(0, 1,
      this.ae))`, i.e. bit 32 on the corridor-view cell one tile straight
      ahead (`DungeonRuntime::ViewGridAt(corridorView, 0, 1) & 32`, this
      port's own equivalent grid read, already existed for other purposes).
      M67 (this session's own earlier work) and M60 (an prior session) both
      substituted plain `shopAheadOfPlayer() >= 0` for `W`, reasoning both
      reduce to "the same tile" -- true of the TILE COORDINATES, but not of
      whether that tile is actually a live shop-portrait marker. For shops
      0-5, `dungeon_generator.cpp` marks their tiles bit-32 permanently at
      hub construction, so the substitution happened to hold. For shop 6
      (Varus), bit 32 is only set while `WardenState::present`
      (`world/warden.cpp`'s own `Arrive`/`Leave`), while
      `Shop::QuestShopAt`'s `questRewardClaimable[6]` starts (and normally
      stays) true regardless of the Warden -- so `shopAheadOfPlayer()`
      returns 6, unconditionally, any tick spent near Varus's fixed hub
      tile. A fresh character SPAWNS one tile from Varus, facing him
      (`player_creation.cpp`'s own `ResetForNewCharacter`: (9, 10) facing
      1, one step from his (9, 9)) -- so `RenderUnknownB` ran, with
      `wardenVisitCount == 0`, throwing M67's own (correctly added, but
      wrongly assumed rare) `tier < 0` guard immediately.

      Fixed both call sites in `main.cpp` to additionally require
      `DungeonRuntime::ViewGridAt(player.corridorView, 0, 1) & 32` before
      treating `shopAheadOfPlayer()`'s result as real -- matching the
      original's own `if (W)` gate at the paint call site, and its
      documented-but-previously-unrestored twin at the nameplate call site
      (main.cpp's own M60-era comment already said the original used "a
      separate bit-32 test" there and consolidated anyway; that
      consolidation is what this milestone corrects). Also revisits M67's
      own "second real crash" framing: with `W` restored, `tier < 0` is now
      believed UNREACHABLE in practice (`WardenState::Arrive` increments
      `visitCount` in the same call that sets bit 32, so `W` can only be
      true once `visitCount >= 1`) -- the guard is kept anyway (faithful to
      a real risk the original's own code shape carries, and "believed
      unreachable" isn't "proven unreachable"), but it was never really an
      independent second crash; it was this same missing-gate bug wearing
      a different face.

      While tracing this, also closed a related, previously-documented-but-
      accepted gap: `PlayerInventory::MarkCampAndReturnToTown` (the
      dungeon-camp-tile auto-warp-to-town mechanic, `player_movement.cpp`'s
      `autoMarkCampOnTile` branch, tile bit 8) was flagged back at M25 as
      not re-running `refreshCorridorView()` for the new hub position the
      way the original does immediately afterward, on the reasoning that no
      caller rendered off `player.corridorView` in the resulting stale
      window. That reasoning no longer holds now that this SAME
      milestone's nameplate-portrait fix reads `player.corridorView`
      directly -- a stale corridor view (still describing wherever the
      player triggered the auto-warp FROM, inside a dungeon) feeding that
      bit-32 check at the hub is a real, if narrower, source of wrong
      nameplate/portrait flashes for however many ticks pass before the
      player's next real move. Fixed at the one call site that already had
      a `LevelLookup` in scope (`CommitMove`'s own auto-camp branch); the
      other two real callers (`PlayerInventory::UseItem`'s camp-marker
      item, `ShopInteraction::VarusDialogue`'s own Warp action) still don't
      take one, so that narrower slice of the original gap remains open --
      see `MarkCampAndReturnToTown`/`WarpToCampMark`'s own header comments.

      Also added a small, port-only (no original counterpart) diagnostic:
      `main.cpp`'s own tick callback now catches `std::exception` at its
      outermost scope, logs `e.what()` to stderr (already redirected to
      `<userDir>/stormhold_port.log` whenever a launcher sets
      `STORMHOLD_USER_DIR`), and only then aborts -- so the NEXT time one of
      this codebase's many deliberate "preserve a real bug as a throw"
      guards fires, whoever's playing has a log line identifying exactly
      which one, instead of a bare crash. Every one of those throws was
      already written with a specific, identifying message; this just
      makes sure at least one of them gets read before the process dies.

      Verified: all 57 smoke tests pass (including the existing
      `paint_unknown_b_smoke.exe`, unchanged -- it tests `RenderUnknownB`
      directly, below the new gate, which lives in `main.cpp` at the call
      site instead). Full clean rebuild, zero `/W4` warnings. **Live-
      verified this time, not just smoke-tested:** drove the real windowed
      exe through character creation via `SendKeys` -- previously reproduced
      the crash exactly (caught via a temporary stderr-logging harness,
      confirmed it was the `tier < 0` throw, before adding the permanent
      diagnostic wrapper above), then confirmed a fresh character now
      spawns, stays up, and remains stable while facing Varus and pressing
      into his door repeatedly, on the fixed build. The camp/corridorView
      fix itself was NOT live-verified the same way (reaching a dungeon's
      one-per-level bit-8 tile needs real pathing through generated
      geometry, not attempted this session) -- fixed on code-reading
      grounds (a direct, narrow instance of an already-documented M25 gap)
      and smoke-test coverage only.

- [x] **M69 -- Help (pause-menu item 6) wired live: 12 topics, list-then-body,
      closing the transcription gap M63's own entry flagged.** The gap
      turned out to be smaller than it looked: `decompiled/ESGame.java`'s
      own `private void a()` (the real `loadHelpTopicBodies()`) was always
      fully present and unambiguous in the decompiled bytecode --
      `src/ESGame.java`'s own hand-transcription had just stopped partway
      through copying it (topics 0-4 done, a `TODO` past that point). Read
      the rest of `private void a()` directly and finished the copy:
      `helpTopicBodies[5..11]`, each a `Shop.dialogue[7][N..M]` row-span
      concatenation exactly like topics 0-4, up through topic 11's single
      row at index 40 (group 7's own last real row, `ShopDialogue`'s
      41-entry span -- confirmed in range, not a guess).

      C++ side: `ui/pause_menu.h`/`.cpp` gain `PauseScreen::Help` (the
      12-topic list, screenGroup 203) and `PauseScreen::HelpTopic` (one
      topic's body, screenGroup 206), mirroring Skills/SkillInfo's own
      list-then-detail shape. `Render` gains one new parameter, `const
      ShopDialogue& dialogue` (titles/bodies are real `ShopDialogue`
      group-7 text, not literals) -- the only public signature change.
      One real, confirmed original quirk preserved rather than
      "corrected": `newHelpTopicUI(topicIndex).nextScreen = this.statsUI`
      -- leaving a help topic body (Ok OR Cancel; screenGroup 206's real
      dispatch doesn't distinguish) lands on the STATS screen, not back on
      the topic list and not on Options either. Same "confirmed shipped
      bug, keep it" treatment `ui/pause_menu.h`'s own "Quit Game" finding
      (shows Credits, doesn't quit) already established.

      Verified: rewrote M63's own `TestHelpIsNotWired` as `TestHelpNavigation`
      (Options -> Help -> a topic -> Stats via the real quirk, plus both
      Cancel paths -- topic-body Cancel also lands on Stats, topic-LIST
      Cancel lands on Options like Skills/Spells' own list screens), and
      added `TestHelpTopicTextIsReal` (all 12 topics render against real
      loaded `ShopDialogue` data without throwing -- the actual proof every
      `kHelpTitleRow`/`kHelpBodyRows` index lands inside group 7's real
      41-row span, not just inside array bounds in the abstract). All 57
      smoke tests pass, full clean rebuild, zero `/W4` warnings. **NOT
      independently re-verified live this session** -- attempted via the
      same scripted-`SendKeys` approach M67/M68 used, but window-focus
      targeting (`SetForegroundWindow`/`AppActivate`) failed silently
      partway through and a screenshot meant to confirm the Help screen
      instead captured an unrelated window, so the live-GUI verification
      approach was abandoned for the rest of this session rather than
      continue blind desktop capture. The dispatch/render logic IS
      exercised directly by the two tests above, just not the live
      keyboard-input/window-focus path end to end.

- [x] **M70 -- fixed a real correctness bug reachable on every ordinary level
      transition, not just the rare camp-mark-tile edge case: `main.cpp`'s
      own `currentLevelMutable` went stale mid-tick.** Found while
      investigating M68's own reported symptoms further -- `currentLevelMutable`
      (`levelLookup(player.currentLevel)`, bound ONCE near the top of each
      tick) was passed to every later-in-the-tick level-keyed call
      (`Camping::Tick`, `InventoryUi::Confirm`, `CollectChestItem`,
      monster-death drop/removal, `CombatResolution::TickMonstersOnLevel`,
      both `SampleSquareView` minimap calls) -- but movement (`up`/`down`/
      `left`/`right`, the single most common player action) runs BEFORE
      most of those, and `PlayerMovement::Move`/`CommitMove` updates
      `player.currentLevel` in place on any ordinary level-to-level step,
      not just the M68-fixed camp-mark-tile auto-warp. So EVERY step that
      crosses a level boundary left the rest of that tick's chest
      collection, monster AI tick, monster death drop, and minimap
      rendering reading/writing the OLD level object the player just LEFT,
      not the one they're now on -- confirmed real, not just theoretical:
      crossing from the 19x19 hub into any 35x35 dungeon left that tick's
      minimap sample combining the player's NEW, larger-range coordinates
      with the OLD, smaller hub level's own tile grid and neighbor
      pointers. Not memory-unsafe (`DungeonRuntime::TileAt`'s own
      neighbor-stepping bounds check means an out-of-range coordinate walks
      to a neighbor level via a valid index, never a raw out-of-bounds
      array read), and self-heals the very next tick (a fresh binding was
      always recomputed at the top of every tick regardless) -- but a real,
      constantly-reachable, wrong-level bug for that one tick on what is
      almost certainly the single most common action in the entire game.

      Fixed by removing the single cached binding entirely: every one of
      the 8 real use sites now calls `levelLookup(player.currentLevel)`
      fresh at its own point of use (a local `deathLevel` binding kept
      only for the 3-line monster-death-drop block, which doesn't cross a
      level change partway through itself). `levelLookup` is a plain O(1)
      array index, so this costs nothing measurable.

      Verified: all 57 smoke tests pass (no test exercised this specific
      mid-tick-staleness path directly -- it's a `main.cpp`-only wiring
      bug, not a unit a smoke test targets, so this relies on the existing
      suite's regression coverage of each individual call plus a live
      check). Full clean rebuild, zero `/W4` warnings. **Live-verified
      this time:** launched the real windowed exe (no desktop screenshots
      this session, after M69's own focus-targeting mishap -- verified via
      process-alive polling and the log file only), completed character
      creation, then walked left/down repeatedly (crossing at least one
      level boundary out of the hub) for several seconds; the process
      stayed up throughout and the log file showed no `FATAL` line from
      the M68 exception logger.

- [x] **M71 -- source-vs-port comparison audit, no functional bugs found (a
      first for a session dedicated to this).** Systematically re-read
      several high-risk areas (numeric-formula-heavy, index-heavy, or
      recently-built) side by side against `../../../src/`, looking for the
      same class of "silent gap" M67-M70 kept finding:
      - `Monster.java` (spawn/move/chase/isStairwayTile/tick/onDeath) vs
        `monster/monster_runtime.cpp`/`combat/combat_resolution.cpp`,
        including the `tickMonsterAI` aiPhase state machine
        (`GameCanvas.java`'s `b(long)`) vs `TickMonstersOnLevel`.
      - `Item.rollLoot` vs `ItemDatabase::RollLoot`, including the signed-
        byte sign-extension subtlety in `int result = low` (`lootTable` is
        `int8_t`, confirmed to sign-extend the same way).
      - `Shop.dialogue()`'s FULL dispatcher, all 7 NPCs, every action
        branch (shops 0-3's quest-turn-in pattern, Beneca, Helga, Varus)
        vs `ShopInteraction::QuestShopDialogue`/`BenecaDialogue`/
        `HelgaDialogue`/`VarusDialogue` -- byte-for-byte match, including
        the `rumorFor`/`Util.replace` multi-tag substitution order and the
        cross-group `dialogue[7][0]` return in Beneca's own training-
        failed branch.
      - `Player.java`'s combat-stat formulas (`defenseSkillValue`/
        `baseEvasion`/`armorValue`/`defenseSkillIndex`/`attackPower`/
        `weaponDamage`) vs `player/player_combat_stats.cpp`, including the
        `Math.abs()` placement (on the CATEGORY value, not the item id) and
        the questFlags-column-doubles-as-magnitude reuse in `armorValue`.
      - The NPC choices menu's numeric hand-offs (`ShopActionCode` reused
        as a rumor-step `extra`; `selectedIndex + 87` for Beneca's Kill/
        Take item id) vs `ui/npc_choices_menu.cpp` -- both match exactly.
      - `Player.toBytes(true)`/`readFrom(..., true)`'s full save-format
        field order vs `player/player_save.cpp` -- every field, in order,
        same type, no drift (the `full=false` lightweight variant is
        already confirmed-and-documented dead code, `toBytes(false)` has
        no real caller anywhere in `../../../src/`).
      - `GameCanvas.java`'s dead-respawn message selection (reusing
        whatever `enteredNewLevelZone`/`leftLevelZone` happen to still be
        set to from the player's last real move, not reset by
        `resetState`) vs `death/death_sequence.cpp`'s `RespawnMessageLines`
        -- already correctly documented and preserved.
      - Re-grepped the whole port for the exact M70 bug SHAPE (a
        `GeneratedLevel&` bound once and reused across a scope that could
        also change `player.currentLevel`) -- the only other bindings of
        that shape are all short-lived, single-function-scope locals in
        `player/player_movement.cpp` that don't cross a later independent
        call, so no second instance of that bug exists.

      **One real finding, upgraded from speculation to confirmed:**
      `Shop.unconfirmedCooldownH`'s own header comment used to say "no
      increment site found in this file, producer likely external, unit
      unconfirmed." Exhaustively grepped across EVERY renamed file in
      `../../../src/` (not just `Shop.java`) for any write to it -- the
      only one anywhere is `Shop.dialogue()`'s own action==4 DECREMENT
      (clamped at 0); `reset()` zeroes it; nothing else touches it. Both
      `> 50` gates that read it (action 1's greeting-cooldown branch and
      action 5's rumor-cooldown branch, mirrored in `QuestShopDialogue`)
      are therefore confirmed dead code in the ORIGINAL shipped game, not
      a decompilation or port gap -- updated both `Shop.java`'s own header
      comment and `shop_interaction.cpp`'s two call sites to say so
      plainly instead of leaving it as an open question. No behavior
      change (the port already only ever decremented it, same as the
      original) -- documentation-confidence only.

      All 57 smoke tests pass, zero new `/W4` warnings. This was a
      targeted sample, not exhaustive -- `Dungeon.java`/`UIScreen.java`/
      the rest of `Player.java` (3155 lines, the largest single file)
      weren't re-read start to finish this session -- but every area
      sampled, including several not touched by M67-M70's own fixes,
      came back a faithful, correct match.

## What's next

With M66, every one of the 7 real NPCs' own `npcChoicesUI` interactive
follow-up menus is built (Varus, shopId 6, is the sole exception -- he has
no real menu content to build at all, per his own confirmed array-bounds
crash finding, M64's own entry). `IsValidShopAction`/`ShopActionCode`
(M56) still only cover shops 0-3's own menu-choice mapping, now confirmed
moot for Beneca/Helga specifically -- neither needed an equivalent (Beneca
never had one; Helga's own Enchant picker turned out to need nothing more
than a plain full-inventory list, M66's own `BuildEnchantWhatList`). No
bug-preservation dilemma blocks any of it -- see M53's own entry for the
corrected `Shop.reset()` finding (it genuinely runs in the real game, via
a static initializer); the one real surviving caveat is that `reset()`
only ever runs ONCE per app launch there, not once per New Game, so
quest-economy state carries over across a same-session death-restart --
not modeled by `ShopState` either way (this port has no live app-lifetime
`Shop` instance to carry state between a
death-restart and the next), just worth keeping in mind for whoever
eventually adds one.

Beyond that, M41's dispatch web is now FULLY wired -- M62 closed the last
branch (`openInventory`), and M63 built and wired the in-game pause/
Options menu (Stats/Inventory/Skills/Spells/Save Game/Load Game) plus a
real Save/Load trigger against M50's `GameSave`.

**RESOLVED, then WIRED (M67):** the confidence gate that used to keep
`paintUnknown_b()`/its C++ wiring deferred is closed, and the C++ side is
now built and live (see M67's own entry above for the full writeup).
`Player.questShopAtPendingTile()` (was `r()`), the value `paintUnknown_b()`
switches on, was flagged LOW CONFIDENCE because `this.ab`/`this.z`/
`this.w` weren't cross-checked against this file's other fields. They
turned out to be confirmable: `r()`'s two sibling methods in
`decompiled/j.java` (`n()`/`h()`, the monster-at/chest-at-pending-tile
lookups, sitting immediately above it) open with the exact same
`this.g(1); if (this.ab<=0) ...` guard and are already ported using
`pendingLevel`/`pendingTileX`/`pendingTileY` -- confirming `r()` uses the
same three fields, not an unconfirmed alias. dawnstar's own `Player.java`
independently corroborates: it names the identical shared-engine
mechanic `pendingLevel`/`pendingTileX`/`pendingTileY` with full
confidence. `src/Player.java`/`src/Shop.java` (the Java reference tree)
updated accordingly (dropped the placeholder `stateByteAb` field, reused
`pendingLevel` directly, both already existed on the C++ `PlayerState`
too). `paintFlashOverlays()`, previously bundled with `paintUnknown_b()`
in `main.cpp`'s own "deliberately NOT wired here" header comment, turned
out to be a stale claim on ITS side too, independent of this session's
fix -- `FlashOverlay::Paint` is already called in the real render pass
(confirmed by reading `main.cpp` directly), just never updated in that
comment. Fixed there now; nothing left in that "what's next" bucket.
**Note for whoever eventually revisits
M62's own `ui/inventory_ui.h` class comment:** the real `openInventory()`
softlock it deliberately didn't reproduce is STILL not reproduced now
that the pause menu exists -- `PauseMenu`'s own "Inventory" entry
(`ui/pause_menu.cpp`) calls `InventoryUi::Open` directly too, the same
"always build fresh" choice M62 made, rather than introducing an
`inventoryUI`-style cache field the hotkey path could skip. This port
simply has no equivalent construct at all, by design, not by oversight --
so that particular real bug remains permanently out of reach here, unlike
the M52 Continue-Game-softlock note below (which IS newly reachable as of
M63's real Save/Load wiring). "Help" (pause-menu item 6) is now fully
wired -- see M69's own entry above; the transcription gap M63 flagged
turned out to be a stalled hand-copy, not a real decompiler gap, and is
closed. M64's own
`npcHelloUI`/`backTarget` softlock finding is now fully closed out end to
end: M65 widened `main.cpp`'s own dismiss handling to cover Beneca and M66
to cover Helga too (`shopId >= 0 && shopId <= 5`), each honoring
`nextScreen`'s own intended target, the same exception M64's own entry
first established. Following dawnstar's own later milestones roughly but
expecting further Stormhold-specific divergences the way
M3/M6/M7/M8/M9/M10/M12/M13/M14/M16/M17/M18/M19/M20/M21/M22/M41/M42/M43/M44/M45/M46/M47/M48/M49/M50/M51/M52/M53/M54/M55/M56/M57/M58/M59/M60/M61/M62/M63/M64/M65/M66
already found.

**Researched but deliberately NOT built this session: `GameCanvas.
isNpcDialogueDue()`'s own proactive "Warden speaks" trigger
(`run()`'s own `if (isNpcDialogueDue() && Shop.wardenVisitCount >
player.wardenLoreStep) { ... Shop.dialogue(player, 6, -1, -1) ... }`,
main.cpp's own comment on the `warden.ShouldVisit`/`Arrive` call site
already flagged this as unwired, same gap M43/M49/M50's own "what's next"
notes once carried). Investigated properly while scoping the next
milestone, and found it's a poor fit for one, for two independent
reasons:**
1. **The dialogue this produces is confirmed to NEVER actually reach the
   player.** `Shop.dialogue(player, 6, -1, -1)` (which, per
   `ShopInteraction::VarusDialogue`'s own doc comment, ignores its
   `action`/`extra` arguments entirely -- this is exactly Varus's own
   greeting state machine, just invoked automatically instead of via
   player interaction) runs, its real side effect on `player.
   wardenLoreStep` genuinely applies, and its result text is used to
   build a real screen (`newWardenSpeaksUI`, screenGroup 102, correctly
   titled "Varus", no dead-write bug this time) -- but that screen is
   only ever ASSIGNED to `this.game.rumorsUI`, never passed to
   `showScreen()` anywhere in the file (confirmed: `rumorsUI` has exactly
   one other reference in the whole of `GameCanvas.java`, this exact
   assignment). screenGroup 102 even has its own real, correctly-working
   dispatch handler (`showScreen(gameCanvas); resumeTicking();`,
   unconditional) -- entirely moot, since nothing ever makes that screen
   the active one. **So standing near Varus silently advances his own
   lore-reveal state every tick once new lore is available, with the
   player never shown the line at all** -- a genuine, confirmed dead-UI
   bug, and a real behavioral coupling worth knowing about (a player who
   lingers near Varus before ever pressing "interact" could find his
   interactive greeting already "caught up," returning a repeat instead
   of fresh lore) but not one with any screen worth building, since the
   original itself never shows one.
2. ~~**The gating condition itself carries a confirmed, unresolved
   transcription gap.**~~ **RESOLVED this session:** found the actual
   original method (`decompiled/k.java:450`, `static boolean a(j var0)`)
   -- `Shop.isAdjacentToVarus(player)`'s transcribed `currentLevel==1
   && adjacent-to-Varus` body is a byte-for-byte-complete match, not an
   approximation; `player.j` is just `currentLevel` under another name
   (confirmed via `decompiled/j.java`'s `g(int)`), already covered by the
   `currentLevel==1` check. See `Shop.java`'s updated doc comment. Reason
   1 above (confirmed dead code, no player-visible screen) still stands
   on its own regardless. The OTHER trigger
   (`isNpcDialogueDue()`'s own level-37/monster-typeIndex-41 branch,
   presumably the final boss, given `newEndOfGameUI`'s own `Shop.
   dialogue[7][4]` "Victory!" text) is completely unexplored by this
   project so far, and -- since the SAME hardcoded `Shop.dialogue(player,
   6, -1, -1)` call fires regardless of which of the two triggers fired --
   its only real, confirmed observable effect would ALSO be silently
   advancing Varus's own `wardenLoreStep`, an odd, likely-unintentional
   coupling between the final boss and the Warden that would need
   independent confirmation before treating as faithful rather than a
   transcription artifact.

Building this now would still mean reproducing confirmed dead code with no
player-visible behavior -- not worth a milestone on its own, even with
reason 2 above now resolved rather than an open gap. Left here as a
confirmed, thoroughly-researched finding, not a "what's next" action item.

**Resolved by M63 (was: "heads up for whoever eventually wires a real Save
trigger", M52's own finding):** `savegame.dat` now actually gets written,
via the in-game pause menu's own "Save Game"/"Load Game" items
(`ui/pause_menu.cpp`), so the confirmed real softlock M52 documented
(`world/game_advancement.h`'s own class comment) is reachable for the
first time in THIS PORT specifically -- a Continue Game (main menu) OR
Load Game (pause menu, same `GameSave::Load` underneath) resumed past
zone 0 leaves the player **completely unable to move, not even in place**
(M52's own words), faithfully matching the original engine's own confirmed
bug. **The conscious decision this section already called for was made,
and the answer is: stay faithful, no exception.** This is a narrower call
than it first looks: M52's own entry already rejected building the fix (a
load-time zone catch-up into `GameSave::Load`) on its OWN terms, before
Load had any real trigger at all -- reasoning it would make this port
STRICTLY MORE correct than the shipped original ever was, "the same
'behavioral gain, not reimplementation' trap M51's own note already warns
future work away from." M63 doesn't reopen that decision; it just makes
the already-accepted consequence reachable for the first time. That's a
different situation from M62's own `openInventory` exception, which
existed BECAUSE the necessary machinery (a pause menu) didn't exist yet --
here the machinery (a `GameAdvancement::OpenUpTo`-equivalent) was always a
choice, deliberately declined on principle, not blocked on anything M63
just built. Every OTHER "not modeled"/"not fixed" note elsewhere in this
doc likewise stays faithful, unaffected by this resolution.
