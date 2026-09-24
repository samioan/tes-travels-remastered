#include "player/player_movement.h"

#include <algorithm>
#include <stdexcept>

#include "monster/monster_runtime.h"
#include "player/player_inventory.h"
#include "world/game_advancement.h"

namespace stormhold {

void PlayerMovement::ComputeMoveTarget(PlayerState& p, int dir, const LevelLookup& levels) {
    if (dir == 1 || dir == 2) {
        int step = (dir == 1) ? 1 : -1;
        p.pendingFacing = p.facing;

        if (p.facing == 1) {
            p.pendingTileX = p.tileX;
            p.pendingTileY = static_cast<int8_t>(p.tileY - step);
        } else if (p.facing == 3) {
            p.pendingTileX = p.tileX;
            p.pendingTileY = static_cast<int8_t>(p.tileY + step);
        } else if (p.facing == 2) {
            p.pendingTileX = static_cast<int8_t>(p.tileX + step);
            p.pendingTileY = p.tileY;
        } else if (p.facing == 4) {
            p.pendingTileX = static_cast<int8_t>(p.tileX - step);
            p.pendingTileY = p.tileY;
        }

        GeneratedLevel& level = levels(p.currentLevel);

        if (p.pendingTileX < 0) {
            p.crossingLevelBoundary = true;
            p.pendingLevel = static_cast<int8_t>(level.neighborWest);
            if (p.pendingLevel <= 0) throw std::runtime_error("PlayerMovement: no west neighbor to cross into");
            GeneratedLevel& neighbor = levels(p.pendingLevel);
            p.pendingTileX = static_cast<int8_t>(neighbor.width - 1);
            if (p.pendingLevel == 1 || p.currentLevel == 1) {
                p.pendingTileY = static_cast<int8_t>(p.pendingTileY + (neighbor.height - level.height) / 2);
            }
        } else if (p.pendingTileX >= level.width) {
            p.crossingLevelBoundary = true;
            p.pendingLevel = static_cast<int8_t>(level.neighborEast);
            if (p.pendingLevel <= 0) throw std::runtime_error("PlayerMovement: no east neighbor to cross into");
            GeneratedLevel& neighbor = levels(p.pendingLevel);
            p.pendingTileX = 0;
            if (p.pendingLevel == 1 || p.currentLevel == 1) {
                p.pendingTileY = static_cast<int8_t>(p.pendingTileY + (neighbor.height - level.height) / 2);
            }
        } else if (p.pendingTileY < 0) {
            p.crossingLevelBoundary = true;
            p.pendingLevel = static_cast<int8_t>(level.neighborNorth);
            if (p.pendingLevel <= 0) throw std::runtime_error("PlayerMovement: no north neighbor to cross into");
            GeneratedLevel& neighbor = levels(p.pendingLevel);
            p.pendingTileY = static_cast<int8_t>(neighbor.height - 1);
            if (p.pendingLevel == 1 || p.currentLevel == 1) {
                p.pendingTileX = static_cast<int8_t>(p.pendingTileX + (neighbor.width - level.width) / 2);
            }
        } else if (p.pendingTileY >= level.height) {
            p.crossingLevelBoundary = true;
            p.pendingLevel = static_cast<int8_t>(level.neighborSouth);
            if (p.pendingLevel <= 0) throw std::runtime_error("PlayerMovement: no south neighbor to cross into");
            GeneratedLevel& neighbor = levels(p.pendingLevel);
            p.pendingTileY = 0;
            if (p.pendingLevel == 1 || p.currentLevel == 1) {
                p.pendingTileX = static_cast<int8_t>(p.pendingTileX + (neighbor.width - level.width) / 2);
            }
        } else {
            p.crossingLevelBoundary = false;
            p.pendingLevel = p.currentLevel;
        }
    } else if (dir == 3) {
        p.pendingLevel = p.currentLevel;
        p.crossingLevelBoundary = false;
        p.pendingFacing = static_cast<int8_t>(p.facing + 1);
        if (p.pendingFacing > 4) p.pendingFacing = 1;
        p.pendingTileX = p.tileX;
        p.pendingTileY = p.tileY;
    } else if (dir == 4) {
        p.pendingLevel = p.currentLevel;
        p.crossingLevelBoundary = false;
        p.pendingFacing = static_cast<int8_t>(p.facing - 1);
        if (p.pendingFacing < 1) p.pendingFacing = 4;
        p.pendingTileX = p.tileX;
        p.pendingTileY = p.tileY;
    }
}

bool PlayerMovement::IsWalkableTileBits(uint8_t tileBits) {
    if (tileBits & 1) return false;
    if (tileBits & 32) return false;
    return (tileBits & 2) == 0;
}

void PlayerMovement::RefreshCorridorView(PlayerState& p, const GeneratedLevel& level, const LevelLookup& levels) {
    DungeonRuntime::LevelLookup constLevels = [&levels](int levelNumber) -> const GeneratedLevel& {
        return levels(levelNumber);
    };
    p.corridorView = DungeonRuntime::SampleCorridorView(level, p.tileX, p.tileY, p.facing, constLevels);
}

std::optional<MonsterState> PlayerMovement::MonsterInFront(PlayerState& p, const LevelLookup& levels,
                                                             const WorldRegistry& world) {
    try {
        ComputeMoveTarget(p, 1, levels);
    } catch (const std::runtime_error&) {
        // The real computeMoveTarget(1) has no equivalent guard at all
        // here -- this is exactly the "no neighbor to cross into" edge
        // ComputeMoveTarget's own doc comment already flags as a real
        // (if believed-unreachable) crash risk for CommitMove's own
        // unguarded call. monsterInFront() explicitly checks
        // `pendingLevel <= 0` and returns null instead -- this port
        // models that same graceful path by catching the exception
        // ComputeMoveTarget already throws for it, rather than
        // duplicating its entire boundary-stitching body just to avoid
        // throwing in the first place.
        return std::nullopt;
    }
    if (p.pendingLevel <= 0) return std::nullopt;

    const GeneratedLevel& pendingLevel = levels(p.pendingLevel);
    return DungeonRuntime::MonsterAt(pendingLevel, world, p.pendingTileX, p.pendingTileY);
}

std::optional<std::array<int8_t, 8>> PlayerMovement::ChestAheadOfPlayer(PlayerState& p, const LevelLookup& levels,
                                                                          const WorldRegistry& world) {
    try {
        ComputeMoveTarget(p, 1, levels);
    } catch (const std::runtime_error&) {
        return std::nullopt;  // Same no-neighbor edge case as MonsterInFront.
    }
    if (p.pendingLevel <= 0) return std::nullopt;

    const auto& chests = world.chests[static_cast<size_t>(p.pendingLevel - 1)];
    auto it = chests.find(PackTileKey(p.pendingTileX, p.pendingTileY));
    if (it == chests.end()) return std::nullopt;
    return it->second;
}

int PlayerMovement::ShopAheadOfPlayer(PlayerState& p, const LevelLookup& levels, const ShopState& shop) {
    try {
        ComputeMoveTarget(p, 1, levels);
    } catch (const std::runtime_error&) {
        return -1;  // Same no-neighbor edge case as MonsterInFront/ChestAheadOfPlayer.
    }
    if (p.pendingLevel <= 0) return -1;
    if (p.pendingLevel != 1) return -1;  // Every NPC stands on the hub town only.

    return Shop::QuestShopAt(shop, p.pendingTileX, p.pendingTileY);
}

bool PlayerMovement::CommitMove(PlayerState& p, int dir, const LevelLookup& levels, WorldRegistry& world,
                                 const ItemDatabase& items, const MonsterDatabase& monsterDb, WardenState& warden) {
    if (p.coreStats[6] <= 0) return false;
    if (dir == 0) return false;

    ComputeMoveTarget(p, dir, levels);
    bool isStep = (dir == 1 || dir == 2);

    if (p.pendingLevel <= 0) return false;

    GeneratedLevel& target = levels(p.pendingLevel);
    // M52: Player.commitMove()'s own real gate -- see this method's own
    // declaration comment for the confirmed GameSave::Load consequence.
    if (!target.populated) return false;

    uint8_t tileBits = target.tiles[static_cast<size_t>(p.pendingTileX)][static_cast<size_t>(p.pendingTileY)];
    if (!IsWalkableTileBits(tileBits)) return false;

    // M19: entering level 37 from anywhere else fully heals the
    // registered type-41 "roaming" monster -- see this method's own
    // declaration comment.
    if (p.pendingLevel == 37 && p.currentLevel != 37) {
        for (auto& [spawnId, record] : world.monsters[36]) {
            MonsterState m = MonsterRuntime::FromBytes(record);
            if (m.typeIndex == 41) {
                m.currentHp = static_cast<int8_t>(MonsterRuntime::Stat(m, monsterDb, 14));
                DungeonRuntime::StoreMonster(world, m);
            }
        }
    }

    GeneratedLevel& oldLevel = levels(p.currentLevel);
    bool oldWalkable = IsWalkableTileBits(oldLevel.tiles[static_cast<size_t>(p.tileX)][static_cast<size_t>(p.tileY)]);
    // newWalkable is trivially always true here -- we already returned
    // false above if the target tile weren't walkable, and
    // Dungeon.isWalkable() reads the exact same bits isWalkableTileBits()
    // does (confirmed by Player.java's own isWalkableTileBits() header
    // comment). A real, confirmed finding: this means `leftLevelZone`
    // (`oldWalkable && !newWalkable`) can NEVER actually become true via
    // commitMove() -- it's dead in practice, even though the original
    // computes it as if it could go either way. Computed faithfully
    // anyway (not hand-simplified to `false`) for line-for-line fidelity
    // with Player.java's own commitMove().
    bool newWalkable = IsWalkableTileBits(tileBits);
    p.enteredNewLevelZone = !oldWalkable && newWalkable;
    p.leftLevelZone = oldWalkable && !newWalkable;

    p.currentLevel = p.pendingLevel;
    p.prevTileX = p.tileX;
    p.prevTileY = p.tileY;
    p.tileX = p.pendingTileX;
    p.tileY = p.pendingTileY;
    p.facing = p.pendingFacing;
    target.visited = true;

    if (isStep) {
        // M19: `Shop.wardenPresent`'s on-any-step clear -- a direct flag
        // reset, NOT a call to `WardenState::Leave` (no tile touched at
        // all). See this method's own declaration comment for the
        // confirmed inconsistency this leaves between the flag and the
        // Warden's own hub tile.
        if (warden.present) warden.present = false;

        int fatigueCostMultiplier = (p.ailmentMask & 1) ? 3 : 1;
        p.coreStats[6] = static_cast<int16_t>(p.coreStats[6] - 1 * fatigueCostMultiplier);
        p.coreStats[6] = std::max<int16_t>(p.coreStats[6], 0);
    }

    // Player.commitMove()'s dropped-item auto-loot block -- see this
    // method's own declaration comment for the confirmed single-item/
    // multi-item bit-test asymmetry preserved below exactly.
    bool hasDroppedItem = (tileBits & 4) != 0;
    if (hasDroppedItem) {
        int levelIndex = p.currentLevel - 1;
        int count = DungeonRuntime::CountDroppedItemsAt(world, levelIndex, p.tileX, p.tileY);
        if (count == 1) {
            std::array<int8_t, 7> record = *DungeonRuntime::FirstDroppedItemAt(world, levelIndex, p.tileX, p.tileY);
            if (record[6] & 4) {
                p.pendingLockedItemFlag = true;
                RefreshCorridorView(p, target, levels);
                return true;
            }

            bool picked = PlayerInventory::TryPickUpItem(p, record);
            if (picked) {
                DungeonRuntime::RemoveDroppedItem(target, world, record);
                if (!(record[6] & 2)) {
                    int itemIndex = record[2] - 1;
                    if (items.category[static_cast<size_t>(itemIndex)] == 11) {
                        p.giftPointsFound = static_cast<int16_t>(p.giftPointsFound +
                                                                  items.subtype[static_cast<size_t>(itemIndex)]);
                        // M52: see this method's own declaration comment.
                        GameAdvancement::OpenZone(GameAdvancement::Level(p.giftPointsFound), levels);
                    }
                }
            }
        } else if (count > 1) {
            // Snapshot the tile's records up front (matching
            // Dungeon.droppedItemsAt()'s own Vector snapshot); `world`'s
            // underlying list is then mutated in-place by
            // RemoveDroppedItem as this loop runs, same as the original's
            // own Enumeration-over-a-mutating-Vector shape.
            std::vector<std::array<int8_t, 7>> records = DungeonRuntime::DroppedItemsAt(world, levelIndex, p.tileX, p.tileY);
            for (const auto& record : records) {
                if (record[6] & 4) {
                    p.pendingLockedItemFlag = true;
                    RefreshCorridorView(p, target, levels);
                    return true;
                }

                bool picked = PlayerInventory::TryPickUpItem(p, record);
                if (picked) {
                    DungeonRuntime::RemoveDroppedItem(target, world, record);
                    // Real, confirmed asymmetry vs. the count==1 branch
                    // above: THIS branch's gift-points condition is the
                    // OPPOSITE bit test (`!= 0` here vs. `== 0` above) --
                    // see this method's own declaration comment.
                    if (record[6] & 2) {
                        int itemIndex = record[2] - 1;
                        if (items.category[static_cast<size_t>(itemIndex)] == 11) {
                            p.giftPointsFound = static_cast<int16_t>(p.giftPointsFound +
                                                                      items.subtype[static_cast<size_t>(itemIndex)]);
                            // M52: see this method's own declaration
                            // comment.
                            GameAdvancement::OpenZone(GameAdvancement::Level(p.giftPointsFound), levels);
                        }
                    }
                }
            }
        }
    }

    RefreshCorridorView(p, target, levels);

    if (isStep && (tileBits & 8)) {
        // Player.commitMove()'s own ordering: refreshCorridorView() runs
        // BEFORE autoMarkCampOnTile(), matching the statement order above
        // exactly. autoMarkCampOnTile() -> markCampAndReturnToTown() calls
        // refreshCorridorView() AGAIN in the original (Player.java line
        // 2509, for the NEW hub position) -- MarkCampAndReturnToTown now
        // does this internally itself (this session's fix closed the gap
        // for its OTHER two real callers, PlayerInventory::UseItem's
        // camp-marker item and ShopInteraction::HelgaDialogue's own Warp
        // action, too -- see that method's own header comment), so no
        // second explicit call is needed here anymore.
        PlayerInventory::MarkCampAndReturnToTown(p, levels);
        p.justMarkedCamp = false;
    }

    return true;
}

bool PlayerMovement::Move(PlayerState& p, int dir, bool strafe, const LevelLookup& levels, WorldRegistry& world,
                           const ItemDatabase& items, const MonsterDatabase& monsterDb, WardenState& warden) {
    if (p.coreStats[6] <= 0) return false;

    if (strafe && dir == 4) {
        CommitMove(p, 4, levels, world, items, monsterDb, warden);
        bool result = CommitMove(p, 1, levels, world, items, monsterDb, warden);
        bool savedCrossing = p.crossingLevelBoundary;
        result = CommitMove(p, 3, levels, world, items, monsterDb, warden);
        p.crossingLevelBoundary = savedCrossing;
        return result;
    } else if (strafe && dir == 3) {
        CommitMove(p, 3, levels, world, items, monsterDb, warden);
        bool result = CommitMove(p, 1, levels, world, items, monsterDb, warden);
        bool savedCrossing = p.crossingLevelBoundary;
        result = CommitMove(p, 4, levels, world, items, monsterDb, warden);
        p.crossingLevelBoundary = savedCrossing;
        return result;
    } else {
        return CommitMove(p, dir, levels, world, items, monsterDb, warden);
    }
}

}  // namespace stormhold
