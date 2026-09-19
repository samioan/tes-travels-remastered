#pragma once
#include <optional>

#include "player/player_state.h"
#include "world/warden.h"

namespace stormhold {

// M29: GameCanvas's own UI-flag static fields `resolveHudIconSet()`
// (../../../src/GameCanvas.java) reads -- deliberately NOT modeled on
// PlayerState, since these are GameCanvas's own class-level state in the
// original, not Player's (same class-boundary distinction M25's own
// header comment on `corridorView` already draws, just the other
// direction). Real meaning of all four is still unconfirmed beyond their
// control-flow role -- named after their original single-letter fields
// the same "unconfirmed_" convention this codebase already uses
// elsewhere (e.g. Player's own unconfirmedFlag2/unconfirmedIntField).
struct HudState {
    bool unconfirmedAa = false;
    bool unconfirmedM = false;
    bool unconfirmedR = false;
    bool unconfirmedW = false;
};

// GameCanvas.targetMonster's minimal shape -- just what
// IsNpcDialogueDue() itself reads (position + typeIndex). `std::nullopt`
// models the original's `targetMonster == null`. **No confirmed setter
// exists anywhere in ../../../src/ yet** -- same class of gap as the
// still-untranscribed tick-loop helpers this field's own real populate
// logic likely lives in. The caller supplies whatever it currently
// believes `targetMonster` to be, same "caller supplies/owns state"
// pattern this port already uses throughout (e.g. CorridorRenderPlan's
// own already-sampled view).
struct TargetMonsterInfo {
    int tileX = 0;
    int tileY = 0;
    int typeIndex = 0;
};

// Shop.isAdjacentToVarus(player): true only on the hub level (1), when
// the player is exactly Manhattan-distance 1 from Varus's fixed position
// (WardenState::kShopX/kShopY, (9,9) -- the same shop-6 position M8's
// WardenState already confirmed). Reuses those constants directly rather
// than duplicating the value or standing up a whole Shop module for one
// distance check -- there is still no ported Shop class beyond
// ShopDialogue's own text (M11) and WardenState's own visit mechanic
// (M8).
//
// **Not reproduced here:** Shop.java's own header comment on this method
// mentions "some confirmed-required state (`player.j == 1` in the
// original)" -- but the method's own actual body (read directly) has NO
// such check at all, only the level/distance test below. Looks like a
// stale comment from an earlier, less certain pass rather than a real
// missing condition; not corrected in ../src/Shop.java this milestone
// (out of scope -- flagged here for whichever future pass actually
// re-reads Shop.java itself).
inline bool IsAdjacentToVarus(int currentLevel, int tileX, int tileY) {
    if (currentLevel != 1) return false;
    int dx = tileX - WardenState::kShopX;
    if (dx < 0) dx = -dx;
    int dy = tileY - WardenState::kShopY;
    if (dy < 0) dy = -dy;
    return dx + dy == 1;
}

// GameCanvas.isNpcDialogueDue(): true when adjacent to Varus, OR when a
// target monster exists, is the level-37 type-41 "roaming" monster (M19)
// on level 37, and adjacent to the player.
inline bool IsNpcDialogueDue(const PlayerState& p, const std::optional<TargetMonsterInfo>& targetMonster) {
    if (IsAdjacentToVarus(p.currentLevel, p.tileX, p.tileY)) return true;
    if (!targetMonster.has_value()) return false;
    if (p.currentLevel != 37 || targetMonster->typeIndex != 41) return false;

    int dx = p.tileX - targetMonster->tileX;
    if (dx < 0) dx = -dx;
    int dy = p.tileY - targetMonster->tileY;
    if (dy < 0) dy = -dy;
    return dx + dy == 1;
}

// GameCanvas.resolveHudIconSet(): also stashed into `hotbarActionSet` in
// the original (read by keyPressed()'s own numeric-hotkey dispatch --
// input handling, out of scope for this port so far). Returns which of
// paintHud()'s three icon rows to show; the caller decides what (if
// anything) to do with that.
inline int ResolveHudIconSet(const HudState& hud, const PlayerState& p,
                              const std::optional<TargetMonsterInfo>& targetMonster) {
    if (hud.unconfirmedAa) return 1;
    if (hud.unconfirmedM || hud.unconfirmedR) return 2;
    return (hud.unconfirmedW && !IsNpcDialogueDue(p, targetMonster)) ? 2 : 0;
}

}  // namespace stormhold
