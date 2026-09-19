#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_map>
#include <vector>

#include "assets/monster_database.h"
#include "monster/monster_runtime.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace stormhold {

// Dungeon.sampleCorridorView()'s output shape (`byte[9][5]` in the
// original) -- see DungeonRuntime::SampleCorridorView/ViewGridAt below.
// [i][depth], i in 0..8, depth in 0..4.
using CorridorViewGrid = std::array<std::array<uint8_t, 5>, 9>;

// Packs a tile position into a single map key -- a plain-int stand-in
// for Util.posKey(x,y)'s "x,y" string (`Dungeon.storeChest`/
// `removeChest`'s real key). The string key has no behavioral
// significance beyond giving each (x,y) a unique Hashtable key; both
// level dimensions are always well under 4096, so this has enormous
// headroom.
inline int32_t PackTileKey(int x, int y) { return x * 4096 + y; }

// Renamed-source counterpart of ../../../src/Dungeon.java's LIVE
// per-level state -- ESGame.monsters[]/chests[]/droppedItems[], the
// Hashtable/Vector registries every other module's own class comment has
// been flagging as missing since M10/M12/M14 ("no live per-level
// registry exists yet"). One WorldRegistry holds every level's
// registries, indexed by levelNumber-1 like a `std::vector<GeneratedLevel>`
// itself.
//
// Deliberately NOT wired into player/player_movement.h,
// player/player_inventory.h, or combat/combat_resolution.h's own
// existing simplifications in this milestone -- those live in
// stormhold_player/stormhold_combat, neither of which may depend on this
// module (stormhold_dungeon depends on THEM, via stormhold_monster/
// stormhold_world -- depending back would cycle). Wiring those call
// sites to actually populate/consume a WorldRegistry, and registering
// M6's generation-time GeneratedMonsterSpawn/GeneratedChestSpawn output
// into one, are both jobs for a later milestone -- this one only
// provides the registry and Dungeon.java's own registry-management
// methods, faithfully, ready for that wiring. Same 3-way split dawnstar's
// own port uses (its M22, this milestone's direct precedent -- M23/M24
// there do the wiring/generation-registration this milestone leaves
// undone).
struct WorldRegistry {
    // ESGame.monsters[]: SPAWN-ID-keyed 28-byte Monster.toBytes() records
    // (monster/monster_runtime.h's ToBytes/FromBytes) -- confirmed
    // Stormhold-specific keying (M14's own finding, reconfirmed while
    // reading Dungeon.java directly for this milestone): unlike
    // dawnstar's own WorldRegistry.monsters (position-keyed), Stormhold's
    // real Monster.store() keys by `String.valueOf(spawnId)` alone.
    std::vector<std::unordered_map<int16_t, std::array<uint8_t, 28>>> monsters;
    // ESGame.chests[]: POSITION-keyed (PackTileKey) 8-byte chest records
    // (Dungeon.placeChests(): [0]/[1]=x/y, [2]=ALWAYS 0 -- a confirmed
    // dead byte, `first` is already flipped false by the time this write
    // runs, see world/dungeon_generator.h's GeneratedChestSpawn doc
    // comment for the same finding -- [3]=tier | a 2-bit random "flavor"
    // in the top bits with no confirmed reader, [4]=itemId low byte,
    // [5]/[6]=spawnId high/low byte, [7]=itemId high byte (only
    // meaningful when [4]==86)).
    std::vector<std::unordered_map<int32_t, std::array<int8_t, 8>>> chests;
    // ESGame.droppedItems[]: an unordered list (Vector, not
    // position-keyed -- multiple items can litter the same tile) of
    // 7-byte records, `int8_t` (not `uint8_t`) to match
    // player/player_inventory.h's own signed-byte convention -- see that
    // header's TryPickUpItem/DropInventoryItem doc comments for the
    // confirmed sign-extension quirk a record's packed value can trigger.
    std::vector<std::vector<std::array<int8_t, 7>>> droppedItems;

    explicit WorldRegistry(size_t levelCount) : monsters(levelCount), chests(levelCount), droppedItems(levelCount) {}
};

class DungeonRuntime {
public:
    // Monster.store(): registers `m` into `world`, keyed by its own
    // spawnId, using m.dungeonLevel (NOT a separate levelIndex parameter)
    // to pick the slot -- matches the real store()'s own
    // `this.dungeonLevel` read exactly.
    static void StoreMonster(WorldRegistry& world, const MonsterState& m) {
        world.monsters[static_cast<size_t>(m.dungeonLevel - 1)][m.spawnId] = MonsterRuntime::ToBytes(m);
    }

    // ESGame.killMonster(dungeonLevel, spawnId): removes spawnId's
    // record from `level`'s registry slot and clears its last-known
    // tile's monster-presence bit (2). Throws std::runtime_error if
    // spawnId isn't actually registered -- the real Java reads
    // `record[4]`/`record[5]` (tileX/tileY) BEFORE its own `if (record
    // != null)` null-check (confirmed directly, ESGame.java's own
    // killMonster), a real latent NullPointerException for exactly this
    // case. Not reproducible as a genuine crash in C++ without
    // deliberately dereferencing something invalid, so this surfaces the
    // same "should never happen, but the original doesn't guard it
    // either" condition loudly instead -- same discipline
    // player/player_movement.h's ComputeMoveTarget already established
    // for its own no-neighbor edge case.
    static void RemoveMonster(GeneratedLevel& level, WorldRegistry& world, int16_t spawnId);

    // Dungeon.monsterAt(x,y): the live Monster at (x,y) if the tile's
    // monster-presence bit (2) is set and it isn't a wall (bit 1) --
    // scans the WHOLE level registry, since (confirmed directly) the
    // original has no position index for monsters either, only the
    // spawnId-keyed Hashtable.
    static std::optional<MonsterState> MonsterAt(const GeneratedLevel& level, const WorldRegistry& world, int x,
                                                  int y);

    // Dungeon.storeChest(record): registers an 8-byte chest record,
    // position-keyed, and sets its tile's chest-presence bit (16).
    static void StoreChest(GeneratedLevel& level, WorldRegistry& world, const std::array<int8_t, 8>& record);
    // Dungeon.removeChest(record): removes the chest at (record[0],
    // record[1]) once its tile is NOT a wall AND still has the
    // chest-presence bit (16) set -- both guards ported exactly.
    static void RemoveChest(GeneratedLevel& level, WorldRegistry& world, const std::array<int8_t, 8>& record);

    // Dungeon.addDroppedItem(record): registers a 7-byte dropped-item
    // record and sets its tile's dropped-item-presence bit (4).
    static void AddDroppedItem(GeneratedLevel& level, WorldRegistry& world, const std::array<int8_t, 7>& record);
    // Dungeon.removeDroppedItem(record): removes one dropped-item record
    // once its tile is NOT a wall AND still has the dropped-item-presence
    // bit (4) set, THEN clears that bit only if no dropped items remain
    // at that tile afterward (there can be several per tile).
    // SIMPLIFIED: the original's Vector.removeElement() matches by Java
    // reference identity (the exact same byte[] instance); this port has
    // no pointer-identity equivalent for a value type, so it removes the
    // first record equal BY CONTENT instead -- indistinguishable from the
    // original unless two genuinely distinct dropped items ever land on
    // the same tile with a bit-for-bit identical 7-byte record (their
    // spawnId bytes alone already make that practically impossible).
    static void RemoveDroppedItem(GeneratedLevel& level, WorldRegistry& world, const std::array<int8_t, 7>& record);
    // Dungeon.countDroppedItemsAt(x,y).
    static int CountDroppedItemsAt(const WorldRegistry& world, int levelIndex, int x, int y);
    // Dungeon.firstDroppedItemAt(x,y): the first-registered match, or
    // nullopt where the original returns null.
    static std::optional<std::array<int8_t, 7>> FirstDroppedItemAt(const WorldRegistry& world, int levelIndex, int x,
                                                                     int y);
    // Dungeon.droppedItemsAt(x,y): every record at (x,y), in registration
    // order (Dungeon.java's own Vector iteration order).
    static std::vector<std::array<int8_t, 7>> DroppedItemsAt(const WorldRegistry& world, int levelIndex, int x,
                                                                int y);

    // Dungeon.refreshTileFlagsFromRegistries(): rebuilds the monster (2)/
    // chest (16)/dropped-item (4) presence bits on every tile of `level`
    // from `world`'s live registries -- used after a save/load or
    // similar full-state resync. SIMPLIFIED: skips the hub-town
    // Shop.wardenPresent bit-32 refresh the original also performs here
    // -- no live Shop/Warden state is wired to a WorldRegistry yet (M8's
    // WardenState is its own standalone, caller-supplied object, not
    // part of this registry).
    static void RefreshTileFlags(GeneratedLevel& level, const WorldRegistry& world);

    // Dungeon.spawnAmbushMonsters(count): the "swarm curse" ailment's
    // on-hit side effect (combat/combat_resolution.h's MonsterTick
    // ailment-2 branch, deferred there since M14 pending exactly this
    // registry) -- drops `count` EXTRA monsters at random walkable
    // positions inside random rooms (not necessarily room centers),
    // registering and store()-ing each. Reads `level.rooms` (added this
    // milestone specifically for this method -- PopulateLevel's own room
    // list wasn't exposed on GeneratedLevel before M16).
    //
    // The RNG draw order per retry attempt (room index, then monster
    // type via MonsterRuntime::PickMonsterType, then x, then y) matches
    // Dungeon.java's own do-while loop exactly, including re-rolling ALL
    // FOUR on every rejected (non-walkable) attempt, not just the
    // eventual successful one -- this is load-bearing for determinism of
    // whatever RNG draws come after this call. `spawnIdCounter` does
    // NOT match one real-game behavior: the original's
    // `Monster.spawn(rng,...)` burns a real spawnId on every rejected
    // attempt too (nextSpawnId() runs before the walkability check even
    // resolves), while this port only consumes one spawnId per
    // ACTUALLY-placed monster. Deliberate: spawnId's specific numeric
    // value has no confirmed observable effect anywhere in the source
    // (it's only ever used as a Hashtable/WorldRegistry key), so this
    // matches the "local counter substitutes for the real global one"
    // simplification player/player_creation.cpp's GrantStartingItems and
    // monster/monster_runtime.h's OnDeath already use, just applied to
    // the retry-loop case specifically.
    static void SpawnAmbushMonsters(GeneratedLevel& level, WorldRegistry& world, int count, JavaRandom& rng,
                                     const MonsterDatabase& monsterDb, int16_t& spawnIdCounter);

    // M18: `Dungeon.populate()`'s own `spawnRoomMonsters()`/`placeChests()`
    // register directly into `ESGame.monsters[]`/`chests[]` AS PART OF
    // generation itself -- so in the real game, `WorldRegistry` is never
    // actually empty for a level that's been generated; M6's
    // `GeneratedLevel::monsters`/`::chests` have held that same data as
    // plain OUTPUT summaries since M6, with nothing to put them into until
    // M16. This converts each `GeneratedMonsterSpawn`/`GeneratedChestSpawn`
    // into the real packed record layout and inserts it, matching dawnstar's
    // own M24 (the last piece of the 3-way registry split M16/M17 worked
    // through). Couldn't live inside `world/dungeon_generator.h` itself:
    // `stormhold_world` is a dependency OF `stormhold_dungeon`, so the
    // reverse would cycle -- same constraint M16's own class comment
    // already documents for this whole module's placement.
    //
    // Deliberately does NOT touch `level.tiles` at all -- `DungeonGenerator`
    // already sets the monster (2) / chest (16) presence bits itself while
    // building `level.tiles` (`PopulateLevel`'s own room-monster/chest
    // loops), so this only adds the missing REGISTRY side, matching
    // `Monster.store()`/`Dungeon.storeChest()`'s own real split (neither of
    // those methods sets a NEW bit blindly here either -- `storeChest()`
    // does re-OR bit 16, but that's already set to the same value from
    // generation, so skipping it changes nothing observable).
    //
    // **A real, confirmed and unavoidable simplification, not a
    // transcription gap:** `Dungeon.placeChests()` draws a random [0,2]
    // "tier bits" value packed into the real record's byte 3 alongside
    // `tier` (top 2 bits) -- M6's own `PlaceChests` already documents
    // preserving that RNG draw (for stream-order fidelity) while
    // discarding its RESULT, since `GeneratedChestSpawn` has nowhere to
    // carry it and no confirmed reader exists anywhere in `../../../src/`
    // for those bits. This means the record this method reconstructs
    // always has those top 2 bits as 0, unlike what the real record would
    // actually contain in memory -- byte 3's low 6 bits (`tier` itself)
    // still match exactly.
    static void RegisterGeneratedSpawns(const GeneratedLevel& level, WorldRegistry& world,
                                         const MonsterDatabase& monsterDb);

    // Looks up a GeneratedLevel by its 1-based level number -- the exact
    // same caller-supplies-world-state shape as `player/player_movement.h`
    // 's own `LevelLookup` (declared independently here rather than reused
    // directly, since `stormhold_dungeon` doesn't depend on
    // `stormhold_player`), CONST since tile sampling never mutates a
    // neighbor level.
    using LevelLookup = std::function<const GeneratedLevel&(int levelNumber)>;

    // M21: Dungeon.tileAt(x,y) -- an absolute tile lookup that, unlike a
    // plain `level.tiles[x][y]` index, transparently reaches across a
    // level boundary (into whichever of `level`'s own 4 confirmed
    // neighbors is relevant, recentered exactly like `player/
    // player_movement.h`'s own `ComputeMoveTarget` cross-level boundary
    // stitching does for player movement) when `x`/`y` falls outside
    // `level`'s own bounds. Returns 1 (wall) when there's no neighbor in
    // that direction, or when the neighbor itself isn't `populated` --
    // matching the original's own fallback exactly (see GeneratedLevel::
    // populated's own header comment on why every level this port's
    // LevelLookup can return is already `populated`, so that branch is
    // presently unreachable here in practice but preserved for fidelity).
    static uint8_t TileAt(const GeneratedLevel& level, int x, int y, const LevelLookup& levels);

    // M21: Dungeon.sampleCorridorView(x,y,facing,outGrid) -- the corridor
    // 3D-renderer's visibility sample: a 9x5 grid of absolute tile bytes
    // (via TileAt, so it can see across a level boundary near an edge)
    // at the forward/lateral offsets the facing direction implies.
    // Consumed by `render/corridor_render_plan.h`'s `CorridorRenderPlan::
    // Plan` via ViewGridAt below -- this method only builds the grid, it
    // doesn't interpret it.
    static CorridorViewGrid SampleCorridorView(const GeneratedLevel& level, int x, int y, int facing,
                                                const LevelLookup& levels);

    // Dungeon.viewGridAt(index, depth, grid): reads a SampleCorridorView
    // grid at a (dx, dy)-shaped relative offset -- confirmed to be the
    // exact same formula as dawnstar's own `Player.tileAt(dx,dy)`, just
    // moved onto `Dungeon` and taking the grid as an explicit parameter
    // instead of a `this.corridorView` field read (Stormhold has no
    // `PlayerState::corridorView` field yet -- see render/
    // corridor_render_plan.h's own class comment on why that's a
    // deliberate scope boundary for this milestone, not an oversight).
    static uint8_t ViewGridAt(const CorridorViewGrid& grid, int index, int depth) {
        if (depth < 4) return grid[static_cast<size_t>(index + depth + 1)][static_cast<size_t>(depth)];
        return grid[static_cast<size_t>(index + depth)][static_cast<size_t>(depth)];
    }
};

}  // namespace stormhold
