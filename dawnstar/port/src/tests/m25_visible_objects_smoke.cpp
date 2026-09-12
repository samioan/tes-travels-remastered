// M25 smoke test: VisibleObjects (player/visible_objects.h) -- Player's
// 13-slot visibleObjects cache, the DATA-MODEL half of
// GameCanvas.paintVisibleObjects() (sprite drawing itself is a later
// milestone, mirroring the M9/M10 split). No JVM ground truth available
// (same reason as M6/M9/M11/M13-M24) -- verified instead by
// independently hand-tracing refreshVisibleObjects' occlusion cascade
// (the same "independently transcribed formula" methodology M9/M13/M19
// used for their own cascades/recentering math) plus integration checks
// against the real 37-level generated world (M6/M24) and a real
// character (M11).
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "dungeon/dungeon_runtime.h"
#include "monster/monster_runtime.h"
#include "player/player_creation.h"
#include "player/player_movement.h"
#include "player/player_state.h"
#include "player/visible_objects.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::DungeonRuntime;
using dawnstar::GeneratedLevel;
using dawnstar::ItemDatabase;
using dawnstar::MonsterDatabase;
using dawnstar::MonsterRuntime;
using dawnstar::MonsterState;
using dawnstar::PlayerCreation;
using dawnstar::PlayerMovement;
using dawnstar::PlayerState;
using dawnstar::VisibleObjects;
using dawnstar::VisibleSlotKind;
using dawnstar::WorldRegistry;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

// Independently-transcribed copy of Player.tileAt(dx,dy)'s re-centering
// formula (the same one visible_objects.cpp's own CorridorTileAt uses),
// used here only to CONSTRUCT synthetic corridorView input -- never to
// call the module under test.
void SetCorridorTile(PlayerState& p, int dx, int dy, uint8_t tile) {
    int row = dy < 4 ? dx + dy + 1 : dx + dy;
    p.corridorView[static_cast<size_t>(row)][static_cast<size_t>(dy)] = tile;
}

const char* KindName(VisibleSlotKind k) {
    switch (k) {
        case VisibleSlotKind::Empty: return "Empty";
        case VisibleSlotKind::WallBlocked: return "WallBlocked";
        case VisibleSlotKind::Occluded: return "Occluded";
        case VisibleSlotKind::Monster: return "Monster";
        case VisibleSlotKind::Chest: return "Chest";
        case VisibleSlotKind::DroppedItem: return "DroppedItem";
        case VisibleSlotKind::Npc: return "Npc";
    }
    return "?";
}

// Checks the 13 slots' kinds against an expected pattern ('E'mpty,
// 'W'allBlocked, 'O'ccluded), printing every mismatch.
void CheckCascade(const std::array<dawnstar::VisibleSlot, 13>& slots, const char expected[13], const char* label) {
    for (int i = 0; i < 13; i++) {
        VisibleSlotKind want = expected[i] == 'W' ? VisibleSlotKind::WallBlocked
                                : expected[i] == 'O' ? VisibleSlotKind::Occluded
                                                      : VisibleSlotKind::Empty;
        if (slots[static_cast<size_t>(i)].kind != want) {
            std::printf("  %s slot %d: expected %c, got %s\n", label, i, expected[i], KindName(slots[static_cast<size_t>(i)].kind));
            g_ok = false;
        }
    }
}

// Finds a tile adjacent to (targetX,targetY) the player can genuinely
// stand on (fully walkable) and turn in place at, facing toward the
// target so a forward offset of (0,1) lands exactly on it -- matching
// PlayerMovement::ComputeMoveTarget's real per-facing deltas (facing 1
// north decreases tileY, 3 south increases it, 2 east increases tileX,
// 4 west decreases it; see M23's own fix for getting this backwards
// once already). The target tile itself only needs to not be a wall
// (it may legitimately have the monster/chest presence bit set).
bool FindApproach(const GeneratedLevel& level, int targetX, int targetY, int* standX, int* standY, int* facing) {
    struct Candidate {
        int dx, dy, f;
    };
    const Candidate candidates[4] = {
        {0, 1, 1},   // stand south, facing north
        {0, -1, 3},  // stand north, facing south
        {-1, 0, 2},  // stand west, facing east
        {1, 0, 4},   // stand east, facing west
    };
    for (const auto& c : candidates) {
        int sx = targetX + c.dx;
        int sy = targetY + c.dy;
        if (sx < 0 || sy < 0 || sx >= level.width || sy >= level.height) continue;
        uint8_t standTile = level.tiles[static_cast<size_t>(sx)][static_cast<size_t>(sy)];
        uint8_t targetTile = level.tiles[static_cast<size_t>(targetX)][static_cast<size_t>(targetY)];
        bool standWalkable = (standTile & (1 | 2 | 32)) == 0;
        bool targetNotWall = (targetTile & 1) == 0;
        if (standWalkable && targetNotWall) {
            *standX = sx;
            *standY = sy;
            *facing = c.f;
            return true;
        }
    }
    return false;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        dawnstar::MonsterDatabase monsterDb = dawnstar::MonsterDatabase::Load(archive);
        dawnstar::CharacterData charData = dawnstar::CharacterData::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);

        std::vector<GeneratedLevel> levels;
        WorldRegistry world(geometry.rows.size());
        for (size_t i = 0; i < geometry.rows.size(); i++) {
            int levelNumber = static_cast<int>(i) + 1;
            levels.push_back(levelNumber == 1
                                  ? dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[0])
                                  : dawnstar::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[i], items,
                                                                               monsterDb));
            DungeonRuntime::RegisterGeneratedSpawns(levels.back(), world);
        }
        std::printf("generated + registered %zu levels\n", levels.size());

        // --- refreshVisibleObjects' occlusion cascade, hand-traced
        // against a scratch (empty) registry so only Refresh's own
        // output is being observed ---
        {
            WorldRegistry scratchWorld(levels.size());
            PlayerState p;
            p.currentLevel = 2;  // index 1, matches scratchWorld's empty slot there

            // Case A: a wall directly ahead (offset (0,1), slot 1) --
            // Java's isOccludedSlot(1) branch occludes ALL 12 other
            // slots unconditionally.
            {
                PlayerState pa = p;
                SetCorridorTile(pa, 0, 1, 1);
                VisibleObjects::Tick(pa, levels, scratchWorld);
                CheckCascade(pa.visibleObjects, "OWOOOOOOOOOOO", "case A (wall at slot 1)");
            }

            // Case B: a wall only at offset (-1,1) (slot 0) --
            // isOccludedSlot(0) occludes 4/8/9 only; nothing else
            // cascades further (4/8/9's own later checks re-occlude
            // already-occluded slots, no new ones).
            {
                PlayerState pb = p;
                SetCorridorTile(pb, -1, 1, 1);
                VisibleObjects::Tick(pb, levels, scratchWorld);
                //                   0123456789012
                CheckCascade(pb.visibleObjects, "WEEEOEEEOOEEE", "case B (wall at slot 0)");
            }

            // Case C: a wall only at offset (0,2) (slot 5) -- a deeper
            // chained cascade: slot5's own check occludes 4/6/9/10/11;
            // by the time the loop reaches slot 6 (now occluded) it
            // additionally occludes 12; by the time it reaches slot 9
            // (now occluded) it additionally occludes 8. Independently
            // hand-traced step by step (see visible_objects.cpp's own
            // Refresh, which is NOT consulted here -- only the original
            // Java's exact sequential re-read semantics were used to
            // derive this).
            {
                PlayerState pc = p;
                SetCorridorTile(pc, 0, 2, 1);
                VisibleObjects::Tick(pc, levels, scratchWorld);
                //                   0123456789012
                CheckCascade(pc.visibleObjects, "EEEEOWOEOOOOO", "case C (wall at slot 5)");
            }

            // Case D: a wall only at offset (-2,2) (slot 3) -- occludes
            // slot 8 alone.
            {
                PlayerState pd = p;
                SetCorridorTile(pd, -2, 2, 1);
                VisibleObjects::Tick(pd, levels, scratchWorld);
                //                   0123456789012
                CheckCascade(pd.visibleObjects, "EEEWEEEEOEEEE", "case D (wall at slot 3)");
            }
        }

        // --- integration: a real pre-placed monster (M24) at a known
        // position, approached so it lands in the closest slot (1) ---
        {
            const size_t kLevelIndex = 1;  // level 2
            GeneratedLevel& level = levels[kLevelIndex];
            Check(!level.monsters.empty(), "level 2 should have at least one generated monster");

            int standX = 0, standY = 0, facing = 0;
            const dawnstar::GeneratedMonsterSpawn* chosen = nullptr;
            for (const auto& spawnCandidate : level.monsters) {
                if (FindApproach(level, spawnCandidate.x, spawnCandidate.y, &standX, &standY, &facing)) {
                    chosen = &spawnCandidate;
                    break;
                }
            }
            Check(chosen != nullptr, "should find a walkable approach tile next to at least one real generated monster");
            if (chosen != nullptr) {
                const auto& spawn = *chosen;
                dawnstar::JavaRandom rng(1);
                PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", charData, items, rng);
                p.currentLevel = 2;
                p.tileX = standX;
                p.tileY = standY;
                p.facing = facing;

                // Turn right then left (net facing unchanged) purely to
                // trigger a real RefreshCorridorView at this position --
                // RefreshCorridorView/Refresh themselves are private, so
                // this is the same "drive it through the public API"
                // approach M13/M19's own tests use.
                PlayerMovement::Move(p, 3, false, levels, world, items);
                PlayerMovement::Move(p, 4, false, levels, world, items);
                Check(p.facing == facing, "the turn-right-then-left round trip should leave facing unchanged");

                VisibleObjects::Tick(p, levels, world);

                Check(p.visibleObjects[1].kind == VisibleSlotKind::Monster,
                      "the pre-placed monster directly ahead should land in the closest slot (1)");
                if (p.visibleObjects[1].kind == VisibleSlotKind::Monster) {
                    MonsterState m = MonsterRuntime::FromBytes(p.visibleObjects[1].monsterRecord);
                    Check(m.x == spawn.x && m.y == spawn.y, "the placed monster's record should be the real one");

                    auto it = world.monsters[kLevelIndex].find(dawnstar::PackPosKey(spawn.x, spawn.y));
                    Check(it != world.monsters[kLevelIndex].end(), "the monster should still be registered");
                    if (it != world.monsters[kLevelIndex].end()) {
                        MonsterState registered = MonsterRuntime::FromBytes(it->second);
                        Check(registered.flag,
                              "MarkLooted should have written the 'seen' flag back into the live registry");
                    }
                }
            }
        }

        // --- integration: the hub town's 5 NPC peddler slots ---
        {
            const GeneratedLevel& hub = levels[0];
            int standX = 0, standY = 0, facing = 0;
            // Shop.SHOP_X/Y[0] -- the hub's peddler 0, at (12,12).
            bool found = FindApproach(hub, 12, 12, &standX, &standY, &facing);
            Check(found, "should find a walkable approach tile next to the hub's shop 0");
            if (found) {
                dawnstar::JavaRandom rng(2);
                PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", charData, items, rng);
                p.currentLevel = 1;
                p.tileX = standX;
                p.tileY = standY;
                p.facing = facing;
                PlayerMovement::Move(p, 3, false, levels, world, items);
                PlayerMovement::Move(p, 4, false, levels, world, items);

                VisibleObjects::Tick(p, levels, world);

                Check(p.visibleObjects[1].kind == VisibleSlotKind::Npc,
                      "hub shop 0 should appear in the closest slot");
                if (p.visibleObjects[1].kind == VisibleSlotKind::Npc) {
                    Check(p.visibleObjects[1].npcShopIndex == 0, "hub shop 0's npcShopIndex should be 0");
                }
            }
        }

        // --- integration: a special-level named shopkeeper (level 3),
        // via GeneratedLevel::specialShopX/Y ---
        {
            const size_t kLevelIndex = 2;  // level 3
            GeneratedLevel& level = levels[kLevelIndex];
            Check(level.specialShopX >= 0, "level 3 should have a real specialShopX/Y from generation");
            int standX = 0, standY = 0, facing = 0;
            bool found = level.specialShopX >= 0 &&
                         FindApproach(level, level.specialShopX, level.specialShopY, &standX, &standY, &facing);
            Check(found, "should find a walkable approach tile next to level 3's special shop room");
            if (found) {
                dawnstar::JavaRandom rng(3);
                PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", charData, items, rng);
                p.currentLevel = 3;
                p.tileX = standX;
                p.tileY = standY;
                p.facing = facing;
                PlayerMovement::Move(p, 3, false, levels, world, items);
                PlayerMovement::Move(p, 4, false, levels, world, items);

                VisibleObjects::Tick(p, levels, world);

                Check(p.visibleObjects[1].kind == VisibleSlotKind::Npc,
                      "level 3's named shopkeeper should appear in the closest slot");
                if (p.visibleObjects[1].kind == VisibleSlotKind::Npc) {
                    Check(p.visibleObjects[1].npcShopIndex == 5, "level 3's shopkeeper should be Shop index 5");
                }
            }
        }

        if (g_ok) {
            std::printf("all visible-objects checks passed\n");
            return 0;
        } else {
            std::printf("SOME CHECKS FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("EXCEPTION: %s\n", e.what());
        return 2;
    }
}
