#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace stormhold {

// M27: renamed-source counterpart of Player.java's visibleObjects slot
// markers -- EMPTY_SLOT/BLOCKED_SLOT/SHADOWED_SLOT there are distinguished
// by Integer reference identity (SLOT_EMPTY=new Integer(0), SLOT_BLOCKED=
// new Integer(1), SLOT_SHADOWED=new Integer(-1)); a tagged enum is the
// natural C++ equivalent, same treatment dawnstar's own identical M25
// milestone already gave its equivalent field. `Warden` has no associated
// record -- Player.refreshVisibleObjects() places the literal String "W"
// there directly (resolveVisibleObjectSlot(5, "W")), never going through
// placeVisibleObject at all (see player/visible_objects.h's own header
// comment), so there's no byte record to carry.
enum class VisibleSlotKind : uint8_t { Empty, Blocked, Shadowed, Monster, Chest, DroppedItem, Warden };

// One of Player.visibleObjects' 13 slots. Only one of the three record
// fields is meaningful, selected by `kind`.
struct VisibleSlot {
    VisibleSlotKind kind = VisibleSlotKind::Empty;
    std::array<uint8_t, 28> monsterRecord{};
    std::array<int8_t, 8> chestRecord{};
    std::array<int8_t, 7> droppedItemRecord{};
};

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
    // --- M20: touched by player/player_save.h. Player.java's own header
    // comment: "no confirmed meaningful read/write site beyond
    // (de)serialization" -- part of the real save-format byte layout
    // (always written/read, always 0 out of applyClassTemplate()) with no
    // other confirmed producer/consumer anywhere in ../../../src/.
    int32_t unconfirmedIntField = 0;
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
    // --- M20: touched by player/player_save.h. Player.java's own header
    // comment: reset alongside the ailment/timer group in resetState(),
    // "no other confirmed use" beyond that and (de)serialization -- same
    // class of gap as unconfirmedIntField above.
    bool unconfirmedFlag2 = false;

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

    // --- M17: touched by player/player_movement.h's CommitMove. Player
    // .java's own `pendingLockedItemFlag` is a STATIC field (class-level,
    // not per-instance) -- modeled here on PlayerState anyway since this
    // port only ever has one live Player at a time, the same reasoning
    // that already applies to every other "really static in the original"
    // field this port carries on PlayerState. Set true (and CommitMove
    // returns early, skipping the rest of its own dropped-item handling)
    // when the tile's dropped item is flagged locked (record[6] bit 4).
    bool pendingLockedItemFlag = false;

    // --- M25: touched by player/player_movement.h's CommitMove and
    // render/game_renderer.h. Player.corridorView's own shape (`byte[9][5]`
    // -- dungeon/dungeon_runtime.h's CorridorViewGrid alias, matched
    // structurally here rather than by #including that header directly,
    // to keep this otherwise-lightweight foundational header free of
    // dungeon_runtime.h's much heavier transitive include graph;
    // CorridorViewGrid IS this exact type, a plain `using` alias, so no
    // conversion is needed at call sites). Refreshed by
    // PlayerMovement::RefreshCorridorView wherever Player.java's own
    // refreshCorridorView() runs -- see that method's own header comment
    // for which of the original's 6 call sites are (CommitMove) and
    // aren't yet (character creation, MarkCampAndReturnToTown/
    // WarpToCampMark) wired.
    std::array<std::array<uint8_t, 5>, 9> corridorView{};

    // --- M27: touched by player/visible_objects.h. Player.visibleObjects
    // itself (a Java `static Vector`) -- the 13-slot "what's renderable
    // this frame" cache `paintObjects()`/`paintMonsters()` (M22) need,
    // moved from a class-level static into PlayerState, same harmless
    // single-player-instance simplification every other Java-`static`-
    // but-really-per-player field on this struct already gets.
    std::array<VisibleSlot, 13> visibleObjects{};

    // --- Deferred to a later milestone, none touched by character
    // creation, core movement, or inventory: UI/rendering-only scratch
    // state (endOfGameTriggered, stateByteAb).
};

}  // namespace stormhold
