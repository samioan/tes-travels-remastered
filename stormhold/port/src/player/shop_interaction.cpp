#include "player/shop_interaction.h"

#include <algorithm>
#include <cstdlib>

#include "player/player_combat_stats.h"
#include "player/player_inventory.h"

namespace stormhold {

bool ShopInteraction::IsValidShopAction(int shopId, int action) {
    switch (shopId) {
        case 0:
            return action == 3 || action == 4 || action == 13;
        case 1:
            return action == 7 || action == 8 || action == 10;
        case 2:
            return action == 1 || action == 6 || action == 12;
        case 3:
            return action == 0 || action == 2 || action == 5;
        default:
            return false;
    }
}

int ShopInteraction::ShopActionCode(int shopId, int choiceIndex) {
    static constexpr int kCodes[4][3] = {{3, 4, 13}, {7, 8, 10}, {1, 6, 12}, {0, 2, 5}};
    if (shopId < 0 || shopId > 3 || choiceIndex < 0 || choiceIndex > 2) return -1;
    return kCodes[shopId][choiceIndex];
}

std::string ShopInteraction::RumorFor(PlayerState& player, const CharacterData& charData, const ShopDialogue& text,
                                       int step) {
    int16_t asked = player.skills[static_cast<size_t>(step)][0];
    const std::string& skillName = charData.skillNames[static_cast<size_t>(step)];
    if (asked == 0) {
        player.skills[static_cast<size_t>(step)][0] = 1;
        std::string result = text.groups[7][1];
        auto pos = result.find("<TAG>");
        if (pos != std::string::npos) result.replace(pos, 5, skillName);
        return result;
    }

    player.skills[static_cast<size_t>(step)][0] = static_cast<int16_t>(asked + 1);
    std::string result = text.groups[7][2];
    std::string values[3] = {skillName, std::to_string(asked), std::to_string(asked + 1)};
    for (const std::string& value : values) {
        auto pos = result.find("<TAG>");
        if (pos == std::string::npos) break;
        result.replace(pos, 5, value);
    }
    return result;
}

std::optional<std::string> ShopInteraction::QuestShopDialogue(PlayerState& player, ShopState& shop,
                                                                const ShopDialogue& text, const CharacterData& charData,
                                                                const ItemDatabase& items, GeneratedLevel& hub,
                                                                JavaRandom& rng, int shopId, int action, int extra) {
    const size_t id = static_cast<size_t>(shopId);
    const std::vector<std::string>& lines = text.groups[id];

    if (action == 1) {
        if (shop.firstVisit[id]) {
            shop.firstVisit[id] = false;
            return lines[0];
        }
        if (shop.unconfirmedCooldownH[id] > 50) return lines[1];
        if (player.coreStats[8] > 50) return lines[2];
        int line = RandomInt0Based(rng, 3);
        return lines[static_cast<size_t>(3 + line)];
    }

    if (action == 2) {
        if (shop.questState1[id] != 0) return lines[6];
        int outcome = PlayerCombatStats::RollShopOutcome(player, charData, action, shop.interactionCount[id], rng);
        if (outcome == 0) {
            shop.questState1[id] = 1;
        } else if (outcome == 1) {
            player.skills[13][2] = static_cast<int16_t>(player.skills[13][2] + 2);
        } else if (outcome == 2) {
            player.skills[13][2] = static_cast<int16_t>(player.skills[13][2] + 5);
            shop.rewardsGiven[id]++;
            shop.questState1[id] = 1;
        } else if (outcome == 3) {
            player.skills[13][2] = static_cast<int16_t>(player.skills[13][2] + 8);
            shop.rewardsGiven[id]++;
            shop.questState1[id] = 1;
        }
        shop.interactionCount[id]++;
        return lines[static_cast<size_t>(7 + outcome)];
    }

    if (action == 3) {
        if (shop.questState2[id] != 0) return lines[6];
        int outcome = PlayerCombatStats::RollShopOutcome(player, charData, action, shop.interactionCount[id], rng);
        int variant = extra <= 1 ? 0 : 1;
        if (outcome == 0) {
            shop.questState2[id] = 2;
        } else if (outcome == 1) {
            player.skills[13][2] = static_cast<int16_t>(player.skills[13][2] + 2);
            shop.questState2[id] = 2;
        } else if (outcome == 2) {
            player.skills[13][2] = static_cast<int16_t>(player.skills[13][2] + 5);
            shop.rewardsGiven[id]++;
            shop.questState2[id] = 1;
        } else if (outcome == 3) {
            player.skills[13][2] = static_cast<int16_t>(player.skills[13][2] + 8);
            shop.rewardsGiven[id]++;
            shop.questState2[id] = 1;
        }
        shop.interactionCount[id]++;
        return lines[static_cast<size_t>(11 + variant)];
    }

    if (action == 4) {
        if (shop.questState1[id] != 2 && shop.questState2[id] != 2) {
            int slot = extra;
            int itemId = std::abs(static_cast<int>(player.inventoryItemIds[static_cast<size_t>(slot)]));
            int category = items.category[static_cast<size_t>(itemId - 1)];

            // Category 15: a "reduce the greeting cooldown" item -- consumed,
            // its RAW (non-bit-extracted) questFlags column value is
            // subtracted from unconfirmedCooldownH, clamped at 0. Distinct
            // from category 11 below; reuses the same itemsin.dat column for
            // an unrelated purpose (Shop.java's own confirmed finding, see
            // world/shop_state.h's QuestFlagsFor doc comment for the sibling
            // bit-extracted use of this same column).
            if (category == 15) {
                PlayerInventory::RemoveInventorySlot(player, slot, items);
                int reduceBy = items.questFlags[static_cast<size_t>(itemId - 1)];
                shop.unconfirmedCooldownH[id] = static_cast<int16_t>(shop.unconfirmedCooldownH[id] - reduceBy);
                shop.unconfirmedCooldownH[id] = static_cast<int16_t>(std::max<int>(shop.unconfirmedCooldownH[id], 0));
                return lines[17];
            }

            // Category 11 ("gift"/quest item): the real quest-item delivery,
            // gated by the bit-extracted QuestFlagsFor.
            if (category == 11) {
                int flags = Shop::QuestFlagsFor(shopId, itemId, items);
                if (flags > 0) {
                    PlayerInventory::RemoveInventorySlot(player, slot, items);
                    shop.rewardsGiven[id] = static_cast<int16_t>(shop.rewardsGiven[id] + flags);
                    shop.questState1[id] = 0;
                    shop.questState2[id] = 0;
                }
                return lines[static_cast<size_t>(13 + flags)];
            }

            return lines[13];
        }
        return lines[13];
    }

    if (action == 5) {
        if (shop.rewardsGiven[id] == 0) return lines[18];
        if (shop.unconfirmedCooldownH[id] > 50) return lines[1];
        if (player.coreStats[8] > 50) return lines[2];
        shop.rewardsGiven[id]--;
        return RumorFor(player, charData, text, extra);
    }

    if (action == 6) {
        shop.questRewardClaimable[id] = false;
        const int x = Shop::kShopX[id];
        const int y = Shop::kShopY[id];
        hub.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] =
            static_cast<uint8_t>(hub.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] & ~uint8_t{32});
        return lines[19];
    }

    return std::nullopt;
}

}  // namespace stormhold
