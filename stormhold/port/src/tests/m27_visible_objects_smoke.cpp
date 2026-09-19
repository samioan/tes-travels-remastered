// M27 smoke test: VisibleObjects, the 13-slot corridor-view object cache
// (data model only, no pixels -- mirrors the M21/M25 selection-logic-vs-
// drawing split, and dawnstar's own identical M25 milestone for its
// equivalent system).
//
// No JVM ground truth possible here (same reasoning M6/M21/M25's own
// tests already give) -- verified via hand-derived synthetic scenarios:
// FacingAxisDistance's exact formula per facing; ResolveSlot's position/
// occlusion-guard cascade at a handful of hand-derived (dx,dy) positions;
// RefreshSlots' wall-occlusion cascade, including a case that
// specifically proves the LITERAL SEQUENTIAL statement order (not a
// fixed-point loop) -- a slot shadowed by an EARLIER check's own cascade
// still triggers a LATER check that reads it; and Refresh()'s own
// world-wiring (dropped item/chest/monster placement, the monster/
// dropped-item "seen" flag write-back, and the confirmed-dead
// `includeWarden` parameter).
#include <cstdio>
#include <string>

#include "player/visible_objects.h"

namespace {

using namespace stormhold;

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

void TestFacingAxisDistance() {
    std::printf("-- FacingAxisDistance: per-facing formula + negative clamp --\n");
    PlayerState p;
    p.tileX = 17;
    p.tileY = 17;

    p.facing = 1;  // north
    Expect(VisibleObjects::FacingAxisDistance(p, 17, 15) == 2, "facing north: distance should be tileY - ty");
    Expect(VisibleObjects::FacingAxisDistance(p, 17, 19) == VisibleObjects::kNotVisible,
           "facing north: a target BEHIND the player (ty > tileY) should clamp to kNotVisible");

    p.facing = 3;  // south
    Expect(VisibleObjects::FacingAxisDistance(p, 17, 19) == 2, "facing south: distance should be ty - tileY");

    p.facing = 2;  // east
    Expect(VisibleObjects::FacingAxisDistance(p, 19, 17) == 2, "facing east: distance should be tx - tileX");

    p.facing = 4;  // west
    Expect(VisibleObjects::FacingAxisDistance(p, 15, 17) == 2, "facing west: distance should be tileX - tx");
}

void TestResolveSlotCanonicalPositions() {
    std::printf("-- ResolveSlot: canonical positions, facing north --\n");
    PlayerState p;
    p.tileX = 17;
    p.tileY = 17;
    p.facing = 1;

    // dx=(toX-17)+3, dy=(toY-17)+3 for facing==1.
    Expect(VisibleObjects::ResolveSlot(p, 17, 16) == 1, "(17,16) -> dx=3,dy=2 -> slot 1 (closest, unconditional)");
    Expect(VisibleObjects::ResolveSlot(p, 16, 15) == 4, "(16,15) -> dx=2,dy=1 -> slot 4 when 0/1/5 are unoccluded");
    Expect(VisibleObjects::ResolveSlot(p, 15, 14) == 8, "(15,14) -> dx=1,dy=0 -> slot 8 when 0/1/3/4/9 are unoccluded");
    Expect(VisibleObjects::ResolveSlot(p, 20, 20) == -1, "an unmapped (dx,dy) should resolve to no slot at all");

    // Occlusion guard: slot 5 blocked should prevent slot 4 from resolving.
    p.visibleObjects[5].kind = VisibleSlotKind::Blocked;
    Expect(VisibleObjects::ResolveSlot(p, 16, 15) == -1,
           "(16,15) should fail to resolve to slot 4 once slot 5 is occluded (guard: !occluded(0,1,5))");
}

void TestRefreshSlotsDirectWallBlocks() {
    std::printf("-- RefreshSlots: a single direct wall block + its own cascade --\n");
    PlayerState p;
    // Slot 0 is (dx=-1,dy=1) -> ViewGridAt(grid,-1,1) = grid[-1+1+1][1] = grid[1][1].
    p.corridorView[1][1] = 1;

    VisibleObjects::RefreshSlots(p);
    Expect(p.visibleObjects[0].kind == VisibleSlotKind::Blocked, "slot 0 should be directly Blocked");
    Expect(p.visibleObjects[4].kind == VisibleSlotKind::Shadowed, "slot 0 blocked should shadow slot 4");
    Expect(p.visibleObjects[8].kind == VisibleSlotKind::Shadowed, "slot 0 blocked should shadow slot 8");
    Expect(p.visibleObjects[9].kind == VisibleSlotKind::Shadowed, "slot 0 blocked should shadow slot 9");
    Expect(p.visibleObjects[1].kind == VisibleSlotKind::Empty, "slot 0's own cascade should NOT touch slot 1");
    Expect(p.visibleObjects[12].kind == VisibleSlotKind::Empty, "slot 0's own cascade should NOT touch slot 12");
}

void TestRefreshSlotsSequentialCascadeOrdering() {
    std::printf("-- RefreshSlots: sequential read-after-write cascade ordering --\n");
    PlayerState p;
    // Slot 5 is (dx=0,dy=2) -> ViewGridAt(grid,0,2) = grid[0+2+1][2] = grid[3][2].
    p.corridorView[3][2] = 1;

    VisibleObjects::RefreshSlots(p);
    Expect(p.visibleObjects[5].kind == VisibleSlotKind::Blocked, "slot 5 should be directly Blocked");
    Expect(p.visibleObjects[9].kind == VisibleSlotKind::Shadowed, "slot 5's own cascade should shadow slot 9 directly");
    Expect(p.visibleObjects[10].kind == VisibleSlotKind::Shadowed, "slot 5's own cascade should shadow slot 10 directly");
    Expect(p.visibleObjects[11].kind == VisibleSlotKind::Shadowed, "slot 5's own cascade should shadow slot 11 directly");
    Expect(p.visibleObjects[4].kind == VisibleSlotKind::Shadowed, "slot 5's own cascade should shadow slot 4 directly");
    Expect(p.visibleObjects[6].kind == VisibleSlotKind::Shadowed, "slot 5's own cascade should shadow slot 6 directly");

    // Slot 8 is NOT in slot 5's own direct shadow list -- it can only end
    // up Shadowed because the LATER `if (occluded(9)) shadow(8);` check
    // observes slot 9's shadow flag, which slot 5's EARLIER cascade just
    // wrote. This only holds if RefreshSlots preserves the original's
    // exact sequential statement order rather than, say, evaluating all
    // the occlusion conditions against a pre-cascade snapshot.
    Expect(p.visibleObjects[8].kind == VisibleSlotKind::Shadowed,
           "slot 8 should ALSO end up Shadowed, purely as a second-order effect of slot 5's cascade via slot 9's "
           "own later check -- proves sequential (not snapshot-based) ordering");
}

WorldRegistry MakeWorldWithOneOfEach() {
    WorldRegistry world(1);
    // Player at (17,17) facing north (see TestResolveSlotCanonicalPositions
    // for the dx/dy derivation): (16,15) -> slot 4, (15,14) -> slot 8.
    std::array<int8_t, 7> droppedItem = {16, 15, 1, 0, 0, 0, 0};
    world.droppedItems[0].push_back(droppedItem);

    std::array<int8_t, 8> chest = {15, 14, 0, 3, 5, 0, 1, 0};
    world.chests[0][PackTileKey(15, 14)] = chest;

    std::array<uint8_t, 28> monster{};
    monster[4] = 17;  // tileX
    monster[5] = 16;  // tileY -> (17,16), slot 1
    world.monsters[0][/*spawnId=*/1] = monster;

    return world;
}

void TestRefreshWiresWorldObjects() {
    std::printf("-- Refresh: places a dropped item/chest/monster and writes back 'seen' flags --\n");
    PlayerState p;
    p.tileX = 17;
    p.tileY = 17;
    p.facing = 1;
    p.currentLevel = 1;
    WorldRegistry world = MakeWorldWithOneOfEach();
    WardenState warden{};  // not present -- Warden placement not exercised here

    VisibleObjects::Refresh(p, world, /*includeWarden=*/true, warden);

    Expect(p.visibleObjects[1].kind == VisibleSlotKind::Monster, "the monster at (17,16) should land in slot 1");
    Expect(p.visibleObjects[4].kind == VisibleSlotKind::DroppedItem, "the dropped item at (16,15) should land in slot 4");
    Expect(p.visibleObjects[8].kind == VisibleSlotKind::Chest, "the chest at (15,14) should land in slot 8");

    Expect(world.monsters[0].at(1)[6] == 1, "the monster's own record[6] should be overwritten to 1 in the registry");
    Expect((world.droppedItems[0][0][6] & 1) != 0, "the dropped item's own record[6] bit 0 should be OR'd in the registry");
    Expect(world.chests[0].at(PackTileKey(15, 14))[3] == 3,
           "the chest's own record should be untouched in the registry (no case for chests in placeVisibleObject)");
}

void TestWardenPlacement() {
    std::printf("-- Refresh: places the Warden marker on level 1 when present --\n");
    PlayerState p;
    p.tileX = WardenState::kShopX;
    p.tileY = WardenState::kShopY + 1;  // one tile south of Varus -> facing north, he's directly ahead
    p.facing = 1;
    p.currentLevel = 1;
    WorldRegistry world(1);
    WardenState warden{};
    warden.present = true;

    VisibleObjects::Refresh(p, world, /*includeWarden=*/true, warden);
    Expect(p.visibleObjects[1].kind == VisibleSlotKind::Warden, "the Warden should resolve into slot 1, directly ahead");

    warden.present = false;
    VisibleObjects::Refresh(p, world, /*includeWarden=*/true, warden);
    Expect(p.visibleObjects[1].kind != VisibleSlotKind::Warden,
           "with the Warden absent, that same slot should NOT be the Warden marker");
}

void TestIncludeWardenParameterIsDead() {
    std::printf("-- Refresh: includeWarden has zero effect on the result --\n");
    PlayerState pTrue;
    pTrue.tileX = 17;
    pTrue.tileY = 17;
    pTrue.facing = 1;
    pTrue.currentLevel = 1;
    WorldRegistry worldTrue = MakeWorldWithOneOfEach();
    WardenState wardenTrue{};
    wardenTrue.present = true;
    VisibleObjects::Refresh(pTrue, worldTrue, /*includeWarden=*/true, wardenTrue);

    PlayerState pFalse = pTrue;
    pFalse.visibleObjects = {};
    WorldRegistry worldFalse = MakeWorldWithOneOfEach();
    WardenState wardenFalse{};
    wardenFalse.present = true;
    VisibleObjects::Refresh(pFalse, worldFalse, /*includeWarden=*/false, wardenFalse);

    bool allMatch = true;
    for (int i = 0; i < 13; i++) {
        if (pTrue.visibleObjects[static_cast<size_t>(i)].kind != pFalse.visibleObjects[static_cast<size_t>(i)].kind) {
            allMatch = false;
        }
    }
    Expect(allMatch, "includeWarden=true vs. false should produce identical slot kinds across the board");
}

}  // namespace

int main() {
    TestFacingAxisDistance();
    TestResolveSlotCanonicalPositions();
    TestRefreshSlotsDirectWallBlocks();
    TestRefreshSlotsSequentialCascadeOrdering();
    TestRefreshWiresWorldObjects();
    TestWardenPlacement();
    TestIncludeWardenParameterIsDead();

    if (!g_ok) {
        std::fprintf(stderr, "m27_visible_objects_smoke: FAILED\n");
        return 1;
    }

    std::printf("all checks passed\n");
    return 0;
}
