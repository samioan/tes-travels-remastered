#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace stormhold {

// Renamed-source counterpart of ../../../src/Player.java's runtime instance
// state -- just what character creation (player_creation.h, M9) touches:
// applyClassTemplate(), resetState(classIndex, false)'s "new character"
// branch, setHubSpawnPosition(false), and grantStartingItems(). Not the
// whole class -- movement, combat, dialogue, and the rest of the save
// format's fields are later milestones (see docs/PORT_ROADMAP.md's "what's
// next").
//
// Unlike dawnstar's own PlayerState, there's no `gold`/currency field here
// at all -- Stormhold genuinely has no economy (Shop.java's own header
// comment: "No buy/sell (action 14/15-style) branch exists anywhere in
// this file"). Player.java's own debug-summary method DOES print something
// labeled "gold", but reading that whole method shows it's actually
// printing `inventoryCount` under a stale/copy-pasted label, not a real
// currency value -- not ported here.
struct PlayerState {
    std::string name;
    int16_t classIndex = 0;
    int16_t raceIndex = 0;

    // level, exp, curHP, maxHP, curMagicka, maxMagicka, curFatigue,
    // maxFatigue, and two more slots ([8]/[9]) whose exact use isn't
    // pinned down yet in ../../../src/Player.java itself.
    std::array<int16_t, 10> coreStats{};
    int8_t levelUpAttributeFlags = 0;
    // 8 attributes as base+bonus pairs (attributes[2*i]=base,
    // attributes[2*i+1]=bonus, the latter always 0 fresh out of character
    // creation).
    std::array<int16_t, 16> attributes{};
    int16_t classMagickaFactor = 0;
    std::array<int16_t, 2> classUnknownPair{};
    // Skills as threshold/rank/exp-toward-next-rank triples.
    std::array<std::array<int16_t, 3>, 14> skills{};

    int8_t inventoryCount = 0;
    // Item-type ids, negative = currently equipped.
    std::array<int8_t, 24> inventoryItemIds{};
    // Packed (spawnId << 16) | (int8_t)charge per inventory slot, matching
    // Player.java's addInventoryItemRaw() exactly.
    std::array<int32_t, 24> inventoryItemData{};
    // Equipped item id per equip slot, indexed by
    // ItemDatabase::EquipSlotOf() -- **not** by item category (see
    // ../../../src/Player.java's own equipItem()/unequipMatchingCategory()
    // header comment: a real phase-1 renaming bug found, and fixed in the
    // renamed source directly, while porting this milestone).
    std::array<int8_t, 7> equippedItems{};

    uint32_t knownSpellsMask = 0;
    int8_t selectedSpellId = 0;

    int16_t giftPointsFound = 0;
    int16_t rumorRevealStep = 0;
    int16_t wardenLoreStep = 0;
    int8_t ailmentMask = 0;
    int16_t vampirismTimer = 0;
    int16_t manaBurnTimer = 0;
    int16_t terrifiedTimer = 0;

    // Hub-town spawn position -- setHubSpawnPosition(false)'s "new
    // character" branch: (9, 10), facing 1. (isRespawn=true's DIFFERENT
    // death/respawn point, (12, 14), isn't reachable at character
    // creation -- a later movement/death milestone's job.)
    int8_t currentLevel = 1;
    int8_t tileX = 9;
    int8_t tileY = 10;
    int8_t facing = 1;
    int8_t pendingLevel = 1;
    int8_t pendingTileX = 9;
    int8_t pendingTileY = 10;
    int8_t pendingFacing = 1;

    int8_t campLevel = 0;
    int8_t campX = 0;
    int8_t campY = 0;
    int8_t campFacing = 0;

    std::array<int8_t, 25> effectDurations{};
    int16_t lastCombatTargetId = 0;
    int16_t spellArmorBonus = 0;
    bool increaseHarmBuff = false;
    bool increaseArmorBuff = false;
    bool safeCampingBuff = false;

    // --- M10: touched by player/player_movement.h.
    int8_t prevTileX = 0;
    int8_t prevTileY = 0;
    bool crossingLevelBoundary = false;
    // Set by PlayerMovement::CommitMove -- see that method's own header
    // comment on `enteredNewLevelZone` and a real finding about
    // `leftLevelZone`: it can NEVER actually become true given
    // commitMove()'s own control flow (confirmed by reading the whole
    // method), even though the original faithfully computes it as if it
    // could.
    bool enteredNewLevelZone = false;
    bool leftLevelZone = false;

    // --- M12: touched by player/player_inventory.h. Set true by both
    // MarkCampAndReturnToTown and WarpToCampMark; campLevel/campX/campY/
    // campFacing above (already added at M9) are the actual camp
    // bookmark itself.
    bool justMarkedCamp = false;

    // --- Deferred to a later milestone, none touched by character
    // creation, core movement, or inventory: the corridor-occlusion view
    // grid (corridorView, PlayerMovement doesn't call
    // refreshCorridorView() yet -- see its own header comment), UI/
    // rendering-only scratch state (endOfGameTriggered, stateByteAb), and
    // visibleObjects (the 13-slot "what's renderable this frame" cache).
};

}  // namespace stormhold
