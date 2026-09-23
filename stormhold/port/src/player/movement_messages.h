#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>

#include "assets/dungeon_names.h"
#include "assets/item_database.h"
#include "player/player_state.h"

namespace stormhold {

// How many new inventory slots a move just produced, so the caller can
// pick the right message -- see MovementMessages::Resolve's own comment.
enum class ItemsFoundKind { None, One, Several };

// What MovementMessages::Resolve found after a move, so the caller
// (main.cpp) can show the right popup(s) without this module depending on
// stormhold_render -- same "game logic returns a signal, caller shows the
// message" pattern player/death_sequence.h's own DeathTickResult and
// combat/spell_casting.h's own Result already use.
//
// **Unlike every other Result-shaped enum in this port, more than one
// field here can be meaningful from a SINGLE call**, matching the real
// original directly: GameCanvas.resolveMovementSideEffects() checks
// `crossingLevelBoundary`/... and `itemsFound` as two INDEPENDENT
// `if`-statements, not one big if/else-if -- both a level-crossing AND a
// same-move item pickup can genuinely happen together (nothing about
// CommitMove's own tile-bit checks makes the two mutually exclusive), and
// the original really does call showMessage() twice in that case, the
// second call's popup simply overwriting the first's. See Resolve's own
// comment for `lockedItemEndOfGame`'s own, genuinely exclusive, gate.
struct MovementMessageResult {
    // Player.pendingLockedItemFlag was set by this move -- see Resolve's
    // own declaration comment for what this means and why it's not
    // modeled further than this flag.
    bool lockedItemEndOfGame = false;
    // Set only when NOT lockedItemEndOfGame (matching the original's own
    // if/else) and crossingLevelBoundary/enteredNewLevelZone/
    // leftLevelZone -- already-formatted 2 lines, ready to show directly.
    std::optional<std::array<std::string, 2>> crossingMessage;
    // Always computed, independent of the two fields above (matching the
    // original's own unconditional post-move itemsFound check).
    ItemsFoundKind itemsFound = ItemsFoundKind::None;
    // Only meaningful when itemsFound == One -- the picked-up item's own
    // display name, NOT yet split into 2 lines (the caller already has a
    // word-split helper for this, main.cpp's own ItemFoundMessageLines --
    // reused rather than duplicated a 4th time, same "reuse the one
    // already-confirmed helper" precedent combat/spell_casting.h's own
    // ResolveSpellCycleInput doc comment already established for the
    // 3rd). "Several items!" (itemsFound == Several) needs no name at all
    // -- a fixed 2-line message, same as the original's own
    // MSG_FOUND_SEVERAL_ITEMS constant -- so the caller can build it
    // directly without this struct carrying anything more for that case.
    std::string foundItemName;
};

class MovementMessages {
public:
    // GameCanvas.resolveMovementSideEffects() (was decompiled/e.java's
    // n()) -- the message-popup half of tickPlayerAction()'s own final
    // dispatch branch. Player.move(dir, strafe) itself (M10/M17/M19/M25)
    // is NOT called here -- the caller (main.cpp) already calls
    // PlayerMovement::Move directly, this method only inspects what
    // changed afterward, same "wrap an already-ported action, add the
    // message layer GameCanvas puts around it" role player/
    // death_sequence.h's own RespawnMessageLines already has relative to
    // PlayerCreation::RespawnAfterDeath.
    //
    // `p` is the PLAYER'S POST-MOVE state; `inventoryCountBefore` is
    // whatever `p.inventoryCount` was immediately BEFORE the caller's own
    // PlayerMovement::Move call (a plain `byte` snapshot in the original
    // too, not re-derived here).
    //
    // **`Player.pendingLockedItemFlag` being true here means the original
    // shows ESGame.newEndOfGameUI() and disables auto-repaint instead of
    // any popup message at all** (`GameCanvas.java`'s own header comment
    // on this method: the confirmed real trigger for Stormhold's
    // end-of-game/victory sequence) -- NOT modeled, this port has no
    // end-of-game screen at all yet. `lockedItemEndOfGame` is set so the
    // caller at least knows this happened, but there is nothing further
    // to do with it today. **Also confirmed, by grepping every reference
    // to the field: `pendingLockedItemFlag` is never reset to `false`
    // anywhere in the whole original codebase either** (Player.java's own
    // two write sites -- both inside CommitMove's dropped-item block,
    // player/player_movement.h -- only ever set it TRUE), so once a
    // player picks up one locked item, EVERY subsequent call here keeps
    // reporting `lockedItemEndOfGame = true` forever, the crossing-
    // message branch permanently unreachable from then on -- a real,
    // if likely inconsequential-in-practice (the original genuinely
    // leaves normal play once this triggers), confirmed original quirk,
    // preserved rather than "fixed" with an invented reset.
    //
    // Otherwise (not locked): `p.enteredNewLevelZone` -> "Warden's Camp";
    // else `p.leftLevelZone` -> "Outer Camp" (confirmed permanently
    // unreachable, player/player_movement.h's own M10 finding, preserved
    // anyway); else, if ONLY `p.crossingLevelBoundary` is set (matching
    // the original's own OR'd outer gate testing 3 flags but its inner
    // if/else-if only testing 2), the CURRENT level's own real display
    // name (DungeonNames::DisplayNames(p.currentLevel)) -- byte-for-byte
    // the SAME 3-way choice player/death_sequence.h's own
    // RespawnMessageLines already makes, confirmed from GameCanvas.java's
    // own header comment as literally the same original 3-way choice
    // repeated at a 2nd call site, not just a structurally-similar one.
    //
    // `itemsFound` is always computed, independent of the above -- see
    // this method's own class-header comment (MovementMessageResult) for
    // why both can be meaningful from the same call.
    static MovementMessageResult Resolve(const PlayerState& p, int8_t inventoryCountBefore, const DungeonNames& names,
                                          const ItemDatabase& items);
};

}  // namespace stormhold
