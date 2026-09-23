#pragma once
#include <array>
#include <cstdint>

#include "assets/item_database.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/Shop.java's LIVE per-NPC
// quest-economy state -- questRewardClaimable[7]/firstVisit[7]/
// questState1[4]/questState2[4]/interactionCount[4]/rewardsGiven[4]/
// unconfirmedCooldownH[4]/benecaPoints/helgaPoints/showSpecialGreeting.
// Deliberately excludes wardenVisitCount/wardenPresent -- those already
// live on world/warden.h's own WardenState (M8/M51), ported and wired
// separately since Shop.java's own header comment splits shop 6 (Varus/
// the Warden) out from shops 0-3/4-5 as a genuinely distinct mechanic.
//
// **Why this struct exists now but still isn't wired into any live call
// site this milestone:** tracing `Shop.clearQuestTurnInState()` (the one
// confirmed real, ALREADY-flagged gap -- see player/player_leveling.h's
// own class comment, open since M13/M14) all the way to its own field
// writes surfaced a second, much larger consequence of M51's own
// `Shop.reset()` finding than M51 itself had connected:
//
// `Shop.reset()` -- the ONLY place `questRewardClaimable`/`firstVisit`/
// `questState1`/`questState2`/`interactionCount`/`rewardsGiven`/
// `unconfirmedCooldownH` (all seven ARRAY fields below) ever get
// allocated -- has zero callers anywhere in `src/*.java` (confirmed
// directly, same grep M51 already ran, re-verified here). Every one of
// those seven fields is therefore a permanently-null Java array for the
// entire life of the real shipped game. Two confirmed, real, live code
// paths index into them unconditionally, with no null check:
//
//  - `ESGame.writeMasterLists()`/`readMasterLists()` (src/ESGame.java,
//    the still-unported "missing half" of the save format M49's own
//    "what's next" note flagged) -- both are reached by `saveGameState()`
//    /`loadGameState()` (confirmed real: `run()`'s own
//    `helperThreadState` 5/6 dispatch), so EVERY Save and EVERY Load in
//    the real shipped game throws a `NullPointerException` on the very
//    first `Shop.firstVisit[i]` access, caught by `saveGameState()`'s/
//    `loadGameState()`'s own try/catch (confirmed by reading both:
//    `saveGameState()` deletes the just-created record store and returns
//    false; `run()`'s own dispatch then shows the Save-error screen
//    instead of resuming gameplay). **Save and Load are both completely
//    non-functional in the real shipped game.**
//  - `Player.consumeLevelExp()` calls `Shop.clearQuestTurnInState()` as
//    its own first statement (src/Player.java:2966, `questState1[i] = 0`
//    unconditionally); its own confirmed sole caller is
//    `ESGame.commandAction()`'s screenGroup-39 handler, the THIRD (final)
//    step of the real level-up-attribute-choice flow (src/ESGame.java
//    line ~1073), called right after the chosen attributes are already
//    applied and derived stats already recomputed, with the screen
//    transition back to gameplay (`showScreen(gameCanvas)`) as the very
//    next statement. **Completing a level-up in the real shipped game
//    throws an uncaught `NullPointerException` from inside a raw MIDP
//    `commandAction()` callback with no surrounding try/catch** -- the
//    attribute boost and `computeDerivedStats()` calls already ran, but
//    `coreStats[1] -= 10` (consumeLevelExp()'s own very next line after
//    the crash) and the return to `gameCanvas` never happen.
//
// Both are genuine, severe, confirmed bugs in the original -- not
// guesses. But UNLIKE M52's softlock (whose only "fix" would be adding a
// call the original never makes), reproducing either one faithfully in
// this port would mean deliberately BREAKING already-shipped, already-
// verified port functionality: M49/M50's own save/load system (which
// already works, and was already, unknowingly, a "behavioral gain" over
// the original the moment M50 landed) and M13/M14's own leveling system.
// That is a materially different, much higher-stakes call than M51/M52's
// own "don't add a call the original never makes" choice, and not one to
// make silently while just building this struct. So, same as M52's own
// "heads up for whoever eventually wires a real Save trigger" note: this
// struct provides no `Reset()`-equivalent method at all (nothing to
// accidentally wire up the same way `Shop.reset()` should have been but
// never was), and is not yet referenced from player/player_leveling.h,
// player/game_save.h, or dungeon/world_save.h. Whether/how to eventually
// close the M13/M14/M49 gaps this unblocks, given what closing them now
// provably means relative to the real game, is flagged in
// docs/PORT_ROADMAP.md's own "what's next" as a conscious decision point
// for later, not decided here.
//
// Every field below simply zero/false-initializes, the same "caller
// supplies/owns state, nothing here invents a live tracker" precedent
// dungeon/dungeon_runtime.h's own HubMinimapMarkers already established
// -- not an attempt at a "faithful default" (there isn't one: the real
// fields never successfully initialize at all).
struct ShopState {
    std::array<bool, 7> questRewardClaimable{};
    std::array<bool, 7> firstVisit{};
    std::array<int8_t, 4> questState1{};
    std::array<int8_t, 4> questState2{};
    std::array<int16_t, 4> interactionCount{};
    std::array<int16_t, 4> rewardsGiven{};
    std::array<int16_t, 4> unconfirmedCooldownH{};
    int16_t benecaPoints = 0;
    int16_t helgaPoints = 0;
    bool showSpecialGreeting = false;
};

class Shop {
public:
    // Shop.SHOP_X[]/SHOP_Y[] -- the 7 hub-town NPCs' fixed world
    // positions. Duplicated locally rather than shared with
    // dungeon/dungeon_runtime.h's own kShopMarkerX/kShopMarkerY (or
    // world/warden.h's own kShopX/kShopY for index 6) -- same
    // cross-layer duplication that pair already uses, confirmed
    // identical data, see either one's own comment for why stormhold_world
    // can't depend "sideways" on stormhold_dungeon.
    static constexpr int kShopX[7] = {12, 3, 15, 6, 7, 12, 9};
    static constexpr int kShopY[7] = {3, 7, 7, 13, 2, 13, 9};

    // Shop.SHOP_CATEGORY[] -- 1 = quest-turn-in shopkeeper (shops 0-3),
    // 2 = bespoke single-NPC (Beneca/Helga, shops 4-5), 3 = Varus/
    // Warden-tied (shop 6).
    static constexpr int kShopCategory[7] = {1, 1, 1, 1, 2, 2, 3};

    // Shop.isQuestShop(shopId): SHOP_CATEGORY[shopId] == 1.
    static bool IsQuestShop(int shopId) { return kShopCategory[shopId] == 1; }

    // Shop.questShopAt(x, y): which of shops 0-6 sits at (x, y) AND still
    // has its own one-shot `questRewardClaimable` flag set -- a
    // flag-gated position lookup, not a plain one. Returns -1 when none
    // match, matching the original exactly.
    static int QuestShopAt(const ShopState& state, int x, int y) {
        for (int i = 0; i < 7; i++) {
            if (x == kShopX[i] && y == kShopY[i] && state.questRewardClaimable[static_cast<size_t>(i)]) return i;
        }
        return -1;
    }

    // Shop.questFlagsFor(shopId, itemId): extracts shop 0-3's own 2-bit
    // quest-turn-in flag from an item's packed questFlags byte
    // (Item.column(3, itemId), src/Item.java, already loaded as
    // ItemDatabase::questFlags, M4). Only ever called for shopId 0-3 in
    // the original (shops 4-6 aren't quest-turn-in shops -- see
    // IsQuestShop); shopId 4+ falls through to the original's own final
    // `else` branch and returns 0, reproduced here as the same fallthrough
    // rather than a narrower guard. Java's `>>> N & 3` on a SIGN-EXTENDED
    // byte-turned-int and a plain unsigned `>> N & 3` on the raw byte
    // reinterpreted unsigned agree bit-for-bit for every N this method
    // actually uses (0/2/4/6): all four extracted bit pairs sit within the
    // byte's own low 8 bits, never reaching the sign-extension bits Java's
    // `>>>` would otherwise zero-fill differently from a narrower shift --
    // confirmed by hand-checking the extremal case (a byte with its own
    // sign bit set) against all four shift amounts.
    static int QuestFlagsFor(int shopId, int itemId, const ItemDatabase& items) {
        int flags = static_cast<uint8_t>(items.questFlags[static_cast<size_t>(itemId - 1)]);
        switch (shopId) {
            case 0:
                return (flags >> 6) & 3;
            case 1:
                return (flags >> 4) & 3;
            case 2:
                return (flags >> 2) & 3;
            case 3:
                return flags & 3;
            default:
                return 0;
        }
    }
};

}  // namespace stormhold
