// M44 smoke test: PassiveTick (passive/passive_tick.h) --
// GameCanvas.java's own run()-tail timed systems: tickStatusCountdowns
// (lines ~1805-1832) and tickPerSecond (lines ~1840-2003).
//
// No JVM ground truth is available (same reason as every prior
// milestone). Every real target is independently re-derived from
// ../../../src/GameCanvas.java directly, not read back from
// passive_tick.cpp -- including the ambush checkpoint tables
// (transcribed AGAIN here), the `1 + Util.randomInt(17)` LingoRandomInt
// idiom, and the X-then-Y retry-roll order, which the twin-seeded replay
// oracle below consumes to predict the exact landing tile of a checkpoint
// spawn (the same technique m22_dungeon_runtime_smoke.cpp established for
// TrySpawnMonsterNear itself).
#include <cstdio>
#include <set>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/spell_database.h"
#include "dungeon/dungeon_runtime.h"
#include "monster/monster_runtime.h"
#include "passive/passive_tick.h"
#include "player/player_combat_stats.h"
#include "player/player_inventory.h"
#include "player/player_state.h"
#include "render/message_popup.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::DungeonRuntime;
using dawnstar::GeneratedLevel;
using dawnstar::ItemDatabase;
using dawnstar::JavaRandom;
using dawnstar::LingoRandomInt;
using dawnstar::MessagePopup;
using dawnstar::MessagePopupState;
using dawnstar::MonsterDatabase;
using dawnstar::MonsterRuntime;
using dawnstar::MonsterState;
using dawnstar::PackPosKey;
using dawnstar::PassiveTick;
using dawnstar::PlayerCombatStats;
using dawnstar::PlayerInventory;
using dawnstar::PlayerState;
using dawnstar::WorldRegistry;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

// The keys of one level's monster registry, as a comparable set.
std::set<int> MonsterKeys(const WorldRegistry& world, int levelIndex) {
    std::set<int> keys;
    for (const auto& [key, record] : world.monsters[static_cast<size_t>(levelIndex)]) {
        keys.insert(key);
        (void)record;
    }
    return keys;
}

// The one key present in `after` but not `before` (the tile a fresh
// spawn landed on), or PackPosKey(-1, -1) if there isn't exactly one.
int NewMonsterKey(const std::set<int>& before, const std::set<int>& after) {
    std::vector<int> added;
    for (int key : after) {
        if (before.count(key) == 0) added.push_back(key);
    }
    return added.size() == 1 ? added[0] : PackPosKey(-1, -1);
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        ItemDatabase items = ItemDatabase::Load(archive);
        MonsterDatabase monsterDb = MonsterDatabase::Load(archive);
        dawnstar::SpellDatabase spells = dawnstar::SpellDatabase::Load(archive);
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
        int16_t counter = 1;
        MessagePopupState popup;
        // Scratch rngs for the scenarios that don't test RNG order
        // (TickPerSecond takes its rng by mutable reference, so
        // temporaries can't be passed directly); E2's twin-match scenario
        // below uses its own named locals instead.
        JavaRandom tickRng1(1);
        JavaRandom tickRng5(5);

        // --- A: tickStatusCountdowns -- the three timed ailment
        // countdowns, their monsterAttacking gate, and the expiry arms'
        // real idempotency (same-bit writes). ---
        {
            PlayerState p;
            p.ailmentMask = static_cast<int8_t>(1 << 3);  // Troll Thirst (hasAilment(4))
            p.trollThirstTimer = 5000;
            p.glacierCurseTimer = 1000;
            p.terrifiedTimer = 100;
            PassiveTick::TickStatusCountdowns(p, 2000, false);
            Check(p.trollThirstTimer == 3000, "Troll Thirst timer should count down by the elapsed ms");
            Check(p.glacierCurseTimer == 1000 && p.terrifiedTimer == 100,
                  "the OTHER two timers must not count down while their own ailments are inactive");
            PassiveTick::TickStatusCountdowns(p, 2000, false);
            Check(p.trollThirstTimer == 1000, "a second countdown call should count down further");
            PassiveTick::TickStatusCountdowns(p, 2000, false);
            Check(p.trollThirstTimer == 0, "an expiring timer should clamp at 0 (not wrap negative)");
            Check((p.ailmentMask & (1 << 3)) != 0 && p.ailmentMask == static_cast<int8_t>(1 << 3),
                  "the expiry arm writes the SAME bit the branch condition already requires -- the real idempotent no-op");

            p.ailmentMask = static_cast<int8_t>(1 << 4);  // Glacier Curse (hasAilment(5))
            p.glacierCurseTimer = 1000;
            PassiveTick::TickStatusCountdowns(p, 2000, false);
            Check(p.glacierCurseTimer == 0 && p.ailmentMask == static_cast<int8_t>(1 << 4),
                  "Glacier Curse expiry should also zero the timer and leave its own (already-set) bit unchanged");

            p.ailmentMask = static_cast<int8_t>(1 << 6);  // Terrified (hasAilment(7))
            p.terrifiedTimer = 100;
            PassiveTick::TickStatusCountdowns(p, 50, false);
            Check(p.terrifiedTimer == 100,
                  "Terrified must not count down while monsterAttacking is false (its real gate)");
            PassiveTick::TickStatusCountdowns(p, 50, true);
            Check(p.terrifiedTimer == 50, "Terrified should count down while monsterAttacking is true");
            PassiveTick::TickStatusCountdowns(p, 100, true);
            Check(p.terrifiedTimer == 0 && p.ailmentMask == static_cast<int8_t>(1 << 6),
                  "Terrified expiry should zero the timer and leave its own bit set");
        }

        // --- B: tickPerSecond's own ailment blocks: Troll Thirst's
        // unclamped HP drain, and Glacier Curse's overflow-resets-Magicka-
        // to-0 quirk (GameCanvas.java lines 1841-1855). ---
        {
            PlayerState p;
            p.ailmentMask = static_cast<int8_t>(1 << 3);  // Troll Thirst
            p.coreStats[2] = 50;                           // HP
            p.coreStats[3] = 100;                         // maxHP
            PassiveTick::TickPerSecond(p, levels, world, items, monsterDb, tickRng1, counter, 100000, popup);
            Check(p.coreStats[2] == 48, "Troll Thirst should drain 2*maxHP/100 = 2 HP per second");
            for (int i = 0; i < 25; i++) {
                PassiveTick::TickPerSecond(p, levels, world, items, monsterDb, tickRng1, counter, 100000, popup);
            }
            Check(p.coreStats[2] == -2,
                  "the drain is NOT clamped at 0 below -- HP can go negative, preserved exactly as the original");

            PlayerState q;
            q.ailmentMask = static_cast<int8_t>(1 << 4);  // Glacier Curse
            q.coreStats[2] = 80;
            q.coreStats[4] = 95;  // Magicka
            q.coreStats[5] = 100;
            PassiveTick::TickPerSecond(q, levels, world, items, monsterDb, tickRng1, counter, 100000, popup);
            Check(q.coreStats[4] == 0 && q.coreStats[2] == 70,
                  "reaching max Magicka should RESET it to 0 and drain maxMagicka/10 = 10 HP -- the real overflow quirk");
            q.coreStats[4] = 50;
            q.coreStats[2] = 80;
            PassiveTick::TickPerSecond(q, levels, world, items, monsterDb, tickRng1, counter, 100000, popup);
            Check(q.coreStats[4] == 60 && q.coreStats[2] == 80,
                  "below max Magicka the regen should just add maxMagicka/10 with no reset or HP drain");
        }

        // --- C: the effectDurations[] per-second countdown and its one
        // special case: index 5 (effect 6, "Safe Camping") expiring
        // removes the EQUIPPED item-101 StarFrost (GameCanvas.java lines
        // 1857-1870, Player.java's findInventorySlotOf). ---
        {
            PlayerState p;
            p.effectDurations[3] = 2;
            p.effectDurations[4] = -1;  // "until cured" -- must never count down
            p.effectDurations[5] = 2;
            p.effectDurations[6] = -2;  // "until a condition check" -- likewise
            p.effectDurations[0] = 1;
            // The equipped (negative-encoded) StarFrost spell 6 grants,
            // plus an unrelated item at slot 0.
            p.inventoryItemIds[0] = 1;
            p.inventoryItemIds[1] = -101;
            p.inventoryCount = 2;
            PassiveTick::TickPerSecond(p, levels, world, items, monsterDb, tickRng1, counter, 100000, popup);
            Check(p.effectDurations[0] == 0 && p.effectDurations[3] == 1 && p.effectDurations[5] == 1,
                  "positive effect durations should count down by 1 per second");
            Check(p.effectDurations[4] == -1 && p.effectDurations[6] == -2,
                  "negative durations (-1/-2) must never count down");
            Check(p.inventoryCount == 2, "a not-yet-expired effect 6 must not remove the StarFrost");
            PassiveTick::TickPerSecond(p, levels, world, items, monsterDb, tickRng1, counter, 100000, popup);
            Check(p.effectDurations[5] == 0 && p.inventoryCount == 1 && p.inventoryItemIds[0] == 1,
                  "effect 6 expiring should remove the equipped StarFrost and compact the inventory");
            Check(PlayerInventory::FindSlotOf(p, 101) == -1, "the removed StarFrost should no longer be findable");

            // The real quirk: an UNEQUIPPED (positive) StarFrost survives
            // the expiry -- findInventorySlotOf only ever matches the
            // negative-encoded form.
            PlayerState q;
            q.effectDurations[5] = 1;
            q.inventoryItemIds[0] = 101;
            q.inventoryCount = 1;
            PassiveTick::TickPerSecond(q, levels, world, items, monsterDb, tickRng1, counter, 100000, popup);
            Check(q.effectDurations[5] == 0 && q.inventoryCount == 1 && q.inventoryItemIds[0] == 101,
                  "an UNEQUIPPED StarFrost survives the effect-6 expiry -- findInventorySlotOf's equipped-only match, preserved");
        }

        // --- D: the per-level scratch-cooldown block is a real NO-OP in
        // the original (decode into a throwaway copy, never stored back)
        // -- preserved as no observable change to any record. ---
        {
            PlayerState p;
            p.currentLevel = 2;
            std::set<int> before = MonsterKeys(world, 1);
            // A hand-built record with its cooldown scratch bytes "live".
            MonsterState m = MonsterRuntime::Spawn(9000, 1, 2, monsterDb);
            m.x = 1;
            m.y = 1;
            m.scratch[6] = 1;
            m.scratch[7] = 1;
            world.monsters[1][PackPosKey(1, 1)] = MonsterRuntime::ToBytes(m);
            PassiveTick::TickPerSecond(p, levels, world, items, monsterDb, tickRng1, counter, 100000, popup);
            MonsterState after = MonsterRuntime::FromBytes(world.monsters[1][PackPosKey(1, 1)]);
            Check(after.scratch[6] == 1 && after.scratch[7] == 1,
                  "the scratch-cooldown block must change NOTHING -- the original never stores its mutated copy back");
            world.monsters[1].erase(PackPosKey(1, 1));  // restore for the sections below
            (void)before;
        }

        // --- E: the ambush spawner -- the checkpoint schedules, the
        // twin-replayed spawn position/type, the "Enemy arrived!" popup,
        // and the >5-monsters Game Over trigger. The HUB level is used
        // (player.currentLevel = 1): it starts with zero registered
        // monsters, exactly like the real post-Reveal situation the
        // original arms this system in (resetToHubPosition teleports the
        // player home first). The checkpoint tables are transcribed AGAIN
        // here from GameCanvas.java lines 1893-1985, independently of
        // passive_tick.cpp. ---
        {
            // E1: an inactive timer (-1) does nothing at all.
            PlayerState p;
            p.currentLevel = 1;
            p.ambushTimer = -1;
            size_t hubBefore = world.monsters[0].size();
            PassiveTick::TickPerSecond(p, levels, world, items, monsterDb, tickRng5, counter, 100000, popup);
            Check(p.ambushTimer == -1 && world.monsters[0].size() == hubBefore,
                  "an inactive (-1) ambushTimer must leave everything untouched (the >= 0 gate)");

            // E2: the NON-NGP elapsed-5 checkpoint, with the twin-seeded
            // replay oracle predicting the landing tile and type.
            p.ambushTimer = 4;  // ++ -> 5, the first non-NGP checkpoint
            std::vector<GeneratedLevel> levelsCopy = levels;
            WorldRegistry worldCopy = world;
            std::set<int> keysBefore = MonsterKeys(world, 0);
            JavaRandom rng(777);
            int16_t counterE = 1;
            MessagePopupState popupE;
            Check(PassiveTick::TickPerSecond(p, levels, world, items, monsterDb, rng, counterE, 100000, popupE) ==
                      PassiveTick::PerSecondResult::None,
                  "a checkpoint spawn into a not-yet-overwhelmed hub is not a Game Over");
            Check(p.ambushTimer == 5, "each armed tick should advance the ambush clock by exactly 1");
            int realKey = NewMonsterKey(keysBefore, MonsterKeys(world, 0));
            Check(realKey != PackPosKey(-1, -1), "exactly one new monster should appear on the hub");
            Check(popupE.visible && popupE.lines[0] == "Enemy" && popupE.lines[1] == "arrived!",
                  "the real MSG_ENEMY_ARRIVED popup should be showing");
            // The twin: a fresh identically-seeded rng replaying the SAME
            // `1 + Util.randomInt(17)` X-then-Y retry order against the
            // pre-spawn snapshots, so its landing tile and rolled type must
            // match the real run exactly (m22's own oracle technique).
            JavaRandom twin(777);
            int16_t twinCounter = 1;
            int twinX = 1 + LingoRandomInt(twin, 17);
            int twinY = 1 + LingoRandomInt(twin, 17);
            while (!DungeonRuntime::TrySpawnMonsterNear(levelsCopy, worldCopy, 0, twinX, twinY, 4, twin, monsterDb,
                                                        twinCounter)) {
                twinX = 1 + LingoRandomInt(twin, 17);
                twinY = 1 + LingoRandomInt(twin, 17);
            }
            int twinKey = NewMonsterKey(keysBefore, MonsterKeys(worldCopy, 0));
            Check(twinKey == realKey, "the twin-seeded replay should predict the exact landing tile");
            MonsterState realMonster = MonsterRuntime::FromBytes(world.monsters[0][realKey]);
            MonsterState twinMonster = MonsterRuntime::FromBytes(worldCopy.monsters[0][twinKey]);
            Check(realMonster.monsterType == twinMonster.monsterType && realMonster.x == twinMonster.x &&
                      realMonster.y == twinMonster.y,
                  "the twin-seeded replay should predict the exact rolled monster too (the tier-4 roll)");

            // E3: the schedules really differ -- elapsed 5 is NOT a
            // newGamePlus checkpoint (and no popup either).
            PlayerState q;
            q.currentLevel = 1;
            q.ambushTimer = 4;
            q.newGamePlus = true;
            size_t hubE3 = world.monsters[0].size();
            MessagePopupState popupE3;
            PassiveTick::TickPerSecond(q, levels, world, items, monsterDb, tickRng5, counter, 100000, popupE3);
            Check(q.ambushTimer == 5 && world.monsters[0].size() == hubE3,
                  "elapsed 5 must NOT spawn while newGamePlus picks the alternate schedule");
            Check(!popupE3.visible, "a non-checkpoint second must not show the Enemy-arrived popup");

            // E4: elapsed 3 IS a newGamePlus checkpoint.
            q.ambushTimer = 2;
            MessagePopupState popupE4;
            PassiveTick::TickPerSecond(q, levels, world, items, monsterDb, tickRng5, counter, 100000, popupE4);
            Check(q.ambushTimer == 3 && world.monsters[0].size() == hubE3 + 1,
                  "elapsed 3 must spawn while newGamePlus picks the alternate schedule");
            Check(popupE4.visible && popupE4.lines[0] == "Enemy",
                  "the newGamePlus checkpoint spawn should also show the popup");
            world.monsters[0].erase(NewMonsterKey(MonsterKeys(worldCopy, 0), MonsterKeys(world, 0)));

            // E5: elapsed 140 always spawns the literal type-42 end-game
            // monster (the 41/42 forced-type special M22 traced), on top of
            // EITHER schedule.
            PlayerState r;
            r.currentLevel = 1;
            r.ambushTimer = 139;
            std::set<int> before140 = MonsterKeys(world, 0);
            PassiveTick::TickPerSecond(r, levels, world, items, monsterDb, tickRng5, counter, 100000, popup);
            int key140 = NewMonsterKey(before140, MonsterKeys(world, 0));
            Check(key140 != PackPosKey(-1, -1), "elapsed 140 should spawn the end-game monster");
            MonsterState m42 = MonsterRuntime::FromBytes(world.monsters[0][key140]);
            Check(m42.monsterType == 42, "elapsed 140's spawn must be the literal type-42 end-game monster");
            world.monsters[0].erase(key140);

            // E6: >5 monsters on the level after a checkpoint spawn ->
            // EndOfGame, and NO popup (the original's two branches are
            // mutually exclusive).
            for (int i = 0; i < 5; i++) {
                MonsterState filler = MonsterRuntime::Spawn(static_cast<int16_t>(9100 + i), 1, 1, monsterDb);
                world.monsters[0][PackPosKey(3, 5 + i)] = MonsterRuntime::ToBytes(filler);
            }
            PlayerState s;
            s.currentLevel = 1;
            s.ambushTimer = 4;
            MessagePopupState popupE6;
            Check(PassiveTick::TickPerSecond(s, levels, world, items, monsterDb, tickRng5, counter, 100000,
                                             popupE6) == PassiveTick::PerSecondResult::EndOfGame,
                  "a checkpoint spawn into a level already holding 5 monsters should report EndOfGame (> 5)");
            Check(!popupE6.visible, "the EndOfGame branch must NOT also show the Enemy-arrived popup");
        }

        // --- F: PlayerInventory::FindSlotOf (Player.java's
        // findInventorySlotOf) in isolation. ---
        {
            PlayerState p;
            p.inventoryItemIds[0] = 5;
            p.inventoryItemIds[1] = -101;
            p.inventoryItemIds[2] = 101;
            p.inventoryCount = 3;
            Check(PlayerInventory::FindSlotOf(p, 101) == 1,
                  "FindSlotOf should find the equipped (negative-encoded) copy");
            Check(PlayerInventory::FindSlotOf(p, 5) == -1,
                  "FindSlotOf should never match a merely-owned positive copy -- the equipped-only quirk");
            Check(PlayerInventory::FindSlotOf(p, 99) == -1, "an absent item should not be found");
        }

        if (g_ok) {
            std::printf("all passive-tick checks passed\n");
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
