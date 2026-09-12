#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace dawnstar {

// Renamed-source counterpart of Player.java's visibleObjects slot
// markers (EMPTY_SLOT/WALL_BLOCKED_SLOT/OCCLUDED_SLOT are distinguished
// there by Integer reference identity; a tagged enum is the natural C++
// equivalent). See player/visible_objects.h (M25) for the system that
// populates this.
enum class VisibleSlotKind : uint8_t { Empty, WallBlocked, Occluded, Monster, Chest, DroppedItem, Npc };

// One of Player.visibleObjects' 13 slots. Only one of the three record
// fields is meaningful, selected by `kind` (Monster/Chest/DroppedItem);
// `npcShopIndex` is meaningful only for kind==Npc.
struct VisibleSlot {
    VisibleSlotKind kind = VisibleSlotKind::Empty;
    std::array<uint8_t, 28> monsterRecord{};
    std::array<uint8_t, 8> chestRecord{};
    std::array<uint8_t, 7> droppedItemRecord{};
    // Shop.SHOP_X/Y's own index (0-4 = the hub town's 5 fixed peddlers,
    // 5-8 = the single named shopkeeper on levels 3/12/21/30) -- matches
    // GameCanvas.paintNpcPortrait's own switch(shopId). No dialogue/
    // quest-state Shop class is ported yet (M8's ShopDialogue only holds
    // the dialogue text) -- this is only the minimal position/id data
    // placeVisibleObject itself needs.
    int npcShopIndex = -1;
};

// Renamed-source counterpart of ../../../src/Player.java's runtime
// instance state: what character creation (player_creation.h) touches
// (stats, attributes, skills, starting inventory/equipment, starting
// known spells, hub-town position) plus the rest of the "full" save
// format's fields (player_save.h, M12). Not the whole class --
// movement, combat, spellcasting, and dialogue are all separate, later
// milestones (see docs/PORT_ROADMAP.md).
struct PlayerState {
    std::string name;
    int classIndex = 0;
    int raceIndex = 0;

    // level, levelExp, curHP, maxHP, curMagicka, maxMagicka, curFatigue,
    // maxFatigue, and two more slots ([8]/[9]) whose use is unconfirmed
    // in the Java source itself -- see Player.java's own field comment.
    std::array<int16_t, 10> coreStats{};
    int8_t attributeIncreaseFlags = 0;
    int gold = 0;
    // 8 attributes as base+bonus pairs (attributes[2*i]=base,
    // attributes[2*i+1]=bonus).
    std::array<int16_t, 16> attributes{};
    int16_t classMagickaFactor = 0;
    std::array<int16_t, 2> classUnknownPair{};
    // Skills as rank/bonus/exp-toward-next-rank triples.
    std::array<std::array<int16_t, 3>, 14> skills{};

    int inventoryCount = 0;
    // Item-type ids, negative = currently equipped.
    std::array<int8_t, 24> inventoryItemIds{};
    // Packed value/charge per inventory slot: (spawnIdOrPacked << 16) +
    // (int8_t)charge, matching Player.java's addInventoryItem() exactly.
    std::array<int32_t, 24> inventoryItemData{};
    // Equipped item-type per equip slot, indexed by Item's equip-slot
    // column (0=weapon,1=shield/offhand,2..6=armor slots).
    std::array<int8_t, 7> equippedItems{};

    // Bitmask of known spell ids (bit (id-1) set = known).
    uint32_t knownSpellsMask = 0;
    int8_t selectedSpellId = 0;

    // Rolled 0-3 at character creation -- the hidden "traitor" index the
    // mystery-subplot rumor system gradually reveals (Shop.java's
    // RUMOR_STRING_OFFSET[traitorIndex]).
    int8_t traitorIndex = 0;

    int currentLevel = 1;
    int tileX = 9;
    int tileY = 9;
    int facing = 1;

    // --- M12: the rest of the "full" save-format's fields (see
    // player_save.h). Not touched by character creation -- all left at
    // Player.java's own field-declaration defaults, matching a freshly
    // `new Player(ESGame)`'d instance before resetState/grantStartingItems.

    // Accumulated from auto-collected category-11 ("gift") dropped items.
    int16_t giftPointsFound = 0;
    // Rumor-reveal-step counter for Shop's mystery-subplot hint feed.
    int16_t rumorRevealStep = 0;
    // 8-bit active-ailment mask.
    int8_t ailmentMask = 0;
    int16_t trollThirstTimer = 0;
    int16_t glacierCurseTimer = 0;
    int16_t terrifiedTimer = 0;
    // Read/written but never observed being used meaningfully.
    bool unconfirmedZ = false;
    // Camp/warp bookmark (level, x, y, facing).
    int8_t campLevel = 0;
    int8_t campX = 0;
    int8_t campY = 0;
    int8_t campFacing = 0;
    // Generic spell/effect duration timers, -1=until cured, -2=until a
    // condition check rather than a countdown.
    std::array<int8_t, 25> effectDurations{};
    // Current combat target's spawnId.
    int16_t combatTargetSpawnId = 0;
    // Scratch magnitude for the effectDurations[17] buff.
    int16_t tempArmorBonus = 0;
    // "Increase Harm"/"Increase Armor"/"Safe Camping" buffs.
    bool increaseHarmBuff = false;
    bool increaseArmorBuff = false;
    bool safeCampingBuff = false;
    // Counts (capped at 3) how many times the player has asked the
    // actual traitor's shop about a topic; packed into the save format
    // alongside traitorIndex (see player_save.cpp's ToBytes/FromBytes --
    // this packing has a faithfully-preserved original-game bug).
    int8_t traitorSuspicionCount = 0;
    // Set once the type-41 "roaming" special monster has been dealt with.
    bool specialEncounterResolved = false;
    // True while the type-41 "roaming" special monster is believed alive
    // on the current level.
    bool roamingSpecialMonsterPresent = false;
    // General-purpose one-time event/dialogue flags (indices 90-95 are
    // Shop's rumor-reveal-step-shown markers; see Player.java's own
    // field comment for the rest).
    std::array<bool, 96> eventFlags{};

    // --- M13: fields movement (player_movement.h) touches, not part of
    // the save format (Player.java doesn't (de)serialize any of these --
    // they're either scratch or trivially re-derived on load).

    // Previous tile, for the "just arrived here" chest/item auto-trigger
    // check -- set by every successful move, but not yet read anywhere
    // in this port (that check itself is deferred, see
    // player_movement.h's class comment).
    int8_t prevTileX = 0;
    int8_t prevTileY = 0;
    // Corridor tile-occlusion view grid (9 wide x 5 deep), populated by
    // PlayerMovement's RefreshCorridorView -- read by the first-person
    // corridor renderer (M9/M10) and, eventually, the minimap.
    std::array<std::array<uint8_t, 5>, 9> corridorView{};
    // One-shot flag set after a camp-mark/warp/death-reset (none ported
    // yet, so always false in practice), consumed by the next Move()
    // call to skip its strafe turn-back step.
    bool suppressStrafeAdjust = false;

    // --- M25: Player.visibleObjects (see player/visible_objects.h).
    // Java declares this `static` (shared/global -- a MIDP-era
    // single-player-only design choice); folded into PlayerState here
    // like every other per-player field in this port rather than kept
    // as a global, since there's only ever one player either way.
    std::array<VisibleSlot, 13> visibleObjects{};

    // --- M14: touched by PlayerCombatStats::GainSkillExp. Consumed
    // elsewhere by ESGame's level-up UI, not ported yet.
    bool levelUpPending = false;
    // Set once grantStarFrostItem has run (that method isn't ported
    // yet); read by SkillValue's +4 bonus.
    bool starFrostBonusActive = false;
};

}  // namespace dawnstar
