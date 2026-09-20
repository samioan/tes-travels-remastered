#include "ui/npc_menu.h"

#include <cstdlib>

#include "player/player_combat_stats.h"
#include "player/player_inventory.h"
#include "player/player_movement.h"
#include "util/text.h"

namespace dawnstar {

namespace {

// The 4 special-shop levels the named shopkeepers (shops 5-8) live on --
// ESGame's own hardcoded `if (var24 == 0) currentLevel = 3; ...` chain.
constexpr int kNamedShopLevel[4] = {3, 12, 21, 30};

const char* PromptTemplateFor(int shopId) { return shopId < 4 ? "Your gold: <TAG>" : "Aid: <TAG>"; }

const std::string& ShopName(int shopId) { return ShopInteraction::kNames[static_cast<size_t>(shopId)]; }

}  // namespace

NpcMenu::NpcMenu()
    : info_(ScreenMode::PlainList),
      buyWhat_(ScreenMode::PromptList),
      sellWhat_(ScreenMode::PromptList),
      sellSure_(ScreenMode::PromptList),
      trainWhat_(ScreenMode::PromptList),
      giveWhat_(ScreenMode::PromptList),
      questWhat_(ScreenMode::PromptList),
      questWhom_(ScreenMode::PromptList),
      warpWhere_(ScreenMode::PromptList),
      warpConfirm_(ScreenMode::PromptList) {
    // ESGame.allocateAllUIs(): NPCChoicesUI[0..3] "Buy"/"Sell", [4] Eustacia's
    // 4-way menu, [5..8] the named shopkeepers' 6-way menu.
    for (int i = 0; i < ShopInteraction::kShopCount; i++) {
        choices_.emplace_back(ScreenMode::PromptList);
        Screen& s = choices_.back();
        if (i < 4) {
            s.SetupPromptList(ShopName(i), PromptTemplateFor(i), {"Buy", "Sell"});
        } else if (i == 4) {
            s.SetupPromptList("Eustacia", "Welcome", {"Rumors", "Cure", "Warp", "Recovery"});
        } else {
            s.SetupPromptList(ShopName(i), PromptTemplateFor(i),
                              {"Train", "Give", "Befriend", "Threaten", "Ask a question", "Warp"});
        }
    }
    // NPCQuestionWhatUI: title/context are set per use (handleNPCChoices).
    questWhat_.SetupPromptList("", "Ask about what?",
                               {"North wall defense", "East wall defense", "Arguing with governor", "Imperial aid",
                                "Ice tribes", "Gates before attack"});
}

Screen& NpcMenu::ActiveScreen() {
    switch (active_) {
        case Active::Info:
            return info_;
        case Active::Choices:
            return choices_[static_cast<size_t>(shopId_)];
        case Active::BuyWhat:
            return buyWhat_;
        case Active::SellWhat:
            return sellWhat_;
        case Active::SellSure:
            return sellSure_;
        case Active::TrainWhat:
            return trainWhat_;
        case Active::GiveWhat:
            return giveWhat_;
        case Active::QuestWhat:
            return questWhat_;
        case Active::QuestWhom:
            return questWhom_;
        case Active::WarpWhere:
            return warpWhere_;
        case Active::WarpConfirm:
            return warpConfirm_;
    }
    return info_;
}

const Screen& NpcMenu::ActiveScreen() const { return const_cast<NpcMenu*>(this)->ActiveScreen(); }

void NpcMenu::OnUp() { ActiveScreen().MoveSelectionUp(); }
void NpcMenu::OnDown() { ActiveScreen().MoveSelectionDown(); }
void NpcMenu::Render(Backbuffer& bb) const { ActiveScreen().Paint(bb); }

void NpcMenu::RefreshAid(int shopId, const PlayerState& player, const ShopState& shop) {
    if (shopId == 4) return;
    int value = 0;
    if (ShopInteraction::IsNamedShop(shopId)) {
        value = shop.rewardsGiven[static_cast<size_t>(shopId - 5)];
    } else if (ShopInteraction::IsGenericPeddler(shopId)) {
        value = player.gold;
    }
    choices_[static_cast<size_t>(shopId)].SetItems(
        ReplaceFirstTag(PromptTemplateFor(shopId), "<TAG>", std::to_string(value)));
}

void NpcMenu::ShowChoices(int shopId) {
    shopId_ = shopId;
    active_ = Active::Choices;
}

void NpcMenu::ShowInfo(int shopId, int infoParam, const std::string& text) {
    info_.SetupMessage(ShopName(shopId), text);
    shopId_ = shopId;
    infoParam_ = infoParam;
    active_ = Active::Info;
}

bool NpcMenu::Open(int shopId, const std::optional<std::string>& greeting, const PlayerState& player,
                   const ShopState& shop) {
    if (greeting.has_value()) {
        ShowInfo(shopId, 8, *greeting);
        RefreshAid(shopId, player, shop);
        return true;
    }
    if (shopId == 4) {
        ShowChoices(4);
        return true;
    }
    return false;
}

void NpcMenu::HandleAction(NpcMenuContext& ctx, int shopId, int infoParam, int action, int extra) {
    std::optional<std::string> text =
        ShopInteraction::Dialogue(ctx.player, ctx.shop, ctx.charData, ctx.items, ctx.dialogue, ctx.levels, shopId,
                                  action, extra, ctx.rng, ctx.nextItemSpawnId);
    // The original hands a possible null straight to setupMessage; no real
    // path here returns one for these actions, so an empty body is the
    // safe stand-in.
    ShowInfo(shopId, infoParam, text.value_or(""));
}

void NpcMenu::BuildBuyWhat(const ItemDatabase& items) {
    std::vector<std::string> labels;
    for (int8_t id : ShopInteraction::kStock[static_cast<size_t>(shopId_)]) {
        size_t idx = static_cast<size_t>(std::abs(static_cast<int>(id)) - 1);
        labels.push_back(items.name[idx] + " (" + std::to_string(items.buyPrice[idx]) + ")");
    }
    buyWhat_ = Screen(ScreenMode::PromptList);
    buyWhat_.SetupPromptList(ShopName(shopId_), "Buy What?", labels);
}

void NpcMenu::BuildSellWhat(const PlayerState& player, const ItemDatabase& items) {
    std::vector<std::string> labels;
    for (int i = 0; i < player.inventoryCount; i++) {
        size_t idx = static_cast<size_t>(std::abs(static_cast<int>(player.inventoryItemIds[static_cast<size_t>(i)])) - 1);
        std::string label = items.name[idx] + " (" + std::to_string(items.sellPrice[idx]) + ")";
        labels.push_back(PlayerInventory::IsEquipped(player, items, i) ? "E:" + label : label);
    }
    sellWhat_ = Screen(ScreenMode::PromptList);
    sellWhat_.SetupPromptList(ShopName(shopId_), "Sell What?", labels);
}

void NpcMenu::BuildGiveWhat(const PlayerState& player, const ItemDatabase& items) {
    std::vector<std::string> labels;
    for (int i = 0; i < player.inventoryCount; i++) {
        size_t idx = static_cast<size_t>(std::abs(static_cast<int>(player.inventoryItemIds[static_cast<size_t>(i)])) - 1);
        labels.push_back(PlayerInventory::IsEquipped(player, items, i) ? "E:" + items.name[idx] : items.name[idx]);
    }
    giveWhat_ = Screen(ScreenMode::PromptList);
    giveWhat_.SetupPromptList(ShopName(shopId_), "Give What?", labels);
}

void NpcMenu::BuildTrainWhat(const PlayerState& player, const CharacterData& charData) {
    std::vector<std::string> labels;
    for (int i = 0; i < 14; i++) {
        if (ShopInteraction::IsValidShopAction(shopId_, i)) {
            int value = PlayerCombatStats::SkillValue(player, charData, i, false);
            labels.push_back(ReplaceFirstTag(charData.skillNames[static_cast<size_t>(i)] + " (<TAG>)", "<TAG>",
                                             std::to_string(value)));
        }
    }
    trainWhat_ = Screen(ScreenMode::PromptList);
    trainWhat_.SetupPromptList(ShopName(shopId_), "Train What?", labels);
}

void NpcMenu::BuildWarpWhere(const ShopState& shop) {
    std::vector<std::string> labels = {"Your last location"};
    for (int i = 0; i < 4; i++) {
        if (!shop.firstVisit[static_cast<size_t>(5 + i)]) labels.push_back(ShopName(5 + i));
    }
    warpWhere_ = Screen(ScreenMode::PromptList);
    warpWhere_.SetupPromptList("Eustacia", "", labels);
}

// ESGame.handleNPCChoices(): dispatches the choices screen's selection.
NpcMenuAction NpcMenu::OnChoicesSelect(NpcMenuContext& ctx) {
    int sel = choices_[static_cast<size_t>(shopId_)].SelectedIndexOrMinusOne();
    int shop = shopId_;
    PlayerState& player = ctx.player;

    if (shop <= 3) {
        if (sel == 0) {
            BuildBuyWhat(ctx.items);
            active_ = Active::BuyWhat;
        } else if (sel == 1) {
            if (player.inventoryCount <= 0) {
                ShowInfo(shop, 53, "You have nothing to give me!");
            } else {
                BuildSellWhat(player, ctx.items);
                active_ = Active::SellWhat;
            }
        }
    } else if (shop == 4) {
        if (sel == 0) {
            std::optional<std::string> rumor =
                ShopInteraction::Dialogue(player, ctx.shop, ctx.charData, ctx.items, ctx.dialogue, ctx.levels, 4, 13, 0,
                                          ctx.rng, ctx.nextItemSpawnId);
            ShowInfo(4, 360, rumor.value_or("I have no new rumors."));
        } else if (sel == 1) {
            HandleAction(ctx, shop, 353, 10, 0);
        } else if (sel == 2) {
            BuildWarpWhere(ctx.shop);
            active_ = Active::WarpWhere;
        } else if (sel == 3) {
            HandleAction(ctx, shop, 355, 12, 0);
        }
    } else {
        if (sel == 0) {
            BuildTrainWhat(player, ctx.charData);
            active_ = Active::TrainWhat;
        } else if (sel == 1) {
            if (player.inventoryCount <= 0) {
                ShowInfo(shop, 23, "You have nothing to give me!");
            } else {
                BuildGiveWhat(player, ctx.items);
                active_ = Active::GiveWhat;
            }
        } else if (sel == 2) {
            HandleAction(ctx, shop, 24, 2, 0);
        } else if (sel == 3) {
            HandleAction(ctx, shop, 25, 3, 0);
        } else if (sel == 4) {
            if (ctx.shop.rewardsGiven[static_cast<size_t>(shop - 5)] == 0) {
                ShowInfo(shop, 26, ctx.dialogue.groups[static_cast<size_t>(shop)][15]);
            } else {
                questWhat_.SetTitle(ShopName(shop));
                active_ = Active::QuestWhat;
            }
        } else if (sel == 5) {
            warpConfirm_ = Screen(ScreenMode::PromptList);
            warpConfirm_.SetupPromptList(ShopName(shop), "Warp to camp", {"Yes", "No"});
            active_ = Active::WarpConfirm;
        }
    }
    return NpcMenuAction::None;
}

// The "Ask a question" answer -- commandAction1's secondaryParam==28
// select branch, transcribed exactly (including the RNG draw's
// short-circuit position and the eventFlags bookkeeping order).
void NpcMenu::OnQuestWhomSelect(NpcMenuContext& ctx) {
    PlayerState& player = ctx.player;
    currentQWhom_ = questWhom_.SelectedIndexOrMinusOne();
    int shop = shopId_;
    int flagIdx = (shop - 5) * 18 + currentQWhat_ * 3 + currentQWhom_;
    std::string text;
    bool textSet = false;
    int suspect = currentQWhom_;
    if (suspect >= shop - 5) suspect++;
    const auto& g9 = ctx.dialogue.groups[9];

    if (player.eventFlags[static_cast<size_t>(flagIdx)]) {
        int tableIdx = currentQWhat_ * 4 + suspect;
        if (player.eventFlags[static_cast<size_t>(72 + currentQWhat_ * 3 + currentQWhom_)]) {
            text = g9[static_cast<size_t>(5 + ShopInteraction::kUnconfirmedB[static_cast<size_t>(tableIdx)])];
        } else {
            text = g9[static_cast<size_t>(5 + ShopInteraction::kUnconfirmedA[static_cast<size_t>(tableIdx)])];
        }
    } else {
        player.eventFlags[static_cast<size_t>(flagIdx)] = true;
        ctx.shop.rewardsGiven[static_cast<size_t>(shop - 5)]--;
        int tableIdx = currentQWhat_ * 4 + suspect;
        if (shop - 5 == player.traitorIndex) {
            if (player.traitorSuspicionCount < 3) player.traitorSuspicionCount++;
            if (player.traitorSuspicionCount == 2 ||
                (player.traitorSuspicionCount == 3 && RandomIntBelow(ctx.rng, 100) < 20)) {
                text = g9[static_cast<size_t>(5 + ShopInteraction::kUnconfirmedB[static_cast<size_t>(tableIdx)])];
                textSet = true;
                player.eventFlags[static_cast<size_t>(72 + currentQWhat_ * 3 + currentQWhom_)] = true;
            }
        }
        if (!textSet) {
            text = g9[static_cast<size_t>(5 + ShopInteraction::kUnconfirmedA[static_cast<size_t>(tableIdx)])];
        }
    }
    ShowInfo(shop, 26, text);
}

// commandAction1's secondaryParam==29 select branch.
NpcMenuAction NpcMenu::OnWarpWhereSelect(NpcMenuContext& ctx) {
    int sel = warpWhere_.SelectedIndexOrMinusOne();
    if (sel == 0) {
        HandleAction(ctx, 4, 41, 11, 0);
        return NpcMenuAction::None;
    }
    int remaining = sel - 1;
    for (int i = 0; i < 4; i++) {
        if (!ctx.shop.firstVisit[static_cast<size_t>(5 + i)]) {
            if (remaining == 0) {
                sel = i;
                break;
            }
            remaining--;
        }
    }
    if (sel >= 0 && sel < 4) {
        int level = kNamedShopLevel[sel];
        const GeneratedLevel& target = ctx.levels[static_cast<size_t>(level - 1)];
        // `SHOP_Y[5 + sel] + 1`: land one tile south of the shopkeeper,
        // facing north (1) toward them.
        PlayerMovement::WarpTo(ctx.player, level, target.specialShopX, target.specialShopY + 1, 1, ctx.levels);
    }
    return NpcMenuAction::ReturnToGame;
}

// The Ok dispatch for the shared info popup, keyed on its secondaryParam.
NpcMenuAction NpcMenu::OnInfoOk(NpcMenuContext& ctx) {
    int shop = shopId_;
    switch (infoParam_) {
        case 8:
        case 360:
            ShowChoices(shop);
            break;
        case 53:
            BuildSellWhat(ctx.player, ctx.items);
            sellWhat_.SetSelectedIndex(currentItemIndex_);
            active_ = Active::SellWhat;
            break;
        case 51:
            BuildBuyWhat(ctx.items);
            buyWhat_.SetSelectedIndex(currentItemIndex_);
            active_ = Active::BuyWhat;
            break;
        case 41:
            ctx.player.suppressStrafeAdjust = false;
            return NpcMenuAction::ReturnToGame;
        default:  // 21, 23, 24, 25, 26, 353, 355
            RefreshAid(shop, ctx.player, ctx.shop);
            ShowChoices(shop);
            break;
    }
    return NpcMenuAction::None;
}

NpcMenuAction NpcMenu::OnSelect(NpcMenuContext& ctx) {
    int shop = shopId_;
    switch (active_) {
        case Active::Info:
            return OnInfoOk(ctx);
        case Active::Choices:
            return OnChoicesSelect(ctx);
        case Active::BuyWhat: {
            int sel = buyWhat_.SelectedIndexOrMinusOne();
            if (sel >= 0) {
                currentItemIndex_ = sel;
                HandleAction(ctx, shop, 51, 14, sel);
            }
            break;
        }
        case Active::SellWhat: {
            int sel = sellWhat_.SelectedIndexOrMinusOne();
            if (sel >= 0) {
                currentItemIndex_ = sel;
                if (PlayerInventory::IsEquipped(ctx.player, ctx.items, sel)) {
                    size_t idx = static_cast<size_t>(
                        std::abs(static_cast<int>(ctx.player.inventoryItemIds[static_cast<size_t>(sel)])) - 1);
                    sellSure_ = Screen(ScreenMode::PromptList);
                    sellSure_.SetupPromptList(ShopName(shop),
                                              "You may sell " + ctx.items.name[idx] + " for " +
                                                  std::to_string(ctx.items.sellPrice[idx]) + ". Confirm?",
                                              {"No", "Yes"});
                    sellSure_.RemoveCommand(CommandId::Cancel);
                    active_ = Active::SellSure;
                } else {
                    HandleAction(ctx, shop, 53, 15, sel);
                }
            }
            break;
        }
        case Active::SellSure:
            if (sellSure_.SelectedIndexOrMinusOne() == 0) {
                active_ = Active::SellWhat;
            } else {
                HandleAction(ctx, shop, 53, 15, currentItemIndex_);
            }
            break;
        case Active::TrainWhat:
            HandleAction(ctx, shop, 21, 5,
                         ShopInteraction::ShopActionCode(shop, trainWhat_.SelectedIndexOrMinusOne()));
            break;
        case Active::GiveWhat: {
            int sel = giveWhat_.SelectedIndexOrMinusOne();
            if (sel >= 0) HandleAction(ctx, shop, 23, 4, sel);
            break;
        }
        case Active::QuestWhat: {
            currentQWhat_ = questWhat_.SelectedIndexOrMinusOne();
            std::vector<std::string> others;
            for (int i = 0; i < 4; i++) {
                if (i + 5 != shop) others.push_back(ShopName(i + 5));
            }
            questWhom_ = Screen(ScreenMode::PromptList);
            questWhom_.SetupPromptList(ShopName(shop), "Ask about whom?", others);
            active_ = Active::QuestWhom;
            break;
        }
        case Active::QuestWhom:
            OnQuestWhomSelect(ctx);
            break;
        case Active::WarpWhere:
            return OnWarpWhereSelect(ctx);
        case Active::WarpConfirm:
            if (warpConfirm_.SelectedIndexOrMinusOne() == 0) {
                PlayerMovement::MarkCampAndReturnToTown(ctx.player, true, ctx.levels, ctx.world);
                ctx.player.suppressStrafeAdjust = false;
                return NpcMenuAction::ReturnToGame;
            }
            ShowChoices(shop);
            break;
    }
    return NpcMenuAction::None;
}

NpcMenuAction NpcMenu::OnCancel(NpcMenuContext& ctx) {
    switch (active_) {
        case Active::Info:
        case Active::SellSure:
            // PlainList popups carry only Ok; SellSure's Cancel was removed.
            break;
        case Active::Choices:
            // backTarget == gameCanvas.
            return NpcMenuAction::ReturnToGame;
        case Active::BuyWhat:
        case Active::SellWhat:
            // No backTarget: the secondaryParam 50/52 cancel branches refresh
            // the aid-points prompt before returning to the choices screen.
            RefreshAid(shopId_, ctx.player, ctx.shop);
            ShowChoices(shopId_);
            break;
        case Active::TrainWhat:
        case Active::GiveWhat:
        case Active::QuestWhat:
        case Active::QuestWhom:
        case Active::WarpWhere:
        case Active::WarpConfirm:
            // backTarget == NPCChoicesUI[shop]: taken by commandAction1's
            // generic cancel check, before any per-screen branch -- so no
            // aid refresh.
            ShowChoices(shopId_);
            break;
    }
    return NpcMenuAction::None;
}

}  // namespace dawnstar
