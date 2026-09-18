#include "player/player_movement.h"

#include <algorithm>
#include <stdexcept>

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

bool PlayerMovement::CommitMove(PlayerState& p, int dir, const LevelLookup& levels) {
    if (p.coreStats[6] <= 0) return false;
    if (dir == 0) return false;

    ComputeMoveTarget(p, dir, levels);
    bool isStep = (dir == 1 || dir == 2);

    if (p.pendingLevel <= 0) return false;

    GeneratedLevel& target = levels(p.pendingLevel);
    // target.populated is always true for every level this port's
    // LevelLookup can actually return -- see GeneratedLevel::populated's
    // own header comment on why the real populated-gating (ESGame's
    // progressive zone-opening system) isn't modeled here, so there's no
    // equivalent of Player.commitMove()'s `if (!target.populated) return
    // false;` check to reproduce.

    uint8_t tileBits = target.tiles[static_cast<size_t>(p.pendingTileX)][static_cast<size_t>(p.pendingTileY)];
    if (!IsWalkableTileBits(tileBits)) return false;

    // Deliberately NOT modeled here (see class header comment): the
    // level-37-entry forced-respawn of the type-41 "roaming" monster,
    // dropped-item auto-pickup, and Shop.wardenPresent's on-any-step
    // clear.

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
        int fatigueCostMultiplier = (p.ailmentMask & 1) ? 3 : 1;
        p.coreStats[6] = static_cast<int16_t>(p.coreStats[6] - 1 * fatigueCostMultiplier);
        p.coreStats[6] = std::max<int16_t>(p.coreStats[6], 0);
    }

    return true;
}

bool PlayerMovement::Move(PlayerState& p, int dir, bool strafe, const LevelLookup& levels) {
    if (p.coreStats[6] <= 0) return false;

    if (strafe && dir == 4) {
        CommitMove(p, 4, levels);
        bool result = CommitMove(p, 1, levels);
        bool savedCrossing = p.crossingLevelBoundary;
        result = CommitMove(p, 3, levels);
        p.crossingLevelBoundary = savedCrossing;
        return result;
    } else if (strafe && dir == 3) {
        CommitMove(p, 3, levels);
        bool result = CommitMove(p, 1, levels);
        bool savedCrossing = p.crossingLevelBoundary;
        result = CommitMove(p, 4, levels);
        p.crossingLevelBoundary = savedCrossing;
        return result;
    } else {
        return CommitMove(p, dir, levels);
    }
}

}  // namespace stormhold
