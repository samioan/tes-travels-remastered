#pragma once
#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "assets/monster_database.h"
#include "monster/monster_runtime.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"
#include "world/dungeon_view.h"

namespace dawnstar {

// Packs a tile position into a single map key -- a plain-int stand-in
// for Util.posKey(x,y)'s "x,y" string. The original's string key has no
// behavioral significance beyond giving each (x,y) a unique Hashtable
// key; both level dimensions are always well under 64, so this has
// enormous headroom.
inline int PackPosKey(int x, int y) { return x * 4096 + y; }
inline void UnpackPosKey(int key, int* outX, int* outY) {
    *outX = key / 4096;
    *outY = key % 4096;
}

// Renamed-source counterpart of ../../../src/Dungeon.java's LIVE
// per-level state: ESGame.monsters[]/chests[]/droppedItems[], the
// Hashtable/Vector registries every other module's own class comment
// has been flagging as missing since M13/M15/M18 ("no live per-level
// monster/dropped-item registry exists yet"). One WorldRegistry holds
// every level's registries, indexed by levelNumber-1 like `levels`
// itself.
//
// Deliberately NOT wired into PlayerMovement/MonsterRuntime/
// CombatResolution's own existing simplifications in this milestone --
// Move()/Chase()/OnDeath()/UseItem() live in dawnstar_player/
// dawnstar_monster/dawnstar_combat, none of which may depend on this
// module (dawnstar_dungeon depends on THEM, via dawnstar_monster/
// dawnstar_world -- depending back would cycle). Wiring those call
// sites to actually populate/consume a WorldRegistry is therefore a
// job for whatever layer already sits above all of them -- in the real
// game that's GameCanvas/ESGame's own screen-wiring loop (Monster.
// onDeath() is itself only ever called from GameCanvas, never from
// Player/Monster's own methods -- see docs/PORT_ROADMAP.md's M22
// entry). This milestone only provides the registry and Dungeon.java's
// own registry-management methods, faithfully, ready for that wiring.
struct WorldRegistry {
    // ESGame.monsters[]: position-keyed 28-byte Monster.toBytes()
    // records (monster/monster_runtime.h's ToBytes/FromBytes).
    std::vector<std::unordered_map<int, std::array<uint8_t, 28>>> monsters;
    // ESGame.chests[]: position-keyed 8-byte chest records (see
    // world/dungeon_generator.h's GeneratedChestSpawn and
    // DungeonGenerator.java's placeChests -- chest[0]/[1]=x/y, [2]=1 for
    // the guaranteed-gift chest / 0 otherwise (write-only -- grepping
    // ../../../src/ confirms nothing ever reads chest[2] back, so this
    // is real but observably inert data, corrected here from an earlier,
    // inaccurate "always 0" note), [3]=tier only (placeChests also rolls
    // a 2-bit random "flavor" into this byte's top bits, but no
    // confirmed read site exists for those either -- see
    // GeneratedChestSpawn's own doc comment), [4]=itemId low byte,
    // [5]/[6]=spawnId high/low byte, [7]=itemId high byte (only
    // meaningful when [4]==86)).
    std::vector<std::unordered_map<int, std::array<uint8_t, 8>>> chests;
    // ESGame.droppedItems[]: an unordered list (Vector, not
    // position-keyed -- multiple items can litter the same tile) of
    // 7-byte records (x, y, itemIdLow, spawnIdHigh, spawnIdLow,
    // itemIdOrDataLow, flags -- see monster/monster_runtime.h's
    // OnDeath::DeathDrop and player/player_movement.h's
    // dropInventoryItem doc comments for the two real record shapes
    // that land here).
    std::vector<std::vector<std::array<uint8_t, 7>>> droppedItems;

    explicit WorldRegistry(size_t levelCount)
        : monsters(levelCount), chests(levelCount), droppedItems(levelCount) {}
};

class DungeonRuntime {
public:
    // Registers a level's already-generated pre-placed monster/chest
    // spawns (world/dungeon_generator.h's GeneratedLevel::monsters/
    // ::chests -- DungeonGenerator.populateLevel's room-monster loop and
    // placeChests, which in the original directly call
    // Monster.spawn(...).store() / ESGame.chests[...].put(...) as part
    // of generation itself) into the live WorldRegistry. A separate
    // step here rather than folded into generation itself, since
    // dawnstar_world (where DungeonGenerator lives) can't depend on
    // dawnstar_dungeon -- this module already depends on dawnstar_world,
    // so the reverse would cycle. Intended to be called once per level
    // right after generation (see main.cpp's BuildWorld) -- the port's
    // substitute for the original's same-pass registration. Tile bits
    // are NOT touched here: DungeonGenerator already sets them (bit 2
    // per monster, bit 16 per chest) while building `level.tiles`
    // itself, so redoing that here would be pure duplication.
    static void RegisterGeneratedSpawns(GeneratedLevel& level, WorldRegistry& world);

    // Dungeon.populateRandomMonsters(count): spawns `count` random
    // monsters at random walkable positions on `levels[levelIndex]`,
    // registering each into `world`. Retries with a freshly-rolled
    // (x,y) until TrySpawnMonsterNear below actually places one --
    // ported as the same unconditional retry loop the original uses (no
    // giving up), which relies on the same implicit level-design
    // invariant DungeonGenerator/PlayerMovement's own doc comments
    // already note: every real generated level has enough walkable
    // space that this always terminates in practice.
    static void PopulateRandomMonsters(std::vector<GeneratedLevel>& levels, WorldRegistry& world, int levelIndex,
                                        int count, JavaRandom& rng, const MonsterDatabase& monsterDb,
                                        int16_t& spawnIdCounter);

    // Dungeon.trySpawnMonsterNear(x,y,forcedTypeOrSentinel): tries to
    // spawn adjacent to (x,y) -- west, east, north, south, then +3
    // south as a last resort (matches the original's exact offsets;
    // that last one isn't actually adjacent, preserved as-is rather
    // than "fixed" -- see world/dungeon_generator.h's
    // trySpawnMonsterNear-equivalent doc comments for the same kind of
    // preserved oddity elsewhere). Returns whether a monster was
    // actually placed.
    //
    // `forcedTypeOrSentinel`'s real, non-obvious semantics (traced
    // straight from Dungeon.java/Monster.java's spawn(), not assumed):
    // 41 or 42 spawn that exact type directly (the special "roaming"
    // monster's own type ids); any OTHER value, including a seemingly
    // "forced" real monster type id, is used as a REPLACEMENT
    // DIFFICULTY TIER for a random weighted roll (via
    // MonsterRuntime::PickMonsterType), not a literal forced type --
    // only 41/42 ever bypass the roll. A negative value (populateRandom
    // Monsters' own -1) just uses the level's own real tier.
    static bool TrySpawnMonsterNear(std::vector<GeneratedLevel>& levels, WorldRegistry& world, int levelIndex, int x,
                                     int y, int forcedTypeOrSentinel, JavaRandom& rng, const MonsterDatabase& monsterDb,
                                     int16_t& spawnIdCounter);

    // ESGame.removeMonster(level,x,y): removes whatever monster record
    // is registered at (x,y), if any, clearing its tile's monster
    // presence bit (2) only when a record was actually found there.
    static void RemoveMonster(GeneratedLevel& level, WorldRegistry& world, int x, int y);

    // Dungeon.addDroppedItem(record): registers a 7-byte dropped-item
    // record and sets its tile's dropped-item presence bit (4).
    static void AddDroppedItem(GeneratedLevel& level, WorldRegistry& world, const std::array<uint8_t, 7>& record);

    // Dungeon.removeChest(record): removes a chest once its tile is not
    // a wall AND actually still has the chest presence bit (16) set --
    // both conditions ported exactly, matching the original's guard.
    static void RemoveChest(GeneratedLevel& level, WorldRegistry& world, const std::array<uint8_t, 8>& record);

    // Dungeon.removeDroppedItem(record): removes one dropped-item
    // record once its tile is not a wall AND still has the dropped-item
    // presence bit (4) set. SIMPLIFIED: the original's
    // Vector.removeElement() matches by Java reference identity (the
    // exact same byte[] instance), so two content-identical-but-
    // distinct records would NOT collide there; this port has no
    // pointer-identity equivalent for a value type, so it removes the
    // first record equal BY CONTENT instead -- the behavior any real
    // caller actually wants, and indistinguishable from the original
    // unless two genuinely distinct dropped items ever end up with
    // bit-for-bit identical 7-byte records at once.
    static void RemoveDroppedItem(GeneratedLevel& level, WorldRegistry& world, const std::array<uint8_t, 7>& record);

    // Dungeon.clearDroppedItemFlag(x,y): clears the tile's dropped-item
    // presence bit alone, without touching the registry itself.
    static void ClearDroppedItemFlag(GeneratedLevel& level, int x, int y);

    // Dungeon.droppedItemsAt(x,y): every dropped-item record at (x,y),
    // in registration order (Dungeon.java's own Vector iteration
    // order).
    static std::vector<std::array<uint8_t, 7>> DroppedItemsAt(const WorldRegistry& world, int levelIndex, int x,
                                                                int y);

    // Dungeon.refreshTileFlags(): rebuilds the monster(2)/dropped-item
    // (4)/chest(16) presence bits on every tile of `level` from `world`'s
    // live registries -- used after loading a save or returning to a
    // level whose bits may be stale (not exercised by anything else in
    // this port yet, since there's no save/load of a WorldRegistry
    // itself).
    static void RefreshTileFlags(GeneratedLevel& level, const WorldRegistry& world, int levelIndex);

    // Dungeon.sampleSquareView(x,y,direction,size,out) -- the minimap's
    // own widening-window tile sample (M29, render/minimap_renderer.h),
    // a close cousin of world/dungeon_view.h's SampleCorridorView but
    // re-mapping each sampled tile's raw bits into 4 minimap categories
    // (wall/monster-seen/"special"/no-spawn-room) instead of passing
    // them through -- and, unlike SampleCorridorView, needing a live
    // WorldRegistry (to test a seen monster's own `flag`), which is
    // exactly why this lives here (dawnstar_dungeon) rather than
    // alongside SampleCorridorView in world/dungeon_view.h: that header
    // is part of dawnstar_world, which dawnstar_dungeon already depends
    // on -- the reverse (world/dungeon_view.h including
    // dungeon/dungeon_runtime.h for WorldRegistry) would cycle. `out` is
    // sized to the largest real caller (17x17, the zoomed-out minimap);
    // only `[0,size)x[0,size)` is written.
    //
    // A real, faithfully-preserved quirk: the monster-presence lookup
    // always queries `world.monsters[levelIndex]` (the level sampling
    // STARTED from) using the sampled tile's own raw, unstitched (x,y)
    // -- even for a sampled tile that DungeonView::TileAt actually
    // resolved from a neighboring level (near a level's edge, sampling a
    // window this size can reach past its bounds). So right at a level
    // boundary, a neighboring level's real "monster area" tile bit can
    // show up on the minimap, yet never actually resolve to a red "seen
    // monster" square (the lookup key never matches anything in the
    // wrong level's own registry) -- ported exactly as Dungeon.java
    // shapes it, not "corrected" to consult the neighbor's own registry.
    static void SampleSquareView(const std::vector<GeneratedLevel>& levels, int levelIndex, int x, int y,
                                  int direction, int size, const WorldRegistry& world,
                                  std::array<std::array<uint8_t, 17>, 17>& out);

    // Dungeon.NAMES[levelNumber-1] via Dungeon.displayName(): the hub
    // town ("Dawnstar", level 1) plus 12 zones x 3 levels each, 37
    // names total. M50's death/respawn message ("you wake up back in
    // <level>") is the first real caller; exposed here rather than in
    // world/dungeon_generator.h since Dungeon.java (this module's own
    // renamed-source counterpart, not DungeonGenerator.java's) is where
    // displayName() itself lives.
    static const char* DisplayName(int levelNumber);
};

}  // namespace dawnstar
