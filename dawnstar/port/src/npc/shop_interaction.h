#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/shop_dialogue.h"
#include "player/player_state.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Shop.java's own static
// per-game state: firstVisit[9]/questState1[4]/questState2[4]/
// interactionCount[4]/rewardsGiven[4]/showDeathGreeting. `unused` (Shop.
// java's own comment: "Declared, reset in reset(), no confirmed read
// site") is dropped entirely, same treatment M3/M15/M16's own
// no-confirmed-reader fields already got.
//
// Kept as its own struct rather than folded into PlayerState (Shop.java's
// fields are genuinely GLOBAL shop state, not per-character -- e.g.
// firstVisit tracks whether the WORLD has seen a given shop, independent
// of any one Player instance). save/game_save.h's `OtherStateInfo` reserves
// the same 26 values as a flat save container; main.cpp mirrors this struct
// into/out of it around every save/load (M46).
struct ShopState {
    std::array<bool, 9> firstVisit{};
    std::array<int8_t, 4> questState1{};
    std::array<int8_t, 4> questState2{};
    std::array<int16_t, 4> interactionCount{};
    std::array<int16_t, 4> rewardsGiven{};
    bool showDeathGreeting = false;

    // Shop.reset()'s own post-condition: every firstVisit entry true,
    // everything else zero/false.
    static ShopState Reset();
};

// Renamed-source counterpart of ../../../src/Shop.java's own static
// tables/methods: NAMES/SHOP_CATEGORY/SHOP_X/SHOP_Y/SHOP_STOCK/
// RUMOR_STRING_OFFSET, isNamedShop()/isGenericPeddler()/hubShopAt()/
// questFlagsFor()/clearQuestTurnInState()/rumorFor(), and the single
// dialogue() dispatcher -- the "real line-selection logic"
// docs/PORT_ROADMAP.md's own M45 entry names as the gap M8's
// `ShopDialogue` (raw npcstrings.dat text only) and M34's `InteractTick`
// (whose npcInSight branch was a documented no-op stand-in) were both
// left with.
//
// Ported in full -- all of dialogue()'s branches, not just the "talk"
// (action 1) greeting this milestone actually wires into InteractTick --
// same "port the whole self-contained method, verified by smoke test,
// even before every branch has a live caller" standard M13/M14/M25 set.
// The real game's own Ok dispatch on the greeting popup proceeds into
// `NPCChoicesUI[shopId]`, the buy/sell/quest-turn-in/rumor-question menu
// graph -- ported in M46 as ui/npc_menu.h, which drives this class's actions
// 2-5/10-15; main.cpp also mirrors this struct into OtherStateInfo around
// save/load.
//
// `Shop.isValidShopAction()`/`shopActionCode()` are also ported (a
// matched pair the original itself documents as having no confirmed call
// site), for the same "port the whole class" completeness reasoning.
class ShopInteraction {
public:
    static constexpr int kShopCount = 9;

    static const std::array<std::string, kShopCount> kNames;
    // 4 = generic hub peddler (shops 0-3), 2 = Jakar's (shop 3's own
    // "actual" category despite sharing the 0-3 switch group -- see
    // Shop.java's own comment), 1 = named quest shopkeeper (shops 5-8).
    static const std::array<int8_t, kShopCount> kCategory;
    // World position: indices 0-4 fixed (hub town); 5-8 are placeholders
    // ([1,1]) overwritten by DungeonGenerator at world-build time (M6),
    // not read here.
    static const std::array<int8_t, kShopCount> kShopX;
    static const std::array<int8_t, kShopCount> kShopY;
    // Shops 0-3's sellable stock (item ids). Shop 3 ("Jakar's" storefront
    // half) stocks the 87-96 "gift"/special-consumable range.
    static const std::array<std::vector<int8_t>, 4> kStock;
    // [traitorId 0-3][revealStep 0-5] -> offset into dialogue.groups[9]
    // (+5) for that step's rumor-fragment text.
    static const std::array<std::array<int8_t, 6>, 4> kRumorStringOffset;
    // Shop.UNCONFIRMED_A/UNCONFIRMED_B: [qWhat*4 + suspectIndex] -> offset
    // into dialogue.groups[9] (+5) for a "still unconfirmed" line and the
    // traitor's own-admission line, respectively. Used by the Clue Log
    // (ui/options_menu.cpp) and the "Ask a question" menu (ui/npc_menu.cpp).
    static const std::array<int8_t, 24> kUnconfirmedA;
    static const std::array<int8_t, 24> kUnconfirmedB;

    static bool IsNamedShop(int shopId) { return kCategory[static_cast<size_t>(shopId)] == 1; }
    static bool IsGenericPeddler(int shopId) { return kCategory[static_cast<size_t>(shopId)] == 4; }

    // Which of the 5 fixed hub shop positions (x, y) is at, or -1 --
    // Shop.hubShopAt(). Only ever checks indices 0-4 (the fixed hub
    // layout); shops 5-8's real, per-world positions come from
    // GeneratedLevel::specialShopX/Y instead (see player/
    // player_movement.h's own NpcInFront).
    static int HubShopAt(int x, int y);

    // Extracts the 2-bit quest-turn-in flag for shop 5-8 from an item's
    // packed questFlags byte (Item.column(3, itemId)) -- Shop.
    // questFlagsFor(). The Java source reads that byte SIGNED then uses
    // `>>>` (unsigned shift); the extracted 2-bit field only ever depends
    // on the byte's own bits 0-7, so reading it here as a plain unsigned
    // byte first and using an ordinary shift gives the identical result
    // without needing an unsigned-shift emulation.
    static int QuestFlagsFor(int shopId, int itemId, const ItemDatabase& items);

    // Clears both quest-turn-in state arrays -- Shop.
    // clearQuestTurnInState(). Genuinely dead code in the original: it has
    // ZERO real callers anywhere in `../src/` (confirmed directly --
    // `Player.levelUp()`, the plausible-sounding candidate, actually calls
    // `Shop.reset()` instead). Ported anyway for a complete, literal
    // mirror of Shop.java's own public surface, same reasoning as
    // ui/screen.h's own unused `CommandId::Exit`.
    static void ClearQuestTurnInState(ShopState& s);

    // Reveals the next traitor-rumor fragment for `player` at rumor topic
    // `step` (0-5), or repeats the current one with a slightly different
    // phrasing citing the previous reveal count if already revealed once
    // -- Shop.rumorFor(). A real, preserved quirk: `step` indexes
    // directly into `player.skills[step][0]` -- the SAME storage cell
    // Player's real skill-rank tracking (PlayerCombatStats::GainSkillExp)
    // uses for skill `step`'s rank -- so the first 6 named skills'
    // "times this rumor topic was asked about" count and their real
    // combat rank are literally the same field in the original. Ported
    // exactly rather than given separate storage.
    static std::string RumorFor(PlayerState& player, const CharacterData& charData, const ShopDialogue& dialogue,
                                 int step);

    // The single dispatcher for every shop/NPC interaction -- Shop.
    // dialogue(). Returns std::nullopt exactly where the original returns
    // `null` (GameCanvas.openNpcDialogue()'s own null check decides
    // whether to show a greeting popup at all, or -- for Jakar's/shop 4
    // specifically -- open Eustacia's NPCChoicesUI menu instead (ui/npc_menu.h)).
    //
    // `levels` is needed only for action 11 at shop 4 (PlayerMovement::
    // WarpToCampMark's own neighbor-lookup signature); `nextItemSpawnId`
    // only for action 14 (buy) at shops 0-3, mirroring
    // PlayerInventory::GrantStarFrostItem's own explicit-counter-
    // parameter convention (Item.nextSpawnId()'s stand-in, M43) --
    // including that same original quirk: the spawn id is drawn (and the
    // counter advanced) BEFORE checking whether the purchase actually
    // fits in the inventory, so a failed buy still burns a spawn id.
    static std::optional<std::string> Dialogue(PlayerState& player, ShopState& shop, const CharacterData& charData,
                                                const ItemDatabase& items, const ShopDialogue& dialogue,
                                                const std::vector<GeneratedLevel>& levels, int shopId, int action,
                                                int extra, JavaRandom& globalRng, int16_t& nextItemSpawnId);

    // Whether `action` is one of shop `shopId`'s 3 special quest actions
    // -- Shop.isValidShopAction(). Paired with ShopActionCode below; NOT
    // called from Dialogue() above (which uses RollShopOutcome's own
    // skill-check roll instead) -- no confirmed call site in the
    // original either (see Shop.java's own doc comment), kept for a
    // complete, literal port of the class.
    static bool IsValidShopAction(int shopId, int action);
    static int ShopActionCode(int shopId, int choiceIndex);

private:
    // Player.rollShopOutcome(): the named shopkeepers' (5-8) quest-
    // turn-in outcome roll -- skill 13 ("Speechcraft"?) vs. that shop's
    // questState1, +3 bonus if action==3. Kept private here (rather than
    // on PlayerCombatStats, where the original method physically sits)
    // since it genuinely needs BOTH Player state AND Shop's own
    // questState1 -- the same "needs both" reasoning combat/
    // combat_resolution.h's own module split already established,
    // scaled down to a single private helper since Dialogue() is this
    // struct's only real caller.
    static int RollShopOutcome(const PlayerState& player, const CharacterData& charData, const ShopState& shop,
                                int shopId, int action, JavaRandom& globalRng);
};

}  // namespace dawnstar
