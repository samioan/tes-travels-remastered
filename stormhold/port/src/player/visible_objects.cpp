#include "player/visible_objects.h"

namespace stormhold {

int VisibleObjects::FacingAxisDistance(const PlayerState& p, int tx, int ty) {
    int dist;
    if (p.facing == 1) {
        dist = p.tileY - ty;
    } else if (p.facing == 3) {
        dist = ty - p.tileY;
    } else if (p.facing == 2) {
        dist = tx - p.tileX;
    } else {
        dist = p.tileX - tx;
    }

    if (dist < 0) dist = kNotVisible;
    return dist;
}

namespace {
bool IsOccluded(const PlayerState& p, int slot) {
    VisibleSlotKind k = p.visibleObjects[static_cast<size_t>(slot)].kind;
    return k == VisibleSlotKind::Blocked || k == VisibleSlotKind::Shadowed;
}
}  // namespace

int VisibleObjects::ResolveSlot(const PlayerState& p, int ox, int oy) {
    std::array<int, 2> offset = DungeonRuntime::RelativeViewOffset(p.tileX, p.tileY, p.facing, ox, oy);
    int dx = offset[0];
    int dy = offset[1];

    if (dx == 3 && dy == 2) {
        return 1;
    } else if (dx == 2 && dy == 1) {
        if (!IsOccluded(p, 0) && !IsOccluded(p, 1) && !IsOccluded(p, 5)) return 4;
    } else if (dx == 3 && dy == 1) {
        if (!IsOccluded(p, 1)) return 5;
    } else if (dx == 4 && dy == 1) {
        if (!IsOccluded(p, 1) && !IsOccluded(p, 2) && !IsOccluded(p, 5)) return 6;
    } else if (dx == 1 && dy == 0) {
        if (!IsOccluded(p, 0) && !IsOccluded(p, 1) && !IsOccluded(p, 3) && !IsOccluded(p, 4) && !IsOccluded(p, 9)) {
            return 8;
        }
    } else if (dx == 2 && dy == 0) {
        if (!IsOccluded(p, 0) && !IsOccluded(p, 1) && !IsOccluded(p, 4) && !IsOccluded(p, 5) && !IsOccluded(p, 10)) {
            return 9;
        }
    } else if (dx == 3 && dy == 0) {
        if (!IsOccluded(p, 1) && !IsOccluded(p, 5)) return 10;
    } else if (dx == 4 && dy == 0) {
        if (!IsOccluded(p, 1) && !IsOccluded(p, 2) && !IsOccluded(p, 5) && !IsOccluded(p, 6) && !IsOccluded(p, 10)) {
            return 11;
        }
    } else if (dx == 5 && dy == 0) {
        if (!IsOccluded(p, 1) && !IsOccluded(p, 2) && !IsOccluded(p, 6) && !IsOccluded(p, 7) && !IsOccluded(p, 11)) {
            return 12;
        }
    }

    return -1;
}

void VisibleObjects::RefreshSlots(PlayerState& p) {
    for (auto& slot : p.visibleObjects) slot = VisibleSlot{};

    auto blockIfWall = [&p](int slotIndex, int dx, int dy) {
        uint8_t v = DungeonRuntime::ViewGridAt(p.corridorView, dx, dy);
        if (v & 1) p.visibleObjects[static_cast<size_t>(slotIndex)].kind = VisibleSlotKind::Blocked;
    };

    blockIfWall(0, -1, 1);
    blockIfWall(1, 0, 1);
    blockIfWall(2, 1, 1);
    blockIfWall(3, -2, 2);
    blockIfWall(4, -1, 2);
    blockIfWall(5, 0, 2);
    blockIfWall(6, 1, 2);
    blockIfWall(7, 2, 2);
    blockIfWall(8, -2, 3);
    blockIfWall(9, -1, 3);
    blockIfWall(10, 0, 3);
    blockIfWall(11, 1, 3);
    blockIfWall(12, 2, 3);

    auto shadow = [&p](int slotIndex) { p.visibleObjects[static_cast<size_t>(slotIndex)].kind = VisibleSlotKind::Shadowed; };

    // Literal sequential order matching Player.refreshVisibleObjectSlots()
    // -- see this class's own RefreshSlots() header comment on why later
    // checks here can observe earlier checks' own writes.
    if (IsOccluded(p, 0)) {
        shadow(4);
        shadow(8);
        shadow(9);
    }
    if (IsOccluded(p, 1)) {
        for (int i = 0; i < 13; i++) {
            if (i != 1) shadow(i);
        }
    }
    if (IsOccluded(p, 2)) {
        shadow(6);
        shadow(11);
        shadow(12);
    }
    if (IsOccluded(p, 3)) {
        shadow(8);
    }
    if (IsOccluded(p, 4)) {
        shadow(8);
        shadow(9);
    }
    if (IsOccluded(p, 5)) {
        shadow(9);
        shadow(10);
        shadow(11);
        shadow(4);
        shadow(6);
    }
    if (IsOccluded(p, 6)) {
        shadow(11);
        shadow(12);
    }
    if (IsOccluded(p, 7)) {
        shadow(12);
    }
    if (IsOccluded(p, 9)) {
        shadow(8);
    }
    if (IsOccluded(p, 10)) {
        shadow(9);
        shadow(11);
    }
    if (IsOccluded(p, 11)) {
        shadow(12);
    }
}

void VisibleObjects::Refresh(PlayerState& p, WorldRegistry& world, bool includeWarden, const WardenState& warden) {
    (void)includeWarden;  // confirmed dead -- see this method's own header comment
    RefreshSlots(p);

    size_t levelIndex = static_cast<size_t>(p.currentLevel - 1);

    for (auto& record : world.droppedItems[levelIndex]) {
        int slot = ResolveSlot(p, record[0], record[1]);
        if (slot < 0) continue;
        p.visibleObjects[static_cast<size_t>(slot)].kind = VisibleSlotKind::DroppedItem;
        p.visibleObjects[static_cast<size_t>(slot)].droppedItemRecord = record;
        record[6] = static_cast<int8_t>(record[6] | 1);
    }

    for (auto& [key, record] : world.chests[levelIndex]) {
        (void)key;
        int slot = ResolveSlot(p, record[0], record[1]);
        if (slot < 0) continue;
        // No `case 4` in Player.placeVisibleObject()'s own switch -- a
        // chest never gets a "seen" flag mutation, nothing written back.
        p.visibleObjects[static_cast<size_t>(slot)].kind = VisibleSlotKind::Chest;
        p.visibleObjects[static_cast<size_t>(slot)].chestRecord = record;
    }

    for (auto& [spawnId, record] : world.monsters[levelIndex]) {
        (void)spawnId;
        int slot = ResolveSlot(p, static_cast<int8_t>(record[4]), static_cast<int8_t>(record[5]));
        if (slot < 0) continue;
        // Player.placeVisibleObject(): `data[6] = 1`, an OVERWRITE, not
        // `|=` -- see GameCanvas.java's own header comment on why this
        // permanently means "has ever been seen".
        record[6] = 1;
        p.visibleObjects[static_cast<size_t>(slot)].kind = VisibleSlotKind::Monster;
        p.visibleObjects[static_cast<size_t>(slot)].monsterRecord = record;
    }

    if (p.currentLevel == 1 && warden.present) {
        int slot = ResolveSlot(p, WardenState::kShopX, WardenState::kShopY);
        if (slot >= 0) p.visibleObjects[static_cast<size_t>(slot)].kind = VisibleSlotKind::Warden;
    }
}

}  // namespace stormhold
