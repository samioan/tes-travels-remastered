#pragma once
#include <array>
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

    // Player.characterSummaryShort() (M40, for the new port-only class-
    // confirm "See Class Info" screen -- ui/menu_flow.h): race/class,
    // then HP/Magicka/Fatigue (PlayerCombatStats::EffectiveStat), then
    // all 8 attributes, then every skill with a nonzero rank, one
    // '\n'-separated entry per line, matching the original's own field
    // order exactly. Uses a plain space rather than the original's ": "
    // between label and value -- graphics/bitmap_font.h has no colon
    // glyph, and every value here is presentation text only (not a
    // byte-exact save/wire format), so this is a harmless port-only
    // formatting simplification.
    static std::string CharacterSummaryShort(const PlayerState& p, const CharacterData& charData);

    // Player.normalizeToMaxStats(short[] stats) (M47, phase-3 port): sets
    // curHP/curMagicka/curFatigue to their own max columns and zeroes
    // stats[8] (an unconfirmed 10th slot, same "no confirmed meaningful
    // use" gap PlayerState::coreStats' own comment already flags for
    // indices 8/9). Generic over any 10-element stats array, matching the
    // real method's own generic `short[]` signature -- the original has
    // TWO call sites (player/death_sequence.h's own respawn handling,
    // mutating `p.coreStats` directly; and Player.toBytes(false)'s
    // lightweight summary-save format, mutating a throwaway scratch COPY
    // instead), only the first of which this port wires (player/
    // player_save.h's own header comment explicitly defers the `full=
    // false` summary format to a later milestone).
    static void NormalizeToMaxStats(std::array<int16_t, 10>& stats);

    // Player.resetState(classIndex, true) -- the "respawn" branch (as
    // opposed to CreateCharacter's own resetState(classIndex, false) "new
    // character" branch above). `classIndex` is accepted by the real
    // method but confirmed NEVER used in its body (same as the `full=
    // false` branch) -- no parameter here either. Resets ailmentMask/
    // vampirismTimer/manaBurnTimer/terrifiedTimer/unconfirmedFlag2,
    // setHubSpawnPosition(true) (the DISTINCT (12, 14) death/respawn hub
    // point -- see PlayerInventory::MarkCampAndReturnToTown's own
    // identical inline write), and effectDurations/combat-scratch
    // (lastCombatTargetId/spellArmorBonus/increaseHarmBuff/
    // increaseArmorBuff/safeCampingBuff), exactly like the `full=false`
    // branch does -- but, confirmed by reading resetState()'s own `if
    // (!full)` guards directly, SKIPS giftPointsFound/rumorRevealStep/
    // wardenLoreStep (preserved across death) AND campLevel/campX/campY/
    // campFacing (the camp mark itself survives death) AND
    // grantStartingItems() (the player keeps whatever equipped gear
    // survived death/player/death_sequence.h's own strip-unequipped-
    // inventory pass, run by the caller immediately before this).
    //
    // NOT modeled: refreshCorridorView() -- same already-documented gap
    // player/player_movement.h's own RefreshCorridorView comment flags
    // for resetState()/MarkCampAndReturnToTown/WarpToCampMark alike;
    // `p.corridorView` stays stale (describing wherever the player died)
    // until their next real move after respawning.
    static void RespawnAfterDeath(PlayerState& p);
};

}  // namespace stormhold
