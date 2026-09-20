#include "npc/shop_interaction.h"

#include <algorithm>
#include <cstdlib>

#include "player/player_combat_stats.h"
#include "player/player_inventory.h"
#include "player/player_movement.h"
#include "util/game_advancement.h"
#include "util/text.h"

namespace dawnstar {

const std::array<std::string, ShopInteraction::kShopCount> ShopInteraction::kNames = {
    "Weapon Peddler", "Heavy Armor Peddler", "Light Armor Peddler", "Jakar's",
    "Eustacia",       "Alhavara",            "Beatrice",            "Chung",
    "Delacroix",
};

const std::array<int8_t, ShopInteraction::kShopCount> ShopInteraction::kCategory = {4, 4, 4, 4, 2, 1, 1, 1, 1};
const std::array<int8_t, ShopInteraction::kShopCount> ShopInteraction::kShopX = {12, 6, 7, 12, 12, 1, 1, 1, 1};
const std::array<int8_t, ShopInteraction::kShopCount> ShopInteraction::kShopY = {12, 11, 7, 8, 6, 1, 1, 1, 1};

const std::array<std::vector<int8_t>, 4> ShopInteraction::kStock = {
    std::vector<int8_t>{1, 2, 3, 4, 5, 7, 8, 9, 10, 12, 13, 14, 15, 17, 18, 19, 20},
    std::vector<int8_t>{22, 23, 24, 25, 32, 33, 34, 35, 47, 48, 49, 50},
    std::vector<int8_t>{27, 28, 29, 30, 37, 38, 39, 40, 42, 43, 44, 45},
    std::vector<int8_t>{87, 88, 89, 90, 91, 93, 94, 95, 96},
};

const std::array<std::array<int8_t, 6>, 4> ShopInteraction::kRumorStringOffset = {{
    {1, 3, 5, 8, 10, 12},
    {1, 2, 4, 7, 9, 12},
    {2, 3, 6, 7, 10, 11},
    {2, 4, 5, 8, 9, 11},
}};

const std::array<int8_t, 24> ShopInteraction::kUnconfirmedA = {13, 19, 25, 31, 14, 20, 26, 32, 15, 21, 27, 33,
                                                                17, 23, 29, 35, 16, 22, 28, 34, 18, 24, 30, 36};
const std::array<int8_t, 24> ShopInteraction::kUnconfirmedB = {37, 43, 49, 55, 38, 44, 50, 56, 39, 45, 51, 57,
                                                                41, 47, 53, 59, 40, 46, 52, 58, 42, 48, 54, 60};

ShopState ShopState::Reset() {
    ShopState s;
    s.firstVisit.fill(true);
    s.questState1.fill(0);
    s.questState2.fill(0);
    s.interactionCount.fill(0);
    s.rewardsGiven.fill(0);
    s.showDeathGreeting = false;
    return s;
}

int ShopInteraction::HubShopAt(int x, int y) {
    for (int i = 0; i < 5; i++) {
        if (x == kShopX[static_cast<size_t>(i)] && y == kShopY[static_cast<size_t>(i)]) return i;
    }
    return -1;
}

int ShopInteraction::QuestFlagsFor(int shopId, int itemId, const ItemDatabase& items) {
    uint32_t flags = static_cast<uint8_t>(items.questFlags[static_cast<size_t>(itemId - 1)]);
    switch (shopId) {
        case 5:
            return static_cast<int>((flags >> 6) & 3);
        case 6:
            return static_cast<int>((flags >> 4) & 3);
        case 7:
            return static_cast<int>((flags >> 2) & 3);
        case 8:
            return static_cast<int>(flags & 3);
        default:
            return 0;
    }
}

void ShopInteraction::ClearQuestTurnInState(ShopState& s) {
    s.questState1.fill(0);
    s.questState2.fill(0);
}

std::string ShopInteraction::RumorFor(PlayerState& player, const CharacterData& charData, const ShopDialogue& dialogue,
                                       int step) {
    int16_t revealed = player.skills[static_cast<size_t>(step)][0];
    if (revealed == 0) {
        player.skills[static_cast<size_t>(step)][0] = 1;
        return ReplaceFirstTag(dialogue.groups[9][1], "<TAG>", charData.skillNames[static_cast<size_t>(step)]);
    }

    player.skills[static_cast<size_t>(step)][0] = static_cast<int16_t>(revealed + 1);
    std::vector<std::string> values = {charData.skillNames[static_cast<size_t>(step)], std::to_string(revealed),
                                        std::to_string(revealed + 1)};
    return ReplaceFirstTag(dialogue.groups[9][2], "<TAG>", values);
}

int ShopInteraction::RollShopOutcome(const PlayerState& player, const CharacterData& charData, const ShopState& shop,
                                      int shopId, int action, JavaRandom& globalRng) {
    int skill = PlayerCombatStats::SkillValue(player, charData, 13, true);
    if (action == 3) skill += 3;

    int threshold = shop.questState1[static_cast<size_t>(shopId - 5)];
    int diff = skill - threshold;
    int defChance = 20 - diff * 5;
    int atkChance = 20 + player.attributes[12] / 2 + diff * 5;
    defChance = std::min(std::max(defChance, 10), 95);
    atkChance = std::min(std::max(atkChance, 10), 95);
    return PlayerCombatStats::RollOutcome(atkChance, defChance, globalRng).outcome;
}

std::optional<std::string> ShopInteraction::Dialogue(PlayerState& player, ShopState& shop,
                                                       const CharacterData& charData, const ItemDatabase& items,
                                                       const ShopDialogue& dialogue,
                                                       const std::vector<GeneratedLevel>& levels, int shopId,
                                                       int action, int extra, JavaRandom& globalRng,
                                                       int16_t& nextItemSpawnId) {
    // --- shops 0-3: generic peddlers (buy/sell) ---
    if (shopId >= 0 && shopId <= 3) {
        if (action == 1) {
            return dialogue.groups[static_cast<size_t>(shopId)][0];
        }

        if (action == 14) {
            int catalogSlot = extra;
            int itemId = kStock[static_cast<size_t>(shopId)][static_cast<size_t>(catalogSlot)];
            int price = items.buyPrice[static_cast<size_t>(itemId - 1)];
            if (price > player.gold) {
                return dialogue.groups[static_cast<size_t>(shopId)][1];
            }

            // Item.nextSpawnId()'s own ++counter-then-use convention (M43's
            // GrantStarFrostItem precedent) -- drawn, and the counter
            // advanced, unconditionally, BEFORE the add is even attempted.
            int16_t spawnId = ++nextItemSpawnId;
            bool added = PlayerInventory::AddItem(player, itemId, spawnId, 0);
            if (added) {
                PlayerInventory::AddGold(player, -price);
                return dialogue.groups[static_cast<size_t>(shopId)][2];
            }

            return std::string("Sorry, but your pack is too full.");
        }

        if (action == 15) {
            int slot = extra;
            int itemId = std::abs(static_cast<int>(player.inventoryItemIds[static_cast<size_t>(slot)]));
            if (items.category[static_cast<size_t>(itemId - 1)] == 11) {
                return std::string(
                    "Sorry, you may not sell a gift item.  It should be given to one of the champions.");
            }

            int saleValue = items.sellPrice[static_cast<size_t>(itemId - 1)];
            PlayerInventory::AddGold(player, saleValue);
            PlayerInventory::RemoveSlot(player, items, slot);
            return "For that you can have " + std::to_string(saleValue) + " gold.";
        }

        return std::string("quack");
    }

    // --- shops 5-8: named quest shopkeepers ---
    if (shopId >= 5 && shopId <= 8) {
        size_t idx = static_cast<size_t>(shopId - 5);

        if (action == 1) {
            if (shop.firstVisit[static_cast<size_t>(shopId)]) {
                shop.firstVisit[static_cast<size_t>(shopId)] = false;
                return dialogue.groups[static_cast<size_t>(shopId)][0];
            }

            int line = RandomIntBelow(globalRng, 3);
            return dialogue.groups[static_cast<size_t>(shopId)][1 + line];
        }

        if (action == 2) {
            if (shop.questState1[idx] != 0) return dialogue.groups[static_cast<size_t>(shopId)][4];

            int outcome = RollShopOutcome(player, charData, shop, shopId, action, globalRng);
            if (outcome == 0) {
                shop.questState1[idx] = 1;
            } else if (outcome == 1) {
                PlayerCombatStats::GainSkillExp(player, charData, 13, 2);
            } else if (outcome == 2) {
                PlayerCombatStats::GainSkillExp(player, charData, 13, 5);
                shop.rewardsGiven[idx]++;
                shop.questState1[idx] = 1;
            } else if (outcome == 3) {
                PlayerCombatStats::GainSkillExp(player, charData, 13, 8);
                shop.rewardsGiven[idx]++;
                shop.questState1[idx] = 1;
            }

            shop.interactionCount[idx]++;
            return dialogue.groups[static_cast<size_t>(shopId)][static_cast<size_t>(5 + outcome)];
        }

        if (action == 3) {
            if (shop.questState2[idx] != 0) return dialogue.groups[static_cast<size_t>(shopId)][4];

            int outcome = RollShopOutcome(player, charData, shop, shopId, action, globalRng);
            int variant = extra <= 1 ? 0 : 1;
            if (outcome == 0) {
                shop.questState2[idx] = 2;
            } else if (outcome == 1) {
                PlayerCombatStats::GainSkillExp(player, charData, 13, 2);
                shop.questState2[idx] = 2;
            } else if (outcome == 2) {
                PlayerCombatStats::GainSkillExp(player, charData, 13, 5);
                shop.rewardsGiven[idx]++;
                shop.questState2[idx] = 1;
            } else if (outcome == 3) {
                PlayerCombatStats::GainSkillExp(player, charData, 13, 8);
                shop.rewardsGiven[idx]++;
                shop.questState2[idx] = 1;
            }

            shop.interactionCount[idx]++;
            return dialogue.groups[static_cast<size_t>(shopId)][static_cast<size_t>(9 + variant)];
        }

        if (action == 4) {
            if (shop.questState1[idx] != 2 && shop.questState2[idx] != 2) {
                int slot = extra;
                int itemId = std::abs(static_cast<int>(player.inventoryItemIds[static_cast<size_t>(slot)]));
                if (items.category[static_cast<size_t>(itemId - 1)] == 11) {
                    int flags = QuestFlagsFor(shopId, itemId, items);
                    if (flags > 0) {
                        PlayerInventory::RemoveSlot(player, items, slot);
                        shop.rewardsGiven[idx] = static_cast<int16_t>(shop.rewardsGiven[idx] + flags);
                        shop.questState1[idx] = 0;
                        shop.questState2[idx] = 0;
                    }

                    return dialogue.groups[static_cast<size_t>(shopId)][static_cast<size_t>(11 + flags)];
                }

                return dialogue.groups[static_cast<size_t>(shopId)][11];
            }

            return dialogue.groups[static_cast<size_t>(shopId)][11];
        }

        if (action == 5) {
            if (shop.rewardsGiven[idx] == 0) return dialogue.groups[static_cast<size_t>(shopId)][15];

            shop.rewardsGiven[idx]--;
            int step = extra;
            return RumorFor(player, charData, dialogue, step);
        }

        if (action == 8) {
            // Shop.java's own switch has no `break` here: shopId 5-8 with
            // action==8 falls straight into shop 4's own if/else chain
            // below. Every one of that chain's branches for action==8
            // (not 1/10/11/12/13) bottoms out at its own `return null`
            // with no side effects, so this is provably equivalent to
            // returning nullopt directly -- see this method's own .h
            // doc comment.
            return std::nullopt;
        }

        return std::string("quack");
    }

    // --- shop 4: Jakar's (rumors, traitor advancement, camp/cure/heal) ---
    if (shopId == 4) {
        if (action == 1) {
            if (shop.firstVisit[4]) {
                shop.firstVisit[4] = false;
                player.rumorRevealStep = 0;
                if (shop.showDeathGreeting) {
                    shop.showDeathGreeting = false;
                    return dialogue.groups[4][13] + "\n \n" + dialogue.groups[4][0] + "\n \n" + dialogue.groups[4][1] +
                           "\n \n" + dialogue.groups[4][2];
                }

                return dialogue.groups[4][0] + "\n \n" + dialogue.groups[4][1] + "\n \n" + dialogue.groups[4][2];
            }

            int advancement = GetGameAdvancementLevel(player.giftPointsFound);
            if (advancement > player.rumorRevealStep) player.rumorRevealStep++;

            if (shop.showDeathGreeting) {
                shop.showDeathGreeting = false;
                return dialogue.groups[4][13];
            }

            return std::nullopt;
        }

        if (action == 13) {
            int revealedCount = 0;
            for (int step = 0; step < 6; step++) {
                if (player.eventFlags[static_cast<size_t>(90 + step)]) revealedCount++;
            }

            if (revealedCount > player.rumorRevealStep) return std::string("I have no new rumors.");

            std::string result = dialogue.groups[4][static_cast<size_t>(3 + player.rumorRevealStep)];
            int pick = 0;
            if (revealedCount < 6) pick = LingoRandomInt(globalRng, 6 - revealedCount) - 1;

            for (int i = 0; i < 6; i++) {
                if (!player.eventFlags[static_cast<size_t>(90 + i)]) {
                    if (pick == 0) {
                        pick = i;
                        break;
                    }
                    pick--;
                }
            }

            player.eventFlags[static_cast<size_t>(90 + pick)] = true;
            int offset = kRumorStringOffset[static_cast<size_t>(player.traitorIndex)][static_cast<size_t>(pick)];
            return ReplaceFirstTag(result, "<TAG>", dialogue.groups[9][static_cast<size_t>(5 + offset)]);
        }

        if (action == 10) {
            player.ailmentMask = 0;
            return dialogue.groups[4][9];
        }

        if (action == 11) {
            if (!PlayerMovement::HasCampMark(player)) return dialogue.groups[4][10];

            PlayerMovement::WarpToCampMark(player, levels);
            return dialogue.groups[4][11];
        }

        if (action == 12) {
            player.coreStats[2] = player.coreStats[3];
            player.coreStats[4] = player.coreStats[5];
            return dialogue.groups[4][12];
        }

        return std::nullopt;
    }

    return std::string("quack2");
}

bool ShopInteraction::IsValidShopAction(int shopId, int action) {
    switch (shopId) {
        case 5:
            return action == 7 || action == 8 || action == 10;
        case 6:
            return action == 1 || action == 3 || action == 4;
        case 7:
            return action == 0 || action == 2 || action == 5;
        case 8:
            return action == 6 || action == 12 || action == 13;
        default:
            return false;
    }
}

int ShopInteraction::ShopActionCode(int shopId, int choiceIndex) {
    switch (shopId) {
        case 5:
            switch (choiceIndex) {
                case 0:
                    return 7;
                case 1:
                    return 8;
                case 2:
                    return 10;
                default:
                    return -1;
            }
        case 6:
            switch (choiceIndex) {
                case 0:
                    return 1;
                case 1:
                    return 3;
                case 2:
                    return 4;
                default:
                    return -1;
            }
        case 7:
            switch (choiceIndex) {
                case 0:
                    return 0;
                case 1:
                    return 2;
                case 2:
                    return 5;
                default:
                    return -1;
            }
        case 8:
            switch (choiceIndex) {
                case 0:
                    return 6;
                case 1:
                    return 12;
                case 2:
                    return 13;
                default:
                    return -1;
            }
        default:
            return -1;
    }
}

}  // namespace dawnstar
