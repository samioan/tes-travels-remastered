# Stormhold -- roadmap

## The target

`roms/TEST-Stormhold.jar` -- a J2ME MIDlet (`MIDP-1.0`/`CLDC-1.0`),
MIDlet-Vendor **Vir2L Studios** (the same studio behind
*The Elder Scrolls Travels: Shadowkey* on N-Gage -- see the sibling
`shadowkey-decomp` project), MIDlet-Name "The Elder Scrolls". Entry point
`ESGame` (`MIDlet-1: The Elder Scrolls, icon3650.png, ESGame`).

`ESGame` extends `ngame.midlet.a` -- structurally identical to how
`dawnstar/`'s `ESGame` extends `ngame.midlet.RegisteredMIDlet`; that class
is unobfuscated over there, so `ngame/midlet/a.java` here is almost
certainly the same class with its real name stripped. This is Vir2L's
in-house "ngame" MIDP engine, shared across (at least) these two titles --
diff the two once both are read through.

## Status

**Phase 0 (this setup) is done:** jar unpacked (`extracted/`), decompiled
to readable-but-unrenamed Java (`decompiled/`, 13 files) via
[`../../tools/decompile.py`](../../tools/decompile.py). Clean recovery,
same as dawnstar -- plain JVM bytecode, no native/ARM step.

**Phase 1 (done): read through and rename.** See
[`CLASS_MAP.md`](CLASS_MAP.md) for the full writeup. All 13 decompiled
classes plus `ngame/midlet/a.java` are renamed, with hand-written source in
`../src/`, and the whole tree **compiles cleanly** (`javac` against
`tools/midp-stubs/*`, zero errors). Several classes turned out field-for-
field identical to dawnstar's own equivalents (same shared "ngame" engine),
though real Stormhold-specific content/mechanic differences were found and
documented rather than assumed away -- a Stormhold-only "Warden visits/
leaves" NPC mechanic, monsters keyed by spawn id rather than tile position
in the live registry, a from-scratch indexed-color image format
(`RawImage`), and a rendering/UI architecture (`ScreenCanvas`/`UIScreen`/
`GameCanvas`) that does NOT map onto dawnstar's Screen/LoadingScreen/
GameCanvas split by content, among others. Left for later phases (not
blocking phase 2/3): `GameCanvas.java`'s tick-loop helpers
(`showMessage`/`tickStatusCountdowns`/`tickPerSecond`/
`rollCampInterrupted`/`tickMovementAndAI`/`setSomeFlag`) and two
brand-new `q()`/`p()` minimap-populate methods phase-3 M22 turned up --
its pixel-rendering `paint*` methods are all transcribed for real now
(phase-3 M21/M22, including two real mapping bugs found and fixed along
the way, see `PORT_ROADMAP.md`), `ESGame.
loadHelpTopicBodies()`'s remaining help topics, and a set
of individually-flagged lower-confidence names throughout (see each file's
header comment and CLASS_MAP.md).

**Phase 2 (done): asset formats.** See [`ASSET_FORMATS.md`](ASSET_FORMATS.md)
for the full writeup. Turned out to already be substantially resolved as a
byproduct of phase 1's class-by-class read-through (every loader lives in
one of the 13 renamed classes) -- this phase was mostly writing that up
plus a couple of remaining gaps:
- All 9 flat game-data tables (`itemsin.dat`, `monstersin.dat`,
  `spellsin.dat`, `dungnamesin.dat`, `geomin.dat`, `droppeditemsin.dat`,
  `charin.dat`, `monsterfilenamesin.dat`, `npcstrings.dat`) have confirmed
  record layouts, each column-oriented (`<u16/u32 count>` then one
  contiguous array per field), not the flat `itemsin.dat`-per-record style
  originally guessed.
- **Correction to this doc's own earlier guess:** the `.cus` files are
  *not* 3D mesh data despite the naming pattern reading that way -- they're
  a from-scratch 2D indexed-color raw sprite format (`RawImage.java`),
  fully decoded (width/height/transparency flag/palette/1-byte-per-pixel
  indices). `far`/`mid`/`near` in filenames is 2D sprite LOD, not mesh LOD.
- Every non-`.class` resource in `extracted/` (75 files) has a confirmed
  loader call site; nothing orphaned. Left open: a handful of individual
  table columns/groups whose exact meaning isn't pinned down yet (doesn't
  block phase 3) -- see `ASSET_FORMATS.md`'s own "what's actually left"
  section.

**Phase 3 (in progress): PC port.** See
[`PORT_ROADMAP.md`](PORT_ROADMAP.md) for the full milestone-by-milestone
writeup, following dawnstar's own port precedent. M0 (scaffold), M1 (real
tick loop/backbuffer/window, the confirmed 250ms cadence), M2 (asset
foundations -- `BinaryReader`/`AssetRoot` plus the `ItemDatabase`/
`SpellDatabase` loaders), M3 (`MonsterDatabase`/`DungeonGeometry`,
`monstersin.dat`/`geomin.dat`), M4 (`CharacterData`, `charin.dat`), M5
(bit-exact `java.util.Random`, JVM-verified), M6 (procedural dungeon
generation, self-consistency-verified across the hub town plus 6 standard
levels), M7 (`RawImage`'s indexed-color `.cus` sprite decoder, verified
against all 37 real sprite files), M8 (the Warden visits/leaves world
event, including a confirmed tile-write index bug preserved rather than
fixed), M9 (player character creation for all 7 classes, including a
real phase-1 renaming bug found in `../src/Player.java`'s `equipItem()`
and fixed at the source rather than preserved), M10 (player movement,
including a confirmed dead-code finding in `leftLevelZone` and two
corrected M6-era modeling gaps around `Dungeon.populated`/`visited`), M11 (NPC dialogue text from `npcstrings.dat`, data only -- the dispatcher
logic is deferred to a later milestone), M12 (general-purpose player
inventory management, including a confirmed original-game sign-extension
quirk in the dropped-item/chest packed-value round trip), M13
(combat resolution primitives -- skill/attack/armor stat math and the
rollOutcome hit-tier roll, independently cross-checked against a fresh
java.util.Random reimplementation), M14 (a live Monster runtime --
spawn/stat/move/chase/onDeath -- plus the two combat entry points that
need both Player and Monster, `PlayerAttack`/`MonsterTick`, including a
confirmed real Stormhold-specific divergence in `Monster.chase()`'s
void return type and internal range gate, absent from dawnstar's own
equivalent), and M15 (leveling -- gainSkillExp/tryRankUpSkills/
consumeLevelExp/the level-up attribute-point allocation ESGame.java
performs inline rather than as a named Player method -- which finally
unblocks the skill-exp awards M13/M14 had to defer, plus a confirmed
finding that levelUpAttributeFlags bits are never cleared once spent),
M16 (a live per-level Monster/chest/dropped-item registry --
WorldRegistry/DungeonRuntime, spawnId-keyed for monsters same as M14's
own confirmed keying, position-keyed for chests -- plus a necessary
GeneratedLevel data-model addition, its own room-rectangle list, needed
by spawnAmbushMonsters' real room-bounded placement), M17 (wiring
that registry into player_movement's dropped-item auto-loot,
player_inventory's DropInventoryItem, and combat_resolution's
target.store()/spawnAmbushMonsters call sites -- including a real,
confirmed bit-test asymmetry between the original's "one item on this
tile" and "several items on this tile" dropped-item-pickup branches,
preserved rather than unified), and M18 (registering M6's own
generation-time monster/chest spawn lists into the registry itself,
closing the last gap of the 3-way registry split -- including a
confirmed-unavoidable simplification where a random 2-bit "tier bits"
value the original packs into a chest record byte has no surviving data
to reconstruct from, and a confirmed subtlety that the record's
low/high item-id byte split uses a different, unrelated condition than
the loot roll's own packing rule), and M19 (the last two CommitMove side
effects -- Shop.wardenPresent's on-any-step clear and the level-37-entry
forced-respawn/heal of the type-41 "roaming" monster -- including a
confirmed inconsistency where the movement-triggered Warden clear is a
direct flag write, never actually calling the buggy tile-mutating
wardenLeaves() at all), and M20 (the player save format -- a new
BinaryWriter, Player's own full=true save-format serialization, and
Monster's second readFrom/writeTo stream serialization, including a
confirmed divergence from dawnstar's own finding: Stormhold's readFrom/
writeTo encode the exact same 28 fields in the exact same order as its
packed toBytes/fromBytes format, not a genuinely different layout), and
M21 (corridor wall-segment selection logic, the start of the rendering
side -- but first a real discovery: GameCanvas.java's own ~15 pixel-
rendering methods were never actually transcribed from decompiled/e.java
at all, only left as signature-only stubs, unlike dawnstar's own
already-complete GameCanvas.java. M21 did the missing phase-1 work for
exactly one of those stubs, paintWalls(), confirmed it as dawnstar's own
paintCorridorWalls() equivalent with two real simplifications -- no
bit-64 gate-tile branch, a single shared floor/wall texture pair, not
per-level ice/plain textures -- then ported its selection logic to C++
as data only, plus found that at least one OTHER stub's existing
placeholder mapping was never verified and is likely wrong), and M22
(the rest of GameCanvas's stubbed paint methods -- pure phase-1 Java
transcription, no C++ yet since the pixel compositor these would feed
doesn't exist -- confirming and fixing, rather than just flagging, THREE
real mapping bugs: paintFloor()/paintObjects() had their real bodies
swapped [the true paintObjects() renders chests/dropped items;
paintFloor() never existed as a separate concept, paintWalls() already
paints the floor itself], likewise paintMessagePopup() and the old
paintUnknown_l() [the flash-overlay renderer had stolen the message-
popup's name], and paintHotbar1()/paintHotbar2() turned out to be the
two minimap zoom levels, gated by the wrong field [hotbarActionSet
instead of hotbarContext] since the very first partial pass), and M23
(Backbuffer::Blit(), the real alpha-test/clip/mirror RawImage
compositor GameCanvas's own drawRawImageFull()/drawRawImageFrame() [M22]
both reduce to -- verified against a real M7-confirmed .cus sprite
pixel-by-pixel, not just synthetic data), and M24 (DecodedImage, a
vendored-stb_image PNG decoder resolving M21's own open question --
floorTexture/wallTexture turn out to be plain .png files, confirmed by
grepping ESGame.java's own asset-loading call sites, not M7's RawImage
format -- plus a matching Backbuffer::Blit(DecodedImage) overload,
cross-checked against the real floor3.png/newwallsnok.png files: their
decoded widths [36px, 144px] match paintWalls()'s/drawWallSegment()'s
own tiling constants exactly), and M25 (GameRenderer, the first real
end-to-end pixel render this port has -- a live PlayerState::corridorView
field wired through PlayerMovement::CommitMove at all three of
Player.commitMove()'s own refreshCorridorView() call sites, plus
GameCanvas.paintWalls() itself finally drawing real pixels into a real
Backbuffer by combining M21's CorridorRenderPlan selection logic with
M23/M24's Blit() compositors, cross-checked pixel-by-pixel against real
floor3.png/newwallsnok.png files and a real M6-generated level across
36,608 floor pixels and 105,565 opaque wall pixels, zero mismatches),
M26 (StatusBarPlan/GameRenderer::RenderStatusBars, paintStatusBars() --
the HP/Magicka/Fatigue HUD bars, the cheapest remaining paint method
since it's entirely self-contained in PlayerState/CharacterData with no
new asset loading; confirmed a real, preserved asymmetry where only the
Fatigue bar's width is clamped to 40, HP/Magicka have no clamp at all),
and M27 (VisibleObjects, the 13-slot corridor-view object cache --
Player.refreshVisibleObjectSlots()/refreshVisibleObjects()/
resolveVisibleObjectSlot()/placeVisibleObject(), data model only, no
pixels, unblocking paintObjects()/paintMonsters() the same way M21
unblocked paintWalls(); confirms, rather than just flags, M22's own
byproduct finding that a monster's unconfirmedFlag really means "has
ever been seen", permanently, matching dawnstar's own identical finding;
also fixed a real doc-comment-only kind-label swap in Player.java's own
resolveVisibleObjectSlot() header comment found while reading it), and
M28 (VisibleObjectRenderer -- the actual sprite drawing off M27's
visibleObjects, paintObjects()/paintMonsters() themselves, against
Backbuffer::Blit() (M23/M24); found a real, reachable, previously-
unknown original-game crash bug by counting a table's rows against its
own header comment's claim: unconfirmedTable_a's comment said "41 rows"
but it actually has 31, and typeIndex 32-40 -- confirmed real,
spawnable monster types at deep dungeon tiers, not dead code -- index
past the end of it if they ever reach the player's near-view slot;
preserved as a thrown exception in the port, not silently read out of
bounds), and M29 (paintHud()'s own SELECTION logic only --
IsAdjacentToVarus/IsNpcDialogueDue/ResolveHudIconSet, data only, no
pixels: paintHud() itself needs a filled-rounded-rect primitive and a
character-glyph text primitive neither of which exist in this port yet,
so those two are deliberately deferred rather than built under time
pressure). Every OTHER paint method GameCanvas needs -- HUD's own pixel
drawing, minimap, message popups -- still needs live state or missing
primitives this port doesn't have yet, and there's still no actual game
loop calling any of this) are done, all
verified against real extracted data/ground truth. Given
the shared Vir2L "ngame" engine with dawnstar, small
identical pieces (tick cadence, backbuffer, window, binary reader/writer) are
copied rather than shared for now -- factoring out a real cross-project
engine library is deferred until Stormhold's own port has enough
milestones to show what's actually worth sharing (see `PORT_ROADMAP.md`'s
own note on this). Real Stormhold-specific divergences from dawnstar found
along the way, not blocking but worth tracking: dungeon generation is
fused directly onto `Dungeon` (no separate `DungeonGenerator` class) and
stairway placement is procedural (`carveStairwell`) rather than
fixed-coordinate.
