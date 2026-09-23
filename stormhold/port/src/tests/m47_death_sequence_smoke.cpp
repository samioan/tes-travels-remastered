// M47 smoke test: DeathSequence/DeathState (GameCanvas.tickDeathAndRegen()
// plus run()'s own already-Java-transcribed `facing != 1` death/respawn
// state machine, finally wired to a real driver) plus the small pieces it
// leans on that had no port-side counterpart before this milestone:
// PlayerCombatStats::TickFatigueRegen, PlayerCreation::NormalizeToMaxStats/
// RespawnAfterDeath, PlayerInventory::IsSlotEquipped, and the new
// assets/dungeon_names.h loader.
#include <cstdio>
#include <string>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/dungeon_names.h"
#include "assets/item_database.h"
#include "player/death_sequence.h"
#include "player/player_combat_stats.h"
#include "player/player_creation.h"
#include "player/player_inventory.h"

namespace {

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

using namespace stormhold;

void TestTickFatigueRegen() {
    std::printf("-- PlayerCombatStats::TickFatigueRegen --\n");

    PlayerState p;
    p.attributes[10] = 40;
    p.attributes[11] = 10;  // (40+10)/2000 * deltaMs
    p.coreStats[6] = 50;
    p.coreStats[7] = 100;
    PlayerCombatStats::TickFatigueRegen(p, 4000);
    // gain = 4000 * 50 / 2000 = 100 -> 50+100=150, clamped to maxFatigue 100.
    Expect(p.coreStats[6] == 100, "TickFatigueRegen clamps at maxFatigue");

    PlayerState q;
    q.attributes[10] = 40;
    q.attributes[11] = 10;
    q.coreStats[6] = 10;
    q.coreStats[7] = 1000;
    PlayerCombatStats::TickFatigueRegen(q, 4000);
    Expect(q.coreStats[6] == 110, "unclamped gain matches deltaMs*(attr10+attr11)/2000 exactly");
}

void TestTickDeathAndRegen(const CharacterData& charData) {
    std::printf("-- DeathSequence::TickDeathAndRegen --\n");

    {
        // Alive: regen ticks, facing/death state untouched.
        PlayerState p;
        p.facing = 1;
        p.coreStats[2] = 50;
        p.coreStats[3] = 100;  // HP 50/100, alive
        p.attributes[10] = 40;
        p.attributes[11] = 10;
        p.coreStats[6] = 0;
        p.coreStats[7] = 1000;
        DeathState death;
        bool justDied = DeathSequence::TickDeathAndRegen(p, death, charData, /*now=*/5000, /*deltaMs=*/250);
        Expect(!justDied, "alive (HP>0) -> TickDeathAndRegen returns false");
        Expect(p.facing == 1, "facing untouched while alive");
        Expect(death.deathAtMs == 0, "deathAtMs untouched while alive");
        Expect(p.coreStats[6] > 0, "Fatigue regen still ticks while alive");
    }

    {
        // HP already at 0: dies this call.
        PlayerState p;
        p.facing = 1;
        p.coreStats[2] = 0;
        p.coreStats[3] = 100;
        DeathState death;
        bool justDied = DeathSequence::TickDeathAndRegen(p, death, charData, /*now=*/9000, /*deltaMs=*/250);
        Expect(justDied, "HP<=0 -> TickDeathAndRegen returns true (just died)");
        Expect(p.facing == 2, "HP<=0 sets facing=2");
        Expect(death.deathAtMs == 9000, "HP<=0 stamps deathAtMs to 'now'");
    }

    {
        // HP negative (a real overshoot -- e.g. a big hit landing well
        // past 0) still counts as dead, matching the original's own
        // `hp <= 0` check exactly, not `== 0`.
        PlayerState p;
        p.facing = 1;
        p.coreStats[2] = -37;
        p.coreStats[3] = 100;
        DeathState death;
        bool justDied = DeathSequence::TickDeathAndRegen(p, death, charData, /*now=*/1, /*deltaMs=*/250);
        Expect(justDied, "negative HP also counts as dead (<=0, not ==0)");
    }
}

void TestTickAliveAndWaiting(const ItemDatabase& items) {
    std::printf("-- DeathSequence::Tick: Alive and Waiting branches --\n");

    {
        PlayerState p;
        p.facing = 1;
        DeathState death;
        DeathTickResult result = DeathSequence::Tick(p, death, items, /*now=*/12345);
        Expect(result == DeathTickResult::Alive, "facing==1 -> Alive, an ordinary tick");
        Expect(p.facing == 1, "Alive leaves facing untouched");
    }

    {
        // facing==2 (just died, per TickDeathAndRegen above): the very
        // next Tick() call transitions to 3, and the timeout hasn't
        // elapsed (0ms since death.deathAtMs==now).
        PlayerState p;
        p.facing = 2;
        DeathState death;
        death.deathAtMs = 10000;
        DeathTickResult result = DeathSequence::Tick(p, death, items, /*now=*/10000);
        Expect(result == DeathTickResult::Waiting, "facing==2, 0ms elapsed -> Waiting (transitions to 3 internally)");
        Expect(p.facing == 3, "facing==2 -> Tick() advances it to 3 on this same call");
    }

    {
        // facing==3, well within the 5s window.
        PlayerState p;
        p.facing = 3;
        DeathState death;
        death.deathAtMs = 10000;
        DeathTickResult result = DeathSequence::Tick(p, death, items, /*now=*/14000);  // 4000ms elapsed
        Expect(result == DeathTickResult::Waiting, "facing==3, 4000ms elapsed (<=5000) -> Waiting");
        Expect(p.facing == 3, "facing untouched while still waiting");
        Expect(death.deathAtMs == 10000, "deathAtMs untouched while still waiting");
    }

    {
        // Exact boundary: <= 5000 is still Waiting (matches
        // `now - deathAtMs <= 5000L` in the port, mirroring the
        // original's own `> 5000L` respawn-trigger condition exactly).
        PlayerState p;
        p.facing = 3;
        DeathState death;
        death.deathAtMs = 0;
        DeathTickResult result = DeathSequence::Tick(p, death, items, /*now=*/5000);
        Expect(result == DeathTickResult::Waiting, "exactly 5000ms elapsed -> still Waiting (needs to EXCEED 5000)");
    }
}

void TestRespawn(const CharacterData& charData, const ItemDatabase& items) {
    std::printf("-- DeathSequence::Tick: the Respawned branch (full respawn) --\n");

    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    // CreateCharacter's own 2 starting items are already granted+equipped
    // (M9) -- add one more, deliberately left UNEQUIPPED, to prove the
    // strip-unequipped-inventory pass actually discriminates.
    int8_t equippedIdBefore = p.equippedItems[0];
    int countBefore = p.inventoryCount;
    PlayerInventory::AddInventoryItemRaw(p, /*itemId=*/5, /*packedValue=*/1, /*charge=*/0);
    Expect(!PlayerInventory::IsSlotEquipped(p, countBefore, items), "the newly-added item starts unequipped");
    Expect(PlayerInventory::IsSlotEquipped(p, 0, items), "CreateCharacter's own starting weapon slot is equipped");

    // Simulate death away from the hub, with a nonzero camp mark and
    // gift-point/rumor progress already accumulated -- respawn should
    // preserve all of that (the `full=true` branch's own skip list),
    // unlike a fresh character-creation reset.
    p.currentLevel = 7;
    p.tileX = 3;
    p.tileY = 4;
    p.facing = 4;
    p.giftPointsFound = 42;
    p.rumorRevealStep = 3;
    p.wardenLoreStep = 2;
    p.campLevel = 5;
    p.campX = 11;
    p.campY = 12;
    p.campFacing = 2;
    p.ailmentMask = static_cast<int8_t>(1 << 2);
    p.vampirismTimer = 500;
    p.effectDurations[0] = 10;
    p.lastCombatTargetId = 99;
    p.spellArmorBonus = 7;
    p.increaseHarmBuff = true;
    p.coreStats[2] = -5;  // dead
    p.coreStats[3] = 200;
    p.coreStats[4] = 3;
    p.coreStats[5] = 60;
    p.coreStats[6] = 1;
    p.coreStats[7] = 80;

    DeathState death;
    death.deathAtMs = 0;
    p.facing = 2;

    // facing==2 -> 3 (Waiting), matching the original's own transition
    // tick never respawning in the same call the timer was just stamped.
    DeathTickResult step1 = DeathSequence::Tick(p, death, items, /*now=*/0);
    Expect(step1 == DeathTickResult::Waiting, "the 2->3 transition tick is still Waiting, not an immediate respawn");

    // Still within the window.
    DeathTickResult step2 = DeathSequence::Tick(p, death, items, /*now=*/3000);
    Expect(step2 == DeathTickResult::Waiting, "3000ms elapsed -- still within the 5s window");

    // Past the window: full respawn.
    DeathTickResult step3 = DeathSequence::Tick(p, death, items, /*now=*/5001);
    Expect(step3 == DeathTickResult::Respawned, "5001ms elapsed -- past the 5s window, respawns");

    Expect(p.coreStats[2] == 200, "respawn fully restores HP");
    Expect(p.coreStats[4] == 60, "respawn fully restores Magicka");
    Expect(p.coreStats[6] == 80, "respawn fully restores Fatigue");
    Expect(p.currentLevel == 1 && p.tileX == 12 && p.tileY == 14 && p.facing == 1,
           "respawn lands at the DEATH/respawn hub point (12, 14), distinct from character creation's (9, 10)");
    Expect(death.deathAtMs == 0, "deathAtMs resets to 0 on respawn");

    Expect(p.giftPointsFound == 42 && p.rumorRevealStep == 3 && p.wardenLoreStep == 2,
           "full=true respawn PRESERVES gift/rumor/warden-lore progress, unlike fresh character creation");
    Expect(p.campLevel == 5 && p.campX == 11 && p.campY == 12 && p.campFacing == 2,
           "full=true respawn PRESERVES the camp mark");

    Expect(p.ailmentMask == 0, "respawn clears the ailment mask");
    Expect(p.vampirismTimer == 0, "respawn clears vampirismTimer");
    Expect(p.effectDurations[0] == 0, "respawn clears effectDurations");
    Expect(p.lastCombatTargetId == 0, "respawn clears lastCombatTargetId");
    Expect(p.spellArmorBonus == 0, "respawn clears spellArmorBonus");
    Expect(!p.increaseHarmBuff, "respawn clears increaseHarmBuff");

    Expect(p.equippedItems[0] == equippedIdBefore, "the equipped starting weapon survives respawn");
    Expect(p.inventoryCount == countBefore, "the un-equipped extra item was stripped on respawn "
                                             "(inventoryCount back to what CreateCharacter alone produced)");
    for (int slot = 0; slot < p.inventoryCount; slot++) {
        Expect(PlayerInventory::IsSlotEquipped(p, slot, items),
               "every surviving inventory slot after respawn is an equipped one");
    }
}

void TestRespawnMessageLines(const DungeonNames& names) {
    std::printf("-- DeathSequence::RespawnMessageLines --\n");

    {
        PlayerState p;
        p.enteredNewLevelZone = true;
        p.currentLevel = 1;
        auto lines = DeathSequence::RespawnMessageLines(p, names);
        Expect(lines[0] == "Warden's" && lines[1] == "Camp", "enteredNewLevelZone -> \"Warden's Camp\"");
    }

    {
        // Confirmed permanently unreachable via real play (player/
        // player_movement.h's own M10 finding: leftLevelZone can never
        // actually become true out of CommitMove) -- exercised here at
        // the unit level anyway, since the field itself still exists and
        // this method still reads it faithfully.
        PlayerState p;
        p.enteredNewLevelZone = false;
        p.leftLevelZone = true;
        p.currentLevel = 1;
        auto lines = DeathSequence::RespawnMessageLines(p, names);
        Expect(lines[0] == "Outer" && lines[1] == "Camp", "leftLevelZone (dead branch) -> \"Outer Camp\", preserved");
    }

    {
        PlayerState p;
        p.enteredNewLevelZone = false;
        p.leftLevelZone = false;
        p.currentLevel = 1;  // respawn always lands in the hub
        auto lines = DeathSequence::RespawnMessageLines(p, names);
        const auto& hubNames = names.DisplayNames(1);
        Expect(lines[0] == hubNames[0] && lines[1] == hubNames[1],
               "neither flag set -> the current (post-respawn, hub) level's own real display name");
        Expect(!lines[0].empty(), "the hub's real dungnamesin.dat name is non-empty");
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assets(root);
        CharacterData charData = CharacterData::Load(assets);
        ItemDatabase items = ItemDatabase::Load(assets);
        DungeonNames names = DungeonNames::Load(assets);

        TestTickFatigueRegen();
        TestTickDeathAndRegen(charData);
        TestTickAliveAndWaiting(items);
        TestRespawn(charData, items);
        TestRespawnMessageLines(names);

        if (!g_ok) {
            std::fprintf(stderr, "m47_death_sequence_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m47_death_sequence_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
