// M32 smoke test: PlayerMovement::MonsterInFront (player/
// player_movement.h) and CombatTick::ProcessAttack/
// RefreshAndResolveTargetMonster (combat/combat_tick.h) -- GameCanvas.
// nearestAttackableMonster()/processAttack()/refreshTargetMonster()/
// resolveMonsterDeath(), the player-initiated attack action and its
// always-run-every-tick target-refresh/death-resolution tail. No JVM
// ground truth is available (same reason as every prior milestone) --
// verified against the real 37-level generated world (M6/M24) plus a
// real created character (M11), with combat's own probabilistic hit/
// miss roll (JavaRandom-seeded, M5) NOT forced to a specific outcome --
// instead every assertion here is an invariant that holds REGARDLESS of
// which way the roll goes (e.g. "hp strictly decreased if and only if
// ProcessAttack returned true"), which is the actual thing this
// milestone's own wiring needs to get right; the underlying hit/miss
// math itself was already verified against real data in M14/M15/M16.
#include <cstdio>
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
#include "player/player_movement.h"
#include "player/player_state.h"
#include "render/message_popup.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::CombatTick;
using dawnstar::GeneratedLevel;
using dawnstar::MessagePopup;
using dawnstar::MessagePopupState;
using dawnstar::MonsterRuntime;
using dawnstar::MonsterState;
using dawnstar::PackPosKey;
using dawnstar::PlayerCreation;
using dawnstar::PlayerMovement;
using dawnstar::PlayerState;
using dawnstar::WorldRegistry;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

// Same helper as m25/m28's own FindApproach -- finds a walkable tile
// adjacent to (targetX,targetY) the player can stand on and face toward
// the target so a forward step lands exactly on it (duplicated per this
// project's established per-test-file self-containment convention).
bool FindApproach(const GeneratedLevel& level, int targetX, int targetY, int* standX, int* standY, int* facing) {
    struct Candidate {
        int dx, dy, f;
    };
    const Candidate candidates[4] = {
        {0, 1, 1},
        {0, -1, 3},
        {-1, 0, 2},
        {1, 0, 4},
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
            dawnstar::DungeonRuntime::RegisterGeneratedSpawns(levels.back(), world);
        }
        std::printf("generated + registered %zu levels\n", levels.size());

        // Find a non-hub level with at least one real pre-placed
        // monster spawn.
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
        int standX = 0, standY = 0, facing = 0;
        Check(FindApproach(level, spawn.x, spawn.y, &standX, &standY, &facing),
              "should find a walkable approach tile facing the real monster spawn");

        dawnstar::JavaRandom globalRng(777);
        PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
        player.currentLevel = levelNumber;
        player.tileX = standX;
        player.tileY = standY;
        player.facing = facing;

        // --- A: MonsterInFront ---
        {
            auto* record = PlayerMovement::MonsterInFront(player, levels, world);
            Check(record != nullptr, "MonsterInFront should find the real registered spawn directly ahead");
            if (record != nullptr) {
                MonsterState decoded = MonsterRuntime::FromBytes(*record);
                Check(decoded.x == spawn.x && decoded.y == spawn.y,
                      "the found record's position should match the real spawn's own position");
            }

            // The hub (level 1) has no generated monster spawns at all
            // (confirmed by M24's own test) -- any position there is a
            // reliable "nothing in front" case.
            PlayerState hubPlayer = PlayerCreation::CreateCharacter(0, "Tester2", charData, items, globalRng);
            Check(PlayerMovement::MonsterInFront(hubPlayer, levels, world) == nullptr,
                  "MonsterInFront should find nothing in the monster-free hub");
        }

        // --- B: ProcessAttack -- cooldown gating + the hp-invariant,
        // regardless of which way the probabilistic hit roll goes ---
        {
            int64_t lastAttackTimeMs = -10000;  // far enough in the past to never block the first call
            auto* recordBefore = PlayerMovement::MonsterInFront(player, levels, world);
            Check(recordBefore != nullptr, "setup: monster should still be in front for the attack test");
            MonsterState before = MonsterRuntime::FromBytes(*recordBefore);

            bool actionTaken = false;
            bool hit = CombatTick::ProcessAttack(player, levels, world, monsterDb, items, charData, globalRng, 1000,
                                                  lastAttackTimeMs, actionTaken);
            auto* recordAfter = PlayerMovement::MonsterInFront(player, levels, world);
            MonsterState after = MonsterRuntime::FromBytes(*recordAfter);
            Check(hit == (after.hp < before.hp),
                  "ProcessAttack's return value should exactly match whether hp actually decreased");
            Check(lastAttackTimeMs == 1000, "a successful (non-cooldown-blocked) attempt should advance lastAttackTimeMs");
            Check(actionTaken, "a non-cooldown-blocked attempt with a target should set actionTaken, hit or miss");

            // Immediately again, well within the 500ms cooldown --
            // should be a hard no-op regardless of the roll.
            MonsterState beforeSecond = after;
            bool actionTakenSecond = false;
            bool hitSecond = CombatTick::ProcessAttack(player, levels, world, monsterDb, items, charData, globalRng,
                                                        1100, lastAttackTimeMs, actionTakenSecond);
            auto* recordThird = PlayerMovement::MonsterInFront(player, levels, world);
            MonsterState afterSecond = MonsterRuntime::FromBytes(*recordThird);
            Check(!hitSecond, "an attempt within the 500ms cooldown should never land a hit");
            Check(afterSecond.hp == beforeSecond.hp, "an attempt within the cooldown should leave hp untouched");
            Check(lastAttackTimeMs == 1000, "a cooldown-blocked attempt should NOT advance lastAttackTimeMs");
            Check(!actionTakenSecond, "a cooldown-blocked attempt should NOT set actionTaken");

            // No monster in front at all -- should also just return
            // false without touching the cooldown timer.
            PlayerState hubPlayer = PlayerCreation::CreateCharacter(0, "Tester3", charData, items, globalRng);
            int64_t hubLastAttack = -10000;
            bool hubActionTaken = false;
            Check(!CombatTick::ProcessAttack(hubPlayer, levels, world, monsterDb, items, charData, globalRng, 2000,
                                              hubLastAttack, hubActionTaken),
                  "ProcessAttack with no monster in front should return false");
            Check(hubLastAttack == -10000, "ProcessAttack with no monster in front should not touch the cooldown timer");
            Check(!hubActionTaken, "ProcessAttack with no monster in front should NOT set actionTaken");
        }

        // --- C: RefreshAndResolveTargetMonster -- alive monster (no
        // death resolution should fire) ---
        {
            player.monsterTargeted = false;
            MessagePopupState popup;
            int16_t nextSpawnId = 1;
            CombatTick::RefreshAndResolveTargetMonster(player, levels, world, monsterDb, items, popup, globalRng,
                                                        3000, nextSpawnId);
            Check(player.monsterTargeted, "monsterTargeted should be set true while a (still-alive) monster is in front");
            Check(!popup.visible, "no death popup should show while the monster is still alive");
            auto* stillThere = PlayerMovement::MonsterInFront(player, levels, world);
            Check(stillThere != nullptr, "an alive monster should NOT be removed from the registry");
        }

        // --- D: RefreshAndResolveTargetMonster -- a dead (hp<=0)
        // monster in front should resolve fully ---
        {
            auto* record = PlayerMovement::MonsterInFront(player, levels, world);
            Check(record != nullptr, "setup: monster should still be in front for the death test");
            MonsterState dead = MonsterRuntime::FromBytes(*record);
            dead.hp = 0;
            dead.monsterType = 3;  // an ordinary type (neither 41 nor 42)
            *record = MonsterRuntime::ToBytes(dead);

            player.ailmentMask = 8;  // ailment 4 (bit 3) -- the kill-heal bonus
            player.coreStats[2] = 10;
            player.coreStats[3] = 100;
            player.minimapDirty = false;
            player.monsterTargeted = true;

            size_t dropsBefore = world.droppedItems[static_cast<size_t>(levelIdx)].size();
            MessagePopupState popup;
            int16_t nextSpawnId = 1;
            CombatTick::RefreshAndResolveTargetMonster(player, levels, world, monsterDb, items, popup, globalRng,
                                                        4000, nextSpawnId);

            Check(!player.monsterTargeted, "monsterTargeted should be cleared once the target is resolved dead");
            Check(player.minimapDirty, "resolving a death should mark the minimap dirty");
            Check(popup.visible && popup.lines[0] == "Creature" && popup.lines[1] == "is dead!" && popup.priority == 1,
                  "the death popup should show \"Creature/is dead!\" at priority 1");

            auto* afterDeath = PlayerMovement::MonsterInFront(player, levels, world);
            Check(afterDeath == nullptr, "the dead monster should be removed from its own level's registry");
            Check((level.tiles[static_cast<size_t>(spawn.x)][static_cast<size_t>(spawn.y)] & 2) == 0,
                  "the dead monster's tile should have its monster-presence bit (2) cleared");

            int expectedHp = 10 + 3 * 100 / 10;  // = 40, well under the 100 cap -- no clamping exercised here
            Check(player.coreStats[2] == expectedHp,
                  "the ailment-4 kill-heal bonus should apply the exact 3*maxHp/10 formula");

            size_t dropsAfter = world.droppedItems[static_cast<size_t>(levelIdx)].size();
            Check(dropsAfter == dropsBefore || dropsAfter == dropsBefore + 1,
                  "a death should register at most one new dropped-item record on the monster's own level");
            if (dropsAfter == dropsBefore + 1) {
                const auto& dropRecord = world.droppedItems[static_cast<size_t>(levelIdx)].back();
                Check(dropRecord[0] == static_cast<uint8_t>(spawn.x) && dropRecord[1] == static_cast<uint8_t>(spawn.y),
                      "a registered drop should sit at the dead monster's own position");
            }
        }

        // --- E: monsterType 41 (the special roaming monster) still
        // rolls a death-drop and clears the special-encounter flags ---
        {
            // Re-derive a fresh in-front monster: E and F each need their
            // own untouched record, so place a brand-new one directly.
            MonsterState special = MonsterRuntime::Spawn(99, 41, levelNumber, monsterDb);
            special.x = static_cast<int8_t>(spawn.x);
            special.y = static_cast<int8_t>(spawn.y);
            special.hp = 0;
            world.monsters[static_cast<size_t>(levelIdx)][PackPosKey(spawn.x, spawn.y)] = MonsterRuntime::ToBytes(special);
            level.tiles[static_cast<size_t>(spawn.x)][static_cast<size_t>(spawn.y)] |= 2;

            player.roamingSpecialMonsterPresent = true;
            player.specialEncounterResolved = false;
            player.ailmentMask = 0;

            MessagePopupState popup;
            int16_t nextSpawnId = 1;
            CombatTick::RefreshAndResolveTargetMonster(player, levels, world, monsterDb, items, popup, globalRng,
                                                        5000, nextSpawnId);

            Check(player.specialEncounterResolved, "type 41's death should set specialEncounterResolved");
            Check(!player.roamingSpecialMonsterPresent, "type 41's death should clear roamingSpecialMonsterPresent");
            Check(PlayerMovement::MonsterInFront(player, levels, world) == nullptr,
                  "type 41 should still be removed from the registry like any other death");
        }

        // --- F: monsterType 42 skips the loot roll entirely, but still
        // runs the rest of resolveMonsterDeath's cleanup ---
        {
            MonsterState endBoss = MonsterRuntime::Spawn(100, 42, levelNumber, monsterDb);
            endBoss.x = static_cast<int8_t>(spawn.x);
            endBoss.y = static_cast<int8_t>(spawn.y);
            endBoss.hp = 0;
            world.monsters[static_cast<size_t>(levelIdx)][PackPosKey(spawn.x, spawn.y)] =
                MonsterRuntime::ToBytes(endBoss);
            level.tiles[static_cast<size_t>(spawn.x)][static_cast<size_t>(spawn.y)] |= 2;

            size_t dropsBefore = world.droppedItems[static_cast<size_t>(levelIdx)].size();
            MessagePopupState popup;
            int16_t nextSpawnId = 1;
            CombatTick::RefreshAndResolveTargetMonster(player, levels, world, monsterDb, items, popup, globalRng,
                                                        6000, nextSpawnId);

            size_t dropsAfter = world.droppedItems[static_cast<size_t>(levelIdx)].size();
            Check(dropsAfter == dropsBefore, "monsterType 42 should never roll a death-drop (onDeath is skipped)");
            Check(popup.visible, "monsterType 42 should still show the death popup (same fallthrough as any other type)");
            Check(PlayerMovement::MonsterInFront(player, levels, world) == nullptr,
                  "monsterType 42 should still be removed from the registry");
        }

        if (g_ok) {
            std::printf("all combat-tick checks passed\n");
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
