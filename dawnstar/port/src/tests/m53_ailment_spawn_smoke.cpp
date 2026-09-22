// M53 smoke test: CombatResolution::MonsterTick's now-real ailment==2
// ("curse of hunger") side effect -- Monster.tick()'s own
// `Dungeon.populateRandomMonsters(3)` call, previously a documented
// no-op (see docs/PORT_ROADMAP.md's M53 entry) -- plus
// CombatTick::TickNearbyMonsters's own `spawnIdCounter` threading.
//
// No JVM ground truth (same reason as every prior milestone). Verified
// against the real 37-level generated world (M6/M24), a real
// MonsterDatabase-driven monster (scanned for a real type whose own
// ailment column is genuinely 2, not assumed), and the real
// CombatResolution::MonsterTick/DungeonRuntime::PopulateRandomMonsters
// call chain -- driven for real, many-round rolls (M36's own
// "outcome-independent invariant, not a forced single-call roll"
// philosophy) rather than hand-tracing the detection/hit/ailment RNG
// sequence, since MonsterTick's own internal roll order is already
// covered by M15/M36.
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "combat/combat_resolution.h"
#include "dungeon/dungeon_runtime.h"
#include "monster/monster_runtime.h"
#include "player/player_creation.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::CombatResolution;
using dawnstar::DungeonRuntime;
using dawnstar::GeneratedLevel;
using dawnstar::MonsterRuntime;
using dawnstar::MonsterState;
using dawnstar::PackPosKey;
using dawnstar::PlayerCreation;
using dawnstar::PlayerState;
using dawnstar::WorldRegistry;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

// Same helper m36_monster_ai_tick_smoke.cpp's own file carries: an
// in-bounds tile at exactly Manhattan distance `dist` from (cx,cy).
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
        dawnstar::CharacterData charData = dawnstar::CharacterData::Load(archive);
        dawnstar::MonsterDatabase monsterDb = dawnstar::MonsterDatabase::Load(archive);
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

        // --- find a REAL monster type whose own ailment column (11) is
        // genuinely 2 ("curse of hunger") -- not assumed. ---
        int hungerType = -1;
        for (int t = 1; t <= monsterDb.TypeCount(); t++) {
            if (monsterDb.Stat(t, 11) == 2) {
                hungerType = t;
                break;
            }
        }
        Check(hungerType > 0, "the real monster database should have at least one type with ailment column == 2");
        if (hungerType <= 0) return 1;
        std::printf("  using real monster type %d (ailment column == 2)\n", hungerType);

        // A real non-hub level with at least one pre-placed monster (so
        // it's a genuine, real-geometry level, same selection m36 uses).
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

        dawnstar::JavaRandom setupRng(53);
        PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", charData, items, setupRng);
        player.currentLevel = levelNumber;
        // Any real in-bounds interior tile for the player; the attacker
        // is then placed at Manhattan distance 1 from it (melee range).
        int ppx = level.width / 2, ppy = level.height / 2;
        player.tileX = ppx;
        player.tileY = ppy;

        int mx = 0, my = 0;
        Check(FindTileAtDistance(level, ppx, ppy, 1, &mx, &my),
              "should find an in-bounds tile at distance 1 from the player");

        size_t before = world.monsters[static_cast<size_t>(levelIdx)].size();

        // --- probe: run real MonsterTick rounds (900ms apart, clearing
        // the 800ms wind-up each time) across a handful of seeds until
        // the population on `levelIdx` actually grows -- the same
        // "drive the real wrapper repeatedly, don't hand-force the RNG
        // sequence" philosophy m36's own test establishes, just also
        // varying the outer seed since this effect additionally needs
        // the ailment roll (<=30%) to land on the SAME tick as a
        // strong-enough hit (outcome >= 3). ---
        bool grew = false;
        size_t after = before;
        int16_t spawnIdCounter = 1;
        int16_t spawnIdBefore = 1;
        // Registry KEYS immediately before the one round that actually
        // grows the population -- used below to find exactly the new
        // entries by set difference. NOT spawnId: this probe's own fresh
        // spawnIdCounter (reset to 1 per seed attempt) can and does
        // collide in VALUE with the level's own pre-existing, unrelated
        // generation-time spawnIds (each level's own local counter,
        // also starting near 1 -- see world/dungeon_generator.h's own
        // doc comment on why that's fine for registry correctness,
        // which is keyed by position, not spawnId).
        std::vector<int> keysBeforeSuccess;
        for (int seed = 1; seed <= 400 && !grew; seed++) {
            MonsterState attacker =
                MonsterRuntime::Spawn(static_cast<int16_t>(9100 + seed), hungerType, levelNumber, monsterDb);
            attacker.x = static_cast<int8_t>(mx);
            attacker.y = static_cast<int8_t>(my);
            dawnstar::JavaRandom tickRng(seed);
            spawnIdCounter = 1;
            for (int round = 0; round < 80 && !grew; round++) {
                std::vector<int> keysBefore;
                for (const auto& [key, record] : world.monsters[static_cast<size_t>(levelIdx)]) keysBefore.push_back(key);

                spawnIdBefore = spawnIdCounter;
                CombatResolution::MonsterTick(attacker, player, charData, items, monsterDb, round * 900LL, tickRng,
                                               levels, world, spawnIdCounter);
                after = world.monsters[static_cast<size_t>(levelIdx)].size();
                if (after != before) {
                    grew = true;
                    keysBeforeSuccess = std::move(keysBefore);
                }
            }
        }

        Check(grew, "the ailment==2 roll should eventually spawn 3 more monsters within a generous probe budget");
        Check(after == before + 3, "a single ailment==2 proc should add exactly 3 monsters to the level's registry");
        Check(spawnIdCounter == static_cast<int16_t>(spawnIdBefore + 3),
              "the spawnId counter should advance by exactly 3 for the 3 newly spawned monsters");

        // Sanity on the new monsters themselves: real, walkable, in
        // bounds, alive -- same invariants M6's own generation smoke
        // test already established for pre-placed spawns. Identified by
        // registry KEY not present just before the successful round
        // (see keysBeforeSuccess's own doc comment above).
        int newlyFound = 0;
        for (const auto& [key, record] : world.monsters[static_cast<size_t>(levelIdx)]) {
            if (std::find(keysBeforeSuccess.begin(), keysBeforeSuccess.end(), key) != keysBeforeSuccess.end()) continue;
            MonsterState m = MonsterRuntime::FromBytes(record);
            newlyFound++;
            int kx = 0, ky = 0;
            dawnstar::UnpackPosKey(key, &kx, &ky);
            Check(kx == m.x && ky == m.y, "a newly spawned monster's registry key should match its own position");
            Check(kx >= 0 && kx < level.width && ky >= 0 && ky < level.height,
                  "a newly spawned monster should be in bounds");
            Check((level.tiles[static_cast<size_t>(kx)][static_cast<size_t>(ky)] & 2) != 0,
                  "a newly spawned monster's tile should carry the monster-presence bit");
            Check(static_cast<uint8_t>(m.hp) > 0, "a newly spawned monster should start with positive hp");
        }
        Check(newlyFound == 3, "exactly 3 newly-spawned monsters (by registry key, not present before) should be found");

        if (g_ok) {
            std::printf("ailment_spawn_smoke: all checks passed\n");
            return 0;
        } else {
            std::printf("ailment_spawn_smoke: FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("ailment_spawn_smoke: exception: %s\n", e.what());
        return 2;
    }
}
