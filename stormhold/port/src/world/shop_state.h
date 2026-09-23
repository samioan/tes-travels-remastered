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
// **Correction (M53, same session):** an earlier draft of this file, and
// M51's own note before it, both claimed `Shop.reset()` -- the only place
// every array field below gets allocated -- has zero callers anywhere in
// `src/*.java`, based on grepping for textual `Shop.reset()`/`reset()`
// call sites only. That grep missed `Shop.java`'s OWN trailing `static {
// reset(); }` initializer block (src/Shop.java:672-674) -- a real call
// site the JVM guarantees runs automatically the first time the `Shop`
// class is touched (JLS 12.4.1), which happens for real early in the
// real game's own startup (`ESGame.runAppload()` calls
// `Shop.loadDialogue()`, a static method reference that alone forces
// class init, at the latest). **`Shop.reset()` DOES run in the real
// game** -- every array field below IS properly allocated with real
// values before any other code ever touches it, and the "Save/Load and
// completing a level-up always throw `NullPointerException`" claim this
// file previously made is WRONG, corrected here rather than left
// standing. (M52's own `enterCurrentZoneStatic()` caveat, in
// world/game_advancement.h, already knew to check for exactly this kind
// of implicit class-load call site -- this file's own earlier draft
// simply didn't apply that same check to `Shop.reset()`.) One real,
// smaller consequence survives the correction: `reset()` runs via a
// static initializer, which JLS semantics guarantee fires at most ONCE
// per class-load -- i.e. once per app launch, not once per "New Game" --
// so if a player restarts a character without closing the app (death and
// respawn `resetState()`s the Player, per src/Player.java, but never
// re-touches `Shop`), quest-economy state carries over from whatever the
// previous character left it at. Not modeled by this struct either way
// (this port has no live app-lifetime Shop instance yet to carry state
// between a death-restart and the next), just noted for whoever wires
// this in for real.
//
// `Shop.clearQuestTurnInState()` (the confirmed real, already-flagged gap
// -- see player/player_leveling.h's own class comment, open since
// M13/M14) and `writeMasterLists()`/`readMasterLists()`'s own missing
// half of the save format (M49's own "what's next" note) are both still
// simply NOT WIRED into any live call site yet -- an ordinary port
// completeness gap now that this struct exists to wire them TO, not a
// bug-preservation question. Left for a later milestone; see
// docs/PORT_ROADMAP.md's own "what's next".
//
// Every array/field below default-initializes to the REAL state
// `Shop.reset()` itself produces (questRewardClaimable/firstVisit true
// for all 7 -- every quest reward starts claimable, every NPC starts on
// their own first-visit greeting -- everything else 0/false), matching
// what every other real code path in the original actually observes from
// the moment the Shop class loads onward.
struct ShopState {
    std::array<bool, 7> questRewardClaimable{true, true, true, true, true, true, true};
    std::array<bool, 7> firstVisit{true, true, true, true, true, true, true};
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
