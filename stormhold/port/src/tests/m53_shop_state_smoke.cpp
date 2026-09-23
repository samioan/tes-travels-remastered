// M53 smoke test: ShopState/Shop's pure lookup helpers (world/shop_state.h)
// -- Shop.isQuestShop/questShopAt/questFlagsFor -- against real itemsin.dat
// data. Does NOT exercise Shop.dialogue()/clearQuestTurnInState() at all
// -- see shop_state.h's own class comment for why those stay unwired for
// now (an ordinary port completeness gap, not a bug-preservation
// question -- see that comment's own correction of an earlier session's
// mistaken `Shop.reset()` finding).
#include <cstdio>

#include "assets/asset_root.h"
#include "assets/item_database.h"
#include "world/shop_state.h"

namespace {

bool Expect(bool cond, const char* what) {
    if (!cond) std::printf("  FAIL: %s\n", what);
    return cond;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        stormhold::AssetRoot assets(root);
        stormhold::ItemDatabase items = stormhold::ItemDatabase::Load(assets);

        bool ok = true;

        // Shop.isQuestShop: shops 0-3 (quest-turn-in), NOT 4/5 (Beneca/
        // Helga), NOT 6 (Varus/Warden).
        for (int i = 0; i < 4; i++) {
            ok &= Expect(stormhold::Shop::IsQuestShop(i), "shops 0-3 should be quest shops");
        }
        ok &= Expect(!stormhold::Shop::IsQuestShop(4), "shop 4 (Beneca) is not a quest shop");
        ok &= Expect(!stormhold::Shop::IsQuestShop(5), "shop 5 (Helga) is not a quest shop");
        ok &= Expect(!stormhold::Shop::IsQuestShop(6), "shop 6 (Varus) is not a quest shop");

        // Shop.questShopAt: position lookup gated on questRewardClaimable,
        // a field this milestone's ShopState default-constructs to TRUE
        // for all 7 -- matching what Shop.reset() itself really sets, via
        // Shop.java's own trailing `static { reset(); }` initializer (see
        // shop_state.h's own class comment) -- so a fresh ShopState finds
        // every real shop position immediately, with no setup needed.
        stormhold::ShopState state;
        ok &= Expect(stormhold::Shop::QuestShopAt(state, stormhold::Shop::kShopX[0], stormhold::Shop::kShopY[0]) ==
                         0,
                     "questShopAt should find shop 0 at its own real position -- a fresh ShopState starts with "
                     "questRewardClaimable true for all 7, matching the real Shop.reset()");
        ok &= Expect(stormhold::Shop::QuestShopAt(state, stormhold::Shop::kShopX[6], stormhold::Shop::kShopY[6]) ==
                         6,
                     "questShopAt should find Varus (shop 6) too -- the lookup itself isn't quest-shop-gated, "
                     "only flag-gated, matching the original exactly");
        ok &= Expect(stormhold::Shop::QuestShopAt(state, 0, 0) == -1,
                     "questShopAt should miss at a position no shop occupies");

        state.questRewardClaimable[0] = false;
        ok &= Expect(stormhold::Shop::QuestShopAt(state, stormhold::Shop::kShopX[0], stormhold::Shop::kShopY[0]) ==
                         -1,
                     "questShopAt should miss once shop 0's own flag is cleared, even at its real position");

        // Shop.questFlagsFor: 2-bit extraction per shop 0-3, real
        // itemsin.dat questFlags bytes. Exercise every real item rather
        // than guessing a specific id/value pair, since the exact bytes
        // aren't independently confirmed -- just the EXTRACTION shape
        // (each shop's own 2-bit field is always 0-3, and the four
        // fields' union reconstructs the whole byte via OR).
        for (int itemId = 1; itemId <= items.ItemCount(); itemId++) {
            int f0 = stormhold::Shop::QuestFlagsFor(0, itemId, items);
            int f1 = stormhold::Shop::QuestFlagsFor(1, itemId, items);
            int f2 = stormhold::Shop::QuestFlagsFor(2, itemId, items);
            int f3 = stormhold::Shop::QuestFlagsFor(3, itemId, items);
            ok &= Expect(f0 >= 0 && f0 <= 3 && f1 >= 0 && f1 <= 3 && f2 >= 0 && f2 <= 3 && f3 >= 0 && f3 <= 3,
                         "every shop's own extracted 2-bit field should be in 0..3");
            int reconstructed = (f0 << 6) | (f1 << 4) | (f2 << 2) | f3;
            int rawByte = static_cast<uint8_t>(items.questFlags[static_cast<size_t>(itemId - 1)]);
            ok &= Expect(reconstructed == rawByte,
                         "OR-ing shop 0-3's own 4 extracted fields back together should reconstruct the raw byte");
        }
        // shopId 4+ falls through to the original's own final `else`
        // branch (0), the same fallthrough this milestone's own
        // QuestFlagsFor reproduces rather than narrowing with a guard.
        ok &= Expect(stormhold::Shop::QuestFlagsFor(4, 1, items) == 0,
                     "questFlagsFor(shopId>=4, ...) should fall through to 0, matching the original's own final "
                     "else branch");

        if (!ok) {
            std::fprintf(stderr, "m53_shop_state_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m53_shop_state_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
