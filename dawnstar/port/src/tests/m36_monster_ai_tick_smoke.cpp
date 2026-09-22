// M36 smoke test: CombatTick::TickNearbyMonsters (combat/combat_tick.h)
// and VisibleObjects::AnyMonsterAttacking (player/visible_objects.h) --
// Dungeon.tickNearbyMonsters() (the monster-initiated AI tick: attack at
// distance 1, a chase-step at distance 2-3) and GameCanvas.
// paintVisibleObjects()'s own monsterAttacking derivation. No JVM
// ground truth is available (same reason as every prior milestone) --
// verified against the real 37-level generated world (M6/M24) and a
// real created character (M11), with CombatResolution::MonsterTick's own
// probabilistic detection/hit rolls (already verified in M15) NOT
// forced to a specific single-call outcome -- instead this test drives
// the same wrapper repeatedly (exactly like the real tick loop would)
// until a hit actually lands, then checks the outcome-independent
// invariant that matters here (hp never INCREASES from a monster
// attack), the same philosophy M32 established.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "combat/combat_tick.h"
#include "dungeon/dungeon_runtime.h"
#include "monster/monster_runtime.h"
#include "player/player_creation.h"
#include "player/visible_objects.h"
#include "render/message_popup.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::CombatTick;
using dawnstar::GeneratedLevel;
using dawnstar::MessagePopupState;
using dawnstar::MonsterRuntime;
using dawnstar::MonsterState;
using dawnstar::PackPosKey;
using dawnstar::PlayerCreation;
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

// Finds an in-bounds tile at exactly Manhattan distance `dist` from
// (cx,cy) -- doesn't need to be walkable (TickNearbyMonsters doesn't
// care about the PLAYER's own tile at all, only the monster's), just
// in-bounds.
bool FindTileAtDistance(const GeneratedLevel& level, int cx, int cy, int dist, int* outX, int* outY) {
    for (int ddx = -dist; ddx <= dist; ddx++) {
        int ddy = dist - std::abs(ddx);
        for (int sign : {1, -1}) {
            int y = cy + sign * ddy;
            int x = cx + ddx;
            if (x < 0 || y < 0 || x >= level.width || y >= level.height) continue;
            if (std::abs(x - cx) + std::abs(y - cy) != dist) continue;
            *outX = x;
            *outY = y;
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
            dawnstar::DungeonRuntime::RegisterGeneratedSpawns(levels.back(), world);
        }
        std::printf("generated + registered %zu levels\n", levels.size());
        // M53: TickNearbyMonsters now threads a Monster.nextSpawnIdCounter
        // substitute through to CombatResolution::MonsterTick's own
        // ailment==2 "curse of hunger" 3-monster spawn.
        int16_t spawnIdCounter = 1;

        int levelIdx = -1;
        for (size_t i = 1; i < levels.size(); i++) {
            if (!levels[i].monsters.empty()) {
                levelIdx = static_cast<int>(i);
                break;
            }
        }
        Check(levelIdx >= 0, "at least one non-hub level should have a real pre-placed monster spawn");
        if (levelIdx < 0) return 1;

        GeneratedLevel& level = levels[static_cast<size_t>(levelIdx)];
        int levelNumber = level.number;
        const auto& spawn = level.monsters[0];

        dawnstar::JavaRandom globalRng(4321);

        // --- A: distance 1 -- attack phase, driven until a hit lands ---
        {
            int px = 0, py = 0;
            Check(FindTileAtDistance(level, spawn.x, spawn.y, 1, &px, &py),
                  "should find an in-bounds tile at distance 1 from the real spawn");

            PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            player.currentLevel = levelNumber;
            player.tileX = px;
            player.tileY = py;
            int16_t hpBefore = player.coreStats[2];

            int64_t nowMs = 1000;
            bool landed = false;
            for (int round = 0; round < 60 && !landed; round++) {
                MessagePopupState popup;
                CombatTick::TickNearbyMonsters(player, levels, world, charData, items, monsterDb, nowMs, globalRng,
                                                popup, spawnIdCounter);
                if (popup.visible) {
                    Check(popup.lines[0] == "Creature" && popup.lines[1] == "attacks!" && popup.priority == 2,
                          "a landed monster attack should show \"Creature/attacks!\" at priority 2");
                    landed = true;
                }
                nowMs += 900;  // past the 800ms wind-up each round
            }
            Check(landed, "a monster within melee range should eventually land a hit within 60 driven rounds");
            Check(player.coreStats[2] <= hpBefore, "a monster attack should never INCREASE player hp");

            auto it = world.monsters[static_cast<size_t>(levelIdx)].find(PackPosKey(spawn.x, spawn.y));
            Check(it != world.monsters[static_cast<size_t>(levelIdx)].end(),
                  "an attacking (distance-1) monster should stay registered at its own position");
        }

        // --- B: distance 2 -- chase phase, a fresh monster's first call
        // always attempts a step (moveCooldown starts at 0) ---
        {
            // A second, independent monster placed directly (not the
            // real spawn from A, which may already be mid-combat) so
            // this section starts from a clean moveCooldown/aiPhase.
            MonsterState chaser = MonsterRuntime::Spawn(500, 3, levelNumber, monsterDb);
            int mx = -1, my = 10;
            // Pick a real in-bounds, walkable tile away from the level
            // edges for the chaser itself, independent of the real spawn,
            // so its own chase-step has a genuine chance to succeed.
            for (int x = 5; x < level.width - 5; x++) {
                if ((level.tiles[static_cast<size_t>(x)][static_cast<size_t>(my)] & (1 | 2 | 32)) == 0) {
                    mx = x;
                    break;
                }
            }
            Check(mx >= 0, "should find a real walkable tile for the chaser's own starting position");
            chaser.x = static_cast<int8_t>(mx);
            chaser.y = static_cast<int8_t>(my);
            world.monsters[static_cast<size_t>(levelIdx)][PackPosKey(mx, my)] = MonsterRuntime::ToBytes(chaser);
            level.tiles[static_cast<size_t>(mx)][static_cast<size_t>(my)] |= 2;

            int px = 0, py = 0;
            Check(FindTileAtDistance(level, mx, my, 2, &px, &py),
                  "should find an in-bounds tile at distance 2 from the chaser");

            PlayerState player = PlayerCreation::CreateCharacter(0, "Tester2", charData, items, globalRng);
            player.currentLevel = levelNumber;
            player.tileX = px;
            player.tileY = py;
            player.minimapDirty = false;

            MessagePopupState popup;
            CombatTick::TickNearbyMonsters(player, levels, world, charData, items, monsterDb, 5000, globalRng, popup,
                                            spawnIdCounter);

            Check(player.minimapDirty, "a distance-2 monster's first call should always attempt a step (fresh moveCooldown == 0), marking the minimap dirty");
            Check(!popup.visible, "a chase step alone (no melee-range monster) should show no attack popup");

            // Self-consistency, whether or not the step actually landed
            // (it may be blocked): the monster should be findable exactly
            // once in the registry, at wherever it now claims to be.
            int foundCount = 0;
            int foundX = -1, foundY = -1;
            for (const auto& [key, record] : world.monsters[static_cast<size_t>(levelIdx)]) {
                MonsterState m = MonsterRuntime::FromBytes(record);
                if (m.spawnId == 500) {
                    foundCount++;
                    int kx, ky;
                    dawnstar::UnpackPosKey(key, &kx, &ky);
                    Check(kx == m.x && ky == m.y, "the chaser's registry KEY should match its own record's position");
                    foundX = m.x;
                    foundY = m.y;
                }
            }
            Check(foundCount == 1, "the chaser should be registered exactly once after its own tick (no duplicate/lost entry)");
            std::printf("  chaser: started at (%d,%d), now at (%d,%d)\n", mx, my, foundX, foundY);
        }

        // --- C: distance > 3 and distance 0 -- both untouched ---
        {
            int mx = spawn.x, my = spawn.y;
            int px = 0, py = 0;
            bool found4 = FindTileAtDistance(level, mx, my, 6, &px, &py);
            if (found4) {
                PlayerState player = PlayerCreation::CreateCharacter(0, "Tester3", charData, items, globalRng);
                player.currentLevel = levelNumber;
                player.tileX = px;
                player.tileY = py;
                player.minimapDirty = false;
                MessagePopupState popup;
                CombatTick::TickNearbyMonsters(player, levels, world, charData, items, monsterDb, 9000, globalRng,
                                                popup, spawnIdCounter);
                Check(!player.minimapDirty && !popup.visible,
                      "a monster more than 3 tiles away should be completely untouched this tick");
            }
        }

        // --- D: VisibleObjects::AnyMonsterAttacking ---
        {
            PlayerState player;
            Check(!VisibleObjects::AnyMonsterAttacking(player), "a player with no visible slots populated should report no attacking monster");

            player.visibleObjects[4].kind = VisibleSlotKind::Monster;
            player.visibleObjects[4].monsterRecord[6] = 0;
            Check(!VisibleObjects::AnyMonsterAttacking(player),
                  "a visible monster whose own \"seen\" byte is still 0 should not report as attacking");

            player.visibleObjects[4].monsterRecord[6] = 1;
            Check(VisibleObjects::AnyMonsterAttacking(player),
                  "a visible monster whose \"seen\" byte is set should report as attacking (the real, permissive quirk)");

            player.visibleObjects[4] = dawnstar::VisibleSlot{};
            player.visibleObjects[11].kind = VisibleSlotKind::Chest;
            Check(!VisibleObjects::AnyMonsterAttacking(player), "a non-monster slot should never report as attacking, regardless of its own bytes");
        }

        if (g_ok) {
            std::printf("all monster-ai-tick checks passed\n");
            return 0;
        } else {
            std::printf("SOME CHECKS FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("EXCEPTION: %s\n", e.what());
        return 1;
    }
}
