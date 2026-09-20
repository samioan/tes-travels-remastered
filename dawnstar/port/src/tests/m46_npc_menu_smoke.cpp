// M46 smoke test: the interactive NPC menu graph (ui/npc_menu.h) -- ESGame's
// NPCChoicesUI[shopId] and every sub-screen hanging off it.
//
// No JVM ground truth (same reason as M6/M9/M11/M13-M45). Every expected
// value is re-derived from ../../../src/ESGame.java's own handleNPCChoices()/
// handleNPCAction()/commandAction1() branches, not read back from
// npc_menu.cpp. Since NpcMenu is pure orchestration on top of the already-
// verified ShopInteraction::Dialogue (M45), each action is checked by running
// the SAME Dialogue call on a twin copy of the player/shop/RNG and comparing
// resulting state, plus checking the screen graph (which screen shows, its
// labels/selection, where Ok/Cancel lead) directly.
#include <cstdio>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/shop_dialogue.h"
#include "dungeon/dungeon_runtime.h"
#include "npc/shop_interaction.h"
#include "player/player_creation.h"
#include "player/player_inventory.h"
#include "player/player_movement.h"
#include "ui/npc_menu.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::CommandId;
using dawnstar::JavaRandom;
using dawnstar::NpcMenu;
using dawnstar::NpcMenuAction;
using dawnstar::NpcMenuContext;
using dawnstar::PlayerState;
using dawnstar::Screen;
using dawnstar::ScreenMode;
using dawnstar::ShopInteraction;
using dawnstar::ShopState;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

bool IsInfo(const NpcMenu& m) { return m.Current().mode() == ScreenMode::PlainList; }

// Expects the current screen to be the info popup titled for `shop` and
// showing exactly `text` (compared via an identically-configured Screen, so
// word-wrap is checked without re-implementing it).
bool InfoShows(const NpcMenu& m, int shop, const std::string& text) {
    Screen expected(ScreenMode::PlainList);
    expected.SetupMessage(ShopInteraction::kNames[static_cast<size_t>(shop)], text);
    return IsInfo(m) && m.Current().Title() == expected.Title() && m.Current().Items() == expected.Items();
}

bool ItemsAre(const NpcMenu& m, const std::vector<std::string>& items) { return m.Current().Items() == items; }

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        dawnstar::MonsterDatabase monsterDb = dawnstar::MonsterDatabase::Load(archive);
        dawnstar::CharacterData charData = dawnstar::CharacterData::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);
        dawnstar::ShopDialogue dialogue = dawnstar::ShopDialogue::Load(root + "/npcstrings.dat");

        std::vector<dawnstar::GeneratedLevel> levels;
        dawnstar::WorldRegistry world(geometry.rows.size());
        for (size_t i = 0; i < geometry.rows.size(); i++) {
            int levelNumber = static_cast<int>(i) + 1;
            levels.push_back(levelNumber == 1
                                 ? dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[0])
                                 : dawnstar::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[i], items,
                                                                             monsterDb));
            dawnstar::DungeonRuntime::RegisterGeneratedSpawns(levels.back(), world);
        }

        JavaRandom creationRng(20260920);
        const PlayerState basePlayer = dawnstar::PlayerCreation::CreateCharacter(0, "Talker", charData, items, creationRng);

        constexpr int32_t kSeed = 4646;
        auto makeCtx = [&](PlayerState& p, ShopState& s, JavaRandom& rng, int16_t& spawn) {
            return NpcMenuContext{p, s, charData, items, dialogue, levels, world, rng, spawn};
        };
        auto select = [](NpcMenu& m, NpcMenuContext& c) { return m.OnSelect(c); };
        auto down = [](NpcMenu& m, int n) {
            for (int i = 0; i < n; i++) m.OnDown();
        };
        // Screens the original keeps alive (NPCChoicesUI, NPCQuestionWhatUI) keep their selection between
        // visits, so navigate by absolute index.
        auto go = [](NpcMenu& m, int index) {
            while (m.Current().SelectedIndex() > 0) m.OnUp();
            for (int i = 0; i < index; i++) m.OnDown();
        };

        // --- A: Open(): greeting popup, direct Eustacia, and no-open. ---
        {
            PlayerState player = basePlayer;
            ShopState shop = ShopState::Reset();
            JavaRandom rng(kSeed);
            int16_t spawn = 1;
            NpcMenuContext ctx = makeCtx(player, shop, rng, spawn);

            NpcMenu m;
            Check(!m.Open(0, std::nullopt, player, shop), "no greeting on a non-Eustacia shop opens nothing");
            Check(m.Open(4, std::nullopt, player, shop), "no greeting on shop 4 opens Eustacia's menu directly");
            Check(ItemsAre(m, {"Rumors", "Cure", "Warp", "Recovery"}) && m.Current().FirstLine() == "Welcome",
                  "Eustacia's menu: Rumors/Cure/Warp/Recovery, prompt 'Welcome'");

            Check(m.Open(0, std::string("Hello there."), player, shop), "a greeting opens the popup");
            Check(InfoShows(m, 0, "Hello there."), "greeting popup shows the line under the shop's name");
            Check(select(m, ctx) == NpcMenuAction::None, "greeting Ok does not leave the menu");
            Check(ItemsAre(m, {"Buy", "Sell"}), "greeting Ok (param 8) opens NPCChoicesUI[0]");
            Check(m.Current().FirstLine() == "Your gold: " + std::to_string(player.gold),
                  "aid prompt was refreshed to the player's gold when the popup opened");
            Check(m.OnCancel(ctx) == NpcMenuAction::ReturnToGame, "Cancel on a choices screen returns to the game");

            // Named shop's prompt shows rewardsGiven, not gold.
            shop.rewardsGiven[1] = 7;
            m.Open(6, std::string("Hi."), player, shop);
            select(m, ctx);
            Check(m.Current().FirstLine() == "Aid: 7", "named shopkeeper's prompt shows rewardsGiven");
            Check(ItemsAre(m, {"Train", "Give", "Befriend", "Threaten", "Ask a question", "Warp"}),
                  "named shopkeepers' 6-way menu");
        }

        // --- B: Buy (shop 0): list labels, purchase vs twin, Ok re-opens the list, Cancel refreshes gold. ---
        {
            PlayerState player = basePlayer;
            ShopState shop = ShopState::Reset();
            JavaRandom rng(kSeed);
            int16_t spawn = 1;
            NpcMenuContext ctx = makeCtx(player, shop, rng, spawn);

            PlayerState twin = player;
            ShopState twinShop = shop;
            JavaRandom twinRng(kSeed);
            int16_t twinSpawn = 1;

            NpcMenu m;
            m.Open(0, std::string("Welcome."), player, shop);
            select(m, ctx);  // -> choices
            select(m, ctx);  // Buy
            const auto& stock = ShopInteraction::kStock[0];
            Check(m.Current().Items().size() == stock.size(), "Buy list has one entry per stocked item");
            {
                size_t idx = static_cast<size_t>(stock[0]) - 1;
                Check(m.Current().Items()[0] == items.name[idx] + " (" + std::to_string(items.buyPrice[idx]) + ")",
                      "Buy entry is 'name (buyPrice)'");
            }
            down(m, 1);
            select(m, ctx);  // buy item at index 1
            ShopInteraction::Dialogue(twin, twinShop, charData, items, dialogue, levels, 0, 14, 1, twinRng, twinSpawn);
            Check(player.gold == twin.gold && player.inventoryCount == twin.inventoryCount && spawn == twinSpawn,
                  "buy result (gold/inventory/spawn id) matches a twin Dialogue(action 14) call");
            Check(IsInfo(m), "a purchase shows the result popup");
            select(m, ctx);  // Ok (param 51) -> Buy list again, same slot selected
            Check(m.Current().mode() == ScreenMode::PromptList && m.Current().SelectedIndex() == 1,
                  "Ok after a buy re-opens the Buy list with the same item selected");
            Check(m.OnCancel(ctx) == NpcMenuAction::None, "Cancel on the Buy list returns to the choices screen");
            Check(m.Current().FirstLine() == "Your gold: " + std::to_string(player.gold),
                  "Cancel from Buy refreshes the gold prompt (no backTarget)");
            Check(m.OnCancel(ctx) == NpcMenuAction::ReturnToGame, "Cancel on choices leaves");
        }

        // --- C: Sell (shop 0): 'E:' labels, equipped-confirm screen, No/Yes, empty inventory. ---
        {
            PlayerState player = basePlayer;
            ShopState shop = ShopState::Reset();
            JavaRandom rng(kSeed);
            int16_t spawn = 1;
            NpcMenuContext ctx = makeCtx(player, shop, rng, spawn);

            int equippedSlot = -1;
            int plainSlot = -1;
            for (int i = 0; i < player.inventoryCount; i++) {
                if (dawnstar::PlayerInventory::IsEquipped(player, items, i)) {
                    if (equippedSlot < 0) equippedSlot = i;
                } else if (plainSlot < 0) {
                    plainSlot = i;
                }
            }
            Check(equippedSlot >= 0, "the starting character has an equipped item (test precondition)");

            NpcMenu m;
            m.Open(0, std::string("Welcome."), player, shop);
            select(m, ctx);
            down(m, 1);
            select(m, ctx);  // Sell
            Check(static_cast<int>(m.Current().Items().size()) == player.inventoryCount,
                  "Sell list has one entry per inventory slot");
            {
                size_t idx = static_cast<size_t>(std::abs(static_cast<int>(player.inventoryItemIds[static_cast<size_t>(equippedSlot)]))) - 1;
                Check(m.Current().Items()[static_cast<size_t>(equippedSlot)] ==
                          "E:" + items.name[idx] + " (" + std::to_string(items.sellPrice[idx]) + ")",
                      "equipped Sell entry is 'E:name (sellPrice)' -- no space after 'E:'");
            }

            down(m, equippedSlot);
            select(m, ctx);  // equipped -> confirm screen
            Check(ItemsAre(m, {"No", "Yes"}), "selling an equipped item opens the No/Yes confirm screen");
            Check(m.OnCancel(ctx) == NpcMenuAction::None && ItemsAre(m, {"No", "Yes"}),
                  "the confirm screen has no Cancel command (Cancel is a no-op)");
            select(m, ctx);  // No (default index 0) -> back to Sell list
            Check(static_cast<int>(m.Current().Items().size()) == player.inventoryCount,
                  "No returns to the same Sell list");

            select(m, ctx);
            down(m, 1);
            PlayerState twin = player;
            ShopState twinShop = shop;
            JavaRandom twinRng(kSeed);
            int16_t twinSpawn = 1;
            int soldSlot = equippedSlot;
            select(m, ctx);  // Yes
            ShopInteraction::Dialogue(twin, twinShop, charData, items, dialogue, levels, 0, 15, soldSlot, twinRng,
                                      twinSpawn);
            Check(player.gold == twin.gold && player.inventoryCount == twin.inventoryCount,
                  "confirmed sell matches a twin Dialogue(action 15) call");
            Check(IsInfo(m), "a sale shows the result popup");

            // Empty inventory: 'nothing to give me' popup; its Ok re-opens an EMPTY Sell list.
            PlayerState empty = basePlayer;
            empty.inventoryCount = 0;
            NpcMenuContext ectx = makeCtx(empty, shop, rng, spawn);
            NpcMenu m2;
            m2.Open(0, std::string("Welcome."), empty, shop);
            select(m2, ectx);
            down(m2, 1);
            select(m2, ectx);
            Check(InfoShows(m2, 0, "You have nothing to give me!"), "empty-inventory Sell shows 'nothing to give me'");
            select(m2, ectx);
            Check(m2.Current().mode() == ScreenMode::PromptList && m2.Current().Items().empty(),
                  "its Ok re-opens an empty Sell list (original quirk)");
        }

        // --- D: Eustacia (shop 4). ---
        {
            PlayerState player = basePlayer;
            ShopState shop = ShopState::Reset();
            JavaRandom rng(kSeed);
            int16_t spawn = 1;
            NpcMenuContext ctx = makeCtx(player, shop, rng, spawn);

            NpcMenu m;
            m.Open(4, std::nullopt, player, shop);

            // Rumors: first ask -> Dialogue's text, or "I have no new rumors." when null.
            {
                PlayerState twin = player;
                ShopState twinShop = shop;
                JavaRandom twinRng(kSeed);
                int16_t twinSpawn = 1;
                auto expected = ShopInteraction::Dialogue(twin, twinShop, charData, items, dialogue, levels, 4, 13, 0,
                                                          twinRng, twinSpawn);
                select(m, ctx);
                Check(InfoShows(m, 4, expected.value_or("I have no new rumors.")), "Rumors shows Dialogue(4,13)'s text");
                Check(player.eventFlags == twin.eventFlags, "Rumors updates eventFlags exactly like a twin call");
                select(m, ctx);
                Check(ItemsAre(m, {"Rumors", "Cure", "Warp", "Recovery"}), "Rumors Ok returns to Eustacia's menu");
            }

            // Cure (index 1) and Recovery (index 3) -> Info -> Ok -> menu.
            for (int idx : {1, 3}) {
                go(m, idx);
                PlayerState twin = player;
                ShopState twinShop = shop;
                JavaRandom twinRng(kSeed);
                int16_t twinSpawn = 1;
                auto expected = ShopInteraction::Dialogue(twin, twinShop, charData, items, dialogue, levels, 4,
                                                          idx == 1 ? 10 : 12, 0, twinRng, twinSpawn);
                JavaRandom liveCopy = rng;
                (void)liveCopy;
                select(m, ctx);
                Check(InfoShows(m, 4, expected.value_or("")), idx == 1 ? "Cure shows Dialogue(4,10)'s text"
                                                                       : "Recovery shows Dialogue(4,12)'s text");
                Check(player.ailmentMask == twin.ailmentMask && player.coreStats == twin.coreStats,
                      "Cure/Recovery mutate the player exactly like a twin call");
                select(m, ctx);
                Check(ItemsAre(m, {"Rumors", "Cure", "Warp", "Recovery"}), "Cure/Recovery Ok returns to the menu");
            }

            // Warp with nothing unlocked: one entry; picking it warps to the camp mark (param 41 -> game).
            player.campLevel = 2;
            player.campX = 5;
            player.campY = 5;
            player.campFacing = 1;
            go(m, 2);
            select(m, ctx);
            Check(ItemsAre(m, {"Your last location"}), "Warp lists only 'Your last location' before any shop is visited");
            player.suppressStrafeAdjust = false;
            select(m, ctx);
            Check(IsInfo(m), "'Your last location' shows the Dialogue(4,11) result popup");
            Check(player.currentLevel == 2 && player.tileX == 5 && player.tileY == 5,
                  "Dialogue(4,11) warped to the camp mark");
            player.suppressStrafeAdjust = true;
            Check(select(m, ctx) == NpcMenuAction::ReturnToGame, "its Ok (param 41) returns to the game");
            Check(!player.suppressStrafeAdjust, "param 41's Ok clears suppressStrafeAdjust");

            // Warp with shop 6 (level 12) unlocked.
            shop.firstVisit[6] = false;
            m.Open(4, std::nullopt, player, shop);
            go(m, 2);
            select(m, ctx);
            Check(ItemsAre(m, {"Your last location", ShopInteraction::kNames[6]}),
                  "an unlocked shop appears in the Warp list by name");
            Check(m.OnCancel(ctx) == NpcMenuAction::None && ItemsAre(m, {"Rumors", "Cure", "Warp", "Recovery"}),
                  "Cancel on the Warp list returns to Eustacia's menu");
            go(m, 2);
            select(m, ctx);
            down(m, 1);
            const auto& lvl12 = levels[11];
            Check(lvl12.specialShopX >= 0, "level 12 has a special shop position (test precondition)");
            Check(select(m, ctx) == NpcMenuAction::ReturnToGame, "warping to a shop returns to the game");
            Check(player.currentLevel == 12 && player.tileX == lvl12.specialShopX &&
                      player.tileY == lvl12.specialShopY + 1 && player.facing == 1,
                  "warp lands one tile south of the shopkeeper, facing north");
        }

        // --- E: Named shopkeeper (shop 5): Train/Give/Befriend/Threaten/Ask/Warp. ---
        {
            PlayerState player = basePlayer;
            ShopState shop = ShopState::Reset();
            JavaRandom rng(kSeed);
            int16_t spawn = 1;
            NpcMenuContext ctx = makeCtx(player, shop, rng, spawn);
            JavaRandom twinRng(kSeed);

            NpcMenu m;
            m.Open(5, std::string("Greetings."), player, shop);
            select(m, ctx);  // -> choices

            // Train.
            select(m, ctx);
            std::vector<int> validActions;
            for (int i = 0; i < 14; i++) {
                if (ShopInteraction::IsValidShopAction(5, i)) validActions.push_back(i);
            }
            Check(m.Current().Items().size() == validActions.size(), "Train lists one entry per valid shop action");
            Check(m.Current().Items()[0].rfind(charData.skillNames[static_cast<size_t>(validActions[0])], 0) == 0,
                  "Train entries start with the skill name");
            {
                PlayerState twin = player;
                ShopState twinShop = shop;
                int16_t twinSpawn = 1;
                ShopInteraction::Dialogue(twin, twinShop, charData, items, dialogue, levels, 5, 5,
                                          ShopInteraction::ShopActionCode(5, 0), twinRng, twinSpawn);
                select(m, ctx);
                Check(IsInfo(m) && player.skills == twin.skills && shop.questState1 == twinShop.questState1,
                      "Train result matches a twin Dialogue(action 5, shopActionCode)");
            }
            shop.rewardsGiven[0] = 5;
            select(m, ctx);  // Ok (param 21) -> choices, aid refreshed
            Check(m.Current().FirstLine() == "Aid: 5" && ItemsAre(m, {"Train", "Give", "Befriend", "Threaten",
                                                                      "Ask a question", "Warp"}),
                  "Train Ok refreshes the aid prompt and returns to the choices");

            // Cancel from Train: backTarget path, NO aid refresh.
            select(m, ctx);
            shop.rewardsGiven[0] = 9;
            Check(m.OnCancel(ctx) == NpcMenuAction::None && m.Current().FirstLine() == "Aid: 5",
                  "Cancel from Train returns WITHOUT refreshing the aid prompt");
            shop.rewardsGiven[0] = 5;

            // Give with an empty inventory.
            {
                PlayerState emptyP = player;
                emptyP.inventoryCount = 0;
                NpcMenuContext ectx = makeCtx(emptyP, shop, rng, spawn);
                go(m, 1);
                select(m, ectx);
                Check(InfoShows(m, 5, "You have nothing to give me!"), "empty-inventory Give shows 'nothing to give me'");
                select(m, ectx);
                Check(ItemsAre(m, {"Train", "Give", "Befriend", "Threaten", "Ask a question", "Warp"}),
                      "its Ok (param 23) returns to the choices");
            }

            // Give with a real inventory: list labels + result vs twin.
            {
                go(m, 1);
                select(m, ctx);
                Check(static_cast<int>(m.Current().Items().size()) == player.inventoryCount,
                      "Give lists every inventory slot");
                PlayerState twin = player;
                ShopState twinShop = shop;
                int16_t twinSpawn = 1;
                int slot = 2 < player.inventoryCount ? 2 : 0;
                down(m, slot);
                ShopInteraction::Dialogue(twin, twinShop, charData, items, dialogue, levels, 5, 4, slot, twinRng,
                                          twinSpawn);
                select(m, ctx);
                Check(IsInfo(m) && player.inventoryCount == twin.inventoryCount &&
                          shop.rewardsGiven == twinShop.rewardsGiven && shop.questState2 == twinShop.questState2,
                      "Give result matches a twin Dialogue(action 4)");
                select(m, ctx);
            }

            // Befriend (2) and Threaten (3): RNG-dependent, checked against the twin RNG stream.
            for (int idx : {2, 3}) {
                go(m, idx);
                PlayerState twin = player;
                ShopState twinShop = shop;
                int16_t twinSpawn = 1;
                auto expected = ShopInteraction::Dialogue(twin, twinShop, charData, items, dialogue, levels, 5,
                                                          idx == 2 ? 2 : 3, 0, twinRng, twinSpawn);
                select(m, ctx);
                Check(InfoShows(m, 5, expected.value_or("")) && shop.questState1 == twinShop.questState1 &&
                          shop.rewardsGiven == twinShop.rewardsGiven,
                      idx == 2 ? "Befriend matches a twin Dialogue(action 2)" : "Threaten matches a twin Dialogue(action 3)");
                select(m, ctx);
            }

            // Ask a question: no rewards -> the shopkeeper's own refusal line (param 26).
            const std::vector<std::string> kNamedMenu = {"Train", "Give", "Befriend", "Threaten", "Ask a question", "Warp"};
            shop.rewardsGiven[0] = 0;
            go(m, 4);
            select(m, ctx);
            Check(InfoShows(m, 5, dialogue.groups[5][15]), "'Ask a question' with 0 aid shows dialogue[shop][15]");
            select(m, ctx);
            Check(ItemsAre(m, kNamedMenu), "param 26's Ok returns to the choices");

            // Ask a question with aid: QuestWhat -> QuestWhom -> answer.
            auto ask = [&](int topic, int whom) {
                go(m, 4);
                select(m, ctx);  // -> QuestWhat (persistent screen: navigate by absolute index)
                go(m, topic);
                select(m, ctx);  // -> QuestWhom (fresh)
                go(m, whom);
                select(m, ctx);  // answer
            };
            shop.rewardsGiven[0] = 3;
            player.traitorIndex = 3;  // not shop 5's own index (0): keeps the RNG out of this branch
            player.traitorSuspicionCount = 0;
            player.eventFlags.fill(false);
            go(m, 4);
            select(m, ctx);
            Check(ItemsAre(m, {"North wall defense", "East wall defense", "Arguing with governor", "Imperial aid",
                               "Ice tribes", "Gates before attack"}) &&
                      m.Current().Title() == ShopInteraction::kNames[5],
                  "with aid, 'Ask a question' opens the 6-topic screen titled for the shop");
            go(m, 1);
            select(m, ctx);
            Check(ItemsAre(m, {ShopInteraction::kNames[6], ShopInteraction::kNames[7], ShopInteraction::kNames[8]}),
                  "'Ask about whom?' lists the other 3 named shopkeepers");
            // whom 0 -> suspect index 1 (since 0 >= shop-5 == 0). flag = 0*18 + 1*3 + 0 = 3; table = 1*4+1 = 5.
            select(m, ctx);
            Check(player.eventFlags[3] && shop.rewardsGiven[0] == 2, "first answer sets the flag and spends one aid point");
            Check(InfoShows(m, 5, dialogue.groups[9][static_cast<size_t>(5 + ShopInteraction::kUnconfirmedA[5])]),
                  "non-traitor first answer uses the UNCONFIRMED_A line");

            // Same question again: flag already set, eventFlags[72+..] unset -> UNCONFIRMED_A again, no aid spent.
            select(m, ctx);  // Ok (param 26) -> choices, aid refreshed
            Check(m.Current().FirstLine() == "Aid: 2", "param 26's Ok refreshes the aid prompt");
            ask(1, 0);
            Check(shop.rewardsGiven[0] == 2 &&
                      InfoShows(m, 5, dialogue.groups[9][static_cast<size_t>(5 + ShopInteraction::kUnconfirmedA[5])]),
                  "re-asking the same question repeats the line and spends no aid");

            // Cancel from QuestWhat / QuestWhom goes back WITHOUT an aid refresh.
            select(m, ctx);
            go(m, 4);
            select(m, ctx);
            shop.rewardsGiven[0] = 8;
            Check(m.OnCancel(ctx) == NpcMenuAction::None && m.Current().FirstLine() == "Aid: 2" &&
                      ItemsAre(m, kNamedMenu),
                  "Cancel from the topic screen returns without an aid refresh");
            go(m, 4);
            select(m, ctx);
            select(m, ctx);
            Check(m.OnCancel(ctx) == NpcMenuAction::None && m.Current().FirstLine() == "Aid: 2" &&
                      ItemsAre(m, kNamedMenu),
                  "Cancel from the whom screen returns without an aid refresh");
            shop.rewardsGiven[0] = 2;

            // Traitor branch: shop 5 IS the traitor (index 0). Suspicion 1 -> 2: guaranteed admission (no RNG).
            player.traitorIndex = 0;
            player.traitorSuspicionCount = 1;
            player.eventFlags.fill(false);
            ask(0, 1);  // whom 1 -> suspect index 2; flag = 0 + 0*3 + 1 = 1; table = 0*4 + 2 = 2.
            Check(player.traitorSuspicionCount == 2 && player.eventFlags[1] && player.eventFlags[72 + 0 * 3 + 1],
                  "the traitor's own shop at suspicion 2 admits: sets the 72+ flag too");
            Check(InfoShows(m, 5, dialogue.groups[9][static_cast<size_t>(5 + ShopInteraction::kUnconfirmedB[2])]),
                  "the admission uses the UNCONFIRMED_B line");

            // Suspicion 3 draws RNG: 20% chance, predicted by a copy of the live RNG stream.
            select(m, ctx);
            player.traitorSuspicionCount = 3;
            player.eventFlags.fill(false);
            shop.rewardsGiven[0] = 4;
            JavaRandom probe = rng;
            bool willAdmit = dawnstar::RandomIntBelow(probe, 100) < 20;
            ask(0, 0);  // suspect 1: flag 0, table 1
            Check(player.eventFlags[72 + 0 * 3 + 0] == willAdmit,
                  "suspicion 3 admits exactly when the predicted RNG draw is < 20");
            Check(InfoShows(m, 5, dialogue.groups[9][static_cast<size_t>(
                                        5 + (willAdmit ? ShopInteraction::kUnconfirmedB[1]
                                                       : ShopInteraction::kUnconfirmedA[1]))]),
                  "suspicion-3 text follows the predicted draw");
            select(m, ctx);

            // Warp: No stays; Yes goes to town.
            go(m, 5);
            select(m, ctx);
            Check(ItemsAre(m, {"Yes", "No"}), "'Warp' opens the Yes/No confirm");
            down(m, 1);
            select(m, ctx);
            Check(ItemsAre(m, kNamedMenu), "'No' returns to the choices");
            go(m, 5);
            select(m, ctx);
            player.currentLevel = 3;
            player.suppressStrafeAdjust = true;
            Check(select(m, ctx) == NpcMenuAction::ReturnToGame, "'Yes' returns to the game");
            Check(player.currentLevel == 1 && !player.suppressStrafeAdjust && !dawnstar::PlayerMovement::HasCampMark(player),
                  "'Yes' warps to the hub and clears suppressStrafeAdjust (skipMark=true: no camp bookmark)");
        }

        // --- F: rendering doesn't crash on any screen. ---
        {
            PlayerState player = basePlayer;
            ShopState shop = ShopState::Reset();
            JavaRandom rng(kSeed);
            int16_t spawn = 1;
            NpcMenuContext ctx = makeCtx(player, shop, rng, spawn);
            NpcMenu m;
            dawnstar::Backbuffer bb;
            m.Open(0, std::string("Welcome."), player, shop);
            m.Render(bb);
            select(m, ctx);
            m.Render(bb);
            select(m, ctx);
            m.Render(bb);
        }

        if (g_ok) {
            std::printf("npc_menu_smoke: all checks passed\n");
            return 0;
        }
        std::printf("npc_menu_smoke: FAILED\n");
        return 1;
    } catch (const std::exception& e) {
        std::printf("npc_menu_smoke: exception: %s\n", e.what());
        return 1;
    }
}
