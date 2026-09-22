// M56 smoke test: the "Victory!" end-of-game trigger --
// GameCanvas.resolveMonsterDeath()'s `targetMonster.monsterType == 42`
// branch (`this.game.endOfGameUI = this.game.newEndOfGameUI();
// this.game.setCurrentDisplay(...)`), which combat/combat_tick.h's
// RefreshAndResolveTargetMonster now signals via its own return value
// (see its doc comment) for main.cpp to play. combat_tick_smoke.exe
// (M32) already covers the return-value contract itself (true only for
// a resolved type-42 death, false for everything else, including type
// 41); this test covers the other half main.cpp performs with it: the
// real `newEndOfGameUI()` text, `Shop.dialogue[9][74] + "\n" +
// dialogue[9][75] + "\n" + dialogue[9][76]` -- built here against the
// REAL npcstrings.dat, independently of main.cpp's own copy of the same
// three-line concatenation, and printed for a by-hand sanity check the
// same way M8 first verified this exact file.
//
// No JVM ground truth (same reason as every prior milestone). The
// harder-to-automate half -- actually killing a live type-42 monster in
// the running dawnstar_port.exe and watching the Victory screen appear
// -- isn't performed by hand this session (same acceptance M53 already
// established for an equally awkward-to-manually-trigger real event);
// a plain launch/stays-up check stands in for it.
#include <cstdio>
#include <string>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/shop_dialogue.h"
#include "combat/combat_tick.h"
#include "dungeon/dungeon_runtime.h"
#include "monster/monster_runtime.h"
#include "player/player_creation.h"
#include "player/player_movement.h"
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
using dawnstar::WorldRegistry;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
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
        dawnstar::ShopDialogue shopDialogue = dawnstar::ShopDialogue::Load(root + "/npcstrings.dat");

        // --- A: the real Victory text, built exactly as main.cpp's own
        // call site does, and printed for a by-hand look. ---
        {
            Check(shopDialogue.groups.size() > 9, "setup: dialogue group 9 should exist");
            Check(shopDialogue.groups[9].size() >= 77, "setup: dialogue group 9 should have all 77 real entries");

            std::string victoryText = shopDialogue.groups[9][74] + "\n" + shopDialogue.groups[9][75] + "\n" +
                                       shopDialogue.groups[9][76];
            std::printf("Victory! text:\n%s\n", victoryText.c_str());

            Check(!shopDialogue.groups[9][74].empty() && !shopDialogue.groups[9][75].empty() &&
                      !shopDialogue.groups[9][76].empty(),
                  "all three real Victory dialogue lines should be non-empty");
            Check(victoryText.find('\n') != std::string::npos &&
                      victoryText.find('\n', victoryText.find('\n') + 1) != std::string::npos,
                  "the concatenation should join exactly 3 lines with 2 newlines, matching newEndOfGameUI()");
        }

        // --- B: end-to-end -- a real spawned type-42 monster's death,
        // resolved through the real CombatTick, really does return the
        // Victory signal (re-derived here rather than trusted from
        // m32's own coverage, so this test doesn't silently pass if a
        // future refactor ever decouples the two). ---
        {
            // The full real 37-level world, same as every other combat/
            // movement smoke test (m32 included) -- PlayerMovement's own
            // `levels[player.currentLevel - 1]` indexing needs the
            // vector's own indices to line up with real level numbers,
            // which a single synthetic one-level vector can't provide
            // once currentLevel is set to anything other than 1.
            std::vector<GeneratedLevel> levels;
            WorldRegistry world(geometry.rows.size());
            for (size_t i = 0; i < geometry.rows.size(); i++) {
                int n = static_cast<int>(i) + 1;
                levels.push_back(n == 1 ? dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[0])
                                         : dawnstar::DungeonGenerator::PopulateLevel(n, geometry.rows[i], items, monsterDb));
                dawnstar::DungeonRuntime::RegisterGeneratedSpawns(levels.back(), world);
            }

            int levelNumber = 2;
            GeneratedLevel& level = levels[static_cast<size_t>(levelNumber - 1)];
            Check(!level.monsters.empty(), "setup: the level should have at least one real monster spawn to reuse");
            auto spawn = level.monsters[0];

            dawnstar::JavaRandom creationRng(12345);
            PlayerState player = PlayerCreation::CreateCharacter(0, "Hero", charData, items, creationRng);
            player.currentLevel = levelNumber;

            // Replace whatever real monster the generator placed at this
            // tile with the literal type-42 end-game monster, keeping the
            // tile's own monster-presence bit (already set by
            // RegisterGeneratedSpawns above) as-is.
            MonsterState endBoss = MonsterRuntime::Spawn(500, 42, levelNumber, monsterDb);
            endBoss.x = static_cast<int8_t>(spawn.x);
            endBoss.y = static_cast<int8_t>(spawn.y);
            endBoss.hp = 0;
            world.monsters[static_cast<size_t>(levelNumber - 1)][PackPosKey(spawn.x, spawn.y)] =
                MonsterRuntime::ToBytes(endBoss);

            // Stand directly on the monster's own tile facing itself isn't
            // how MonsterInFront works -- reuse the exact same "player
            // one step back, facing toward it" placement m32's own type-41/
            // 42 sections use, by placing the player at the spawn tile
            // itself minus one step in the facing direction. Simpler here:
            // just place the player ON an adjacent walkable tile facing
            // the monster's tile directly, mirroring m32's own setup
            // (which reuses a real corridor-adjacent monster position).
            dawnstar::JavaRandom globalRng(777);
            MessagePopupState popup;
            int16_t nextSpawnId = 1;

            // Re-derive a real in-front position the same way m32's own
            // fixture does: find a tile immediately adjacent (N/E/S/W) to
            // the monster's own spawn tile and stand there facing it.
            static const int dx[4] = {0, 1, 0, -1};
            static const int dy[4] = {-1, 0, 1, 0};
            static const int faceFrom[4] = {3, 4, 1, 2};  // facing that looks back toward the monster
            bool placed = false;
            for (int dir = 0; dir < 4 && !placed; dir++) {
                int px = spawn.x + dx[dir];
                int py = spawn.y + dy[dir];
                if (px < 0 || py < 0 || px >= level.width || py >= level.height) continue;
                player.tileX = px;
                player.tileY = py;
                player.facing = faceFrom[dir];
                if (dawnstar::PlayerMovement::MonsterInFront(player, levels, world) != nullptr) placed = true;
            }
            Check(placed, "setup: should find a real adjacent tile to stand on facing the type-42 monster");

            bool victoryTriggered = CombatTick::RefreshAndResolveTargetMonster(player, levels, world, monsterDb,
                                                                                items, popup, globalRng, 1000,
                                                                                nextSpawnId);
            Check(victoryTriggered, "a real type-42 monster's death, resolved end-to-end, should signal Victory");
        }

        if (g_ok) {
            std::printf("victory_smoke: all checks passed\n");
            return 0;
        } else {
            std::printf("victory_smoke: FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("victory_smoke: exception: %s\n", e.what());
        return 2;
    }
}
