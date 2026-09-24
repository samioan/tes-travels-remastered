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
        // Confirmed dead in the original, not just "unconfirmed producer"
        // (Shop.java's own header comment's original phrasing): exhaustively
        // grepped every renamed source file in ../../../src/ for a write to
        // Shop.unconfirmedCooldownH/`h` -- the ONLY one anywhere is this
        // action's own category-15-item decrement below (QuestShopDialogue's
        // action==4 branch), which only ever subtracts and clamps at 0.
        // reset() zeroes it and nothing else increments it, so it can never
        // exceed 0 in practice -- this check, and its action==5 twin below,
        // are both faithfully-preserved dead branches in the ORIGINAL game
        // too, not a port gap.
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

std::optional<std::string> ShopInteraction::BenecaDialogue(PlayerState& player, ShopState& shop,
                                                             const ShopDialogue& text, const ItemDatabase& items,
                                                             int16_t& spawnIdCounter, int action, int extra) {
    const std::vector<std::string>& lines = text.groups[4];

    if (action == 1) {
        if (shop.firstVisit[4]) {
            shop.firstVisit[4] = false;
            return lines[0];
        }
        return std::nullopt;
    }

    if (action == 4) {
        int slot = extra;
        int itemId = std::abs(static_cast<int>(player.inventoryItemIds[static_cast<size_t>(slot)]));
        int category = items.category[static_cast<size_t>(itemId - 1)];
        if (category != 13 && category != 15 && category != 17) {
            shop.benecaPoints++;
            PlayerInventory::RemoveInventorySlot(player, slot, items);
            return lines[2];
        }
        return lines[1];
    }

    if (action == 7) {
        if (shop.benecaPoints / 3 > 0) {
            // `extra` is an item id here, NOT a slot -- see this method's
            // own header comment for the real naming trap in Shop.java.
            int itemId = extra;
            int16_t spawnId = ++spawnIdCounter;
            bool trained = PlayerInventory::AddInventoryItemRaw(player, itemId, spawnId, 0);
            if (!trained) {
                // A real cross-group return in the original: the
                // training-failed message lives in group 7 (the generic
                // pool), not group 4 -- confirmed independently by M11's
                // own smoke test, which found this exact string sitting at
                // dialogue[7][0].
                return text.groups[7][0];
            }
            shop.benecaPoints = static_cast<int16_t>(shop.benecaPoints - 3);
            return lines[3];
        }
        return lines[4];
    }

    return std::nullopt;
}

std::optional<std::string> ShopInteraction::HelgaDialogue(PlayerState& player, ShopState& shop,
                                                            const ShopDialogue& text, const ItemDatabase& items,
                                                            int action, int extra,
                                                            const GameAdvancement::LevelLookup& levels) {
    const std::vector<std::string>& lines = text.groups[5];

    if (action == 1) {
        if (shop.firstVisit[5]) {
            shop.firstVisit[5] = false;
            player.rumorRevealStep = 0;
            if (shop.showSpecialGreeting) {
                shop.showSpecialGreeting = false;
                return lines[21] + "\n" + lines[0] + "\n" + lines[2];
            }
            return lines[0] + "\n" + lines[2];
        }

        int advancement = GameAdvancement::Level(player.giftPointsFound);
        if (advancement > player.rumorRevealStep) {
            player.rumorRevealStep++;
            if (shop.showSpecialGreeting) {
                shop.showSpecialGreeting = false;
                return lines[21] + "\n" + lines[static_cast<size_t>(2 + player.rumorRevealStep)];
            }
            return lines[static_cast<size_t>(2 + player.rumorRevealStep)];
        }

        if (shop.showSpecialGreeting) {
            shop.showSpecialGreeting = false;
            return lines[21];
        }
        return std::nullopt;
    }

    if (action == 13) {
        return lines[static_cast<size_t>(2 + player.rumorRevealStep)];
    }

    if (action == 4) {
        int slot = extra;
        int itemId = std::abs(static_cast<int>(player.inventoryItemIds[static_cast<size_t>(slot)]));
        if (items.category[static_cast<size_t>(itemId - 1)] == 13) {
            // Player.itemSubtypeAtSlot(slot): Item.column(2, itemId),
            // inlined here -- see this method's own class-level comment.
            int quality = items.subtype[static_cast<size_t>(itemId - 1)];
            if (quality > 3) {
                shop.helgaPoints = static_cast<int16_t>(shop.helgaPoints + 5);
            } else {
                shop.helgaPoints = static_cast<int16_t>(shop.helgaPoints + 3);
            }
            PlayerInventory::RemoveInventorySlot(player, slot, items);
            return lines[11];
        }
        return lines[12];
    }

    if (action == 8) {
        if (shop.helgaPoints < 7) return lines[1];
        int slot = extra;
        int itemId = std::abs(static_cast<int>(player.inventoryItemIds[static_cast<size_t>(slot)]));
        // Both conditions must hold -- see this method's own class-level
        // comment on why InitializeItemCharge's own internal gate alone
        // isn't equivalent to the original's real double-check.
        if (items.IsEquipmentCategory(itemId) && !PlayerInventory::IsItemCharged(player, slot)) {
            shop.helgaPoints = static_cast<int16_t>(shop.helgaPoints - 7);
            PlayerInventory::InitializeItemCharge(player, slot, items);
            return lines[13];
        }
        return lines[14];
    }

    if (action == 9) {
        if (shop.helgaPoints < 2) return lines[1];
        if (player.safeCampingBuff) return lines[15];
        player.safeCampingBuff = true;
        shop.helgaPoints = static_cast<int16_t>(shop.helgaPoints - 2);
        return lines[16];
    }

    if (action == 10) {
        if (shop.helgaPoints < 1) return lines[1];
        player.ailmentMask = 0;
        shop.helgaPoints--;
        return lines[17];
    }

    if (action == 11) {
        if (shop.helgaPoints < 1) return lines[1];
        if (!PlayerInventory::HasCampMark(player)) return lines[18];
        shop.helgaPoints--;
        PlayerInventory::WarpToCampMark(player, levels);
        return lines[19];
    }

    if (action == 12) {
        player.coreStats[2] = player.coreStats[3];
        player.coreStats[4] = player.coreStats[5];
        return lines[20];
    }

    return std::nullopt;
}

std::optional<std::string> ShopInteraction::VarusDialogue(PlayerState& player, const WardenState& warden,
                                                            const ShopDialogue& text) {
    const std::vector<std::string>& lines = text.groups[6];

    if (warden.visitCount == 0) return std::nullopt;

    if (warden.visitCount == 1 && player.wardenLoreStep == 0) {
        player.wardenLoreStep = 1;
        return lines[0];
    }
    if (warden.visitCount == 2 && player.wardenLoreStep <= 1) {
        player.wardenLoreStep = 2;
        return lines[1];
    }
    if (warden.visitCount == 3 && player.wardenLoreStep <= 2) {
        player.wardenLoreStep = 3;
        return lines[2];
    }
    // Confirmed unreachable in the real game -- WardenState::visitCount can
    // never actually reach 4 (see this method's own declaration comment).
    // Reproduced anyway rather than dropped.
    if (warden.visitCount == 4 && player.wardenLoreStep <= 3) {
        player.wardenLoreStep = 4;
        return lines[3] + "\n" + lines[4];
    }

    return std::nullopt;
}

}  // namespace stormhold
