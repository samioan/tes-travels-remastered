#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/shop_dialogue.h"
#include "dungeon/dungeon_runtime.h"
#include "graphics/backbuffer.h"
#include "npc/shop_interaction.h"
#include "player/player_state.h"
#include "ui/screen.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace dawnstar {

// Everything NpcMenu::OnSelect needs to act on the world -- bundled into
// one struct (rather than 9 parameters) since every screen's dispatch
// draws on a different subset of it.
struct NpcMenuContext {
    PlayerState& player;
    ShopState& shop;
    const CharacterData& charData;
    const ItemDatabase& items;
    const ShopDialogue& dialogue;
    std::vector<GeneratedLevel>& levels;
    WorldRegistry& world;
    JavaRandom& rng;
    int16_t& nextItemSpawnId;
};

enum class NpcMenuAction { None, ReturnToGame };

// M46: the interactive NPC menus the M45 greeting popup leads into --
// ../src/ESGame.java's `NPCChoicesUI[shopId]` (Buy/Sell for the 4 hub
// peddlers, Rumors/Cure/Warp/Recovery for shop 4, Train/Give/Befriend/
// Threaten/Ask a question/Warp for the 4 named shopkeepers) and every
// sub-screen hanging off it: `handleNPCChoices()`, `handleNPCAction()`,
// `setAidPointsForNPC()`, and the NPC-related `secondaryParam` branches of
// `commandAction1()` (8/360, 20-29, 41, 50-54, 69, 21/23-26/353/355).
// Every action ultimately calls `ShopInteraction::Dialogue` (M45) and shows
// its returned text in the one shared "GenericInfoUI" popup, exactly like
// `handleNPCAction` -- so this class is pure screen-graph orchestration on
// top of already-verified logic.
//
// Same shape as ui/options_menu.h's OptionsMenu: one Screen per real
// screen, an `Active` state standing in for `setCurrentDisplay`, the
// caller edge-detects physical keys, and OnSelect/OnCancel return whether
// the flow has left back to the game view. Screen.backTarget/secondaryParam
// /contextIndex have no Screen fields (see ui/screen.h); they're the
// `Active` state, `infoParam_` and `shopId_` here.
//
// Screens the original builds ONCE and reuses (NPCChoicesUI[9],
// NPCQuestionWhatUI -- so a stale selection genuinely persists across
// visits) are persistent members; screens it rebuilds via `newXxxUI()` on
// every entry (Buy/Sell/Give/Train/QuestionWhom/WarpWhere/Warp/SellSure)
// are fully replaced on each entry.
//
// Preserved quirks: Buy/Sell/Give/Train item lists show the original's
// exact labels ("E:" with no space in Sell/Give vs "E: " in Inventory);
// cancelling a screen whose backTarget is set (Train/Give/Question*/Warp*)
// returns to the choices screen WITHOUT refreshing the aid-points prompt,
// while cancelling Buy/Sell (no backTarget) refreshes it; the sell-list
// re-open after a sale re-selects the old slot (clamped by Screen); the
// "nothing to give me" popup's Ok (param 53) re-opens an EMPTY Sell list.
// The shop-id -> level mapping for Warp uses GeneratedLevel::specialShopX/Y
// in place of `Shop.SHOP_X/Y[5..8]` (see dungeon_generator.h).
class NpcMenu {
public:
    NpcMenu();

    // GameCanvas.openNpcDialogue(shopId): `greeting` is what Dialogue(...,
    // action 1) returned. A line -> the greeting popup (secondaryParam 8,
    // with setAidPointsForNPC); no line and shop 4 (Eustacia) -> her
    // choices screen directly; anything else -> nothing opens. Returns
    // whether a screen opened.
    bool Open(int shopId, const std::optional<std::string>& greeting, const PlayerState& player,
              const ShopState& shop);

    void OnUp();
    void OnDown();
    NpcMenuAction OnSelect(NpcMenuContext& ctx);
    NpcMenuAction OnCancel(NpcMenuContext& ctx);

    void Render(Backbuffer& bb) const;

    // The screen currently showing, and the shop the flow is talking to --
    // read-only, for main.cpp-independent inspection (the smoke test).
    const Screen& Current() const { return ActiveScreen(); }
    int ShopId() const { return shopId_; }

private:
    enum class Active {
        Info,
        Choices,
        BuyWhat,
        SellWhat,
        SellSure,
        TrainWhat,
        GiveWhat,
        QuestWhat,
        QuestWhom,
        WarpWhere,
        WarpConfirm
    };

    Screen& ActiveScreen();
    const Screen& ActiveScreen() const;

    // ESGame.setAidPointsForNPC(shopId).
    void RefreshAid(int shopId, const PlayerState& player, const ShopState& shop);
    void ShowChoices(int shopId);
    // ESGame.handleNPCAction(shopId, param, action, extra): runs the real
    // Dialogue and stages its text in the shared info popup.
    void HandleAction(NpcMenuContext& ctx, int shopId, int infoParam, int action, int extra);
    void ShowInfo(int shopId, int infoParam, const std::string& text);
    NpcMenuAction OnChoicesSelect(NpcMenuContext& ctx);
    NpcMenuAction OnInfoOk(NpcMenuContext& ctx);
    NpcMenuAction OnWarpWhereSelect(NpcMenuContext& ctx);
    void OnQuestWhomSelect(NpcMenuContext& ctx);

    void BuildBuyWhat(const ItemDatabase& items);
    void BuildSellWhat(const PlayerState& player, const ItemDatabase& items);
    void BuildGiveWhat(const PlayerState& player, const ItemDatabase& items);
    void BuildTrainWhat(const PlayerState& player, const CharacterData& charData);
    void BuildWarpWhere(const ShopState& shop);

    Active active_ = Active::Info;
    // GenericInfoUI.contextIndex / every list screen's own contextIndex --
    // the shop the whole flow is currently talking to.
    int shopId_ = 0;
    // GenericInfoUI.secondaryParam (8, 21, 23-26, 41, 51, 53, 353, 355, 360).
    int infoParam_ = 8;
    // ESGame.currentItemIndex/currentQWhat/currentQWhom.
    int currentItemIndex_ = 0;
    int currentQWhat_ = 0;
    int currentQWhom_ = 0;

    std::vector<Screen> choices_;  // NPCChoicesUI[0..8]
    Screen info_;
    Screen buyWhat_;
    Screen sellWhat_;
    Screen sellSure_;
    Screen trainWhat_;
    Screen giveWhat_;
    Screen questWhat_;  // NPCQuestionWhatUI: persistent
    Screen questWhom_;
    Screen warpWhere_;
    Screen warpConfirm_;
};

}  // namespace dawnstar
