#pragma once
#include <cstdint>
#include <optional>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/spell_database.h"
#include "dungeon/dungeon_runtime.h"
#include "monster/monster_state.h"
#include "player/player_state.h"
#include "util/java_random.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/Player.java's castOnSelf(int)/
// castOnMonster(int, Monster)/cycleSelectedSpell(), plus
// ../../../src/GameCanvas.java's resolveSpellCastInput(long)/
// resolveSpellCycleInput(long) (both phase-3 port M41, decompiled/e.java's
// h(long)/g(long)) -- M41's own dispatch web finally gets its spell-casting
// half. Same "player+monster paired action, its own module" placement as
// combat/combat_resolution.h (castOnMonster needs a live MonsterState +
// WorldRegistry for target.store(), and its case-14 branch calls
// CombatResolution::PlayerAttack directly -- Player.attack(target) itself,
// confirmed from decompiled/j.java's own case-14 body -- so this module
// depends on combat_resolution.h, never the reverse); castOnSelf doesn't
// need a Monster at all but is kept alongside it since GameCanvas dispatches
// both from the exact same resolveSpellCastInput() call, same reasoning
// resolveSpellCastInput/resolveSpellCycleInput's own Java header comments
// give for staying close together.
//
// **Real, deliberately-preserved inconsistency, confirmed by reading
// castOnMonster's whole switch side by side:** only 5 of its 9
// scratch[]-mutating cases (4, 11, 12, 13, 15) also call target.store() --
// cases 10, 16, 18, and 19 mutate target.scratch[] and/or
// player.effectDurations[] but never persist the monster back to the
// registry. A real asymmetry in the original (near-certainly just missed
// call sites, not intentional), preserved exactly rather than "fixed" by
// adding the missing DungeonRuntime::StoreMonster calls -- same
// "confirmed real bug, preserved" discipline as world/dungeon_generator.h's
// dead chest-record byte and world/warden.h's wardenLeaves() index bug.
//
// **Real, confirmed DEAD BRANCH, found by dumping spellsin.dat's actual 25
// rows (assets/spell_database.h) while scoping this milestone:** spell id
// 17 ("Sanctuary") has school=1 in the real data, NOT school=2 -- meaning
// Spell.isOffensive(17) is always false, so ResolveSpellCastInput's own
// offensive/self dispatch NEVER routes spell 17 into CastOnMonster at all;
// it always goes to CastOnSelf instead, where spell 17 falls into that
// switch's own big empty/no-op case block. So CastOnMonster's own case 17
// (`effectDurations[16]=-2; spellArmorBonus=10+skillValue(6,false);` --
// itself clearly written as a DEFENSIVE self-buff, an odd thing for the
// "offensive spell" method to contain in the first place) is confirmed
// UNREACHABLE through any real play session. Transcribed exactly anyway,
// same "port a confirmed-dead branch faithfully, don't delete it" treatment
// M40's own classInfoUI dead-cmdBack-branch finding already established.
//

// **NAMING WARNING:** unlike PlayerCombatStats::RollOutcome's own doc
// comment ("rollOutcome(atkChance, defChance)"), castOnSelf calls it as
// `rollOutcome(defendChance, attackChance)` -- defendChance FIRST, the
// reverse of the "normal" attacker-first order castOnMonster itself uses
// (`rollOutcome(attackChance, defendChance)`). Confirmed directly from
// decompiled/j.java's own `p(int)`, not a transcription slip -- preserved
// exactly (see CastOnSelf's own body below), matching this port's "field
// read doesn't match its own name" preservation precedent
// (player/player_combat_stats.h's own IsEffectActive comment; Player.java's
// own header comment on castOnSelf flags the analogous school/
// durationMultiplier field-read oddity the same way).
class SpellCasting {
public:
    // Player.spellSkillIndexFor(spellId): which of the 14 skills governs a
    // given spell id (1-5 -> skill 1, 6-10 -> skill 3, 11-15 -> skill 4,
    // 16-20 -> skill 6, 21+ -> skill 10). Pure, spell-id-range-keyed, no
    // state.
    static int SpellSkillIndexFor(int spellId);

    // Player.castOnSelf(spellId): rolls a hit tier (defendChance,
    // attackChance -- see class comment's NAMING WARNING) fed by
    // SpellSkillIndexFor's governing skill vs. the spell's OWN icon/
    // durationMultiplier fields (icon feeds the "difficulty" half,
    // durationMultiplier feeds the "attack" half -- NOT school, despite
    // school being the field whose name would fit; Player.java's own
    // header comment already flags this exact finding), spends Magicka
    // scaled by tier (miss=3x, graze=1.5x/1x, clean hit=1x but double
    // magnitude), and on anything but a miss dispatches a big per-spellId
    // switch for the self-targeted effect (buffs via effectDurations[],
    // a flat heal for spell 21, ailment-mask writes for 23/24, repeated
    // PlayerCombatStats::CureRandomAilment calls for 25, and a
    // grant-and-equip-a-scroll branch for spell 6 via
    // PlayerInventory::AddInventoryItemRaw(109,0,0)+EquipLastPickedUpItem).
    // Always ends with a flat 5-per-tier-multiplier Fatigue cost (tripled
    // while ailment 1 is active) and a self-damage tick while ailment 6
    // ("Wounded"-shaped) is active -- the SAME trailing block
    // castOnMonster's own body repeats verbatim (see below), matching the
    // original's own two separate, near-identical copies rather than
    // factored into a shared helper.
    static void CastOnSelf(PlayerState& p, int spellId, const SpellDatabase& spells, const CharacterData& charData,
                            const ItemDatabase& items, JavaRandom& rng);

    // Player.castOnMonster(spellId, target): the offensive counterpart of
    // CastOnSelf -- same tier/Magicka-cost shape, but the hit-tier roll
    // feeds off the TARGET's own offense/defense stat columns (10/9), and
    // reads no icon/durationMultiplier at all (confirmed directly against
    // decompiled/j.java's `b(int, d)`: no school/duration mixup here,
    // unlike CastOnSelf). Its own local `Spell.power` read is confirmed
    // NEVER used anywhere in the whole switch (every damage case derives
    // its own magnitude from skill values/targetOffense instead) -- a real
    // dead read in the original, not reproduced (see the .cpp's own
    // comment at the read site). Case 14 (`this.attack(target)`) calls
    // CombatResolution::PlayerAttack directly with effectDurations[13]
    // forced to -1 (always-active) around the call and reset to 0
    // afterward -- a real, if odd, "spell that just triggers a normal
    // weapon attack" effect, transcribed exactly. See the class comment
    // for the confirmed target.store() asymmetry across cases
    // 10/11/12/13/15/16/18/19.
    static void CastOnMonster(PlayerState& p, int spellId, MonsterState& target, const SpellDatabase& spells,
                               const CharacterData& charData, const ItemDatabase& items,
                               const MonsterDatabase& monsterDb, JavaRandom& rng, WorldRegistry& world);

    // Player.cycleSelectedSpell(): a PURE query, does NOT mutate
    // p.selectedSpellId itself (ResolveSpellCycleInput below does that,
    // matching resolveSpellCycleInput()'s own separate assignment) --
    // returns the NEXT known spell id after the currently-selected one
    // (wrapping around Spell.count, skipping unknown spells via
    // knownSpellsMask), the player's OWN first known spell id if
    // selectedSpellId isn't currently valid, or 0 if no spell is known at
    // all.
    static int CycleSelectedSpell(const PlayerState& p, const SpellDatabase& spells);

    // What ResolveSpellCastInput found this call, so the caller (main.cpp)
    // can show the right popup / trigger the right flash-overlay flag
    // without this module depending on stormhold_render -- same
    // "game logic returns a signal, the caller shows the message" pattern
    // player/camp_state.h's CampTickResult and combat/combat_resolution.h's
    // TickMonstersOnLevel/ResolveAttackInput already use.
    enum class Result {
        NotRequested,
        // Spell.isValidId(selectedSpellId) failed -- defensive in the
        // original (`System.out.println("Invalid spell id,= " + spellId)`,
        // not reproduced here, same "debug println, not user-visible, not
        // reproduced at the C++ tick-loop level" precedent every other
        // milestone's own dropped println already established), not
        // confirmed reachable.
        InvalidSpell,
        NotEnoughMagicka,
        // Offensive spell, no monster currently targeted.
        NoMonster,
        // Cooldown (lastSpellCastTimeMs, 500ms) hasn't elapsed yet -- the
        // original does nothing at all here, silently swallowing the
        // press (no message, cooldown NOT restamped) -- preserved exactly.
        OnCooldown,
        CastOnMonster,
        CastOnSelf,
    };

    // GameCanvas.resolveSpellCastInput(long) (was decompiled/e.java's
    // h(long)) -- tickPlayerAction/e(long)'s own unconfirmed_ap branch (key
    // '3'). UNLIKE resolveAttackInput()'s own C++ port (M39), this key is
    // confirmed genuinely unconditional in keyPressed() -- no
    // hotbarActionSet gate at all (see decompiled/e.java's keyPressed(),
    // key==51) -- so main.cpp binds it to the REAL key code directly
    // ('3'), not a pragmatic stand-in the way 'C'/'F' stand in for the
    // hotbar-gated camp/interact keys.
    //
    // Does NOT re-check `castRequested` internally (same precedent
    // CombatResolution::ResolveAttackInput's own header comment
    // establishes for `attackRequested`): the original's own top-level
    // `if (unconfirmed_ap)` is provably redundant here too, since
    // tickPlayerAction's else-if dispatch chain only ever calls this when
    // that flag is already true. Also NOT modeled: `unconfirmed_at` (set
    // true right before dispatching a cast either way) -- its only real
    // consumer, tickDeathAndRegen()'s own passive-fatigue-regen gate, isn't
    // ported either, same gap ResolveAttackInput's own header comment
    // already flags for its own read of the same field. Always clears
    // `castRequested` before returning, matching unconfirmed_ap's own
    // unconditional final reset.
    static Result ResolveSpellCastInput(PlayerState& player, std::optional<MonsterState>& target,
                                         bool& castRequested, int64_t now, int64_t& lastSpellCastTimeMs,
                                         const SpellDatabase& spells, const CharacterData& charData,
                                         const ItemDatabase& items, const MonsterDatabase& monsterDb,
                                         JavaRandom& rng, WorldRegistry& world);

    // GameCanvas.resolveSpellCycleInput(long) (was decompiled/e.java's
    // g(long)) -- tickPlayerAction/e(long)'s own unconfirmed_U branch (key
    // '5'), same "real, unconditional key" finding as key '3' above.
    // Returns CycleSelectedSpell()'s own result (0 = "no spells known" --
    // the caller's cue for MSG_NO_SPELLS) and, on a nonzero result, also
    // stamps player.selectedSpellId itself (redundant with whatever
    // CycleSelectedSpell already determined internally, preserved exactly
    // rather than assumed dead, matching resolveSpellCycleInput()'s own
    // header comment). Always clears `cycleRequested`.
    static int ResolveSpellCycleInput(PlayerState& player, bool& cycleRequested, const SpellDatabase& spells);
};

}  // namespace stormhold
