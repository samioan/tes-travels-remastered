#pragma once
#include <cstdint>
#include <string>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "player/player_state.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/Player.java's character-
// creation pipeline: applyClassTemplate(classIndex) fused with
// resetState(classIndex, false)'s "new character" branch (giftPoints/
// rumor/warden-lore reset, ailment/timer reset, setHubSpawnPosition(false),
// effectDurations/combat-scratch reset, grantStartingItems()) -- the real
// game calls these from separate steps (class-select, then a confirm
// screen), fused here since this port doesn't model the UI screens between
// them, only the resulting character data. Same fusion dawnstar's own M11
// PlayerCreation does for its own two-step call.
//
// **A real, notable divergence from dawnstar, confirmed by reading this
// entire pipeline end to end:** Stormhold's character creation involves NO
// randomness at all -- no Util.randomInt/ESGame.*Random* call appears
// anywhere in applyClassTemplate(), resetState(classIndex, false),
// setHubSpawnPosition(), or grantStartingItems(). Two starting items and a
// fixed class-driven stat/skill/spell template, deterministically, every
// time -- unlike dawnstar's own character creation, which rolls a hidden
// "traitor index" via Util.randomInt(4) at this exact step. Consistent
// with Shop.java's own header note that Stormhold may have no hidden-
// traitor subplot at all: this milestone confirms the creation path at
// least has no such roll.
class PlayerCreation {
public:
    // `classIndex` is 0-based, matching CharacterData::classNames'/
    // classTemplates' own indexing. `spawnId` models the value
    // Item.nextSpawnId() would return at this point in a real game
    // session -- Item.java's own counter is a single mutable static shared
    // across the whole game (dungeon generation, dropped items, chest
    // loot, ...), which this port doesn't model as persistent state yet
    // (M6's GeneratedChestSpawn/GeneratedMonsterSpawn::spawnId made the
    // same call, a local counter rather than that same global one) -- so
    // the caller supplies it explicitly here rather than this method
    // inventing a fake owner for that global counter. grantStartingItems()
    // calls Item.nextSpawnId() exactly ONCE and reuses that single value
    // for BOTH starting items (not once per item) -- confirmed by reading
    // the whole method, reproduced here exactly.
    static PlayerState CreateCharacter(int classIndex, const std::string& name, int16_t spawnId,
                                        const CharacterData& charData, const ItemDatabase& items);

    // Player.computeDerivedStats(): maxHP/maxMagicka/maxFatigue from
    // attributes[]/classMagickaFactor. Exposed standalone (not just
    // inlined into ApplyClassTemplate/CreateCharacter above) because
    // player/player_leveling.h's own ApplyLevelUpAttributeChoices needs
    // to re-run this exact formula after a level-up attribute boost --
    // confirmed as the SAME real call in the original (Player.java's own
    // resetState() and ESGame.java's level-up-confirm handler both call
    // this one method).
    static void ComputeDerivedStats(PlayerState& p);
};

}  // namespace stormhold
