// M45 smoke test: Shop.java's own static state + the real dialogue()
// line-selection logic (npc/shop_interaction.h), plus InteractTick's own
// new npcInSight wiring (interact/interact_tick.h).
//
// No JVM ground truth is available (same reason as every prior milestone
// since M6/M9/M11/M13-M18 -- ESGame's stub jars throw the instant any of
// their surface actually executes). Every expected value is independently
// re-derived from ../../../src/Shop.java directly (buy/sell/quest-turn-in/
// rumor branch results, the RollShopOutcome chance formula, the
// RUMOR_STRING_OFFSET table, isValidShopAction/shopActionCode), not read
// back from shop_interaction.cpp -- including a twin JavaRandom, seeded
// identically to the one Dialogue() itself advances, used to independently
// predict every RNG-dependent branch (the shops 5-8 random greet line,
// RollShopOutcome's own hit-tier roll, and Jakar's rumor-pick roll) before
// calling the real function and checking the result matches.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/shop_dialogue.h"
#include "assets/spell_database.h"
#include "dungeon/dungeon_runtime.h"
#include "interact/interact_tick.h"
#include "npc/shop_interaction.h"
#include "player/player_combat_stats.h"
#include "player/player_creation.h"
#include "player/player_inventory.h"
#include "player/player_movement.h"
#include "player/player_state.h"
#include "render/message_popup.h"
#include "util/java_random.h"
#include "util/text.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::CharacterData;
using dawnstar::GeneratedLevel;
using dawnstar::InteractTick;
using dawnstar::ItemDatabase;
using dawnstar::JavaRandom;
using dawnstar::MessagePopupState;
using dawnstar::PlayerCombatStats;
using dawnstar::PlayerCreation;
using dawnstar::PlayerInventory;
using dawnstar::PlayerMovement;
using dawnstar::PlayerState;
using dawnstar::ShopDialogue;
using dawnstar::ShopInteraction;
using dawnstar::ShopState;
using dawnstar::WorldRegistry;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

// Independently re-derives Player.rollShopOutcome()'s own chance formula
// (Shop.java isn't the source here -- Player.java is) against a twin RNG,
// using PlayerCombatStats::SkillValue as an already-trusted building
// block (same precedent m41's own test set for reusing already-verified
// primitives rather than re-deriving them).
int ExpectedRollShopOutcome(const PlayerState& player, const CharacterData& charData, const ShopState& shop,
                            int shopId, int action, JavaRandom& twin) {
    int skill = PlayerCombatStats::SkillValue(player, charData, 13, true);
    if (action == 3) skill += 3;
    int threshold = shop.questState1[static_cast<size_t>(shopId - 5)];
    int diff = skill - threshold;
    int defChance = 20 - diff * 5;
    int atkChance = 20 + player.attributes[12] / 2 + diff * 5;
    defChance = std::min(std::max(defChance, 10), 95);
    atkChance = std::min(std::max(atkChance, 10), 95);
    return PlayerCombatStats::RollOutcome(atkChance, defChance, twin).outcome;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        ItemDatabase items = ItemDatabase::Load(archive);
        dawnstar::MonsterDatabase monsterDb = dawnstar::MonsterDatabase::Load(archive);
        dawnstar::SpellDatabase spells = dawnstar::SpellDatabase::Load(archive);
        CharacterData charData = CharacterData::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);
        ShopDialogue dialogue = ShopDialogue::Load(root + "/npcstrings.dat");

        std::vector<GeneratedLevel> levels;
        WorldRegistry world(geometry.rows.size());
        for (size_t i = 0; i < geometry.rows.size(); i++) {
            int levelNumber = static_cast<int>(i) + 1;
            levels.push_back(levelNumber == 1
                                  ? dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[0])
                                  : dawnstar::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[i], items,
                                                                              monsterDb));
            dawnstar::DungeonRuntime::RegisterGeneratedSpawns(levels.back(), world);
        }

        JavaRandom creationRng(20260915);
        PlayerState basePlayer = PlayerCreation::CreateCharacter(0, "Shopper", charData, items, creationRng);

        // --- A: ShopState::Reset() -- Shop.reset()'s own post-condition. ---
        {
            ShopState s = ShopState::Reset();
            bool allFirstVisit = true;
            for (bool v : s.firstVisit) allFirstVisit = allFirstVisit && v;
            Check(allFirstVisit, "Reset() should set all 9 firstVisit entries true");
            bool restZero = !s.showDeathGreeting;
            for (int8_t v : s.questState1) restZero = restZero && v == 0;
            for (int8_t v : s.questState2) restZero = restZero && v == 0;
            for (int16_t v : s.interactionCount) restZero = restZero && v == 0;
            for (int16_t v : s.rewardsGiven) restZero = restZero && v == 0;
            Check(restZero, "Reset() should zero every other field");
        }

        // --- B: static tables + small helpers. ---
        {
            Check(ShopInteraction::kNames[3] == "Jakar's", "kNames[3] should be Jakar's");
            Check(ShopInteraction::kNames[8] == "Delacroix", "kNames[8] should be Delacroix");
            for (int i = 0; i < 9; i++) {
                bool named = ShopInteraction::IsNamedShop(i);
                bool generic = ShopInteraction::IsGenericPeddler(i);
                Check(!(named && generic), "no shop should be both named and generic");
                Check(named == (i >= 5 && i <= 8), "IsNamedShop should be true for exactly shops 5-8");
                Check(generic == (i >= 0 && i <= 3), "IsGenericPeddler should be true for exactly shops 0-3");
            }
            Check(ShopInteraction::HubShopAt(12, 12) == 0, "hub position (12,12) should resolve to shop 0");
            Check(ShopInteraction::HubShopAt(12, 8) == 3, "hub position (12,8) should resolve to shop 3 (Jakar's)");
            Check(ShopInteraction::HubShopAt(0, 0) == -1, "an unoccupied position should resolve to -1");

            // isValidShopAction/shopActionCode -- transcribed independently
            // from Shop.java's own two parallel switches.
            Check(ShopInteraction::IsValidShopAction(5, 7) && ShopInteraction::IsValidShopAction(5, 8) &&
                      ShopInteraction::IsValidShopAction(5, 10) && !ShopInteraction::IsValidShopAction(5, 9),
                  "shop 5's valid actions should be exactly {7,8,10}");
            Check(ShopInteraction::ShopActionCode(5, 0) == 7 && ShopInteraction::ShopActionCode(5, 1) == 8 &&
                      ShopInteraction::ShopActionCode(5, 2) == 10 && ShopInteraction::ShopActionCode(5, 3) == -1,
                  "shop 5's choiceIndex->action mapping should be {7,8,10}, -1 beyond");
            Check(ShopInteraction::IsValidShopAction(8, 6) && ShopInteraction::IsValidShopAction(8, 12) &&
                      ShopInteraction::IsValidShopAction(8, 13) && !ShopInteraction::IsValidShopAction(8, 0),
                  "shop 8's valid actions should be exactly {6,12,13}");
            Check(!ShopInteraction::IsValidShopAction(4, 1), "shop 4 (Jakar's) has no valid quest actions at all");

            // QuestFlagsFor -- a synthetic questFlags byte (0b11_10_01_00 =
            // 0xE4) exercising all 4 shops' own 2-bit windows, independent
            // of what any real item happens to have.
            ItemDatabase synthetic = items;
            synthetic.questFlags[0] = static_cast<int8_t>(0xE4);
            Check(ShopInteraction::QuestFlagsFor(5, 1, synthetic) == 3, "shop 5 should read bits 6-7 (0b11)");
            Check(ShopInteraction::QuestFlagsFor(6, 1, synthetic) == 2, "shop 6 should read bits 4-5 (0b10)");
            Check(ShopInteraction::QuestFlagsFor(7, 1, synthetic) == 1, "shop 7 should read bits 2-3 (0b01)");
            Check(ShopInteraction::QuestFlagsFor(8, 1, synthetic) == 0, "shop 8 should read bits 0-1 (0b00)");
        }

        // --- C: shops 0-3, generic peddlers. ---
        {
            PlayerState p = basePlayer;
            ShopState shop = ShopState::Reset();
            JavaRandom rng(1);
            int16_t nextSpawnId = 1;

            auto line = ShopInteraction::Dialogue(p, shop, charData, items, dialogue, levels, 0, 1, 0, rng,
                                                   nextSpawnId);
            Check(line.has_value() && *line == dialogue.groups[0][0], "shop 0 greet should be its own dialogue[0][0]");

            // Buy: enough gold.
            int itemId = ShopInteraction::kStock[0][0];
            int price = items.buyPrice[static_cast<size_t>(itemId - 1)];
            p.gold = price + 50;
            int goldBefore = p.gold;
            int countBefore = p.inventoryCount;
            int16_t spawnBefore = nextSpawnId;
            auto bought = ShopInteraction::Dialogue(p, shop, charData, items, dialogue, levels, 0, 14, 0, rng,
                                                     nextSpawnId);
            Check(bought.has_value() && *bought == dialogue.groups[0][2], "a successful buy should return dialogue[0][2]");
            Check(p.gold == goldBefore - price, "a successful buy should deduct exactly the item's buyPrice");
            Check(p.inventoryCount == countBefore + 1, "a successful buy should add exactly one inventory slot");
            Check(nextSpawnId == spawnBefore + 1, "a successful buy should draw exactly one spawn id");

            // Buy: not enough gold -- no state change at all.
            p.gold = price - 1;
            goldBefore = p.gold;
            countBefore = p.inventoryCount;
            spawnBefore = nextSpawnId;
            auto tooExpensive = ShopInteraction::Dialogue(p, shop, charData, items, dialogue, levels, 0, 14, 0, rng,
                                                           nextSpawnId);
            Check(tooExpensive.has_value() && *tooExpensive == dialogue.groups[0][1],
                  "an unaffordable buy should return dialogue[0][1]");
            Check(p.gold == goldBefore, "an unaffordable buy should not touch gold");
            Check(p.inventoryCount == countBefore, "an unaffordable buy should not touch inventory");
            Check(nextSpawnId == spawnBefore, "an unaffordable buy should not draw a spawn id (never reached)");

            // Buy: enough gold, but a full pack -- the spawn-id-burned-on-
            // failure quirk (Item.nextSpawnId() drawn before the add is
            // even attempted).
            p.gold = price + 50;
            while (p.inventoryCount < 24) {
                PlayerInventory::AddItem(p, itemId, ++nextSpawnId, 0);
            }
            goldBefore = p.gold;
            spawnBefore = nextSpawnId;
            auto packFull = ShopInteraction::Dialogue(p, shop, charData, items, dialogue, levels, 0, 14, 0, rng,
                                                       nextSpawnId);
            Check(packFull.has_value() && *packFull == "Sorry, but your pack is too full.",
                  "a full-pack buy should return the pack-full message");
            Check(p.gold == goldBefore, "a full-pack buy should not deduct gold");
            Check(nextSpawnId == spawnBefore + 1,
                  "a full-pack buy should STILL draw a spawn id -- a real, preserved quirk");

            // Sell: a non-gift item.
            p = basePlayer;
            int nonGiftItemId = -1;
            for (int id = 1; id <= items.ItemCount(); id++) {
                if (items.category[static_cast<size_t>(id - 1)] != 11 &&
                    items.sellPrice[static_cast<size_t>(id - 1)] > 0) {
                    nonGiftItemId = id;
                    break;
                }
            }
            Check(nonGiftItemId > 0, "the real itemsin.dat should have at least one sellable non-gift item");
            p.inventoryItemIds[0] = static_cast<int8_t>(nonGiftItemId);
            if (p.inventoryCount == 0) p.inventoryCount = 1;
            int sellPrice = items.sellPrice[static_cast<size_t>(nonGiftItemId - 1)];
            int goldBeforeSell = p.gold;
            int countBeforeSell = p.inventoryCount;
            auto sold = ShopInteraction::Dialogue(p, shop, charData, items, dialogue, levels, 0, 15, 0, rng,
                                                   nextSpawnId);
            std::string expectedSellMsg = "For that you can have " + std::to_string(sellPrice) + " gold.";
            Check(sold.has_value() && *sold == expectedSellMsg, "selling should report the exact real sellPrice");
            Check(p.gold == goldBeforeSell + sellPrice, "selling should credit exactly sellPrice gold");
            Check(p.inventoryCount == countBeforeSell - 1, "selling should remove exactly one inventory slot");

            // Sell: a gift (category 11) item -- blocked, no state change.
            p = basePlayer;
            int giftItemId = -1;
            for (int id = 1; id <= items.ItemCount(); id++) {
                if (items.category[static_cast<size_t>(id - 1)] == 11) {
                    giftItemId = id;
                    break;
                }
            }
            Check(giftItemId > 0, "the real itemsin.dat should have at least one category-11 gift item");
            p.inventoryItemIds[0] = static_cast<int8_t>(giftItemId);
            if (p.inventoryCount == 0) p.inventoryCount = 1;
            int goldBeforeBlocked = p.gold;
            int countBeforeBlocked = p.inventoryCount;
            auto blocked = ShopInteraction::Dialogue(p, shop, charData, items, dialogue, levels, 0, 15, 0, rng,
                                                      nextSpawnId);
            Check(blocked.has_value() &&
                      blocked->find("may not sell a gift item") != std::string::npos,
                  "selling a gift item should be blocked with the exact real message");
            Check(p.gold == goldBeforeBlocked, "a blocked sell should not touch gold");
            Check(p.inventoryCount == countBeforeBlocked, "a blocked sell should not touch inventory");
        }

        // --- D: shops 5-8, named quest shopkeepers. ---
        {
            int shopId = 6;
            PlayerState p = basePlayer;
            ShopState shop = ShopState::Reset();

            // First visit: fixed line, firstVisit flips.
            JavaRandom rng(42);
            int16_t spawn1 = 1;
            auto first = ShopInteraction::Dialogue(p, shop, charData, items, dialogue, levels, shopId, 1, 0, rng,
                                                    spawn1);
            Check(first.has_value() && *first == dialogue.groups[shopId][0],
                  "a named shop's first visit should be its own dialogue[shopId][0]");
            Check(!shop.firstVisit[static_cast<size_t>(shopId)], "firstVisit should flip false after the first visit");

            // Second visit: a random line among 1-3, predicted by a twin RNG
            // seeded identically.
            JavaRandom liveRng(777);
            JavaRandom twinRng(777);
            int16_t spawn2 = 1;
            auto second = ShopInteraction::Dialogue(p, shop, charData, items, dialogue, levels, shopId, 1, 0, liveRng,
                                                     spawn2);
            int expectedLine = dawnstar::RandomIntBelow(twinRng, 3);
            Check(second.has_value() && *second == dialogue.groups[shopId][static_cast<size_t>(1 + expectedLine)],
                  "a subsequent visit's random line should match a twin-predicted ESGame.nextInt(3) roll");

            // action==2: quest ask, outcome roll predicted by a twin RNG.
            JavaRandom liveRng2(555);
            JavaRandom twinRng2(555);
            int16_t spawn3 = 1;
            int expectedOutcome = ExpectedRollShopOutcome(p, charData, shop, shopId, 2, twinRng2);
            int rewardsBefore = shop.rewardsGiven[static_cast<size_t>(shopId - 5)];
            int interactionsBefore = shop.interactionCount[static_cast<size_t>(shopId - 5)];
            auto askResult = ShopInteraction::Dialogue(p, shop, charData, items, dialogue, levels, shopId, 2, 0,
                                                        liveRng2, spawn3);
            Check(askResult.has_value() &&
                      *askResult == dialogue.groups[shopId][static_cast<size_t>(5 + expectedOutcome)],
                  "action 2's result line should match the twin-predicted RollShopOutcome");
            Check(shop.interactionCount[static_cast<size_t>(shopId - 5)] == interactionsBefore + 1,
                  "action 2 should always increment interactionCount");
            if (expectedOutcome >= 2) {
                Check(shop.rewardsGiven[static_cast<size_t>(shopId - 5)] == rewardsBefore + 1,
                      "outcome 2/3 should grant a reward");
                Check(shop.questState1[static_cast<size_t>(shopId - 5)] == 1,
                      "outcome 2/3 should set questState1 to 1");
            } else if (expectedOutcome == 0) {
                Check(shop.questState1[static_cast<size_t>(shopId - 5)] == 1, "outcome 0 should set questState1 to 1");
                Check(shop.rewardsGiven[static_cast<size_t>(shopId - 5)] == rewardsBefore,
                      "outcome 0 should NOT grant a reward");
            } else {
                Check(shop.questState1[static_cast<size_t>(shopId - 5)] == 0,
                      "outcome 1 should leave questState1 untouched (still 0)");
                Check(shop.rewardsGiven[static_cast<size_t>(shopId - 5)] == rewardsBefore,
                      "outcome 1 should NOT grant a reward");
            }

            // action==2 again: already turned in (questState1 != 0 from
            // outcome>=2's path, or unaffected otherwise) -- re-run enough
            // times from a fresh shop to exercise the "already done" branch
            // directly instead of relying on the roll above.
            ShopState doneShop = ShopState::Reset();
            doneShop.questState1[static_cast<size_t>(shopId - 5)] = 1;
            JavaRandom unusedRng(1);
            int16_t spawn4 = 1;
            auto already = ShopInteraction::Dialogue(p, doneShop, charData, items, dialogue, levels, shopId, 2, 0,
                                                      unusedRng, spawn4);
            Check(already.has_value() && *already == dialogue.groups[shopId][4],
                  "action 2 with questState1 already set should return dialogue[shopId][4] without rolling");

            // action==4: quest item turn-in, found by scanning the real
            // item table for a category-11 item with a nonzero quest flag
            // for this shop.
            int questItemId = -1;
            for (int id = 1; id <= items.ItemCount(); id++) {
                if (items.category[static_cast<size_t>(id - 1)] == 11 &&
                    ShopInteraction::QuestFlagsFor(shopId, id, items) > 0) {
                    questItemId = id;
                    break;
                }
            }
            if (questItemId > 0) {
                ShopState turnInShop = ShopState::Reset();
                PlayerState turnInPlayer = basePlayer;
                turnInPlayer.inventoryItemIds[0] = static_cast<int8_t>(questItemId);
                if (turnInPlayer.inventoryCount == 0) turnInPlayer.inventoryCount = 1;
                int flags = ShopInteraction::QuestFlagsFor(shopId, questItemId, items);
                int countBefore = turnInPlayer.inventoryCount;
                JavaRandom rng4(9);
                int16_t spawn5 = 1;
                auto turnedIn = ShopInteraction::Dialogue(turnInPlayer, turnInShop, charData, items, dialogue, levels,
                                                           shopId, 4, 0, rng4, spawn5);
                Check(turnedIn.has_value() &&
                          *turnedIn == dialogue.groups[shopId][static_cast<size_t>(11 + flags)],
                      "a real quest item's turn-in should return dialogue[shopId][11+flags]");
                Check(turnInPlayer.inventoryCount == countBefore - 1, "turning in a quest item should remove it");
                Check(turnInShop.rewardsGiven[static_cast<size_t>(shopId - 5)] == flags,
                      "turning in should grant exactly `flags` rewards");
            } else {
                std::printf("  (skipping action-4 quest-turn-in check -- no real item has a nonzero quest flag for shop %d)\n",
                            shopId);
            }

            // action==5: ask about a rumor -- gated on rewardsGiven, calls
            // RumorFor (checked in detail in section E's skill-storage
            // check below; here just the gate + consumption).
            ShopState rumorShop = ShopState::Reset();
            JavaRandom rng5(3);
            int16_t spawn6 = 1;
            auto noRewards = ShopInteraction::Dialogue(p, rumorShop, charData, items, dialogue, levels, shopId, 5, 0,
                                                        rng5, spawn6);
            Check(noRewards.has_value() && *noRewards == dialogue.groups[shopId][15],
                  "asking with 0 rewardsGiven should return dialogue[shopId][15]");
            rumorShop.rewardsGiven[static_cast<size_t>(shopId - 5)] = 2;
            PlayerState rumorPlayer = basePlayer;
            auto withRewards = ShopInteraction::Dialogue(rumorPlayer, rumorShop, charData, items, dialogue, levels,
                                                          shopId, 5, 0, rng5, spawn6);
            Check(withRewards.has_value(), "asking with rewardsGiven>0 should return a real rumor line");
            Check(rumorShop.rewardsGiven[static_cast<size_t>(shopId - 5)] == 1,
                  "asking should consume exactly one reward");

            // action==8: the documented switch-fallthrough case -- always
            // nullopt, no side effects.
            ShopState fallthroughShop = ShopState::Reset();
            PlayerState fallthroughPlayer = basePlayer;
            JavaRandom rng8(1);
            int16_t spawn8 = 1;
            auto fallthrough = ShopInteraction::Dialogue(fallthroughPlayer, fallthroughShop, charData, items,
                                                          dialogue, levels, shopId, 8, 0, rng8, spawn8);
            Check(!fallthrough.has_value(), "action 8 at a named shop should return nullopt (the fallthrough case)");
        }

        // --- E: shop 4, Jakar's. ---
        {
            PlayerState p = basePlayer;
            ShopState shop = ShopState::Reset();
            JavaRandom rng(1);
            int16_t spawn = 1;

            // First visit, no death greeting: 3-part concatenated intro.
            auto first = ShopInteraction::Dialogue(p, shop, charData, items, dialogue, levels, 4, 1, 0, rng, spawn);
            std::string expectedIntro =
                dialogue.groups[4][0] + "\n \n" + dialogue.groups[4][1] + "\n \n" + dialogue.groups[4][2];
            Check(first.has_value() && *first == expectedIntro,
                  "Jakar's first visit (no death greeting) should be the 3-part concatenated intro");
            Check(!shop.firstVisit[4], "firstVisit[4] should flip false");
            Check(p.rumorRevealStep == 0, "rumorRevealStep should reset to 0 on first visit");

            // First visit WITH a pending death greeting: a 4-part intro.
            ShopState deathShop = ShopState::Reset();
            deathShop.showDeathGreeting = true;
            PlayerState deathPlayer = basePlayer;
            auto firstWithDeath = ShopInteraction::Dialogue(deathPlayer, deathShop, charData, items, dialogue, levels,
                                                             4, 1, 0, rng, spawn);
            std::string expectedDeathIntro = dialogue.groups[4][13] + "\n \n" + expectedIntro;
            Check(firstWithDeath.has_value() && *firstWithDeath == expectedDeathIntro,
                  "a pending death greeting should prepend dialogue[4][13] to the intro");
            Check(!deathShop.showDeathGreeting, "showDeathGreeting should clear after being shown");

            // Subsequent visit: no advancement, no death greeting -> null.
            p.giftPointsFound = 0;  // advancement level 0
            p.rumorRevealStep = 0;
            auto subsequent =
                ShopInteraction::Dialogue(p, shop, charData, items, dialogue, levels, 4, 1, 0, rng, spawn);
            Check(!subsequent.has_value(), "a subsequent visit with no advancement/death-greeting should be null");

            // Subsequent visit: advancement bumps rumorRevealStep, still
            // null (no death greeting).
            p.giftPointsFound = 20;  // advancement level 1 (>= 17, < 29)
            p.rumorRevealStep = 0;
            auto advanced =
                ShopInteraction::Dialogue(p, shop, charData, items, dialogue, levels, 4, 1, 0, rng, spawn);
            Check(!advanced.has_value(), "advancement alone (no death greeting) should still be null");
            Check(p.rumorRevealStep == 1, "advancement level 1 should bump rumorRevealStep from 0 to 1");

            // Subsequent visit WITH a pending death greeting -> dialogue[4][13] alone.
            shop.showDeathGreeting = true;
            auto subsequentDeath =
                ShopInteraction::Dialogue(p, shop, charData, items, dialogue, levels, 4, 1, 0, rng, spawn);
            Check(subsequentDeath.has_value() && *subsequentDeath == dialogue.groups[4][13],
                  "a subsequent visit's death greeting should be dialogue[4][13] alone");

            // action==10: cure.
            PlayerState cureP = basePlayer;
            cureP.ailmentMask = 0x7F;
            auto cured = ShopInteraction::Dialogue(cureP, shop, charData, items, dialogue, levels, 4, 10, 0, rng, spawn);
            Check(cured.has_value() && *cured == dialogue.groups[4][9], "action 10 should return dialogue[4][9]");
            Check(cureP.ailmentMask == 0, "action 10 should clear ailmentMask entirely");

            // action==11: warp, no camp mark set -> the "no mark" line, no
            // position change.
            PlayerState warpP = basePlayer;
            warpP.campLevel = 0;
            int levelBefore = warpP.currentLevel;
            auto noMark = ShopInteraction::Dialogue(warpP, shop, charData, items, dialogue, levels, 4, 11, 0, rng, spawn);
            Check(noMark.has_value() && *noMark == dialogue.groups[4][10],
                  "action 11 with no camp mark should return dialogue[4][10]");
            Check(warpP.currentLevel == levelBefore, "action 11 with no camp mark should not move the player");

            // action==11: warp, WITH a camp mark -> real position change.
            warpP.campLevel = 2;
            warpP.campX = 5;
            warpP.campY = 5;
            warpP.campFacing = 1;
            auto warped = ShopInteraction::Dialogue(warpP, shop, charData, items, dialogue, levels, 4, 11, 0, rng, spawn);
            Check(warped.has_value() && *warped == dialogue.groups[4][11],
                  "action 11 with a camp mark should return dialogue[4][11]");
            Check(warpP.currentLevel == 2 && warpP.tileX == 5 && warpP.tileY == 5,
                  "action 11 with a camp mark should really warp to it");

            // action==12: heal HP/Magicka to max.
            PlayerState healP = basePlayer;
            healP.coreStats[2] = 1;
            healP.coreStats[4] = 1;
            auto healed = ShopInteraction::Dialogue(healP, shop, charData, items, dialogue, levels, 4, 12, 0, rng, spawn);
            Check(healed.has_value() && *healed == dialogue.groups[4][12], "action 12 should return dialogue[4][12]");
            Check(healP.coreStats[2] == healP.coreStats[3] && healP.coreStats[4] == healP.coreStats[5],
                  "action 12 should set current HP/Magicka to their own max");

            // action==13: rumor reveal -- eventFlags progression, the
            // shared-skills-storage quirk (via RumorFor at action 5 for
            // named shops, checked directly here too since action 13's own
            // pick roll writes eventFlags, not skills), and traitorIndex
            // substitution.
            PlayerState rumorP = basePlayer;
            rumorP.rumorRevealStep = 0;
            ShopState rumorShop = ShopState::Reset();
            JavaRandom liveRumorRng(2468);
            JavaRandom twinRumorRng(2468);
            int expectedPick = dawnstar::LingoRandomInt(twinRumorRng, 6) - 1;
            auto rumor =
                ShopInteraction::Dialogue(rumorP, rumorShop, charData, items, dialogue, levels, 4, 13, 0,
                                          liveRumorRng, spawn);
            Check(rumor.has_value(), "action 13 with revealedCount<=rumorRevealStep should return a real line");
            Check(rumorP.eventFlags[static_cast<size_t>(90 + expectedPick)],
                  "action 13 should set eventFlags[90+pick] for the twin-predicted pick");
            int offset =
                ShopInteraction::kRumorStringOffset[static_cast<size_t>(rumorP.traitorIndex)]
                                                    [static_cast<size_t>(expectedPick)];
            std::string expectedFragment = dialogue.groups[9][static_cast<size_t>(5 + offset)];
            Check(rumor->find(expectedFragment) != std::string::npos,
                  "the rumor line should embed the traitorIndex-selected fragment at the twin-predicted pick");

            // action==13 with no new rumors available (revealedCount >
            // rumorRevealStep).
            PlayerState noRumorsP = basePlayer;
            noRumorsP.rumorRevealStep = 0;
            for (int i = 0; i < 6; i++) noRumorsP.eventFlags[static_cast<size_t>(90 + i)] = true;
            ShopState noRumorsShop = ShopState::Reset();
            auto noRumors = ShopInteraction::Dialogue(noRumorsP, noRumorsShop, charData, items, dialogue, levels, 4,
                                                       13, 0, rng, spawn);
            Check(noRumors.has_value() && *noRumors == "I have no new rumors.",
                  "action 13 with everything already revealed beyond rumorRevealStep should say so");
        }

        // --- F: Player.skills[][0] / rumor-topic storage sharing (via
        // RumorFor, a named shop's action 5). ---
        {
            PlayerState p = basePlayer;
            int step = 2;
            p.skills[static_cast<size_t>(step)][0] = 0;  // fresh rank
            std::string firstAsk = ShopInteraction::RumorFor(p, charData, dialogue, step);
            Check(p.skills[static_cast<size_t>(step)][0] == 1,
                  "RumorFor's first ask should write 1 into skills[step][0] -- the SAME cell GainSkillExp uses");
            std::string expectedFirst =
                dawnstar::ReplaceFirstTag(dialogue.groups[9][1], "<TAG>", charData.skillNames[static_cast<size_t>(step)]);
            Check(firstAsk == expectedFirst, "the first ask should use dialogue[9][1] with the skill name substituted");

            std::string secondAsk = ShopInteraction::RumorFor(p, charData, dialogue, step);
            Check(p.skills[static_cast<size_t>(step)][0] == 2, "a second ask should increment the same cell to 2");
            std::vector<std::string> expectedValues = {charData.skillNames[static_cast<size_t>(step)], "1", "2"};
            std::string expectedSecond = dawnstar::ReplaceFirstTag(dialogue.groups[9][2], "<TAG>", expectedValues);
            Check(secondAsk == expectedSecond,
                  "a second ask should use dialogue[9][2] with 3 <TAG> substitutions in order");
        }

        // --- G: InteractTick::ProcessInteract wiring -- calling through
        // the real interact entry point should match calling Dialogue()
        // directly with the same inputs. ---
        {
            PlayerState p = basePlayer;
            p.npcInSight = 0;  // shop 0, a generic peddler
            ShopState shop = ShopState::Reset();
            MessagePopupState popup;
            JavaRandom rng(1);
            int16_t spawn = 1;
            auto viaInteract = InteractTick::ProcessInteract(p, levels, world, items, charData, dialogue, shop,
                                                              popup, rng, spawn, 0);
            Check(viaInteract.has_value() && *viaInteract == dialogue.groups[0][0],
                  "ProcessInteract's npcInSight branch should return the same greeting Dialogue() itself would");

            // No NPC and no chest in sight -- nullopt, no crash.
            PlayerState idleP = basePlayer;
            idleP.npcInSight = -1;
            idleP.chestInSight = false;
            ShopState idleShop = ShopState::Reset();
            MessagePopupState idlePopup;
            JavaRandom idleRng(1);
            int16_t idleSpawn = 1;
            auto nothing = InteractTick::ProcessInteract(idleP, levels, world, items, charData, dialogue, idleShop,
                                                          idlePopup, idleRng, idleSpawn, 0);
            Check(!nothing.has_value(), "ProcessInteract with nothing in sight should return nullopt");
        }

        if (g_ok) {
            std::printf("all shop-interaction checks passed\n");
            return 0;
        } else {
            std::printf("SOME CHECKS FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("EXCEPTION: %s\n", e.what());
        return 1;
    }
}
