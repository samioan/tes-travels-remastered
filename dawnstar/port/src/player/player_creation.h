#pragma once
#include <string>

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

    // Player.java's private computeStartingSpellMask() -- exposed
    // publicly so player/player_save.h's ToBytesSummary can call it
    // against a live (already-created) character, exactly as
    // Player.java's toBytes(false) does. See player_creation.cpp's doc
    // comment on the definition for the mutation-as-a-side-effect
    // consequence of that.
    static uint32_t ComputeStartingSpellMask(PlayerState& p, const CharacterData& charData);

    // Player.java's buildCreationSummary() -- M40's own "See Class
    // Info" preview screen (ui/character_creation_flow.h) reads this;
    // exposed here (not in that UI file) since it's real Player.java
    // gameplay logic, not UI, matching player/player_combat_stats.h's
    // own EffectiveStat/HasAilment precedent of small, self-contained,
    // reusable Player.java ports living next to the class they came
    // from rather than the one UI screen that happens to call them.
    static std::string BuildCreationSummary(const PlayerState& p, const CharacterData& charData);

    // Player.java's recalcMaxStats(): max HP/Magicka/Fatigue from the
    // current attributes. Public since M49's level-up menu re-runs it after
    // the attribute increases (ESGame's secondaryParam 39 branch).
    static void RecalcMaxStats(PlayerState& p);
};

}  // namespace dawnstar
