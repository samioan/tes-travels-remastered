#pragma once
#include <optional>
#include <string>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/shop_dialogue.h"
#include "player/player_state.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"
#include "world/shop_state.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/Shop.java's dialogue()
// dispatcher -- SCOPED to shops 0-3 (the quest-turn-in shopkeepers:
// Arantamo/Celegil/Favela Dralor/Vander, `Shop::IsQuestShop`, world/
// shop_state.h) only. Shops 4 (Beneca)/5 (Helga)/6 (Varus) are each
// bespoke single-NPC branches sharing no pattern with this one or each
// other (Shop.java's own header comment already says so) -- deliberately
// left for later milestones, one coherent slice at a time, the same
// "primitive first, wired later, don't do it all at once" shape this
// project's own M53/M54/M55 trio already used for ShopState itself. This
// closes the bulk of the remaining gap docs/PORT_ROADMAP.md's "what's
// next" flagged after M55 -- talkToNpc() still can't be wired for real
// until shops 4-6 exist too.
//
// Lives in stormhold_player, not alongside world/shop_state.h's own
// `Shop` class (stormhold_world) -- this needs `PlayerState` directly,
// and stormhold_world cannot depend on stormhold_player (the dependency
// runs the other way: stormhold_player already links stormhold_world),
// so this can't live any lower in the layer stack than here.
class ShopInteraction {
public:
    // Shop.isValidShopAction(shopId, action) / Shop.shopActionCode(shopId,
    // choiceIndex): a matched pair, confirmed shops-0-3-ONLY by reading
    // Shop.java directly -- the original's own switch has no case at all
    // for shopId 4-6, falling straight to `default: return false` / `-1`.
    // No confirmed call site anywhere in Shop.java itself either way
    // (likely ESGame's own menu-building code, same caveat this project's
    // own M11 `ShopDialogue` note already carries for its data).
    static bool IsValidShopAction(int shopId, int action);
    static int ShopActionCode(int shopId, int choiceIndex);

    // Shop.rumorFor(player, step): reveals the <TAG>-templated rumor-pool
    // fragment (`text.groups[7][1]`/`[2]`) naming skill `step`
    // (`charData.skillNames[step]`). **A real, confirmed original quirk,
    // ported exactly rather than "fixed":** the per-step ask counter this
    // reads/writes (`player.skills[step][0]`) is the SAME storage cell
    // that skill's own real combat rank lives in (PlayerCombatStats::
    // SkillValue/GainSkillExp, M13/M15) -- not separate bookkeeping.
    static std::string RumorFor(PlayerState& player, const CharacterData& charData, const ShopDialogue& text,
                                 int step);

    // Shop.dialogue(player, shopId, action, extra) -- shops 0-3 only (see
    // class comment). Returns std::nullopt wherever the original returns
    // `null` (no branch matches for this action). `hub` is
    // `ESGame.dungeons[0]` (action 6's own tile-bit-32 clear at this
    // shop's own world position, `Shop::kShopX`/`kShopY`); `rng` models
    // the shared game-wide RNG `rollShopOutcome`/`lingoRandomInt` draw
    // from, same caller-supplies-the-stream pattern
    // `PlayerCombatStats::RollOutcome` already established.
    // Precondition: 0 <= shopId <= 3 (`Shop::IsQuestShop(shopId)`) --
    // shops 4-6 are NOT handled here.
    static std::optional<std::string> QuestShopDialogue(PlayerState& player, ShopState& shop,
                                                          const ShopDialogue& text, const CharacterData& charData,
                                                          const ItemDatabase& items, GeneratedLevel& hub,
                                                          JavaRandom& rng, int shopId, int action, int extra);
};

}  // namespace stormhold
