#pragma once
#include "assets/character_data.h"
#include "assets/item_database.h"
#include "player/player_state.h"
#include "util/java_random.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Player.java's character-
// creation pipeline: applyClassTemplate() -> resetState(false) [minus
// its roaming-special-monster cleanup, a no-op for a freshly-created
// character anyway since that field starts false] ->
// resetToHubPosition(false) -> grantStartingItems() (addInventoryItem +
// auto-equip via equipItem, both now in player/player_inventory.h). See
// docs/PORT_ROADMAP.md's M11 entry.
class PlayerCreation {
public:
    // `globalRng` models ESGame.r, the shared game-wide RNG the real
    // traitor-index roll (Util.randomInt(4), the no-Random-argument
    // overload) draws from -- seeded from System.currentTimeMillis() in
    // the original, so unlike DungeonGenerator's per-level Random this
    // was never meant to be reproducible bit-for-bit; callers decide how
    // to seed it. `characterClass` is 0-based (classIndex), matching
    // CharacterData.classNames'/classTemplates' own indexing.
    static PlayerState CreateCharacter(int characterClass, const std::string& name,
                                       const CharacterData& charData, const ItemDatabase& items,
                                       JavaRandom& globalRng);
};

}  // namespace dawnstar
