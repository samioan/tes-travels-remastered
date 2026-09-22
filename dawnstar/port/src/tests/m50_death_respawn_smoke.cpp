// M50 smoke test: DeathTick::TickDeathState (death/death_tick.h),
// PlayerMovement::ResetState (player/player_movement.h) and the
// PlayerState::clearDeathGreetingPending flag PlayerMovement::Move raises
// on a forward/backward step -- GameCanvas.processIdleTick()'s `hp <= 0`
// half (that half itself stays in main.cpp, see docs/PORT_ROADMAP.md's
// M50 entry), run()'s own `deathState != 1` state machine, Player.
// resetState(respawning) and Player.commitMove()'s own
// `Shop.showDeathGreeting = false` reset.
//
// No JVM ground truth (same reason as every prior milestone). Verified
// against the real 37-level generated world (M6/M24) and a real created
// character (M11): the timeline (alive -> just-died -> counting-down ->
// resolved) hand-derived from ../../../src/GameCanvas.java's own
// deathState chain; the resolve step's stat/inventory/position/flag
// changes hand-derived from ../../../src/Player.java's
// normalizeForSummary/resetState/resetToHubPosition and GameCanvas.
// java's own death branch (lines ~1327-1354).
#include <cstdio>
#include <string>
#include <vector>

#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/character_data.h"
#include "death/death_tick.h"
#include "dungeon/dungeon_runtime.h"
#include "npc/shop_interaction.h"
#include "player/player_creation.h"
#include "player/player_inventory.h"
#include "player/player_movement.h"
#include "render/message_popup.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::DeathTick;
using dawnstar::DungeonRuntime;
using dawnstar::GeneratedLevel;
using dawnstar::MessagePopupState;
using dawnstar::PlayerCreation;
using dawnstar::PlayerInventory;
using dawnstar::PlayerMovement;
using dawnstar::PlayerState;
using dawnstar::ShopState;
using dawnstar::WorldRegistry;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

bool FindInteriorWalkable(const GeneratedLevel& level, int* outX, int* outY) {
    for (int x = 1; x < level.width - 1; x++) {
        for (int y = 1; y < level.height - 1; y++) {
            if (PlayerMovement::IsWalkable(level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)])) {
                *outX = x;
                *outY = y;
                return true;
            }
        }
    }
    return false;
}

// A tile whose NORTH neighbor is also walkable, so a facing-north forward
// step is guaranteed to succeed.
bool FindWalkablePairFacingNorth(const GeneratedLevel& level, int* outX, int* outY) {
    for (int x = 1; x < level.width - 1; x++) {
        for (int y = 2; y < level.height - 1; y++) {
            bool here = PlayerMovement::IsWalkable(level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)]);
            bool north = PlayerMovement::IsWalkable(level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y - 1)]);
            if (here && north) {
                *outX = x;
                *outY = y;
                return true;
            }
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

        dawnstar::JavaRandom globalRng(50);

        // --- Dungeon.NAMES via DungeonRuntime::DisplayName ---
        Check(std::string(DungeonRuntime::DisplayName(1)) == "Dawnstar", "level 1's display name is \"Dawnstar\"");
        Check(std::string(DungeonRuntime::DisplayName(37)) == "Ice Council 3",
              "level 37's display name is \"Ice Council 3\" (the last of 37)");

        // --- A: alive (deathState == 1): a pure no-op ---
        {
            PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            ShopState shop = ShopState::Reset();
            MessagePopupState popup;
            int64_t deathTimeMs = 0;
            bool suppress = false;
            bool runTick =
                DeathTick::TickDeathState(p, levels, world, items, shop, popup, deathTimeMs, 999999, suppress);
            Check(runTick, "deathState 1 should leave runTick true (no branch of the elseif chain matches)");
            Check(p.deathState == 1 && !popup.visible && !suppress && !shop.showDeathGreeting,
                  "deathState 1 should touch nothing");
        }

        // --- B: just died (deathState == 2): transitions to 3, clears
        // the popup outright, freezes the tick ---
        {
            PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            ShopState shop = ShopState::Reset();
            MessagePopupState popup;
            dawnstar::MessagePopup::Show(popup, {"Some", "Message"}, 1, 0);
            p.deathState = 2;
            int64_t deathTimeMs = 5000;
            bool suppress = false;
            bool runTick =
                DeathTick::TickDeathState(p, levels, world, items, shop, popup, deathTimeMs, 5000, suppress);
            Check(!runTick, "the just-died tick should freeze (runTick false)");
            Check(p.deathState == 3, "deathState 2 should transition to 3 on its very next tick");
            Check(!popup.visible && popup.priority == 0, "the death transition should clear any visible popup outright");
            Check(!suppress, "the just-died tick shouldn't touch suppressMoveThisTick");
        }

        // --- C: counting down (deathState == 3, < 5000ms elapsed): a
        // hard no-op ---
        {
            PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            ShopState shop = ShopState::Reset();
            MessagePopupState popup;
            p.deathState = 3;
            int64_t deathTimeMs = 1000;
            bool suppress = false;
            bool runTick =
                DeathTick::TickDeathState(p, levels, world, items, shop, popup, deathTimeMs, 5999, suppress);
            Check(!runTick && p.deathState == 3 && deathTimeMs == 1000,
                  "under 5000ms since death should stay frozen, untouched");
            Check(!shop.showDeathGreeting && !popup.visible, "no respawn side effects yet");
        }

        // --- D: resolves (deathState == 3, > 5000ms elapsed): the full
        // respawn ---
        {
            PlayerState p = PlayerCreation::CreateCharacter(1, "Tester", charData, items, globalRng);
            p.deathState = 3;
            ShopState shop = ShopState::Reset();
            MessagePopupState popup;

            // Half-damaged HP/Magicka/Fatigue plus a nonzero [8], so
            // normalizeForSummary's healing is actually observable.
            p.coreStats[2] = static_cast<int16_t>(p.coreStats[3] / 2);
            p.coreStats[4] = static_cast<int16_t>(p.coreStats[5] / 2);
            p.coreStats[6] = static_cast<int16_t>(p.coreStats[7] / 2);
            p.coreStats[8] = 7;

            // One extra UNEQUIPPED item (dropped) alongside the class's
            // starting EQUIPPED gear (kept).
            int equippedSlots = p.inventoryCount;
            Check(equippedSlots > 0, "setup: a fresh character should start with equipped gear");
            PlayerInventory::AddItem(p, 30, 0, 0);  // an ordinary, unequipped item
            int totalBefore = p.inventoryCount;
            Check(totalBefore == equippedSlots + 1, "setup: exactly one extra unequipped slot");

            p.ailmentMask = static_cast<int8_t>(1 << 3);
            p.trollThirstTimer = 5;
            p.glacierCurseTimer = 6;
            p.terrifiedTimer = 7;
            p.unconfirmedZ = true;
            p.effectDurations[0] = 9;
            p.effectDurations[24] = -1;
            p.combatTargetSpawnId = 42;
            p.tempArmorBonus = 11;
            p.increaseHarmBuff = true;
            p.increaseArmorBuff = true;
            p.safeCampingBuff = true;
            p.starFrostBonusActive = true;
            p.currentLevel = 5;
            p.tileX = 3;
            p.tileY = 4;
            p.facing = 2;
            p.minimapDirty = false;

            int64_t deathTimeMs = 0;
            bool suppress = false;
            bool runTick =
                DeathTick::TickDeathState(p, levels, world, items, shop, popup, deathTimeMs, 5001, suppress);

            Check(runTick, "a resolved death should return runTick true (the rest of the tick resumes)");
            Check(p.deathState == 1 && deathTimeMs == 0, "a resolved death should reset deathState to 1 and clear deathTimeMs");
            Check(suppress, "a resolved death should set suppressMoveThisTick (the respawned player doesn't walk off at once)");

            Check(p.coreStats[2] == p.coreStats[3] && p.coreStats[4] == p.coreStats[5] &&
                      p.coreStats[6] == p.coreStats[7] && p.coreStats[8] == 0,
                  "normalizeForSummary: current = max HP/Magicka/Fatigue, coreStats[8] zeroed");

            Check(p.inventoryCount == equippedSlots, "every UNEQUIPPED item should be dropped, equipped gear kept");
            for (int slot = 0; slot < p.inventoryCount; slot++) {
                Check(PlayerInventory::IsEquipped(p, items, slot), "every surviving slot should still be equipped");
            }

            Check(!p.starFrostBonusActive, "starFrostBonusActive should be cleared");

            Check(p.ailmentMask == 0 && p.trollThirstTimer == 0 && p.glacierCurseTimer == 0 && p.terrifiedTimer == 0 &&
                      !p.unconfirmedZ,
                  "resetState should clear the ailment mask and the 3 timers and unconfirmedZ");
            Check(p.effectDurations[0] == 0 && p.effectDurations[24] == 0, "resetState should zero every effectDurations entry");
            Check(p.combatTargetSpawnId == 0 && p.tempArmorBonus == 0 && !p.increaseHarmBuff && !p.increaseArmorBuff &&
                      !p.safeCampingBuff,
                  "resetState should clear the combat target and buff flags");

            Check(p.currentLevel == 1 && p.tileX == 13 && p.tileY == 6 && p.facing == 4,
                  "resetState(true) should reposition to the hub's ALT spawn point (13,6,facing 4)");

            Check(shop.showDeathGreeting, "the resolve should set Shop.showDeathGreeting");
            Check(p.minimapDirty, "the resolve should raise minimapDirty");
            Check(popup.visible && popup.priority == 1,
                  "the resolve should show the (now-current, i.e. hub) level's name as a popup");
            // resetState already moved the player to the hub by the time
            // the message is built, matching GameCanvas's own statement
            // order (resetState(true) runs BEFORE displayName() is read)
            // -- so this always reads "Dawnstar", never the level died on.
            Check(popup.lines == dawnstar::MessagePopup::WrapToTwoLines("Dawnstar"),
                  "the respawn message names the HUB level (\"Dawnstar\"), not the level died on");
        }

        // --- E: campState takes priority -- main.cpp's own call-site
        // contract (campStateBeforeTick == 0 gates whether TickDeathState
        // is even called) isn't exercised by this unit test directly, but
        // TickDeathState itself has no campState awareness at all, by
        // design: verified here as "deathState 1 is untouched regardless
        // of campState", confirming this function makes no campState
        // decision of its own for main.cpp's caller to accidentally rely
        // on.
        {
            PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            p.campState = 1;
            ShopState shop = ShopState::Reset();
            MessagePopupState popup;
            int64_t deathTimeMs = 0;
            bool suppress = false;
            bool runTick =
                DeathTick::TickDeathState(p, levels, world, items, shop, popup, deathTimeMs, 100, suppress);
            Check(runTick && p.deathState == 1, "TickDeathState itself doesn't consult campState -- the caller must gate it");
        }

        // --- F: PlayerMovement::Move raises clearDeathGreetingPending on
        // a committed forward step, not on a blocked one or a turn ---
        {
            int x = 0, y = 0;
            Check(FindWalkablePairFacingNorth(levels[0], &x, &y), "need a walkable north-facing pair on the hub level");

            PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            p.currentLevel = 1;
            p.tileX = x;
            p.tileY = y;
            p.facing = 1;  // north
            p.coreStats[6] = 50;
            Check(!p.clearDeathGreetingPending, "setup: the flag starts clear");

            bool moved = PlayerMovement::Move(p, 1, false, levels, world, items);
            Check(moved, "setup: the forward step should have succeeded");
            Check(p.clearDeathGreetingPending, "a committed forward step should raise clearDeathGreetingPending");
        }
        {
            // A turn (direction 3) never commits a position change and
            // never raises the flag.
            int x = 0, y = 0;
            Check(FindInteriorWalkable(levels[0], &x, &y), "need an interior walkable hub tile");
            PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
            p.currentLevel = 1;
            p.tileX = x;
            p.tileY = y;
            p.facing = 1;
            p.coreStats[6] = 50;
            PlayerMovement::Move(p, 3, false, levels, world, items);
            Check(!p.clearDeathGreetingPending, "a turn should not raise clearDeathGreetingPending");
        }

        if (g_ok) {
            std::printf("death_respawn_smoke: all checks passed\n");
            return 0;
        } else {
            std::printf("death_respawn_smoke: FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("death_respawn_smoke: exception: %s\n", e.what());
        return 2;
    }
}
