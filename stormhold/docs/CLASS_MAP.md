# Stormhold -- class map (phase 1 findings)

Read-through of `decompiled/` to identify and rename every class, same goal
and method as `../../dawnstar/docs/CLASS_MAP.md`, but **not** a search-and-
replace of dawnstar's own findings onto this codebase. Stormhold and
Dawnstar share the Vir2L "ngame" engine (confirmed: `ngame/midlet/a.java`
is the same MIDlet-lifecycle base class in both), and several core data/
logic classes turned out to be **field-for-field identical** to their
dawnstar counterparts (see below) -- but the UI/rendering layer is
genuinely different between the two games, and even the "identical" classes
have real, confirmed content differences (extra methods, different table
sizes, different gameplay mechanics). Every finding below was reached by
reading Stormhold's own decompiled source directly (diffed against
dawnstar's decompiled/ output where useful as a *hint*, never assumed).

## Status

Confirmed and fully renamed, with hand-written source in `../src/`:
- `ngame/midlet/a.java` -> `RegisteredMIDlet` (`../src/ngame/midlet/RegisteredMIDlet.java`)
- `a.java` -> `Item` (`../src/Item.java`)
- `b.java` -> `Spell` (`../src/Spell.java`)
- `d.java` -> `Monster` (`../src/Monster.java`)
- `f.java` -> `Util` (`../src/Util.java`)
- `k.java` -> `Shop` (`../src/Shop.java`)
- `i.java` -> `Dungeon` (`../src/Dungeon.java`) -- **merged with generation
  logic**, see its own section below; there is no separate
  `DungeonGenerator` class in this codebase.
- `g.java` -> `RawImage` (`../src/RawImage.java`)
- `c.java` -> `ScreenCanvas` (`../src/ScreenCanvas.java`)
- `h.java` -> `UIScreen` (`../src/UIScreen.java`) -- outer structure fully
  transcribed, see its own section below.
- `e.java` -> `GameCanvas` (`../src/GameCanvas.java`) -- **partial pass**,
  outer structure transcribed, deep rendering internals left as TODO
  stubs, see its own section below.
- `j.java` -> `Player` (`../src/Player.java`) -- fully transcribed, see its
  own section below.
- `ESGame.java` -> `ESGame` (`../src/ESGame.java`), see its own section
  below.

**Phase 1 is complete**: all 13 decompiled classes (plus `ngame/midlet/a`)
are renamed, and a follow-up integration pass resolved every cross-file
`TODO_*` placeholder the individual class passes had left. `../src/`
**compiles cleanly** end-to-end (`javac` against `tools/midp-stubs/*`,
zero errors). What's left for later phases, not phase 1: `GameCanvas.java`'s
~15 still-stubbed pixel-rendering methods (the class's own header comment
flags exactly which), `ESGame.loadHelpTopicBodies()` transcribed through
only help topic 4 of 12, and a set of individually-flagged low-confidence
field/method names throughout (`TODO_`/`unconfirmed*` prefixes, searchable
across `../src/`) that compile and behave plausibly but aren't
independently proven -- see each file's own header comment.

## The shared "ngame" engine

`ESGame extends ngame.midlet.a`, confirmed to be the same shared Vir2L
MIDlet-lifecycle base class dawnstar's `ESGame` extends (there:
`ngame.midlet.RegisteredMIDlet`, already unobfuscated in dawnstar's
decompiled output). Unlike dawnstar's copy, this build's `ngame/midlet/a.java`
**is** single-letter obfuscated, and is missing the trial/unlock-code
gating and billing/error-alert stub methods dawnstar's copy has
(`registerApp`, `getUnlockCode`, `confirmedGetUnlockCode`,
`checkUnlockCode`, `errorAlert`, `fatalErrorAlert`, `billingEvent`,
`postUserData`) -- either a leaner build of the same class or dead code
the compiler stripped as provably unused. The surviving methods (MIDlet
lifecycle, three identical "Exit" `Command`s, `commandAction` dispatch)
match dawnstar's 1:1. Renamed `RegisteredMIDlet` for consistency.

## `a` -> `Item` (item database + loot tables)

Fully renamed, see `../src/Item.java`. **Field-for-field identical layout**
to dawnstar's `a.java`/`Item.java` (same field letters in the same
declaration order) -- confirmed by direct diff. Real differences found (not
copied from dawnstar, transcribed from this file directly):

- `specialEffectText` (was `l`) is `String[13][2]` here (each "gift" item's
  flavor text pre-split into up to 2 display lines, e.g.
  `{"Grants level","experience"}`) vs. dawnstar's flat `String[13]`.
- The 3 instant-kill scroll entries (ids 97-99) all read the literal
  `"Kill monster"` here, not dawnstar's differentiated "Kills weak/normal/
  strong monster" -- Stormhold doesn't distinguish them by flavor text.
- A second boolean predicate, `Item.isEquipmentCategory(int)` (was `b`,
  distinct from `isEquippable`/was `c`, which checks the `equipSlot`
  column): checks the `category` column is 1-10. Confirmed call sites in
  `j.java` (`h(int)`, gates initializing a per-slot "charge" byte to 3)
  and `k.java` (a buy-time stacking check). No dawnstar analog found --
  exact semantic name not pinned down further, see the field's doc comment
  in `Item.java`.
- `Item.specialItemNames()` (was `a.b()`, a **zero-arg** static method --
  distinct from the 1-arg `isEquipmentCategory`/`b(int)` above, a genuine
  overload) returns the real item **names** of ids 87-99 read from
  `itemsin.dat` (`name[86..98]`), not flavor text. Confirmed caller:
  `ESGame.java:547`.

## `b` -> `Spell` (spell database)

Fully renamed, see `../src/Spell.java`. Field-for-field identical to
dawnstar's `b.java`/`Spell.java` (confirmed by direct diff): same
`spellsin.dat` column loader, same `school==2` "offensive" convention. Only
difference: a debug `println` reporting the loaded spell count, and
resource loading goes through this build's `Util.openResource` wrapper.

## `d` -> `Monster` (monster instances + AI)

Fully renamed, see `../src/Monster.java`. Same overall shape as dawnstar's
`d.java`/`Monster.java` (monstersin.dat type table, packed 28-byte
`toBytes`/`fromBytes` record plus a separate stream `readFrom`/`writeTo`
format, `stat()`'s masked `&0xFF` reads vs. `tick()`/`onDeath()`'s raw-
signed-byte reads of the same type-stats table, the same cascading
N/S/W/E `isStairwayTile` priority-order quirk dawnstar's own M15 port
milestone found) -- but two confirmed real differences, not assumed from
dawnstar:

1. `spawnId` (was `a`) is the per-spawn **unique-id counter value**
   (from `nextSpawnId()`/the static counter), confirmed by `spawn()`'s
   constructor call `new Monster(nextSpawnId(), typeIndex, level.levelNumber)`
   -- **not** a "type id" the way dawnstar's `CLASS_MAP.md` labels its own
   `a` field. `typeIndex` (was `l`) is the actual type index into the
   static type tables.
2. `store()` (was `d()`) keys the live per-level registry
   (`ESGame.monsters[level]`, a `Hashtable`) by `String.valueOf(spawnId)`,
   **not** a `"x,y"` position key like dawnstar's `Util.posKey` -- a real
   architecture difference.

Also confirmed: `move(int)` does **not** call `store()` itself at the end
(unlike dawnstar's `Monster.move`), leaving the registry write to the
caller -- confirmed by direct diff, not an oversight in this port pass.

**Dungeon-side names now confirmed** (see the `i` -> `Dungeon` section
below): `width`/`height`/`tiles`/`isWalkable`/`stairsUpDir`/
`stairsDownDir`/`tier`/`levelNumber`/`Dungeon.MONSTER_TYPE_BY_TIER` all
matched what `Monster.java` already used; the ambush-trigger hook
(originally `i.c(int)`) is `Dungeon.spawnAmbushMonsters(int)` -- confirmed
to be the supplemental random-monster spawner, called from `Monster.tick`'s
ailment-2 ("swarm curse"-style) branch. `Monster.java` has been updated to
use this real name.

**Player-side names in `tick()` are now confirmed** (see the `j` ->
`Player` section below): the four calls are `defenseSkillValue(boolean)`,
`baseEvasion()`, `armorValue()`, and `gainSkillExp(defenseSkillIndex(), 1)`
-- `Monster.java` has been updated to use these real names. The two
ailment-timer field names were also corrected from dawnstar's
`trollThirstTimer`/`glacierCurseTimer` (mistakenly carried over during the
`Monster.java` pass) to Stormhold's own `vampirismTimer`/`manaBurnTimer`
(ailments 4/5 are named "Vampirism"/"Mana Burn" here, not "Troll Thirst"/
"Glacier Curse" -- see the ailment-names list in the `Player` section).

## `f` -> `Util` (misc stateless helpers)

Fully renamed, see `../src/Util.java`. Near-identical to dawnstar's
`f.java`/`Util.java`: string find/replace for `<TAG>` substitution, bitset
helpers on `byte`/`int`, an 8-byte big-endian long decode, thin wrappers
around `ESGame`'s resource/RNG statics. One real addition not in dawnstar's
`Util.java`: `Util.splitWords(String)` (was `f.c(String)`), a run-of-spaces
tokenizer -- likely feeds a word-wrap path in the UI layer (`h`/`e`, not
yet renamed).

Confirmed `ESGame` RNG helper identities (needed by `Util`/`Item`/`Monster`
already, ahead of `ESGame.java`'s own rename pass): `ESGame.lingoRandomInt`
(was `h`, 0-based: `abs(rng.nextInt() % bound)`) and `ESGame.nextInt` (was
`f`, 1-based: `1 + abs(rng.nextInt() % bound)`) -- same convention as
dawnstar's M5 port milestone (`java.util.Random`-exact `LingoRandomInt`/
`RandomIntBelow`). `ESGame.getResource(String)` (was `a`) reads a resource
straight off the classpath (`getResourceAsStream`) and buffers it fully
into a `DataInputStream` -- **not** read from a packed `datfiles.lmp`
archive the way dawnstar's `ESGame.getResource` is; Stormhold's `extracted/`
has every `*in.dat` table as its own top-level resource, no `datfiles.lmp`
at all. `geomin.dat` and `monsterfilenamesin.dat` are loaded directly by
`ESGame` itself (fields `ai`/`T`), not delegated to a data-table class.

## `k` -> `Shop` (NPC dialogue, quest tracking -- no buy/sell found)

Fully renamed, see `../src/Shop.java`. Structurally similar to dawnstar's
`k.java`/`Shop.java` in a few places (same `npcstrings.dat` loader shape,
the same `isValidShopAction`/`shopActionCode` matched pair with identical
action-code tables, the same `questFlagsFor` 2-bit extraction), but
**Stormhold's NPC roster and quest system are genuinely different** --
confirmed by reading this file directly, not assumed from dawnstar:

- **7 NPCs, not 9**: `Arantamo`, `Celegil`, `Favela Dralor`, `Vander`
  (`SHOP_CATEGORY` 1 -- a shared quest-turn-in pattern, shops 0-3),
  `Beneca`, `Helga` (`SHOP_CATEGORY` 2 -- bespoke single-NPC branches,
  shops 4-5), `Varus` (`SHOP_CATEGORY` 3 -- tied to the Warden event,
  shop 6). **No buy/sell branch (dawnstar's action 14/15) exists anywhere
  in this file** -- shops 0-3 here are structurally the counterpart of
  dawnstar's *named* quest shopkeepers (5-8), not its generic peddlers.
  If Stormhold has an item economy at all, it isn't in this class.
- Shops 0-3's quest pattern (actions 1-6) tracks state through **five**
  per-shop arrays, not dawnstar's four: `questState1`/`questState2` (byte,
  quest progress), `interactionCount`/`rewardsGiven` (short), plus a fifth,
  `unconfirmedCooldownH` (short, was `h`) gating both the greeting and
  reward-claim branches with a `>50` check -- easy to conflate with
  `interactionCount` since both are per-shop `short[4]`s read/reset
  identically, but confirmed distinct by their separate declarations and
  the fact `interactionCount` is only ever incremented, never compared.
  No increment site for `unconfirmedCooldownH` was found in this file, so
  its producer is external (most likely a per-tick decay from `ESGame`/
  the game-canvas-equivalent `e`, neither renamed yet).
- Action 4 (item-based quest progress) has two real, distinct branches:
  category-15 items reduce `unconfirmedCooldownH` by their raw (not
  bit-extracted) `questFlags` column value, while category-11 ("gift")
  items are the real quest-item delivery, gated by `questFlagsFor`.
- Two DISTINCT one-shot boolean arrays, both `boolean[7]` reset all-true
  (`reset()`, was `b()`): `firstVisit` (was `q`, gates every shop's own
  first-greeting line, shops 0-3 AND 4/5) and `questRewardClaimable` (was
  `b`, shops 0-3 only, consumed by action 6 -- clears dungeon tile bit 32
  at that shop's own world position, the same bit the Warden mechanic
  uses at a different position).
- **Beneca** (shop 4) and **Helga** (shop 5) each run their own separate
  spendable-points economy (`benecaPoints`/was `a`, `helgaPoints`/was
  `g`) -- Helga's action branches (8=learn/charge an equipped item via
  `Item.isEquipmentCategory`+`Player.isItemCharged`/`initializeItemCharge`
  -- confirmed independently while renaming `Item.java`, 9=safe-camping
  buff, 10=cure ailments, 11=warp to camp, 12=full HP+Magicka heal) are
  the closest match to dawnstar's Jakar's role, but on a completely
  separate points pool from Beneca's.
- **`Varus` (shop 6) is Stormhold-only**: a "Warden" world event with no
  dawnstar equivalent at all. `Shop.wardenArrives()`/`wardenLeaves()`
  (was `c()`/`a()`, zero-arg) print `"WARDEN VISITS!!"`/`"WARDEN LEAVES!!"`
  and flip dungeon tile bit 32 at Varus's own world position;
  `shouldWardenVisit(int)` (was `a(int)`) gates escalating visits at
  elapsed-counter thresholds 13/26/39. Varus's own dialogue branch (shop
  6 in `dialogue()`) is a simple 4-step lore reveal gated by
  `wardenVisitCount` and an unconfirmed `Player` field (`TODO_wardenLoreStep`,
  was `var0.m` -- not the same `m` as any other class's field of that
  letter).
- The rumor system (`rumorFor`, was `a(j,int)`) piggybacks directly on
  `Player.skills[step][0]` as an ask-counter and `Player.skills[13][2]`
  as an exp-like accumulator (both confirmed by role-match against
  dawnstar's `Player.skills[14][3]` layout) -- there is **no** separate
  `eventFlags`-based reveal table or `traitorIndex`-keyed offset table
  like dawnstar's `RUMOR_STRING_OFFSET`/`UNCONFIRMED_A`/`UNCONFIRMED_B`.
  No evidence of a "hidden traitor" subplot was found anywhere in this
  file; it may not exist in Stormhold, or may live entirely outside
  `Shop`.
- `dialogue()`'s greeting branch (shops 0-3, action 1) reads
  `player.coreStats[8] > 50` as a gate before rolling a random filler
  line -- same field dawnstar leaves as an unconfirmed "reputation/luck"-
  shaped slot (`Player.coreStats[8]`/was `E[8]`), reused here for the
  same apparent purpose.

**Six `Player` (`j`, not yet renamed) call sites are forward-referenced
by best-guess name, not yet confirmed**: two field reads with no
corroborating evidence beyond their use here (`TODO_modeFlag`, was
`var0.j`, checked `==1` in `isAdjacentToVarus`; `TODO_wardenLoreStep`,
was `var0.m`, Varus's own dialogue-step counter), one stat-quality method
(`TODO_itemQualityTier(int slot)`, was `var0.D(int)`, gates Helga's
gift-point reward tier), and one 3-arg method
(`TODO_trainSkill(int,int,int)`, was `var0.b(int,int,int)` -- note this
is a DIFFERENT overload from the confirmed 2-arg `rollShopOutcome(int,int)`,
used by Beneca's "spend 3 points" action). The remaining `Player`
references (`tileX`/`tileY` as `l`/`k`, `coreStats` as `U`, `ailmentMask`
as `A`, `skills` as `R`, `inventoryItemIds` as `H`,
`hasCampMark`/`warpToCampMark` as `x()`/`e()`,
`removeInventorySlot` as `y(int)`, `rollShopOutcome` as `b(int,int)`,
`giftPointsFound`/`rumorRevealStep` as `W`/`Y`, `safeCampingBuff` as
`f`, and `isItemCharged`/`initializeItemCharge` as `j(int)`/`h(int)`)
are corroborated either by exact role-match against dawnstar's confirmed
`Player` layout, or (the last pair) by an independent direct read of
`j.java` itself while renaming `Item.java` -- see
`Item.isEquipmentCategory`'s doc comment. None of this is authoritative
until `Player.java`/`j.java` itself is renamed and cross-checked.

## `i` -> `Dungeon` (one dungeon level's live state, PLUS generation)

Fully renamed, see `../src/Dungeon.java`. Confirmed via the `dungnamesin.dat`
loader (matches dawnstar's `Dungeon.NAMES`), but at 1076 lines -- roughly
double dawnstar's Dungeon-only 556 -- because **there is no separate
DungeonGenerator class here**: `c.java` (38 lines) is confirmed to be
something else entirely (a thin FullCanvas UI shim, see below), so the
room-carving/corridor-connection/monster-placement/chest-placement
algorithm lives directly on this class, fused with the live per-level
state dawnstar keeps separate. Full method-by-method writeup is in
`Dungeon.java`'s own header/doc comments; highlights:

- Two real, confirmed differences from dawnstar's generator: per-level RNG
  seed is `levelNumber * 5000` (dawnstar: `* 8000`), and level 37's last
  placed room gets a forced monster type 41 (dawnstar's own scripted
  end-game monster, for comparison, is type 42).
- `Monster.java`'s `unconfirmedAmbushHook(int)` placeholder (originally
  `i.c(int)`) is confirmed to be `Dungeon.spawnAmbushMonsters(int)`, a
  supplemental random-monster spawner triggered by `Monster.tick`'s
  ailment-2 status effect; `Monster.java` has been updated to use the real
  name.
- Cross-references `Shop`'s NPC-position fields directly
  (`Shop.SHOP_X[]`/`SHOP_Y[]`/`wardenPresent`) for the hub town's
  special-cased rendering/tile-marking paths (level 1 has no monsters, so
  NPC positions stand in where monster positions would otherwise go). One
  surprising, faithfully-preserved finding: the per-NPC gate on whether a
  marker is drawn in `sampleView()` is `Shop.questRewardClaimable[i]` --
  the same one-time "reward collectible" flag `Shop.java` documents for
  its quest-turn-in action, not a general visibility/active flag. Read
  directly off `i.java`, not assumed.
- Two things left genuinely unconfirmed rather than guessed (see the
  file's own header comment): the 2nd column of `NAMES` (`dungnamesin.dat`
  stores 2 UTF strings per level, only ever read as a pair), and tile bit
  3 (mask 8)'s exact purpose (blocks both `isWalkable()` and chest
  placement, consistent with dawnstar's own "no-spawn/special" bit3, but
  what marks it during generation wasn't pinned down further).

## `j` -> `Player` (player state, inventory, combat, spellcasting)

Fully renamed, see `../src/Player.java`. Confirmed via the `charin.dat`
loader (matches dawnstar's `Player.loadCharacterData`), by far the largest
class (2692 lines, close to dawnstar's `Player` at 2668). No mechanical-
rename blocker was found here the way dawnstar's own `j`/`Player.java`
needed one (bare `a.`/`k.` references are unambiguous once followed by
`.methodName(`, since no primitive field could support that) -- still
hand-traced one member at a time regardless, cross-checked against
`Monster.java`, `Dungeon.java`, and `Shop.java`'s already-renamed call
sites and forward-references. Same broad shape as dawnstar's `Player.java`
(`coreStats[10]`, `attributes[16]` as base+bonus pairs, `skills[14][3]`, a
24-slot inventory, a 9x5 `corridorView` + 13-slot `visibleObjects`
wall-occlusion cache, `tileX`/`tileY`/`currentLevel`/`facing` +
`pending*`/`prev*` movement staging, `campLevel`/`campX`/`campY`/
`campFacing`, `effectDurations[25]`, a two-format full/summary save pair)
but every name was derived independently, not copied -- real, confirmed
differences:

- **8 named ailments, but different names**: `Stone Blood`, `Delusions`,
  `Blind`, `Vampirism`, `Mana Burn`, `Grievous Harm`, `Terrified`,
  `Haunted` (vs. dawnstar's ice-themed `Frost Limbs`/`Snow Mirage`/
  `Troll Thirst`/`Glacier Curse`/`Grievous Harm`/`Terrified`/`Winter
  Worn` -- 3 of 8 identical, the rest reflecting a non-ice-themed game).
  Only 2 have confirmed countdown timer fields (`vampirismTimer`/
  `manaBurnTimer`, ailments 4/5, both set to 30000 by `Monster.tick`'s
  status-effect roll) vs. dawnstar's 3; a third, `terrifiedTimer`, is
  named by analogy only (ailment 7) with no confirmed write site found in
  this pass.
- **Two hub-town spawn positions, not one**: `setHubSpawnPosition(boolean
  isRespawn)` (was a 1-bool private `c(boolean)`) spawns a brand-new
  character at (9,10) but a death/respawn at a DIFFERENT point, (12,14) --
  dawnstar's own M11 port milestone only documents a single hub spawn
  position.
- **A genuinely surprising cross-system coupling**: `consumeLevelExp()`
  (was a bare no-arg `d()`) calls `Shop.clearQuestTurnInState()` (was
  `k.d()`) as a side effect of spending 10 level-exp on a rank-up --
  confirmed by reading both files together, not a transcription slip.
  Leveling up resets the 4 quest-shops' quest-turn-in progress.
- **`rollShopOutcome(int shopId, int action)`** (was a 2-int `b(int,int)`)
  reads `Shop.interactionCount[shopId]` (was `k.r[shopId]`) as a threshold
  in its chance formula -- confirmed via `Shop.dialogue()`'s own call
  sites (actions 2/3, the quest-turn-in steps). This retroactively
  resolves the "no confirmed caller" note `Shop.java`'s own pass left on
  `Shop.interactionCount`.
- **`Item.column(3, itemId)`** (Item's `questFlags` column) is confirmed
  here to double as an item's armor/weapon **magnitude** stat --
  `armorValue()` sums it (weighted 4/2/2/1/1, /10) across 5 equip slots
  exactly the way dawnstar's own `Item.java` documents for its
  `questFlags` field. Not a naming conflict; the same packed byte column
  serves both roles depending on item category, in both games.
- **`Dungeon.unconfirmedH`** (in `../src/Dungeon.java`, from the earlier
  `Dungeon` rename pass, flagged there as "never read anywhere traced in
  this file") **does have a write site after all**: `commitMove()` sets it
  `true` on every successful move into a level -- almost certainly
  dawnstar's own documented `visited` flag. Left as `unconfirmedH` in both
  files rather than renamed here, to avoid a cross-file rename outside
  this pass's own file; rename to `visited` the next time `Dungeon.java`
  is touched.
- **`Shop.java`'s `TODO_modeFlag`** (`isAdjacentToVarus`'s gate,
  `player.TODO_modeFlag != 1`) is just `player.currentLevel != 1` -- Varus
  can only be talked to from the hub town. Not fixed in `Shop.java` here,
  to avoid touching a file outside this pass's own scope.

**Left as `TODO_*`/`unconfirmed*` rather than guessed** (see `Player.java`'s
own header/field comments for each): `TODO_isInRegion(int,int,int)` (no
confirmed caller found at all), `unconfirmedIntField` (an `int` in the
save format with no confirmed meaningful use), `unconfirmedFlag2` (a
`boolean`, same situation), `staticUnconfirmedInt`/`staticUnconfirmedString`
(two `static` fields, no confirmed read/write site found anywhere in this
file), `enteredNewLevelZone`/`leftLevelZone` (set around a walkability
transition in `commitMove()`, exact purpose not pinned down), and
`rumorRevealStep`/`wardenLoreStep` (MEDIUM confidence only -- named from
`Shop.java`'s own forward references to two narrative-progression
counters, matched to the two fields that reset alongside `giftPointsFound`
in `resetState()`, which is consistent but not independently proven for
either specific field).

`Shop.java`'s own remaining `Player` placeholders this pass did NOT
resolve (left for whoever next touches `Shop.java`): `TODO_trainSkill`
(a 3-arg method used by Beneca's "spend 3 points" action -- no Player
method with a matching 3-arg shape was found in this pass) and
`TODO_itemQualityTier` (gates Helga's gift-point reward tier -- a
plausible candidate, reading the extended/high bits of
`inventoryItemData[slot]`, was not confidently confirmed).

## Remaining work (not yet renamed -- next passes)

(none left besides `ESGame.java` -- see its own section below)

## `g`/`c`/`h`/`e` -> `RawImage`/`ScreenCanvas`/`UIScreen`/`GameCanvas` (rendering/UI cluster)

None of these map onto dawnstar's `Screen`(`g`)/`LoadingScreen`(`h`)/
`GameCanvas`(`e`) by content despite matching letters for some -- confirmed
by direct reading, not assumed. `g`/`c` are fully transcribed
(`../src/RawImage.java`, `../src/ScreenCanvas.java`); `h`/`e` had their
outer structure fully confirmed and transcribed, but real internal-
rendering-method bodies left as explicit `TODO`/`UnsupportedOperationException`
stubs (signatures preserved) for a follow-up pass -- see each file's own
header comment for exactly what's covered vs. stubbed.

- **`g` -> `RawImage`**: a from-scratch indexed-color image decoder
  (width/height/transparency-flag/transparent-color/up-to-255-color
  palette/1-byte-per-pixel data, converted to ARGB4444-ish `short` pixels
  as it decodes) -- completely unrelated to dawnstar's `Screen` menu
  framework, no dawnstar analog.
- **`c` -> `ScreenCanvas`**: `extends com.nokia.mid.ui.FullCanvas`, a thin
  shim with no rendering of its own -- every method (`paint`/`addCommand`/
  `removeCommand`/`setCommandListener`/`keyPressed`) forwards to a held
  `UIScreen` instance.
- **`h` -> `UIScreen`** (named `UIScreen`, not `Screen` -- this class
  itself holds a field of the real `javax.microedition.lcdui.Screen`
  type, which would collide with a same-named class in the same default
  package). Fills the COMBINED role dawnstar splits into two classes,
  `Screen` + `LoadingScreen`: confirmed by its `mode` dispatch using the
  exact same mode-number split dawnstar's `g`/`h` pair uses (modes 3/5/6
  list/prompt-list, mode 4 message, modes 1/2/8-11 splash/progress-bar).
  Real architectural difference from dawnstar's fully hand-painted UI:
  this class builds real MIDP `List`/`Form`/`ChoiceGroup`/`StringItem`
  widgets, but only as typed data containers (their `append`/`get`/
  `getString` accessors) -- it NEVER displays them as Displayables; all
  actual painting is manual Graphics work, reading strings back out of
  those containers by hand. Double-buffered manually via a static
  offscreen `Image`, blitted onto the real Graphics each paint. Two
  `ScreenCanvas` instances are alternated between
  (`canvasInstances[0]`/`[1]` via `activeCanvasIndex`) -- purpose of the
  alternation itself not confirmed (possibly a period-appropriate device
  flicker workaround). A handful of fields are left as `unconfirmed*`/
  `TODO_*` placeholders (a constructor-supplied `screenGroup` int gating a
  repaint check against `ESGame.activeScreen`, two untyped scratch fields
  where only one's use -- "next screen after splash" -- was confirmed, one
  possibly-dead `Font`) -- see the file's own field-level doc comments.
- **`e` -> `GameCanvas`**: `extends com.nokia.mid.ui.FullCanvas implements
  Runnable`, with a `5x6x4 int[][][]` table in the exact shape of
  dawnstar's confirmed `GameCanvas.CORRIDOR_WALL_TABLE` -- this build's
  real first-person corridor-view renderer + HUD + main tick loop,
  separate from the `ScreenCanvas`/`UIScreen` pair (which handle all
  menus/dialogs/splash via the completely different LCDUI-widget-backed
  architecture above). Confirmed and fully transcribed: the message-popup
  string table (same `MSG_*` convention as dawnstar, content-identical for
  every shared entry, **plus two Stormhold-only entries**
  `MSG_WARDENS_CAMP`/`MSG_OUTER_CAMP` tied to the Warden mechanic; a
  duplicate of `Shop.NAMES[0..6]` for on-screen NPC name display; the
  constructor; the top-level `paint()` dispatch (dead screen / camp screen
  / main game view); the full 250ms `run()` tick loop (camping state
  machine, death/respawn sequence, the Warden-visit check, per-tick/
  per-second status ticks, frame pacing); `keyPressed`/`keyReleased`
  (numeric-key hotbar/strafe hotkeys, `*` minimap toggle, arrow-key
  movement); and game-thread start/stop. **Not yet transcribed** (left as
  signature-preserving stubs): roughly 15 private paint helper methods
  that do the actual pixel-level wall/floor/monster/object/HUD rendering
  (~1400 of the file's 1823 lines) -- this needs the same full hand-trace
  treatment dawnstar's own `e`/`GameCanvas.java` pass got, not yet done
  here.
  - **Field-name/class-name collision, same trap dawnstar's CLASS_MAP.md
    documents for its own `e`/`j`**: this class declares its own fields
    `nearbyMonsterScratch`/`targetMonster` (both type `Monster`, originally
    named `k`/`j`) -- identical original letters to the `Shop`(`k`)/
    `Player`(`j`) classes. Confirmed by cross-checking against Shop's
    already-renamed `shouldWardenVisit`/`wardenArrives`: `run()`'s
    `k.a(this.ax.W)`/`k.c()` calls are static calls to the **`Shop`
    class**, not reads of this file's own `nearbyMonsterScratch` field --
    printed decompiled text alone cannot disambiguate these. Any
    remaining un-transcribed method should be re-checked for the same
    ambiguity before trusting a bare `k`/`j` token.
  - Six more `Player` (`j`, not yet renamed) references are forward-
    referenced by placeholder name in the transcribed portions
    (`TODO_someCorridorFacingStat`, `TODO_dialogueOrScreenActive`,
    `TODO_wardenOrTownResponse`/`TODO_onCampInterrupted`,
    `TODO_setResting`, `TODO_respawn`/`TODO_respawnLevel`,
    `TODO_isSlotEquipped`/`TODO_removeInventorySlot`,
    `TODO_resetState`/`TODO_hubSpawnFlag`, `TODO_wardenFlag_u`/
    `TODO_outerFlag_O`, `TODO_wardenElapsedCounter_W`,
    `TODO_npcProximityThreshold_m`, `TODO_isDead`) -- none authoritative
    until `Player.java`/`j.java` is renamed and cross-checked. Two
    `ESGame` references are similarly placeholder
    (`TODO_aV`/`TODO_e`/`TODO_ag`/`TODO_a`/`TODO_j`, all on the `game`
    field).

## `ESGame.java`

Renamed, see `../src/ESGame.java` (2314 lines originally). Unlike
dawnstar's `ESGame` (readable member names, a light retype pass), this
build's member names were **not** readable -- needed the same full
hand-trace treatment as `e`/`j`. Confirmed overall shape matches dawnstar's
own `ESGame` despite every identifier being independently derived: owns
every `UIScreen` instance as a named field, owns the world's static
per-level arrays, `getResource`/`createImage`, a `run()` state machine over
`helperThreadState` (same exact convention as dawnstar's own
`helperThreadState`: 1=unused legacy download path, 2=`runAppload`,
4=`createNewGame`, 5=save, 6=load), and the same startup order
(`runAppload` -> `allocateESGame` -> `allocAllDungeons` -> `allocateAllUIs`).

**How most of the ~50 `UIScreen` fields got confidently named**: many of
the screen-factory methods still carry their *original* debug-`println`
names almost verbatim (`"Start of newSkillsListUI"`, `"Start of
newInventoryItemUI: getting item "`, `"In getGameAdvancementLevel,
giftPoints = "`, `"In checkOpenAndPopulateDungeons, gameAdvLevel = "`, `"In
killMonster! dungid is "`, ...) -- these were treated as strong, direct
evidence for the method's real name, not just a hint, wherever present.

Real, confirmed differences from dawnstar's `ESGame`:

- **No `datfiles.lmp` archive.** `getResource(String)` reads a resource
  straight off the classpath and buffers it fully into a `DataInputStream`
  -- see `Util.java`'s header comment; `extracted/` has every `*in.dat`
  table as its own top-level resource.
- **Dungeon geometry and monster image file names loaded directly by
  `ESGame` itself** (`geomRows`/`monsterImageFileNames`, was `ai`/`T`),
  not delegated to a data-table class the way dawnstar's `ESGame` delegates
  `geomin.dat` to `DungeonGenerator`.
- **The hub town's fixed 19x19 tile template is built here**
  (`buildHubTileTemplate()`, was `E()`) rather than inside a separate
  `DungeonGenerator` class -- consistent with `Dungeon.java`'s own finding
  that there is no separate generator class anywhere in this codebase.
- **Two `Player` fields, not one**: `player` (the live/active character)
  and `newCharacterDraft` (a scratch `Player` used during character
  creation, assigned into `player` only once the player confirms their
  class choice on `classConfirmUI`).
- **A from-scratch `RecordStore`-based save format**, not documented
  anywhere in dawnstar's own `ESGame` writeup: one record for `player`
  (`Player.toBytes(true)`/`fromBytes(_, true)`), one "master lists" record
  (`writeMasterLists`/`readMasterLists`, was `k()`/`b(byte[])` -- packs
  `Item.nextSpawnId`, `Monster.nextSpawnIdCounter`, and most of `Shop`'s
  static per-shop state), then 37 records each for monsters/chests/dropped
  items (`writeAllLevelRegistries`/`readAllLevelRegistries`, was
  `a(RecordStore)`/`a(RecordStore,int)`). Confirmed real difference from
  `Monster.java`'s own registry-keying note: **monsters are keyed by
  `spawnId`** (`String.valueOf`) but **chests are keyed by tile position**
  (`Util.posKey`) in their respective `Hashtable`s -- two different keying
  conventions in the same save format, read directly off this file, not
  assumed. Dropped items are unkeyed (`Vector`).
- **`checkOpenAndPopulateDungeons(int)` vs. `openAndPopulateAllUpTo(int)`**
  (was `n(int)`/`i(int)`): two distinct "open a zone's dungeons" methods
  that call two different `Dungeon` methods per level (`Dungeon.TODO_e()`
  vs. `Dungeon.TODO_h()`, both still unconfirmed pending a closer
  `Dungeon.java` cross-check) -- the former is used incrementally by
  `createNewGame()`, the latter in one pass by the load-game path when
  catching up a save that's several zones ahead of a fresh game's zone 0.

**Integration pass (this session, after the initial ESGame hand-trace):**
every `TODO_*` cross-file placeholder above was resolved by cross-reading
`Player.java`/`GameCanvas.java`/`Dungeon.java`/`UIScreen.java`/`Shop.java`
side-by-side with `ESGame.java` (and, where none of those already had a
matching real name, tracing the original single-letter call site by hand
in `decompiled/j.java`/`e.java`/`i.java`/`h.java`/`k.java`/`ESGame.java`
directly -- never guessed to make something compile). `../src/` now
**compiles cleanly** (`javac -cp tools/midp-stubs/* -d out src/*.java
src/ngame/midlet/*.java`, zero errors, only the expected `new Integer(int)`
deprecation warnings the original code's own pattern produces on a modern
JDK). Notable findings from this pass, beyond simple retypes:

- `GameCanvas`'s `e()`/`b()` pair (was mis-identified by an earlier pass as
  a "level-up" trigger named `onPlayerDeath`) are actually the exact
  `threadStarted` on/off counterpart pair -- renamed `pauseTicking()`/
  `resumeTicking()`. The run()-loop call site that originally looked
  "death"-shaped is really "freeze ticking while the level-up screen is
  shown", which does still make sense once `e()`'s real one-line body
  was read.
- `Dungeon.TODO_e()`/`TODO_h()` are genuinely different methods:
  `populate()` (spawn fresh monsters+chests, used by incremental
  new-game zone-opening) vs. `refreshTileFlagsFromRegistries()` (resync
  tile bits from already-saved registries, used when a load jumps
  straight to a later advancement zone) -- confirmed by reading both
  method bodies in `decompiled/i.java`, not assumed to be the same call.
- `UIScreen`'s `stateObjectS`/`stateObjectC`/`unconfirmedN` placeholders
  are `backTarget`/`nextScreen`/`contextIndex` -- the same two
  ESGame-only-touched fields dawnstar's own `Screen` class documents,
  confirmed here by the exact same evidence (set at construction, never
  read/written from inside `UIScreen.java` itself, only by `ESGame`'s
  navigation code).
- `Shop.rumorFor(Player,int,int,int)` doesn't exist -- every 4-arg call
  the earlier pass guessed was actually a call to the already-renamed
  4-arg `Shop.dialogue(Player,int,int,int)` (confirmed: `dialogue()` and
  the real, 2-arg `rumorFor(Player,int)` are two genuinely different
  overloads in the original, both named `a` by the decompiler).
- Added a handful of small methods that were genuinely missing (not just
  misnamed), each ported from its exact original body rather than
  invented: `Player.isSlotEquipped(int)`, `Player.itemSubtypeAtSlot(int)`,
  `Player.canUseItem(int)`, `Player.applyRestRecovery(boolean)` (the real
  camp-rest HP/Magicka/Fatigue/ailment-cure logic, confirmed to clear the
  3 temporary combat buffs via a field-declaration-order match),
  `Dungeon.spawnAmbushMonsterNearPlayer(Player)` (the "a monster appears
  when your rest is disturbed" behavior), `ESGame.readPerLevelRecords`
  (the real per-level monster/chest/dropped-item RecordStore reader the
  save/load path needs, previously just an arity-mismatched call).
- Two low-confidence, low-stakes methods were deliberately left as thin,
  explicitly-flagged placeholders rather than fully hand-traced, since
  each only feeds an already-known-incomplete or debug-only consumer:
  `Player.questShopAtPendingTile()` (was `r()`, feeds a still-stubbed
  `GameCanvas` pixel-rendering method) and `Player.debugSummary()` (was
  `K()`, feeds ESGame's dev-only "Debug" Form).

**Still open for a future pass** (does not block compilation): `Dungeon`'s
own `unusedHook1`/`unusedHook2`-shaped no-ops if any remain, `loadHelpTopicBodies()`
transcribed only through help topic 4 of 12 (an explicit gap, not silently
truncated), and the ~15 `GameCanvas` pixel-rendering methods `GameCanvas.java`'s
own header comment already flags as unread stubs -- none of these were
in scope for "make it compile," only for "finish the behavior."

## Open questions carried into `Monster.java` (see its own doc comments)

`Monster.unconfirmedFlag` (was `i`), `Monster.scratch[]` indices beyond
5-8, `Monster.unconfirmedTimestamp` (was `k`, likely a timestamp given the
8-byte width) -- same open status dawnstar left these in, not re-resolved
here.
