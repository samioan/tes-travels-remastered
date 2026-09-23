// M56/M57 smoke test: ShopInteraction (player/shop_interaction.h) -- Shop.
// dialogue()'s dispatcher. M56: shops 0-3 (the quest-turn-in shopkeepers).
// M57: shop 4 (Beneca). Shops 5 (Helga)/6 (Varus) are each their own bespoke
// single-NPC branch, deliberately NOT covered here -- see shop_interaction
// .h's own class comment.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/shop_dialogue.h"
#include "player/player_combat_stats.h"
#include "player/player_creation.h"
#include "player/player_inventory.h"
#include "player/shop_interaction.h"
#include "world/dungeon_generator.h"

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

GeneratedLevel MakeHubLevel() {
    GeneratedLevel level;
    level.number = 1;
    level.width = 35;
    level.height = 35;
    level.tiles.assign(35, std::vector<uint8_t>(35, 0));
    level.populated = true;
    return level;
}

void TestIsValidShopActionAndShopActionCode() {
    std::printf("-- IsValidShopAction / ShopActionCode: shops 0-3 only --\n");
    // Shop 0: 3/4/13. Shop 1: 7/8/10. Shop 2: 1/6/12. Shop 3: 0/2/5.
    const int expected[4][3] = {{3, 4, 13}, {7, 8, 10}, {1, 6, 12}, {0, 2, 5}};
    for (int shopId = 0; shopId < 4; shopId++) {
        for (int choice = 0; choice < 3; choice++) {
            int code = expected[shopId][choice];
            Expect(ShopInteraction::IsValidShopAction(shopId, code), "expected action code should be valid");
            Expect(ShopInteraction::ShopActionCode(shopId, choice) == code,
                   "ShopActionCode should return the matching action code");
        }
        Expect(!ShopInteraction::IsValidShopAction(shopId, 99), "an unlisted action should be invalid");
    }
    for (int shopId = 4; shopId <= 6; shopId++) {
        Expect(!ShopInteraction::IsValidShopAction(shopId, 1), "shops 4-6 have no valid actions in this pair");
        Expect(ShopInteraction::ShopActionCode(shopId, 0) == -1, "shops 4-6 fall through to -1");
    }
}

void TestRumorFor(const CharacterData& charData, const ShopDialogue& text) {
    std::printf("-- RumorFor: first ask vs. repeat ask, piggybacked on skills[step][0] --\n");
    PlayerState p;
    int step = 2;
    Expect(p.skills[static_cast<size_t>(step)][0] == 0, "a fresh character's skill ask-counter starts at 0");

    std::string first = ShopInteraction::RumorFor(p, charData, text, step);
    Expect(p.skills[static_cast<size_t>(step)][0] == 1, "first ask should set the counter to 1");
    Expect(first.find(charData.skillNames[static_cast<size_t>(step)]) != std::string::npos,
           "first-ask template should have the skill name substituted for <TAG>");
    Expect(first.find("<TAG>") == std::string::npos, "no literal <TAG> should survive substitution");

    std::string second = ShopInteraction::RumorFor(p, charData, text, step);
    Expect(p.skills[static_cast<size_t>(step)][0] == 2, "second ask should bump the counter to 2");
    Expect(second.find(charData.skillNames[static_cast<size_t>(step)]) != std::string::npos,
           "repeat-ask template should also have the skill name substituted");
    Expect(second != first, "repeat-ask phrasing should differ from the first-ask phrasing");
}

void TestGreetBranches(const CharacterData& charData, const ShopDialogue& text, const ItemDatabase& items,
                        GeneratedLevel& hub) {
    std::printf("-- action 1 (greet): first-visit, cooldown, fatigued, and random-line branches --\n");
    int shopId = 1;
    const auto& lines = text.groups[static_cast<size_t>(shopId)];

    // First visit.
    {
        ShopState shop;
        PlayerState p;
        JavaRandom rng(1);
        auto result = ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, shopId, 1, 0);
        Expect(result.has_value() && *result == lines[0], "first visit should return the shop's own line 0");
        Expect(!shop.firstVisit[static_cast<size_t>(shopId)], "firstVisit should clear after the first greeting");
    }

    // Cooldown still high.
    {
        ShopState shop;
        shop.firstVisit[static_cast<size_t>(shopId)] = false;
        shop.unconfirmedCooldownH[static_cast<size_t>(shopId)] = 51;
        PlayerState p;
        JavaRandom rng(2);
        auto result = ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, shopId, 1, 0);
        Expect(result.has_value() && *result == lines[1], "cooldown > 50 should return line 1");
    }

    // Fatigued (coreStats[8] > 50).
    {
        ShopState shop;
        shop.firstVisit[static_cast<size_t>(shopId)] = false;
        PlayerState p;
        p.coreStats[8] = 51;
        JavaRandom rng(3);
        auto result = ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, shopId, 1, 0);
        Expect(result.has_value() && *result == lines[2], "coreStats[8] > 50 should return line 2");
    }

    // Otherwise: a lingoRandomInt(3)-selected line, cross-checked against a
    // twin JavaRandom seeded identically.
    for (int32_t seed : {4, 5, 6, 7, 8}) {
        ShopState shop;
        shop.firstVisit[static_cast<size_t>(shopId)] = false;
        PlayerState p;

        JavaRandom predict(seed);
        int expectedLine = RandomInt0Based(predict, 3);

        JavaRandom rng(seed);
        auto result = ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, shopId, 1, 0);
        Expect(result.has_value() && *result == lines[static_cast<size_t>(3 + expectedLine)],
               "the random greeting line should match lingoRandomInt(3)'s own twin-predicted pick");
    }
}

void TestQuestAsk1(const PlayerState& baseline, const CharacterData& charData, const ShopDialogue& text,
                    const ItemDatabase& items, GeneratedLevel& hub) {
    std::printf("-- action 2 (first quest ask): outcome-dependent state transitions --\n");
    int shopId = 2;
    const auto& lines = text.groups[static_cast<size_t>(shopId)];

    // Already turned in.
    {
        ShopState shop;
        shop.questState1[static_cast<size_t>(shopId)] = 1;
        PlayerState p = baseline;
        JavaRandom rng(1);
        auto result = ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, shopId, 2, 0);
        Expect(result.has_value() && *result == lines[6],
               "a shop with questState1 already set should short-circuit to line 6");
    }

    for (int32_t seed : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
        ShopState shop;
        PlayerState p = baseline;
        int16_t startExp = p.skills[13][2];

        JavaRandom predict(seed);
        int expectedOutcome = PlayerCombatStats::RollShopOutcome(p, charData, 2, shop.interactionCount[static_cast<size_t>(shopId)], predict);

        JavaRandom rng(seed);
        auto result = ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, shopId, 2, 0);

        Expect(result.has_value() && *result == lines[static_cast<size_t>(7 + expectedOutcome)],
               "the returned line should be lines[7 + the twin-predicted outcome]");
        Expect(shop.interactionCount[static_cast<size_t>(shopId)] == 1,
               "interactionCount should increment exactly once regardless of outcome");

        switch (expectedOutcome) {
            case 0:
                Expect(shop.questState1[static_cast<size_t>(shopId)] == 1, "outcome 0 sets questState1 = 1");
                Expect(p.skills[13][2] == startExp, "outcome 0 grants no rumor exp");
                break;
            case 1:
                Expect(shop.questState1[static_cast<size_t>(shopId)] == 0, "outcome 1 leaves questState1 untouched");
                Expect(p.skills[13][2] == startExp + 2, "outcome 1 grants +2 rumor exp");
                Expect(shop.rewardsGiven[static_cast<size_t>(shopId)] == 0, "outcome 1 grants no reward");
                break;
            case 2:
                Expect(shop.questState1[static_cast<size_t>(shopId)] == 1, "outcome 2 sets questState1 = 1");
                Expect(p.skills[13][2] == startExp + 5, "outcome 2 grants +5 rumor exp");
                Expect(shop.rewardsGiven[static_cast<size_t>(shopId)] == 1, "outcome 2 grants one reward");
                break;
            case 3:
                Expect(shop.questState1[static_cast<size_t>(shopId)] == 1, "outcome 3 sets questState1 = 1");
                Expect(p.skills[13][2] == startExp + 8, "outcome 3 grants +8 rumor exp");
                Expect(shop.rewardsGiven[static_cast<size_t>(shopId)] == 1, "outcome 3 grants one reward");
                break;
            default:
                Expect(false, "RollOutcome should only ever return 0-3");
        }
    }
}

void TestQuestAsk2(const PlayerState& baseline, const CharacterData& charData, const ShopDialogue& text,
                    const ItemDatabase& items, GeneratedLevel& hub) {
    std::printf("-- action 3 (second quest ask): variant selection + outcome-dependent transitions --\n");
    int shopId = 3;
    const auto& lines = text.groups[static_cast<size_t>(shopId)];

    {
        ShopState shop;
        shop.questState2[static_cast<size_t>(shopId)] = 2;
        PlayerState p = baseline;
        JavaRandom rng(1);
        auto result = ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, shopId, 3, 0);
        Expect(result.has_value() && *result == lines[6],
               "a shop with questState2 already set should short-circuit to line 6");
    }

    for (int extra : {0, 1, 2}) {
        int variant = extra <= 1 ? 0 : 1;
        int32_t seed = 100 + extra;
        ShopState shop;
        PlayerState p = baseline;

        JavaRandom predict(seed);
        int expectedOutcome = PlayerCombatStats::RollShopOutcome(p, charData, 3, shop.interactionCount[static_cast<size_t>(shopId)], predict);

        JavaRandom rng(seed);
        auto result = ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, shopId, 3, extra);

        Expect(result.has_value() && *result == lines[static_cast<size_t>(11 + variant)],
               "the returned line should reflect extra<=1's own variant selection");

        int8_t expectedState2 = (expectedOutcome == 3) ? 1 : (expectedOutcome == 2) ? 1 : 2;
        Expect(shop.questState2[static_cast<size_t>(shopId)] == expectedState2,
               "questState2 should land on the outcome-specific value (1 for outcome 2/3, 2 otherwise)");
    }
}

void TestQuestItemTurnIn(const PlayerState& baseline, const CharacterData& charData, const ShopDialogue& text,
                          const ItemDatabase& items, GeneratedLevel& hub) {
    std::printf("-- action 4: category 15 (cooldown reducer) and category 11 (quest item) turn-ins --\n");

    // Category 15: reduces unconfirmedCooldownH by the item's own raw
    // questFlags byte, clamped at 0.
    int cooldownItemId = -1;
    for (int id = 1; id <= items.ItemCount(); id++) {
        if (items.category[static_cast<size_t>(id - 1)] == 15) {
            cooldownItemId = id;
            break;
        }
    }
    if (cooldownItemId < 0) {
        std::printf("  (no real category-15 item found in itemsin.dat -- skipping this sub-case)\n");
    } else {
        int shopId = 0;
        const auto& lines = text.groups[static_cast<size_t>(shopId)];
        ShopState shop;
        shop.unconfirmedCooldownH[static_cast<size_t>(shopId)] = 5;
        PlayerState p = baseline;
        bool added = PlayerInventory::AddInventoryItemRaw(p, cooldownItemId, 0, 0);
        Expect(added, "adding the category-15 item to a fresh character's inventory should succeed");
        int slot = p.inventoryCount - 1;

        JavaRandom rng(1);
        auto result = ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, shopId, 4, slot);
        Expect(result.has_value() && *result == lines[17], "a category-15 turn-in should return line 17");
        Expect(p.inventoryItemIds[static_cast<size_t>(slot)] != cooldownItemId,
               "the category-15 item should be removed from that slot");

        int reduceBy = items.questFlags[static_cast<size_t>(cooldownItemId - 1)];
        int16_t expectedCooldown = static_cast<int16_t>(std::max(5 - reduceBy, 0));
        Expect(shop.unconfirmedCooldownH[static_cast<size_t>(shopId)] == expectedCooldown,
               "unconfirmedCooldownH should drop by the item's raw questFlags byte, clamped at 0");
    }

    // Category 11: the real quest-item delivery, gated by QuestFlagsFor.
    int giftItemId = -1, giftShopId = -1, giftFlags = 0;
    for (int id = 1; id <= items.ItemCount() && giftItemId < 0; id++) {
        if (items.category[static_cast<size_t>(id - 1)] != 11) continue;
        for (int s = 0; s < 4; s++) {
            int f = Shop::QuestFlagsFor(s, id, items);
            if (f > 0) {
                giftItemId = id;
                giftShopId = s;
                giftFlags = f;
                break;
            }
        }
    }
    if (giftItemId < 0) {
        std::printf("  (no real category-11 item with a nonzero QuestFlagsFor found -- skipping this sub-case)\n");
    } else {
        const auto& lines = text.groups[static_cast<size_t>(giftShopId)];
        ShopState shop;
        shop.questState1[static_cast<size_t>(giftShopId)] = 1;
        PlayerState p = baseline;
        bool added = PlayerInventory::AddInventoryItemRaw(p, giftItemId, 0, 0);
        Expect(added, "adding the category-11 gift item should succeed");
        int slot = p.inventoryCount - 1;

        JavaRandom rng(1);
        auto result =
            ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, giftShopId, 4, slot);
        Expect(result.has_value() && *result == lines[static_cast<size_t>(13 + giftFlags)],
               "a successful gift turn-in should return lines[13 + flags]");
        Expect(p.inventoryItemIds[static_cast<size_t>(slot)] != giftItemId, "the gift item should be removed");
        Expect(shop.rewardsGiven[static_cast<size_t>(giftShopId)] == giftFlags,
               "rewardsGiven should increase by the item's own extracted flags");
        Expect(shop.questState1[static_cast<size_t>(giftShopId)] == 0, "questState1 should reset on a gift turn-in");
        Expect(shop.questState2[static_cast<size_t>(giftShopId)] == 0, "questState2 should reset on a gift turn-in");
    }

    // Both quest states already complete (2/2): action 4 always returns
    // line 13 regardless of what's in the slot.
    {
        int shopId = 0;
        const auto& lines = text.groups[static_cast<size_t>(shopId)];
        ShopState shop;
        shop.questState1[static_cast<size_t>(shopId)] = 2;
        shop.questState2[static_cast<size_t>(shopId)] = 2;
        PlayerState p = baseline;
        JavaRandom rng(1);
        auto result = ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, shopId, 4, 0);
        Expect(result.has_value() && *result == lines[13],
               "both quest states at 2 should short-circuit action 4 to line 13");
    }
}

void TestRumorRequest(const PlayerState& baseline, const CharacterData& charData, const ShopDialogue& text,
                       const ItemDatabase& items, GeneratedLevel& hub) {
    std::printf("-- action 5: reward-gated rumor request --\n");
    int shopId = 1;
    const auto& lines = text.groups[static_cast<size_t>(shopId)];

    {
        ShopState shop;
        PlayerState p = baseline;
        JavaRandom rng(1);
        auto result = ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, shopId, 5, 0);
        Expect(result.has_value() && *result == lines[18], "no rewards given yet should return line 18");
    }
    {
        ShopState shop;
        shop.rewardsGiven[static_cast<size_t>(shopId)] = 1;
        shop.unconfirmedCooldownH[static_cast<size_t>(shopId)] = 51;
        PlayerState p = baseline;
        JavaRandom rng(1);
        auto result = ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, shopId, 5, 0);
        Expect(result.has_value() && *result == lines[1], "cooldown > 50 should gate the rumor request too");
    }
    {
        ShopState shop;
        shop.rewardsGiven[static_cast<size_t>(shopId)] = 1;
        PlayerState p = baseline;
        int step = 4;
        JavaRandom rng(1);
        auto result = ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, shopId, 5, step);
        Expect(shop.rewardsGiven[static_cast<size_t>(shopId)] == 0, "a successful rumor request spends one reward");
        Expect(result.has_value() && result->find(charData.skillNames[static_cast<size_t>(step)]) != std::string::npos,
               "the rumor response should name the requested skill");
        Expect(p.skills[static_cast<size_t>(step)][0] == 1,
               "the rumor request should drive RumorFor's own skills[step][0] ask counter");
    }
}

void TestRewardClaim(const PlayerState& baseline, const CharacterData& charData, const ShopDialogue& text,
                      const ItemDatabase& items) {
    std::printf("-- action 6: reward claim clears questRewardClaimable and the hub tile's bit 32 --\n");
    int shopId = 2;
    const auto& lines = text.groups[static_cast<size_t>(shopId)];
    GeneratedLevel hub = MakeHubLevel();
    int x = Shop::kShopX[shopId];
    int y = Shop::kShopY[shopId];
    hub.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] = 32;

    ShopState shop;
    PlayerState p = baseline;
    JavaRandom rng(1);
    auto result = ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, shopId, 6, 0);

    Expect(result.has_value() && *result == lines[19], "action 6 should return line 19");
    Expect(!shop.questRewardClaimable[static_cast<size_t>(shopId)], "questRewardClaimable should clear");
    Expect((hub.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] & 32) == 0,
           "the hub tile's bit 32 should clear at this shop's own world position");
}

void TestUnhandledAction(const PlayerState& baseline, const CharacterData& charData, const ShopDialogue& text,
                          const ItemDatabase& items, GeneratedLevel& hub) {
    std::printf("-- an action with no matching branch should return std::nullopt --\n");
    ShopState shop;
    PlayerState p = baseline;
    JavaRandom rng(1);
    auto result = ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, 0, 42, 0);
    Expect(!result.has_value(), "an unrecognized action should return std::nullopt, matching the original's null");
}

// M57: shop 4 (Beneca).
void TestBenecaDialogue(const PlayerState& baseline, const ItemDatabase& items, const ShopDialogue& text) {
    std::printf("-- Beneca (shop 4): greet, donation, and training-reward branches --\n");
    const auto& lines = text.groups[4];

    // Greet: first visit, then nullopt on a repeat.
    {
        ShopState shop;
        PlayerState p = baseline;
        int16_t spawnCounter = 10000;
        auto first = ShopInteraction::BenecaDialogue(p, shop, text, items, spawnCounter, 1, 0);
        Expect(first.has_value() && *first == lines[0], "Beneca's first visit should return line 0");
        Expect(!shop.firstVisit[4], "firstVisit should clear after the first greeting");
        auto second = ShopInteraction::BenecaDialogue(p, shop, text, items, spawnCounter, 1, 0);
        Expect(!second.has_value(), "a repeat greeting should return std::nullopt");
    }

    // Donation: an eligible item (category not 13/15/17) earns a point and
    // is consumed; an ineligible one is refused and kept.
    int eligibleItemId = -1, ineligibleItemId = -1;
    for (int id = 1; id <= items.ItemCount(); id++) {
        int cat = items.category[static_cast<size_t>(id - 1)];
        if (cat != 13 && cat != 15 && cat != 17 && eligibleItemId < 0) eligibleItemId = id;
        if ((cat == 13 || cat == 15 || cat == 17) && ineligibleItemId < 0) ineligibleItemId = id;
    }
    Expect(eligibleItemId > 0, "itemsin.dat should have at least one donation-eligible item");
    {
        ShopState shop;
        PlayerState p = baseline;
        bool added = PlayerInventory::AddInventoryItemRaw(p, eligibleItemId, 0, 0);
        Expect(added, "adding the eligible item should succeed");
        int slot = p.inventoryCount - 1;
        int16_t spawnCounter = 10000;
        auto result = ShopInteraction::BenecaDialogue(p, shop, text, items, spawnCounter, 4, slot);
        Expect(result.has_value() && *result == lines[2], "an eligible donation should return line 2");
        Expect(shop.benecaPoints == 1, "an eligible donation should award exactly one point");
        Expect(p.inventoryItemIds[static_cast<size_t>(slot)] != eligibleItemId,
               "the donated item should be removed from inventory");
    }
    if (ineligibleItemId > 0) {
        ShopState shop;
        PlayerState p = baseline;
        bool added = PlayerInventory::AddInventoryItemRaw(p, ineligibleItemId, 0, 0);
        Expect(added, "adding the ineligible item should succeed");
        int slot = p.inventoryCount - 1;
        int16_t spawnCounter = 10000;
        auto result = ShopInteraction::BenecaDialogue(p, shop, text, items, spawnCounter, 4, slot);
        Expect(result.has_value() && *result == lines[1], "an ineligible donation (category 13/15/17) should return line 1");
        Expect(shop.benecaPoints == 0, "an ineligible donation should award no point");
        Expect(p.inventoryItemIds[static_cast<size_t>(slot)] == ineligibleItemId,
               "an ineligible item should NOT be removed from inventory");
    } else {
        std::printf("  (no real category 13/15/17 item found -- skipping the ineligible-donation sub-case)\n");
    }

    // Training reward: not enough points yet.
    {
        ShopState shop;
        shop.benecaPoints = 2;
        PlayerState p = baseline;
        int16_t spawnCounter = 10000;
        auto result = ShopInteraction::BenecaDialogue(p, shop, text, items, spawnCounter, 7, eligibleItemId);
        Expect(result.has_value() && *result == lines[4], "fewer than 3 points should return line 4");
        Expect(shop.benecaPoints == 2, "an insufficient-points request should not spend any points");
    }

    // Training reward: enough points, room in the pack -- succeeds, spends
    // exactly 3 points, and burns exactly one spawn id.
    {
        ShopState shop;
        shop.benecaPoints = 5;
        PlayerState p = baseline;
        int16_t spawnCounter = 10000;
        auto result = ShopInteraction::BenecaDialogue(p, shop, text, items, spawnCounter, 7, eligibleItemId);
        Expect(result.has_value() && *result == lines[3], "a successful training reward should return line 3");
        Expect(shop.benecaPoints == 2, "a successful training reward should spend exactly 3 points");
        Expect(spawnCounter == 10001, "a successful training reward should burn exactly one spawn id");
        bool found = false;
        for (int i = 0; i < p.inventoryCount; i++) {
            if (std::abs(static_cast<int>(p.inventoryItemIds[static_cast<size_t>(i)])) == eligibleItemId) found = true;
        }
        Expect(found, "the trained item should actually land in the player's inventory");
    }

    // Training reward: enough points, but a full pack -- the cross-group
    // failure message (dialogue[7][0], not dialogue[4][...]), and the
    // spawn id is still burned even though the item wasn't granted (the
    // same "counter advances before the outcome is known" shape M43's own
    // GrantStarFrostItem precedent already documented for this project).
    {
        ShopState shop;
        shop.benecaPoints = 5;
        PlayerState p = baseline;
        p.inventoryCount = 24;
        int16_t spawnCounter = 10000;
        auto result = ShopInteraction::BenecaDialogue(p, shop, text, items, spawnCounter, 7, eligibleItemId);
        Expect(result.has_value() && *result == text.groups[7][0],
               "a full-pack training reward should return the GENERIC group's line 0, not Beneca's own group");
        Expect(shop.benecaPoints == 5, "a failed training reward should not spend any points");
        Expect(spawnCounter == 10001, "a failed training reward still burns the spawn id, matching the original");
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assets(root);
        ItemDatabase items = ItemDatabase::Load(assets);
        CharacterData charData = CharacterData::Load(assets);
        ShopDialogue text = ShopDialogue::Load(assets);
        GeneratedLevel hub = MakeHubLevel();

        PlayerState baseline = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);

        TestIsValidShopActionAndShopActionCode();
        TestRumorFor(charData, text);
        TestGreetBranches(charData, text, items, hub);
        TestQuestAsk1(baseline, charData, text, items, hub);
        TestQuestAsk2(baseline, charData, text, items, hub);
        TestQuestItemTurnIn(baseline, charData, text, items, hub);
        TestRumorRequest(baseline, charData, text, items, hub);
        TestRewardClaim(baseline, charData, text, items);
        TestUnhandledAction(baseline, charData, text, items, hub);
        TestBenecaDialogue(baseline, items, text);

        if (!g_ok) {
            std::fprintf(stderr, "m56_shop_interaction_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m56_shop_interaction_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
