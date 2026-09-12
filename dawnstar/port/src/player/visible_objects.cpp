#include "player/visible_objects.h"

namespace dawnstar {

namespace {

// Shop.java's SHOP_X/SHOP_Y[0..4] -- the hub town's 5 fixed peddler
// positions. Indices 5-8 (the single named shopkeeper on levels
// 3/12/21/30) come from GeneratedLevel::specialShopX/Y instead (already
// captured by M6's DungeonGenerator) -- see visible_objects.h's
// VisibleSlot::npcShopIndex doc comment for why no full Shop class
// exists here.
constexpr int kHubShopX[5] = {12, 6, 7, 12, 12};
constexpr int kHubShopY[5] = {12, 11, 7, 8, 6};

// Player.tileAt(dx,dy)'s exact re-centering formula over corridorView --
// the same formula render/corridor_render_plan.cpp's own local `tileAt`
// lambda implements, but reading PlayerState::corridorView (populated by
// PlayerMovement::RefreshCorridorView) rather than a freshly-resampled
// grid: corridorView exists in this port specifically for this call
// site (see player_state.h's own doc comment on that field), which
// hadn't been ported until now.
uint8_t CorridorTileAt(const PlayerState& p, int dx, int dy) {
    return dy < 4 ? p.corridorView[static_cast<size_t>(dx + dy + 1)][static_cast<size_t>(dy)]
                  : p.corridorView[static_cast<size_t>(dx + dy)][static_cast<size_t>(dy)];
}

bool IsOccludedSlot(const VisibleSlot& slot) {
    return slot.kind == VisibleSlotKind::WallBlocked || slot.kind == VisibleSlotKind::Occluded;
}

}  // namespace

void VisibleObjects::Refresh(const PlayerState& p, std::array<VisibleSlot, 13>& slots) {
    slots.fill(VisibleSlot{});

    // dx/dy offsets ported straight from refreshVisibleObjects's own 13
    // tileAt() calls, in the same order.
    static constexpr int kOffsets[13][2] = {{-1, 1}, {0, 1}, {1, 1}, {-2, 2}, {-1, 2}, {0, 2}, {1, 2},
                                             {2, 2},  {-2, 3}, {-1, 3}, {0, 3}, {1, 3}, {2, 3}};
    for (int i = 0; i < 13; i++) {
        uint8_t tile = CorridorTileAt(p, kOffsets[i][0], kOffsets[i][1]);
        if (tile & 1) slots[static_cast<size_t>(i)].kind = VisibleSlotKind::WallBlocked;
    }

    auto occlude = [&](int i) { slots[static_cast<size_t>(i)].kind = VisibleSlotKind::Occluded; };

    if (IsOccludedSlot(slots[0])) {
        occlude(4);
        occlude(8);
        occlude(9);
    }
    if (IsOccludedSlot(slots[1])) {
        for (int i = 0; i < 13; i++) {
            if (i != 1) occlude(i);
        }
    }
    if (IsOccludedSlot(slots[2])) {
        occlude(6);
        occlude(11);
        occlude(12);
    }
    if (IsOccludedSlot(slots[3])) {
        occlude(8);
    }
    if (IsOccludedSlot(slots[4])) {
        occlude(8);
        occlude(9);
    }
    if (IsOccludedSlot(slots[5])) {
        occlude(9);
        occlude(10);
        occlude(11);
        occlude(4);
        occlude(6);
    }
    if (IsOccludedSlot(slots[6])) {
        occlude(11);
        occlude(12);
    }
    if (IsOccludedSlot(slots[7])) {
        occlude(12);
    }
    if (IsOccludedSlot(slots[9])) {
        occlude(8);
    }
    if (IsOccludedSlot(slots[10])) {
        occlude(9);
        occlude(11);
    }
    if (IsOccludedSlot(slots[11])) {
        occlude(12);
    }
}

int VisibleObjects::FindPlacementSlot(const PlayerState& p, const std::array<VisibleSlot, 13>& slots, int objX,
                                       int objY) {
    int col = 0;
    int row = 0;
    if (p.facing == 1 || p.facing == 3) {
        int sign = p.facing == 1 ? 1 : -1;
        col = sign * (objX - p.tileX) + 3;
        row = sign * (objY - p.tileY) + 3;
    } else if (p.facing == 2 || p.facing == 4) {
        int sign = p.facing == 2 ? 1 : -1;
        col = sign * (objY - p.tileY) + 3;
        row = 3 - sign * (objX - p.tileX);
    }

    int slotIndex = -1;
    if (col == 3 && row == 2) {
        slotIndex = 1;
    } else if (col == 2 && row == 1) {
        slotIndex = 4;
    } else if (col == 3 && row == 1) {
        slotIndex = 5;
    } else if (col == 4 && row == 1) {
        slotIndex = 6;
    } else if (col == 1 && row == 0) {
        slotIndex = 8;
    } else if (col == 2 && row == 0) {
        slotIndex = 9;
    } else if (col == 3 && row == 0) {
        slotIndex = 10;
    } else if (col == 4 && row == 0) {
        slotIndex = 11;
    } else if (col == 5 && row == 0) {
        slotIndex = 12;
    }

    if (slotIndex < 0) return -1;
    if (slots[static_cast<size_t>(slotIndex)].kind != VisibleSlotKind::Empty) return -1;
    return slotIndex;
}

void VisibleObjects::MarkLooted(WorldRegistry& world, int levelIndex, VisibleSlot& slot) {
    if (slot.kind == VisibleSlotKind::Monster) {
        std::array<uint8_t, 28> record = slot.monsterRecord;
        record[6] = 1;
        MonsterState scratch = MonsterRuntime::FromBytes(record);
        scratch.flag = true;
        std::array<uint8_t, 28> reencoded = MonsterRuntime::ToBytes(scratch);
        world.monsters[static_cast<size_t>(levelIndex)][PackPosKey(scratch.x, scratch.y)] = reencoded;
        slot.monsterRecord = reencoded;
    } else if (slot.kind == VisibleSlotKind::DroppedItem) {
        slot.droppedItemRecord[6] = static_cast<uint8_t>(slot.droppedItemRecord[6] | 1);
    }
}

void VisibleObjects::Tick(PlayerState& p, const std::vector<GeneratedLevel>& levels, WorldRegistry& world) {
    Refresh(p, p.visibleObjects);
    int levelIndex = p.currentLevel - 1;

    for (const auto& [key, record] : world.monsters[static_cast<size_t>(levelIndex)]) {
        (void)key;
        int slotIndex = FindPlacementSlot(p, p.visibleObjects, record[4], record[5]);
        if (slotIndex < 0) continue;
        VisibleSlot& slot = p.visibleObjects[static_cast<size_t>(slotIndex)];
        slot.kind = VisibleSlotKind::Monster;
        slot.monsterRecord = record;
        MarkLooted(world, levelIndex, slot);
    }

    for (const auto& [key, record] : world.chests[static_cast<size_t>(levelIndex)]) {
        (void)key;
        int slotIndex = FindPlacementSlot(p, p.visibleObjects, record[0], record[1]);
        if (slotIndex < 0) continue;
        VisibleSlot& slot = p.visibleObjects[static_cast<size_t>(slotIndex)];
        slot.kind = VisibleSlotKind::Chest;
        slot.chestRecord = record;
    }

    for (const auto& record : world.droppedItems[static_cast<size_t>(levelIndex)]) {
        int slotIndex = FindPlacementSlot(p, p.visibleObjects, record[0], record[1]);
        if (slotIndex < 0) continue;
        VisibleSlot& slot = p.visibleObjects[static_cast<size_t>(slotIndex)];
        slot.kind = VisibleSlotKind::DroppedItem;
        slot.droppedItemRecord = record;
        MarkLooted(world, levelIndex, slot);
    }

    // tickVisibleObjects's NPC tagging: all 5 hub peddlers in the hub
    // town, or the single named shopkeeper for levels 3/12/21/30 (via
    // GeneratedLevel::specialShopX/Y -- -1 on every other level, so this
    // is naturally a no-op there without needing an explicit level check
    // beyond "which shop index applies").
    if (p.currentLevel == 1) {
        for (int i = 0; i < 5; i++) {
            int slotIndex = FindPlacementSlot(p, p.visibleObjects, kHubShopX[i], kHubShopY[i]);
            if (slotIndex < 0) continue;
            VisibleSlot& slot = p.visibleObjects[static_cast<size_t>(slotIndex)];
            slot.kind = VisibleSlotKind::Npc;
            slot.npcShopIndex = i;
        }
    } else {
        int shopIndex = -1;
        if (p.currentLevel == 3) shopIndex = 5;
        else if (p.currentLevel == 12) shopIndex = 6;
        else if (p.currentLevel == 21) shopIndex = 7;
        else if (p.currentLevel == 30) shopIndex = 8;

        if (shopIndex >= 0) {
            const GeneratedLevel& level = levels[static_cast<size_t>(levelIndex)];
            if (level.specialShopX >= 0) {
                int slotIndex = FindPlacementSlot(p, p.visibleObjects, level.specialShopX, level.specialShopY);
                if (slotIndex >= 0) {
                    VisibleSlot& slot = p.visibleObjects[static_cast<size_t>(slotIndex)];
                    slot.kind = VisibleSlotKind::Npc;
                    slot.npcShopIndex = shopIndex;
                }
            }
        }
    }
}

}  // namespace dawnstar
