// M34 smoke test: InteractTick::ProcessInteract (interact/
// interact_tick.h) -- GameCanvas.processInteract()'s chest-looting half.
// (The npcInSight half is now real too, M45 -- see
// m45_shop_interaction_smoke.cpp for its own deep coverage; every
// scenario here keeps npcInSight at -1, so this file's own additions are
// just the extra parameters ProcessInteract's signature grew, unused in
// every one of its own checks.) No JVM ground truth is available
// (same reason as every prior milestone) -- verified against the real
// 37-level generated world (M6/M24) and a real created character (M11),
// including a real chest the world generator itself registered with the
// "extended itemId" (low byte 86) encoding, to exercise a real preserved
// bug: pickUpDroppedItem() never resolves that encoding back into the
// real item id, so it silently picks up item 86 literally instead.
#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/shop_dialogue.h"
#include "dungeon/dungeon_runtime.h"
#include "interact/interact_tick.h"
#include "npc/shop_interaction.h"
#include "player/player_creation.h"
#include "player/player_movement.h"
#include "player/player_state.h"
#include "render/message_popup.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::GeneratedLevel;
using dawnstar::InteractTick;
using dawnstar::MessagePopupState;
using dawnstar::PlayerCreation;
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

// Same helper as m25/m28/m32/m33's own FindApproach (duplicated per
// this project's established per-test-file self-containment
// convention).
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
        // M45: ProcessInteract's grown signature -- unused by every
        // scenario in this file (npcInSight stays -1 throughout), see
        // this file's own top doc comment.
        dawnstar::ShopDialogue shopDialogue = dawnstar::ShopDialogue::Load(root + "/npcstrings.dat");
        ShopState shop = ShopState::Reset();
        int16_t nextItemSpawnId = 1;

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

        int levelIdx = -1;
        for (size_t i = 1; i < levels.size(); i++) {
            if (!levels[i].chests.empty()) {
                levelIdx = static_cast<int>(i);
                break;
            }
        }
        Check(levelIdx >= 0, "at least one non-hub level should have a real pre-placed chest spawn");
        if (levelIdx < 0) return 1;

        GeneratedLevel& level = levels[static_cast<size_t>(levelIdx)];
        int levelNumber = level.number;
        const auto& spawn = level.chests[0];
        int standX = 0, standY = 0, facing = 0;
        Check(FindApproach(level, spawn.x, spawn.y, &standX, &standY, &facing),
              "should find a walkable approach tile facing the real chest spawn");

        dawnstar::JavaRandom globalRng(321);
        PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
        player.currentLevel = levelNumber;
        player.tileX = standX;
        player.tileY = standY;
        player.facing = facing;

        // --- A: chestInSight == false -> a hard no-op ---
        {
            player.chestInSight = false;
            player.npcInSight = -1;
            MessagePopupState popup;
            int inventoryBefore = player.inventoryCount;
            InteractTick::ProcessInteract(player, levels, world, items, charData, shopDialogue, shop, popup,
                                          globalRng, nextItemSpawnId, 1000);
            Check(!popup.visible, "no chest in sight should show no message");
            Check(player.inventoryCount == inventoryBefore, "no chest in sight should not change the inventory");
            Check(PlayerMovement::ChestInFront(player, levels, world) != nullptr,
                  "setup check: the real chest should still be registered");
        }

        // --- B: npcInSight >= 0 -> still a no-op (npcInSight outranks
        // chestInSight, and openNpcDialogue() isn't ported -- see
        // interact_tick.h's own doc comment) ---
        {
            player.chestInSight = true;
            player.npcInSight = 3;
            MessagePopupState popup;
            int inventoryBefore = player.inventoryCount;
            InteractTick::ProcessInteract(player, levels, world, items, charData, shopDialogue, shop, popup,
                                          globalRng, nextItemSpawnId, 2000);
            Check(!popup.visible, "npcInSight >= 0 should show no message (openNpcDialogue is unported)");
            Check(player.inventoryCount == inventoryBefore, "npcInSight >= 0 should not touch the inventory");
            Check(PlayerMovement::ChestInFront(player, levels, world) != nullptr,
                  "npcInSight >= 0 should leave the real chest untouched in the registry");
            player.npcInSight = -1;
        }

        // --- C: a real chest, successfully looted ---
        {
            player.chestInSight = true;
            auto* chestBefore = PlayerMovement::ChestInFront(player, levels, world);
            Check(chestBefore != nullptr, "setup: the real chest should still be directly ahead");
            uint8_t expectedItemId = (*chestBefore)[4];
            int expectedItemIdx = expectedItemId - 1;
            Check(expectedItemIdx >= 0 && expectedItemIdx < items.ItemCount(),
                  "the real chest's own itemId should be a valid real item index");

            int inventoryBefore = player.inventoryCount;
            MessagePopupState popup;
            InteractTick::ProcessInteract(player, levels, world, items, charData, shopDialogue, shop, popup,
                                          globalRng, nextItemSpawnId, 3000);

            Check(player.inventoryCount == inventoryBefore + 1, "a successful loot should add exactly one inventory item");
            Check(static_cast<uint8_t>(std::abs(static_cast<int>(player.inventoryItemIds[inventoryBefore]))) ==
                      expectedItemId,
                  "the newly added inventory slot should hold the chest's own real itemId");
            Check(!player.chestInSight, "a successful loot should clear chestInSight");
            Check(player.minimapDirty, "a successful loot should mark the minimap dirty");
            Check(PlayerMovement::ChestInFront(player, levels, world) == nullptr,
                  "a successfully looted chest should be removed from the registry");
            Check((level.tiles[static_cast<size_t>(spawn.x)][static_cast<size_t>(spawn.y)] & 16) == 0,
                  "a successfully looted chest's tile should have its chest-presence bit (16) cleared");

            auto wrapped = dawnstar::MessagePopup::WrapToTwoLines(items.name[static_cast<size_t>(expectedItemIdx)]);
            Check(popup.visible && popup.lines[0] == wrapped[0] && popup.lines[1] == wrapped[1] &&
                      popup.priority == 10,
                  "a successful loot should show the real item's own name (internal priority 10, from a -1 arg)");
        }

        // --- D: a full inventory -- the item is dropped to the floor
        // instead, and the chest is STILL removed from the registry
        // (matching the original's own floor-drop branch exactly) ---
        {
            // Face a second real chest (or reuse this level's own first
            // chest position again by placing a synthetic one there --
            // simplest is a fresh synthetic chest at the same spot,
            // since section C's real one is already gone).
            std::array<uint8_t, 8> chest{};
            chest[0] = static_cast<uint8_t>(spawn.x);
            chest[1] = static_cast<uint8_t>(spawn.y);
            chest[4] = 5;  // an arbitrary real, ordinary (non-86) item id
            chest[5] = 0;
            chest[6] = 42;
            chest[7] = 0;
            world.chests[static_cast<size_t>(levelIdx)][dawnstar::PackPosKey(spawn.x, spawn.y)] = chest;
            level.tiles[static_cast<size_t>(spawn.x)][static_cast<size_t>(spawn.y)] |= 16;

            while (player.inventoryCount < 24) {
                player.inventoryItemIds[player.inventoryCount] = 1;
                player.inventoryItemData[player.inventoryCount] = 0;
                player.inventoryCount++;
            }

            player.chestInSight = true;
            size_t dropsBefore = world.droppedItems[static_cast<size_t>(levelIdx)].size();
            MessagePopupState popup;
            InteractTick::ProcessInteract(player, levels, world, items, charData, shopDialogue, shop, popup,
                                          globalRng, nextItemSpawnId, 4000);

            Check(player.inventoryCount == 24, "a full-inventory loot should not add an inventory slot");
            Check(popup.visible && popup.lines[0] == "Inventory" && popup.lines[1] == "full!" && popup.priority == 10,
                  "a full-inventory loot should show \"Inventory/full!\" (internal priority 10, from a -1 arg)");
            Check(PlayerMovement::ChestInFront(player, levels, world) == nullptr,
                  "a full-inventory loot should STILL remove the chest from the registry (matches the original)");

            size_t dropsAfter = world.droppedItems[static_cast<size_t>(levelIdx)].size();
            Check(dropsAfter == dropsBefore + 1, "a full-inventory loot should register exactly one new floor item");
            if (dropsAfter == dropsBefore + 1) {
                const auto& floorRec = world.droppedItems[static_cast<size_t>(levelIdx)].back();
                Check(floorRec[0] == static_cast<uint8_t>(spawn.x) && floorRec[1] == static_cast<uint8_t>(spawn.y) &&
                          floorRec[2] == 5,
                      "the floor-dropped record should carry the chest's own position and itemId");
            }
        }

        // --- E: the real "extended itemId" (low byte 86) preserved bug
        // -- pickUpDroppedItem never resolves the high byte back, so it
        // silently adds item 86 literally ---
        {
            // Collect matching (level, x, y, highByte) tuples FIRST --
            // ProcessInteract mutates world.chests[li] (RemoveChest), so
            // this can't erase-while-iterating the live map itself.
            struct Found {
                size_t li;
                int x, y, high;
            };
            std::vector<Found> candidates;
            for (size_t li = 0; li < levels.size(); li++) {
                for (const auto& [key, record] : world.chests[li]) {
                    if (record[4] != 86) continue;
                    int x, y;
                    dawnstar::UnpackPosKey(key, &x, &y);
                    candidates.push_back({li, x, y, record[7]});
                }
            }
            std::printf("  %zu real extended-itemId chest(s) found in the generated world\n", candidates.size());

            bool tested = false;
            for (const Found& c : candidates) {
                GeneratedLevel& chestLevel = levels[c.li];
                int sx = 0, sy = 0, f = 0;
                if (!FindApproach(chestLevel, c.x, c.y, &sx, &sy, &f)) continue;  // no walkable approach; try the next one

                PlayerState p2 = PlayerCreation::CreateCharacter(0, "Tester2", charData, items, globalRng);
                p2.currentLevel = chestLevel.number;
                p2.tileX = sx;
                p2.tileY = sy;
                p2.facing = f;
                p2.chestInSight = true;

                int inventoryBefore = p2.inventoryCount;
                MessagePopupState popup;
                InteractTick::ProcessInteract(p2, levels, world, items, charData, shopDialogue, shop, popup,
                                              globalRng, nextItemSpawnId, 5000);

                Check(p2.inventoryCount == inventoryBefore + 1 &&
                          std::abs(static_cast<int>(p2.inventoryItemIds[inventoryBefore])) == 86,
                      "an extended-itemId chest should still add item 86 literally (the preserved bug)");
                std::printf("  extended-itemId chest at level %d (%d,%d): high byte=%d, added itemId=86 (bug "
                            "preserved)\n",
                            chestLevel.number, c.x, c.y, c.high);
                tested = true;
                break;
            }
            if (!tested) {
                std::printf("  (no real extended-itemId chest had a walkable approach this run -- skipping)\n");
            }
        }

        if (g_ok) {
            std::printf("all interact-tick checks passed\n");
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
