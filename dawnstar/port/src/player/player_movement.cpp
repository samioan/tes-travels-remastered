#include "player/player_movement.h"

#include "player/player_inventory.h"
#include "world/dungeon_view.h"

namespace dawnstar {

namespace {

// Player.java's fatigueCostMultiplier(): 3x while "Frost Limbs"
// (ailment bit 0) is active, else 1x.
int FatigueCostMultiplier(const PlayerState& p) { return (p.ailmentMask & 1) == 1 ? 3 : 1; }

// Player.tileAt(dx,dy)'s exact re-centering formula over corridorView --
// a third independent copy of the same formula render/
// corridor_render_plan.cpp and player/visible_objects.cpp each already
// carry their own local copy of (see visible_objects.cpp's
// CorridorTileAt doc comment) -- no shared Shop/tile-query module exists
// yet to consolidate them into.
uint8_t TileAt(const PlayerState& p, int dx, int dy) {
    return dy < 4 ? p.corridorView[static_cast<size_t>(dx + dy + 1)][static_cast<size_t>(dy)]
                  : p.corridorView[static_cast<size_t>(dx + dy)][static_cast<size_t>(dy)];
}

// Shop.SHOP_X/Y[0..4] -- the hub town's 5 fixed peddler positions. Same
// values player/visible_objects.cpp's own kHubShopX/kHubShopY carry
// (see that file's doc comment for why no shared Shop class exists yet).
constexpr int kHubShopX[5] = {12, 6, 7, 12, 12};
constexpr int kHubShopY[5] = {12, 11, 7, 8, 6};

// Shop.hubShopAt(x, y).
int HubShopAt(int x, int y) {
    for (int i = 0; i < 5; i++) {
        if (x == kHubShopX[i] && y == kHubShopY[i]) return i;
    }
    return -1;
}

}  // namespace

void PlayerMovement::CleanupRoamingMonsterIfPresent(PlayerState& p, std::vector<GeneratedLevel>& levels,
                                                     WorldRegistry& world) {
    if (!p.roamingSpecialMonsterPresent) return;

    GeneratedLevel& level = levels[static_cast<size_t>(p.currentLevel - 1)];
    for (const auto& [key, record] : world.monsters[static_cast<size_t>(p.currentLevel - 1)]) {
        MonsterState m = MonsterRuntime::FromBytes(record);
        if (m.monsterType == 41) {
            p.roamingSpecialMonsterPresent = false;
            DungeonRuntime::RemoveMonster(level, world, m.x, m.y);
            break;
        }
    }
    // "Remove roaming gehen failed" console message: not ported, see
    // header doc comment.
}

bool PlayerMovement::IsWalkable(uint8_t tileBits) {
    if ((tileBits & 1) != 0) return false;
    if ((tileBits & 32) != 0) return false;
    return (tileBits & 2) == 0;
}

PlayerMovement::PendingMove PlayerMovement::ComputeMoveTarget(PlayerState& p, int direction,
                                                                std::vector<GeneratedLevel>& levels,
                                                                WorldRegistry& world) {
    PendingMove pm;

    if (direction == 1 || direction == 2) {
        int delta = direction == 1 ? 1 : -1;
        pm.facing = p.facing;
        pm.tileX = p.tileX;
        pm.tileY = p.tileY;

        if (p.facing == 1) {
            pm.tileY = p.tileY - delta;
        } else if (p.facing == 3) {
            pm.tileY = p.tileY + delta;
        } else if (p.facing == 2) {
            pm.tileX = p.tileX + delta;
        } else if (p.facing == 4) {
            pm.tileX = p.tileX - delta;
        }

        const GeneratedLevel& level = levels[static_cast<size_t>(p.currentLevel - 1)];
        if (pm.tileX < 0) {
            pm.levelChanged = true;
            pm.level = level.neighborWest;
            if (pm.level > 0) {
                const GeneratedLevel& target = levels[static_cast<size_t>(pm.level - 1)];
                pm.tileX = target.width - 1;
                if (!(pm.level != 1 && p.currentLevel != 1)) {
                    pm.tileY = pm.tileY + (target.height - level.height) / 2;
                }
            }
        } else if (pm.tileX >= level.width) {
            pm.levelChanged = true;
            pm.level = level.neighborEast;
            if (pm.level > 0) {
                const GeneratedLevel& target = levels[static_cast<size_t>(pm.level - 1)];
                pm.tileX = 0;
                if (!(pm.level != 1 && p.currentLevel != 1)) {
                    pm.tileY = pm.tileY + (target.height - level.height) / 2;
                }
            }
        } else if (pm.tileY < 0) {
            pm.levelChanged = true;
            pm.level = level.neighborNorth;
            if (pm.level > 0) {
                const GeneratedLevel& target = levels[static_cast<size_t>(pm.level - 1)];
                if (!(pm.level != 1 && p.currentLevel != 1)) {
                    pm.tileX = pm.tileX + (target.width - level.width) / 2;
                }
                pm.tileY = target.height - 1;
            }
        } else if (pm.tileY >= level.height) {
            pm.levelChanged = true;
            pm.level = level.neighborSouth;
            if (pm.level > 0) {
                const GeneratedLevel& target = levels[static_cast<size_t>(pm.level - 1)];
                if (!(pm.level != 1 && p.currentLevel != 1)) {
                    pm.tileX = pm.tileX + (target.width - level.width) / 2;
                }
                pm.tileY = 0;
            }
        } else {
            pm.levelChanged = false;
            pm.level = p.currentLevel;
        }

        if (pm.levelChanged) {
            CleanupRoamingMonsterIfPresent(p, levels, world);
        }
    } else if (direction == 3) {
        pm.level = p.currentLevel;
        pm.levelChanged = false;
        pm.facing = p.facing + 1;
        if (pm.facing > 4) pm.facing = 1;
        pm.tileX = p.tileX;
        pm.tileY = p.tileY;
    } else if (direction == 4) {
        pm.level = p.currentLevel;
        pm.levelChanged = false;
        pm.facing = p.facing - 1;
        if (pm.facing < 1) pm.facing = 4;
        pm.tileX = p.tileX;
        pm.tileY = p.tileY;
    }

    return pm;
}

bool PlayerMovement::CommitMove(PlayerState& p, int direction, std::vector<GeneratedLevel>& levels,
                                 WorldRegistry& world, const ItemDatabase& items, bool& outLevelChanged) {
    if (p.coreStats[6] <= 0) return false;
    if (direction == 0) return false;

    PendingMove pm = ComputeMoveTarget(p, direction, levels, world);
    outLevelChanged = pm.levelChanged;
    if (pm.level <= 0) return false;

    GeneratedLevel& target = levels[static_cast<size_t>(pm.level - 1)];
    // (every level in `levels` is always "populated" -- see class comment)
    uint8_t tile = target.tiles[static_cast<size_t>(pm.tileX)][static_cast<size_t>(pm.tileY)];
    if (!IsWalkable(tile)) return false;

    p.currentLevel = pm.level;
    p.prevTileX = static_cast<int8_t>(p.tileX);
    p.prevTileY = static_cast<int8_t>(p.tileY);
    p.tileX = pm.tileX;
    p.tileY = pm.tileY;
    p.facing = pm.facing;
    target.visited = true;

    if (direction == 1 || direction == 2) {
        // Shop.showDeathGreeting reset: SKIPPED, Shop not ported yet.
        int cost = 1 * FatigueCostMultiplier(p);
        int16_t newFatigue = static_cast<int16_t>(p.coreStats[6] - cost);
        p.coreStats[6] = newFatigue < 0 ? int16_t{0} : newFatigue;
    }

    bool hasDroppedItems = (tile & 4) != 0;
    if (hasDroppedItems && (direction == 1 || direction == 2)) {
        bool allLooted = true;
        auto droppedItems =
            DungeonRuntime::DroppedItemsAt(world, static_cast<int>(pm.level - 1), p.tileX, p.tileY);
        for (const auto& record : droppedItems) {
            int itemId = record[2];
            int packed = (record[3] << 8) + record[4];
            int charge = record[5];
            bool looted = PlayerInventory::AddItem(p, itemId, packed, charge);
            if (looted) {
                DungeonRuntime::RemoveDroppedItem(target, world, record);
                if ((record[6] & 2) == 0) {
                    int itemIdx = itemId - 1;
                    if (items.category[static_cast<size_t>(itemIdx)] == 11) {
                        p.giftPointsFound = static_cast<int16_t>(p.giftPointsFound + items.subtype[static_cast<size_t>(itemIdx)]);
                    }
                }
            } else {
                allLooted = false;
            }
        }

        if (allLooted) {
            DungeonRuntime::ClearDroppedItemFlag(target, p.tileX, p.tileY);
        }
    }

    if ((tile & 8) == 0 || (direction != 1 && direction != 2)) {
        RefreshCorridorView(p, levels);
    } else {
        MarkCampAndReturnToTown(p, false, levels, world);
    }

    return true;
}

void PlayerMovement::ResetToHubPosition(PlayerState& p, bool altSpawn, std::vector<GeneratedLevel>& levels,
                                         WorldRegistry& world) {
    CleanupRoamingMonsterIfPresent(p, levels, world);

    if (!altSpawn) {
        p.currentLevel = 1;
        p.tileX = 9;
        p.tileY = 9;
        p.facing = 1;
    } else {
        p.currentLevel = 1;
        p.tileX = 13;
        p.tileY = 6;
        p.facing = 4;
    }

    RefreshCorridorView(p, levels);
    // chest/NPC-visibility refresh: SKIPPED, see class comment.
}

void PlayerMovement::MarkCampAndReturnToTown(PlayerState& p, bool skipMark, std::vector<GeneratedLevel>& levels,
                                              WorldRegistry& world) {
    if (!skipMark) {
        p.campLevel = static_cast<int8_t>(p.currentLevel);
        p.campX = static_cast<int8_t>(p.tileX);
        p.campY = static_cast<int8_t>(p.tileY);
        p.campFacing = static_cast<int8_t>(p.facing);
    }

    ResetToHubPosition(p, true, levels, world);
    p.suppressStrafeAdjust = true;
}

void PlayerMovement::WarpToCampMark(PlayerState& p, const std::vector<GeneratedLevel>& levels) {
    p.currentLevel = p.campLevel;
    p.tileX = p.campX;
    p.tileY = p.campY;
    p.facing = p.campFacing;

    RefreshCorridorView(p, levels);
    p.suppressStrafeAdjust = true;
    // chest/NPC-visibility refresh: SKIPPED, see class comment.
}

void PlayerMovement::RefreshCorridorView(PlayerState& p, const std::vector<GeneratedLevel>& levels) {
    DungeonView view(levels, p.currentLevel - 1);
    uint8_t out[9][5];
    view.SampleCorridorView(p.tileX, p.tileY, p.facing, out);
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 5; j++) {
            p.corridorView[static_cast<size_t>(i)][static_cast<size_t>(j)] = out[i][j];
        }
    }
}

bool PlayerMovement::Move(PlayerState& p, int direction, bool strafe, std::vector<GeneratedLevel>& levels,
                           WorldRegistry& world, const ItemDatabase& items) {
    if (p.coreStats[6] <= 0) return false;

    bool moved = false;
    bool levelChanged = false;
    bool savedLevelChanged = false;

    if (strafe && direction == 4) {
        CommitMove(p, 4, levels, world, items, levelChanged);
        moved = CommitMove(p, 1, levels, world, items, levelChanged);
        if (!p.suppressStrafeAdjust) {
            savedLevelChanged = levelChanged;
            moved = CommitMove(p, 3, levels, world, items, levelChanged);
            levelChanged = savedLevelChanged;
        }
    } else if (strafe && direction == 3) {
        CommitMove(p, 3, levels, world, items, levelChanged);
        moved = CommitMove(p, 1, levels, world, items, levelChanged);
        if (!p.suppressStrafeAdjust) {
            savedLevelChanged = levelChanged;
            moved = CommitMove(p, 4, levels, world, items, levelChanged);
            levelChanged = savedLevelChanged;
        }
    } else {
        moved = CommitMove(p, direction, levels, world, items, levelChanged);
    }

    p.suppressStrafeAdjust = false;
    return moved;
}

int PlayerMovement::NpcInFront(PlayerState& p, std::vector<GeneratedLevel>& levels, WorldRegistry& world) {
    PendingMove pm = ComputeMoveTarget(p, 1, levels, world);
    if (pm.level <= 0) return -1;

    if (pm.level == 3 || pm.level == 12 || pm.level == 21 || pm.level == 30) {
        int shopIndex = pm.level == 3 ? 5 : pm.level == 12 ? 6 : pm.level == 21 ? 7 : 8;
        const GeneratedLevel& target = levels[static_cast<size_t>(pm.level - 1)];
        if (pm.tileX == target.specialShopX && pm.tileY == target.specialShopY) return shopIndex;
        return -1;
    }

    return pm.level != 1 ? -1 : HubShopAt(pm.tileX, pm.tileY);
}

void PlayerMovement::RefreshNpcInSight(PlayerState& p, std::vector<GeneratedLevel>& levels, WorldRegistry& world) {
    uint8_t tile = TileAt(p, 0, 1);
    if ((tile & 32) != 0) {
        p.npcInSight = NpcInFront(p, levels, world);
    } else {
        p.npcInSight = -1;
    }
}

const std::array<uint8_t, 8>* PlayerMovement::ChestInFront(PlayerState& p, std::vector<GeneratedLevel>& levels,
                                                            WorldRegistry& world) {
    PendingMove pm = ComputeMoveTarget(p, 1, levels, world);
    if (pm.level <= 0) return nullptr;

    auto& chestMap = world.chests[static_cast<size_t>(pm.level - 1)];
    auto it = chestMap.find(PackPosKey(pm.tileX, pm.tileY));
    return it != chestMap.end() ? &it->second : nullptr;
}

}  // namespace dawnstar
