#include "death/death_tick.h"

#include "player/player_inventory.h"
#include "player/player_movement.h"

namespace dawnstar {

bool DeathTick::TickDeathState(PlayerState& player, std::vector<GeneratedLevel>& levels, WorldRegistry& world,
                                const ItemDatabase& items, ShopState& shop, MessagePopupState& messagePopup,
                                int64_t& deathTimeMs, int64_t nowMs, bool& suppressMoveThisTick) {
    if (player.deathState == 1) return true;

    if (player.deathState == 2) {
        player.deathState = 3;
        MessagePopup::Clear(messagePopup);
    }

    if (nowMs - deathTimeMs <= 5000) return false;

    // Player.normalizeForSummary(coreStats): current = max for HP/
    // Magicka/Fatigue, coreStats[8] zeroed -- here applied straight to
    // the live stats (player_save.cpp's own ToBytesSummary runs the same
    // four assignments against a COPY instead).
    player.coreStats[2] = player.coreStats[3];
    player.coreStats[4] = player.coreStats[5];
    player.coreStats[6] = player.coreStats[7];
    player.coreStats[8] = 0;

    // Drop every unequipped item, highest slot first (matches the
    // original's own downward loop, so RemoveSlot's compaction never
    // shifts a not-yet-visited slot out from under it).
    for (int slot = player.inventoryCount - 1; slot >= 0; slot--) {
        if (!PlayerInventory::IsEquipped(player, items, slot)) {
            PlayerInventory::RemoveSlot(player, items, slot);
        }
    }

    player.starFrostBonusActive = false;
    PlayerMovement::ResetState(player, true, levels, world);

    deathTimeMs = 0;
    player.deathState = 1;
    shop.showDeathGreeting = true;
    suppressMoveThisTick = true;
    player.minimapDirty = true;
    MessagePopup::Show(messagePopup, MessagePopup::WrapToTwoLines(DungeonRuntime::DisplayName(player.currentLevel)),
                        1, nowMs);
    return true;
}

}  // namespace dawnstar
