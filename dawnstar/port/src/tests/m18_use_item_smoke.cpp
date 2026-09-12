// M18 smoke test: PlayerMovement's new camp-system methods
// (HasCampMark/ResetToHubPosition/MarkCampAndReturnToTown/WarpToCampMark),
// PlayerInventory::CanUseItem, and CombatResolution::UseItem -- the
// "gift"/special-consumable item switch (ids 87-99), against real
// ItemDatabase/MonsterDatabase data, a real generated world, a real
// character (M11's PlayerCreation), and a real spawned monster (M15's
// MonsterRuntime::Spawn). No JVM ground truth available (same reason as
// M6/M9/M11/M13/M14/M15/M16/M17).
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "combat/combat_resolution.h"
#include "monster/monster_runtime.h"
#include "player/player_creation.h"
#include "player/player_inventory.h"
#include "player/player_movement.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::CharacterData;
using dawnstar::CombatResolution;
using dawnstar::GeneratedLevel;
using dawnstar::ItemDatabase;
using dawnstar::MonsterDatabase;
using dawnstar::MonsterRuntime;
using dawnstar::MonsterState;
using dawnstar::PlayerCreation;
using dawnstar::PlayerInventory;
using dawnstar::PlayerMovement;
using dawnstar::PlayerState;

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
        CharacterData charData = CharacterData::Load(archive);
        ItemDatabase items = ItemDatabase::Load(archive);
        MonsterDatabase monsters = MonsterDatabase::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);

        std::vector<GeneratedLevel> levels;
        for (size_t i = 0; i < geometry.rows.size(); i++) {
            int levelNumber = static_cast<int>(i) + 1;
            levels.push_back(levelNumber == 1
                                  ? dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[i])
                                  : dawnstar::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[i], items,
                                                                              monsters));
        }
        std::printf("generated %zu levels\n", levels.size());

        // Confirm the 87-99 gift-item ids Player.java's useItem() switch
        // hardcodes really are category-13 in the real item table.
        for (int itemId = 87; itemId <= 99; itemId++) {
            Check(items.category[static_cast<size_t>(itemId - 1)] == 13,
                  "gift item ids 87-99 should all be real category-13 items");
        }

        // --- HasCampMark / ResetToHubPosition / MarkCampAndReturnToTown /
        // WarpToCampMark ---
        {
            dawnstar::JavaRandom rng(2024);
            PlayerState p = PlayerCreation::CreateCharacter(0, "Camper", charData, items, rng);
            Check(!PlayerMovement::HasCampMark(p), "a fresh character should have no camp mark yet");

            p.currentLevel = 5;
            p.tileX = 7;
            p.tileY = 11;
            p.facing = 3;
            PlayerMovement::MarkCampAndReturnToTown(p, false, levels);

            Check(PlayerMovement::HasCampMark(p), "markCampAndReturnToTown should set a camp mark");
            Check(p.campLevel == 5 && p.campX == 7 && p.campY == 11 && p.campFacing == 3,
                  "markCampAndReturnToTown should bookmark the pre-call position exactly");
            Check(p.currentLevel == 1 && p.tileX == 13 && p.tileY == 6 && p.facing == 4,
                  "markCampAndReturnToTown should place the character at the alt-spawn hub position");
            Check(p.suppressStrafeAdjust, "markCampAndReturnToTown should set suppressStrafeAdjust");

            p.suppressStrafeAdjust = false;
            p.currentLevel = 9;
            p.tileX = 1;
            p.tileY = 1;
            p.facing = 1;
            PlayerMovement::WarpToCampMark(p, levels);
            Check(p.currentLevel == 5 && p.tileX == 7 && p.tileY == 11 && p.facing == 3,
                  "warpToCampMark should restore the bookmarked position exactly");
            Check(p.suppressStrafeAdjust, "warpToCampMark should set suppressStrafeAdjust");

            p.currentLevel = 3;
            p.tileX = 2;
            p.tileY = 2;
            PlayerMovement::ResetToHubPosition(p, false, levels);
            Check(p.currentLevel == 1 && p.tileX == 9 && p.tileY == 9 && p.facing == 1,
                  "resetToHubPosition(false) should use the normal (9,9,facing1) spawn");
            PlayerMovement::ResetToHubPosition(p, true, levels);
            Check(p.currentLevel == 1 && p.tileX == 13 && p.tileY == 6 && p.facing == 4,
                  "resetToHubPosition(true) should use the alt (13,6,facing4) spawn");
        }

        // --- canUseItem ---
        {
            dawnstar::JavaRandom rng(4096);
            PlayerState p = PlayerCreation::CreateCharacter(1, "Gifted", charData, items, rng);
            PlayerInventory::AddItem(p, 89, 0, 0);  // a real gift item.
            int giftSlot = p.inventoryCount - 1;
            PlayerInventory::AddItem(p, 1, 0, 0);  // a real non-gift item (category != 13).
            int nonGiftSlot = p.inventoryCount - 1;
            Check(PlayerInventory::CanUseItem(p, items, giftSlot), "canUseItem should accept a category-13 item");
            Check(!PlayerInventory::CanUseItem(p, items, nonGiftSlot),
                  "canUseItem should reject a non-category-13 item");
        }

        // --- useItem: 88-96, one at a time on a scratch character ---
        {
            dawnstar::JavaRandom creationRng(1111);
            PlayerState base = PlayerCreation::CreateCharacter(3, "Consumer", charData, items, creationRng);
            int startingCount = base.inventoryCount;  // the class's starting kit, still present throughout.
            dawnstar::JavaRandom useRng(2222);

            // Adds `itemId` to the LAST free slot (grantStartingItems
            // already occupies the earlier ones) and uses it from there.
            auto useNewItem = [&](PlayerState& p, int itemId) {
                PlayerInventory::AddItem(p, itemId, 0, 0);
                int slot = p.inventoryCount - 1;
                CombatResolution::UseItem(p, slot, nullptr, items, monsters, levels, useRng);
            };

            {  // 88: cureRandomAilment, RNG-free single-ailment case.
                PlayerState p = base;
                p.ailmentMask = static_cast<int8_t>(1 << 2);
                useNewItem(p, 88);
                Check(p.ailmentMask == 0, "item 88 should cure the sole active ailment");
                Check(p.inventoryCount == startingCount, "item 88 should be consumed");
            }
            {  // 89: full HP heal.
                PlayerState p = base;
                p.coreStats[2] = static_cast<int16_t>(p.coreStats[3] - 5);
                useNewItem(p, 89);
                Check(p.coreStats[2] == p.coreStats[3], "item 89 should fully heal HP");
            }
            {  // 90: full Magicka heal.
                PlayerState p = base;
                p.coreStats[4] = static_cast<int16_t>(p.coreStats[5] - 5);
                useNewItem(p, 90);
                Check(p.coreStats[4] == p.coreStats[5], "item 90 should fully heal Magicka");
            }
            {  // 91: Fatigue += 3*maxMagicka, unclamped -- ported exactly,
               // no Math.min against maxFatigue in the original either.
                PlayerState p = base;
                int16_t before = p.coreStats[6];
                useNewItem(p, 91);
                Check(p.coreStats[6] == static_cast<int16_t>(before + 3 * p.coreStats[5]),
                      "item 91 should add 3xmaxMagicka Fatigue, uncapped");
            }
            {  // 92: +1 levelExp, no level-up check (unlike GainSkillExp).
                PlayerState p = base;
                p.coreStats[1] = 9;
                useNewItem(p, 92);
                Check(p.coreStats[1] == 10, "item 92 should add exactly 1 levelExp point");
                Check(!p.levelUpPending, "item 92 should NOT trigger levelUpPending itself");
            }
            {  // 93: full HP+Magicka heal.
                PlayerState p = base;
                p.coreStats[2] = 1;
                p.coreStats[4] = 1;
                useNewItem(p, 93);
                Check(p.coreStats[2] == p.coreStats[3] && p.coreStats[4] == p.coreStats[5],
                      "item 93 should fully heal both HP and Magicka");
            }
            {  // 94/95: harm/armor buffs.
                PlayerState p = base;
                useNewItem(p, 94);
                Check(p.increaseHarmBuff, "item 94 should set increaseHarmBuff");
            }
            {
                PlayerState p = base;
                useNewItem(p, 95);
                Check(p.increaseArmorBuff, "item 95 should set increaseArmorBuff");
            }
            {  // 96: Safe Camping -- the one gift item NOT consumed.
                PlayerState p = base;
                useNewItem(p, 96);
                Check(p.safeCampingBuff, "item 96 should set safeCampingBuff");
                Check(p.inventoryCount == startingCount + 1,
                      "item 96 should NOT be consumed, unlike every other gift item");
            }
        }

        // --- useItem: 87 (warp-to-camp-or-mark), both branches ---
        {
            dawnstar::JavaRandom creationRng(3333);
            PlayerState p = PlayerCreation::CreateCharacter(2, "Warper", charData, items, creationRng);
            dawnstar::JavaRandom useRng(4444);

            // Not in town (or no camp mark yet): marks the current spot
            // and returns to the alt-spawn hub position.
            p.currentLevel = 6;
            p.tileX = 4;
            p.tileY = 8;
            p.facing = 2;
            PlayerInventory::AddItem(p, 87, 0, 0);
            CombatResolution::UseItem(p, p.inventoryCount - 1, nullptr, items, monsters, levels, useRng);
            Check(p.campLevel == 6 && p.campX == 4 && p.campY == 8 && p.campFacing == 2,
                  "item 87 away from town should bookmark the pre-use position");
            Check(p.currentLevel == 1 && p.tileX == 13 && p.tileY == 6,
                  "item 87 away from town should return to the alt-spawn hub position");

            // Now in town (level 1) with a camp mark set: warps to it
            // instead of re-marking.
            p.tileX = 1;
            p.tileY = 1;
            p.facing = 1;
            PlayerInventory::AddItem(p, 87, 0, 0);
            CombatResolution::UseItem(p, p.inventoryCount - 1, nullptr, items, monsters, levels, useRng);
            Check(p.currentLevel == 6 && p.tileX == 4 && p.tileY == 8 && p.facing == 2,
                  "item 87 in town with a camp mark should warp to the bookmarked position instead");
        }

        // --- useItem: 97/98/99 instant-kill scrolls, against a real
        // monster, both the kill and no-effect thresholds, plus a
        // nullptr target ---
        {
            dawnstar::JavaRandom creationRng(5555);
            PlayerState p = PlayerCreation::CreateCharacter(4, "Assassin", charData, items, creationRng);
            dawnstar::JavaRandom useRng(6666);

            // Find one real monster type whose (def,evasion) both clear
            // item 99's threshold (<=29) and one whose def or evasion
            // exceeds it (so item 99 should have no effect), by directly
            // querying the same MonsterRuntime::Stat the item switch
            // itself uses -- ground truth from the real data table, not
            // an assumption about which types happen to qualify.
            int killableType = -1;
            int immuneType = -1;
            for (int type = 1; type <= 60 && (killableType < 0 || immuneType < 0); type++) {
                MonsterState probe = MonsterRuntime::Spawn(1, type, 1, monsters);
                int def = MonsterRuntime::Stat(probe, monsters, 4);
                int evasion = MonsterRuntime::Stat(probe, monsters, 10);
                if (killableType < 0 && def <= 29 && evasion <= 29) killableType = type;
                if (immuneType < 0 && (def > 29 || evasion > 29)) immuneType = type;
            }
            Check(killableType > 0, "real monster data should contain at least one type killable by item 99");
            Check(immuneType > 0, "real monster data should contain at least one type immune to item 99");

            if (killableType > 0) {
                MonsterState target = MonsterRuntime::Spawn(10, killableType, 1, monsters);
                PlayerInventory::AddItem(p, 99, 0, 0);
                CombatResolution::UseItem(p, p.inventoryCount - 1, &target, items, monsters, levels, useRng);
                Check(static_cast<uint8_t>(target.hp) == 0, "item 99 should zero hp for a type within its threshold");
            }
            if (immuneType > 0) {
                MonsterState target = MonsterRuntime::Spawn(11, immuneType, 1, monsters);
                int startingHp = static_cast<uint8_t>(target.hp);
                PlayerInventory::AddItem(p, 99, 0, 0);
                CombatResolution::UseItem(p, p.inventoryCount - 1, &target, items, monsters, levels, useRng);
                Check(static_cast<uint8_t>(target.hp) == static_cast<uint8_t>(startingHp),
                      "item 99 should leave hp untouched for a type above its threshold");
            }

            // A null target should be handled safely: no crash, item
            // still consumed (Java's null check is only inside the
            // per-branch body, not around the whole case).
            int beforeCount = p.inventoryCount;
            PlayerInventory::AddItem(p, 97, 0, 0);
            CombatResolution::UseItem(p, p.inventoryCount - 1, nullptr, items, monsters, levels, useRng);
            Check(p.inventoryCount == beforeCount, "item 97 with a null target should still consume the item");
        }

        if (!g_ok) {
            std::fprintf(stderr, "m18_use_item_smoke: FAILED\n");
            return 1;
        }
        std::printf("all use-item/camp checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m18_use_item_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
