#pragma once
#include <optional>
#include <string>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/shop_dialogue.h"
#include "player/player_state.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"
#include "world/game_advancement.h"
#include "world/shop_state.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/Shop.java's dialogue()
// dispatcher, ported one NPC-group at a time (docs/PORT_ROADMAP.md's own
// "what's next" note after M55 flagged the whole dispatcher as "worth its
// own multi-part treatment" rather than one milestone, the same "primitive
// first, wired later, don't do it all at once" shape this project's own
// M53/M54/M55 trio already used for ShopState itself):
// - M56: `QuestShopDialogue`, shops 0-3 (the quest-turn-in shopkeepers:
//   Arantamo/Celegil/Favela Dralor/Vander, `Shop::IsQuestShop`).
// - M57: `BenecaDialogue`, shop 4.
// - M58 (this milestone): `HelgaDialogue`, shop 5.
// Shop 6 (Varus) is its own bespoke single-NPC branch, sharing no pattern
// with any of the above (Shop.java's own header comment already says so)
// -- still deliberately left for a later milestone. talkToNpc() can't be
// wired for real into the live port until shop 6 exists too.
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

    // Shop.dialogue(player, 4, action, extra) -- shop 4 (Beneca) only.
    // Returns std::nullopt wherever the original returns `null`.
    // `spawnIdCounter` models `Item.nextSpawnId()`'s global counter, same
    // caller-supplies-the-counter pattern every other spawn site in this
    // port already uses (combat/combat_resolution.h, dungeon/
    // dungeon_runtime.h) -- pre-incremented exactly like those (`++
    // spawnIdCounter`), not reproduced as a ported static.
    //
    // **A real naming trap in Shop.java itself, confirmed by reading
    // Player.addInventoryItemRaw(int itemId, int packedValue, int charge)'s
    // own real signature, not assumed from the local variable's name:** the
    // original's action==7 branch reuses the identifier `slot` for its
    // local (copy-pasted from the action==4 branch just above it, where
    // `slot` genuinely IS an inventory slot index) -- but here it's passed
    // as `addInventoryItemRaw`'s FIRST parameter, `itemId`, not a slot at
    // all. So action 7's own `extra` argument is really the ITEM ID being
    // requested as a training reward, matching the plain
    // `addInventoryItemRaw(itemId, packedValue, extra)` call shape used
    // everywhere else in `../../../src/Player.java`. Named `itemId` here,
    // not `slot`, to not carry the original's own misleading name forward.
    static std::optional<std::string> BenecaDialogue(PlayerState& player, ShopState& shop, const ShopDialogue& text,
                                                       const ItemDatabase& items, int16_t& spawnIdCounter, int action,
                                                       int extra);

    // Shop.dialogue(player, 5, action, extra) -- shop 5 (Helga) only.
    // Returns std::nullopt wherever the original returns `null`. By far the
    // busiest single-NPC branch in the original (8 distinct actions: greet/
    // rumor-reveal, a repeat-rumor query, item-quality turn-in, item-charge
    // init, a one-shot safe-camping buff, ailment cure, camp-mark warp, and
    // a full HP/Magicka heal) -- no shared pattern with `BenecaDialogue` or
    // `QuestShopDialogue` at all, confirmed by reading the whole branch
    // directly, matching `Shop.java`'s own header comment that Helga is her
    // own bespoke economy.
    //
    // **A real double-gate, easy to flatten into a single call by mistake:**
    // action 8's item-charge branch checks `Item.isEquipmentCategory(itemId)
    // && !player.isItemCharged(slot)` BEFORE calling `initializeItemCharge`
    // -- `PlayerInventory::InitializeItemCharge` (M12) already re-checks the
    // equipment-category half internally and would happily re-stamp an
    // ALREADY-charged item's charge back to the same value 3, silently
    // masking the "already charged" case if this method only checked
    // `InitializeItemCharge`'s own return value instead of replicating both
    // original conditions explicitly.
    //
    // `Player.itemSubtypeAtSlot(slot)` (action 4's own "quality tier" read)
    // is a one-line `Item.column(2, itemId)` in the original -- inlined
    // directly here as `items.subtype[itemId - 1]` rather than given its own
    // wrapper method, matching how `QuestShopDialogue`/`BenecaDialogue`
    // already read `items.category`/`items.questFlags` directly for
    // equally one-line original reads.
    static std::optional<std::string> HelgaDialogue(PlayerState& player, ShopState& shop, const ShopDialogue& text,
                                                      const ItemDatabase& items, int action, int extra);
};

}  // namespace stormhold
