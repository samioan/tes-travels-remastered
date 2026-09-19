#pragma once
#include <array>
#include <cstdint>
#include <optional>

#include "assets/item_database.h"
#include "dungeon/dungeon_runtime.h"
#include "player/player_state.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/Player.java's general-purpose
// inventory/equip/camp-bookmark methods -- the pieces `player_creation.h`
// (M9) only needed a private, minimal slice of (`AddInventoryItemRaw`/
// `EquipItem`/`UnequipSlot`, now moved here and made public so both
// character creation and this file share one real implementation instead
// of two copies).
//
// **M17: `DropInventoryItem` now wires its own real registry side
// effect** (`this.currentDungeon().addDroppedItem(record)`), now that
// `dungeon/dungeon_runtime.h`'s `WorldRegistry` exists (M16) --
// takes the player's current `GeneratedLevel&`/`WorldRegistry&` as
// explicit caller-supplied parameters (this port has no persistent
// `ESGame`-equivalent session object to look `currentDungeon()` up from
// itself), same "caller supplies/owns world state" pattern M10's
// `CommitMove` already established for the analogous pickup case (which
// M17 also wires, in `player/player_movement.h`).
//
// **Still deliberately NOT included here:** `rollShopOutcome()` (needs
// `skillValue()`/`rollOutcome()`, combat-adjacent stat/RNG machinery -- a
// later combat milestone's job, see docs/PORT_ROADMAP.md). `TryPickUpItem`
// itself still does NOT touch any registry -- confirmed by reading the
// original directly, `tryPickUpItem()` never did either; it's always the
// CALLER (`commitMove()`) that removes the picked-up record from the
// registry right after calling it, which is exactly what `player/
// player_movement.h`'s own M17 wiring now does.
class PlayerInventory {
public:
    // Player.addInventoryItemRaw(itemId, packedValue, charge).
    static bool AddInventoryItemRaw(PlayerState& p, int itemId, int packedValue, int charge);

    // Player.removeInventorySlot(slot): unequips first if equipped, then
    // compacts the remaining slots down. Matches the original exactly,
    // including NOT clearing inventoryItemData for the slot that ends up
    // past the new inventoryCount after compaction (stale leftover data,
    // harmless since nothing reads past inventoryCount).
    //
    // Throws std::runtime_error for `slot < 0`, a case the real
    // decompiled `y(int)` has NO guard for either -- Java's own array
    // bounds check would throw ArrayIndexOutOfBoundsException reading
    // `this.H[-1]`, a real latent crash the original never guards
    // against. C++'s `operator[]` has no such free safety net, so a
    // negative slot here would silently read/write out of bounds
    // instead -- strictly worse than the original's own crash, not a
    // faithful port of it. Same discipline as `dungeon/dungeon_runtime.h`'s
    // `RemoveMonster`/M28's `unconfirmedTable_a` OOB guard. (`slot >=
    // inventoryCount` is a REAL, deliberate guard in the original --
    // that case still just returns false, unchanged.)
    static bool RemoveInventorySlot(PlayerState& p, int slot, const ItemDatabase& items);

    // Player.findEquippedSlotForItem(itemId) (M36, phase-3 port): the
    // slot index currently holding `itemId` in its EQUIPPED (negative)
    // encoding, or -1 if not found -- confirmed the "equipped" sign
    // convention `EquipItem`'s own `-std::abs(id)` write already
    // establishes, not independently re-derived. Only real confirmed
    // caller so far: `PlayerCombatStats::TickPerSecond`'s own "remove
    // the daedric weapon when its temporary effect expires" branch.
    static int FindEquippedSlotForItem(const PlayerState& p, int itemId);

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

    // Player.dropInventoryItem(slot): builds the 7-byte record and, EXCEPT
    // for item id 109 (a non-droppable special id), registers it into
    // `world` via DungeonRuntime::AddDroppedItem(level, world, record) --
    // matching the original's own `this.currentDungeon().addDroppedItem
    // (record)` call exactly, `level`/`world` standing in for that
    // session lookup. For item id 109, no record is produced at all
    // (empty optional) and `AddDroppedItem` is never called, but the slot
    // is still removed -- matching the original exactly. `level` MUST be
    // the GeneratedLevel for the player's OWN `p.currentLevel` (this
    // method does not check `level.number == p.currentLevel` -- caller's
    // responsibility, same trust-the-caller convention as `player/
    // player_movement.h`'s LevelLookup).
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
    static std::optional<std::array<int8_t, 7>> DropInventoryItem(PlayerState& p, int slot, const ItemDatabase& items,
                                                                    GeneratedLevel& level, WorldRegistry& world);

    // Player.hasCampMark().
    static bool HasCampMark(const PlayerState& p);

    // Player.markCampAndReturnToTown(): bookmarks the current position as
    // the camp mark, then respawns at the hub-town's DEATH/RESPAWN
    // position (12, 14) -- setHubSpawnPosition(true), NOT the (9, 10)
    // new-character position player_creation.cpp's ResetForNewCharacter
    // uses (setHubSpawnPosition(false)). The two are genuinely different
    // fixed points in the original (../../../src/Player.java's own
    // setHubSpawnPosition() header comment).
    //
    // **Real, deliberately unwired gap (M25):** the original calls
    // `this.refreshCorridorView()` as its own very next statement after
    // this (Player.java line 2493) -- not reproduced here, since doing so
    // faithfully needs a `PlayerMovement::LevelLookup` this method
    // doesn't take. `p.corridorView` is left stale (still describing the
    // pre-camp position) until the player's next `CommitMove`, which DOES
    // refresh it (see `PlayerMovement::RefreshCorridorView`). Harmless for
    // any caller that doesn't render off `corridorView` between this call
    // and the next move -- true of every caller so far (M12's smoke test,
    // player_movement.cpp's own auto-camp-on-tile branch).
    static void MarkCampAndReturnToTown(PlayerState& p);

    // Player.warpToCampMark(): restores position from the camp bookmark.
    // Faithfully does NOT touch `facing`/`pendingFacing` at all -- reading
    // the whole method confirms the original genuinely never resets
    // facing here, unlike markCampAndReturnToTown()'s own
    // setHubSpawnPosition(true) call, which does.
    //
    // Same real, deliberately unwired `refreshCorridorView()` gap as
    // MarkCampAndReturnToTown above (Player.java line 2501) -- see that
    // method's own header comment.
    static void WarpToCampMark(PlayerState& p);
};

}  // namespace stormhold
