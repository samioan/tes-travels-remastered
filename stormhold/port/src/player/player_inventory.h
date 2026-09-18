#pragma once
#include <array>
#include <cstdint>
#include <optional>

#include "assets/item_database.h"
#include "player/player_state.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/Player.java's general-purpose
// inventory/equip/camp-bookmark methods -- the pieces `player_creation.h`
// (M9) only needed a private, minimal slice of (`AddInventoryItemRaw`/
// `EquipItem`/`UnequipSlot`, now moved here and made public so both
// character creation and this file share one real implementation instead
// of two copies).
//
// **Deliberately NOT included here:** `dropInventoryItem()`'s real
// counterpart, `rollShopOutcome()` (needs `skillValue()`/`rollOutcome()`,
// combat-adjacent stat/RNG machinery -- a later combat milestone's job,
// see docs/PORT_ROADMAP.md), and `tryPickUpItem()`'s/`dropInventoryItem()`
// 's own world-registry side effect (`Dungeon.addDroppedItem()` -- no
// persistent per-level dropped-item registry exists in this port yet,
// same "caller supplies/owns world state" gap M10's `CommitMove` already
// flagged for the analogous pickup case).
class PlayerInventory {
public:
    // Player.addInventoryItemRaw(itemId, packedValue, charge).
    static bool AddInventoryItemRaw(PlayerState& p, int itemId, int packedValue, int charge);

    // Player.removeInventorySlot(slot): unequips first if equipped, then
    // compacts the remaining slots down. Matches the original exactly,
    // including NOT clearing inventoryItemData for the slot that ends up
    // past the new inventoryCount after compaction (stale leftover data,
    // harmless since nothing reads past inventoryCount).
    static bool RemoveInventorySlot(PlayerState& p, int slot, const ItemDatabase& items);

    // Player.unequipSlot(slot).
    static void UnequipSlot(PlayerState& p, int slot, const ItemDatabase& items);

    // Player.equipItem(slot, allowSwap) -- see player_creation.h's own
    // header comment and ../../../src/Player.java's equipItem() header
    // comment for the real phase-1 renaming bug this replaces (equip-slot
    // indexing, not raw item category).
    static bool EquipItem(PlayerState& p, int slot, bool allowSwap, const ItemDatabase& items);

    // Player.equipLastPickedUpItem(allowSwap).
    static bool EquipLastPickedUpItem(PlayerState& p, bool allowSwap, const ItemDatabase& items);

    // Player.initializeItemCharge(slot) -- gates on Item.isEquipmentCategory,
    // sets a fixed charge value of 3 in the low byte of inventoryItemData[slot].
    static bool InitializeItemCharge(PlayerState& p, int slot, const ItemDatabase& items);

    // Player.isItemCharged(slot).
    static bool IsItemCharged(const PlayerState& p, int slot);

    // Player.tryPickUpItem(record): `record` is a dropped-item's 7-byte
    // on-tile record ([tileX, tileY, itemId, packedHi, packedLo, charge,
    // flags], matching DropInventoryItem's own layout below and
    // Dungeon.java's dropped-item format), kept as SIGNED bytes (int8_t),
    // matching Java's own `byte[]` exactly -- see DropInventoryItem's own
    // header comment for why the sign matters here, not just the bit
    // pattern. Does NOT touch any world registry -- the caller removes
    // `record` from wherever it came from.
    static bool TryPickUpItem(PlayerState& p, const std::array<int8_t, 7>& record);

    // Player.dropInventoryItem(slot): builds the 7-byte record
    // `Dungeon.addDroppedItem()` would receive and removes the slot --
    // EXCEPT for item id 109 (a non-droppable special id), where no
    // record is produced at all (empty optional) but the slot is still
    // removed, matching the original exactly. The caller is responsible
    // for actually inserting a non-empty result into a real dropped-item
    // world registry, once one exists.
    //
    // **A real, confirmed original-game quirk, preserved rather than
    // fixed:** bytes 3/4 pack the inventory slot's "packed value" (most
    // often `Item.nextSpawnId()`'s result) as two Java `byte`s (SIGNED,
    // -128..127 each). tryPickUpItem() reconstructs it via `(record[3] <<
    // 8) + record[4]` -- since record[3] is a signed byte, this
    // SIGN-EXTENDS whenever the packed value's high byte is >= 0x80 (i.e.
    // the value is >= 32768), reconstructing a NEGATIVE value instead of
    // the original positive one. Confirmed real Java `byte` arithmetic
    // (not a transcription slip on this port's side), and the exact same
    // shape as `Player.collectChestItem()`'s own `(record[5] << 8) +
    // record[6]` chest-value unpacking. Whether real gameplay can ever
    // actually reach a value in this range isn't exhaustively traced here
    // -- `Item.nextSpawnId()` alone is bounded to a Java `short`'s
    // positive range under normal play, so that specific producer
    // wouldn't reach it, but other producers aren't ruled out. Reproduced
    // exactly (int8_t, not uint8_t) rather than "corrected" to always
    // round-trip cleanly -- see `player_inventory_smoke.cpp`'s own test
    // demonstrating the exact reconstructed value for a synthetic
    // out-of-range input.
    static std::optional<std::array<int8_t, 7>> DropInventoryItem(PlayerState& p, int slot,
                                                                    const ItemDatabase& items);

    // Player.hasCampMark().
    static bool HasCampMark(const PlayerState& p);

    // Player.markCampAndReturnToTown(): bookmarks the current position as
    // the camp mark, then respawns at the hub-town's DEATH/RESPAWN
    // position (12, 14) -- setHubSpawnPosition(true), NOT the (9, 10)
    // new-character position player_creation.cpp's ResetForNewCharacter
    // uses (setHubSpawnPosition(false)). The two are genuinely different
    // fixed points in the original (../../../src/Player.java's own
    // setHubSpawnPosition() header comment).
    static void MarkCampAndReturnToTown(PlayerState& p);

    // Player.warpToCampMark(): restores position from the camp bookmark.
    // Faithfully does NOT touch `facing`/`pendingFacing` at all -- reading
    // the whole method confirms the original genuinely never resets
    // facing here, unlike markCampAndReturnToTown()'s own
    // setHubSpawnPosition(true) call, which does.
    static void WarpToCampMark(PlayerState& p);
};

}  // namespace stormhold
