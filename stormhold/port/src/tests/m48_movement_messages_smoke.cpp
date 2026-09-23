// M48 smoke test: MovementMessages::Resolve (GameCanvas.
// resolveMovementSideEffects()'s own message-popup layer).
#include <cstdio>
#include <string>

#include "assets/asset_root.h"
#include "assets/dungeon_names.h"
#include "assets/item_database.h"
#include "player/movement_messages.h"

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

void TestLockedItemTakesPrecedence(const DungeonNames& names, const ItemDatabase& items) {
    std::printf("-- MovementMessages::Resolve: pendingLockedItemFlag takes precedence --\n");

    PlayerState p;
    p.pendingLockedItemFlag = true;
    p.enteredNewLevelZone = true;  // would otherwise produce a crossing message
    p.currentLevel = 1;

    MovementMessageResult result = MovementMessages::Resolve(p, /*inventoryCountBefore=*/0, names, items);
    Expect(result.lockedItemEndOfGame, "pendingLockedItemFlag set -> lockedItemEndOfGame reported");
    Expect(!result.crossingMessage.has_value(),
           "the crossing-message branch is SKIPPED when pendingLockedItemFlag is set (matches the "
           "original's own if/else, not an independent check)");
}

void TestCrossingMessages(const DungeonNames& names, const ItemDatabase& items) {
    std::printf("-- MovementMessages::Resolve: the 3-way crossing-message choice --\n");

    {
        PlayerState p;
        p.enteredNewLevelZone = true;
        p.leftLevelZone = true;  // enteredNewLevelZone wins, tested below
        p.currentLevel = 5;
        MovementMessageResult result = MovementMessages::Resolve(p, 0, names, items);
        Expect(result.crossingMessage.has_value() && (*result.crossingMessage)[0] == "Warden's" &&
                   (*result.crossingMessage)[1] == "Camp",
               "enteredNewLevelZone -> \"Warden's Camp\", takes precedence over leftLevelZone");
    }

    {
        // Confirmed permanently unreachable via real play (player/
        // player_movement.h's own M10 finding) -- exercised at the unit
        // level anyway, since the field and this method's own read of it
        // both still exist.
        PlayerState p;
        p.enteredNewLevelZone = false;
        p.leftLevelZone = true;
        p.currentLevel = 5;
        MovementMessageResult result = MovementMessages::Resolve(p, 0, names, items);
        Expect(result.crossingMessage.has_value() && (*result.crossingMessage)[0] == "Outer" &&
                   (*result.crossingMessage)[1] == "Camp",
               "leftLevelZone (dead branch) -> \"Outer Camp\", preserved");
    }

    {
        // ONLY crossingLevelBoundary set (neither entered nor left) --
        // the outer gate's 3rd flag, falling through to the current
        // level's own real display name.
        PlayerState p;
        p.crossingLevelBoundary = true;
        p.enteredNewLevelZone = false;
        p.leftLevelZone = false;
        p.currentLevel = 3;
        MovementMessageResult result = MovementMessages::Resolve(p, 0, names, items);
        const auto& expected = names.DisplayNames(3);
        Expect(result.crossingMessage.has_value() && (*result.crossingMessage)[0] == expected[0] &&
                   (*result.crossingMessage)[1] == expected[1],
               "crossingLevelBoundary alone -> the CURRENT level's own real dungnamesin.dat display name");
    }

    {
        // None of the 3 flags set -> no crossing message at all.
        PlayerState p;
        p.currentLevel = 3;
        MovementMessageResult result = MovementMessages::Resolve(p, 0, names, items);
        Expect(!result.crossingMessage.has_value(), "no crossing flag set -> no crossing message");
    }
}

void TestItemsFound(const ItemDatabase& items, const DungeonNames& names) {
    std::printf("-- MovementMessages::Resolve: itemsFound (None/One/Several) --\n");

    {
        PlayerState p;
        p.inventoryCount = 5;
        MovementMessageResult result = MovementMessages::Resolve(p, /*inventoryCountBefore=*/5, names, items);
        Expect(result.itemsFound == ItemsFoundKind::None, "no inventoryCount change -> None");
    }

    {
        PlayerState p;
        p.inventoryCount = 6;
        p.inventoryItemIds[5] = 3;  // item id 3, positive (unequipped)
        MovementMessageResult result = MovementMessages::Resolve(p, /*inventoryCountBefore=*/5, names, items);
        Expect(result.itemsFound == ItemsFoundKind::One, "+1 inventoryCount -> One");
        Expect(result.foundItemName == items.name[2], "foundItemName matches the real ItemDatabase name for id 3");
    }

    {
        // Negative (equipped, per the sign convention M12 established) --
        // Math.abs()'d, matching the original's own itemFoundMessageLines().
        PlayerState p;
        p.inventoryCount = 6;
        p.inventoryItemIds[5] = -3;
        MovementMessageResult result = MovementMessages::Resolve(p, /*inventoryCountBefore=*/5, names, items);
        Expect(result.foundItemName == items.name[2],
               "foundItemName resolves correctly even for a NEGATIVE (equipped) id -- abs()'d");
    }

    {
        PlayerState p;
        p.inventoryCount = 8;
        MovementMessageResult result = MovementMessages::Resolve(p, /*inventoryCountBefore=*/5, names, items);
        Expect(result.itemsFound == ItemsFoundKind::Several, "+3 inventoryCount -> Several");
    }
}

void TestCrossingAndItemsFoundAreIndependent(const DungeonNames& names, const ItemDatabase& items) {
    std::printf("-- MovementMessages::Resolve: crossing + items-found fire TOGETHER --\n");

    // Proves MovementMessageResult's own header comment: unlike every
    // other Result enum in this port, these two fields are genuinely
    // independent, matching the original's own two separate
    // if-statements (not one if/else-if).
    PlayerState p;
    p.enteredNewLevelZone = true;
    p.currentLevel = 1;
    p.inventoryCount = 6;
    p.inventoryItemIds[5] = 3;
    MovementMessageResult result = MovementMessages::Resolve(p, /*inventoryCountBefore=*/5, names, items);
    Expect(result.crossingMessage.has_value(), "crossing message present");
    Expect(result.itemsFound == ItemsFoundKind::One, "items-found ALSO present in the same call");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assets(root);
        DungeonNames names = DungeonNames::Load(assets);
        ItemDatabase items = ItemDatabase::Load(assets);

        TestLockedItemTakesPrecedence(names, items);
        TestCrossingMessages(names, items);
        TestItemsFound(items, names);
        TestCrossingAndItemsFoundAreIndependent(names, items);

        if (!g_ok) {
            std::fprintf(stderr, "m48_movement_messages_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m48_movement_messages_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
