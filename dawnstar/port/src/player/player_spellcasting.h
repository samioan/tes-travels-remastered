#pragma once
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/spell_database.h"
#include "player/player_state.h"
#include "util/java_random.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Player.java's self-targeted
// spellcasting and known-spell bookkeeping: spellSkillIndexFor()/
// castOnSelf()/activeAilmentCount()/cureRandomAilment()/canLearnSpell()/
// learnSpellFromScroll()/knownSpellsSummary()/nthKnownSpellId()/
// cycleSelectedSpell()/spellTooltip(). Deliberately NOT castOnMonster()
// -- like Player.attack()/Monster.tick(), it needs a live Monster target
// (target.stat()/takeDamage()/scratch[]/store(), and it can itself call
// attack()), so it lives in combat/combat_resolution.h alongside those,
// following the same three-module split M15 established (see
// docs/PORT_ROADMAP.md's M15 entry).
//
// A real quirk preserved rather than "fixed": nthKnownSpellId() and
// spellTooltip() both use a 0-based Spell.all[]/knownSpellsMask BIT
// index, despite nthKnownSpellId's own doc comment in Player.java
// calling it a spell "id" and spellTooltip's parameter being named
// spellId -- every OTHER spell entry point here (castOnSelf/
// castOnMonster/canLearnSpell/learnSpellFromScroll/cycleSelectedSpell's
// return value/Spell.byId/Spell.isValidId) uses the 1-based spellId
// convention (bit index + 1). The two families are internally
// consistent with each other (ESGame.java passes nthKnownSpellId's
// return straight into spellTooltip) but NOT with the 1-based
// convention used everywhere else -- confirmed by reading both call
// sites in ../../../src/ESGame.java (lines ~1189 and ~2296-2297).
class PlayerSpellcasting {
public:
    // Maps a spell id (1-based) to a governing skill index: 1-5->1,
    // 6-10->3, 11-15->4, 16-20->6, 21+->10.
    static int SpellSkillIndexFor(int spellId);

    // Casts `spellId` (1-based) on self: rolls a rollOutcome() hit-tier
    // against the spell's own school/duration figures, spends Magicka
    // scaled by the outcome, applies the spell's effect by id (buffs via
    // effectDurations, direct Health restore, "learn to cure poison"
    // item grant+equip for spell 6, a random-ailment cure loop for spell
    // 25), then spends Fatigue and applies the "Winter Worn" (ailment 6)
    // drain, same shape as CombatResolution::PlayerAttack.
    static void CastOnSelf(PlayerState& p, const CharacterData& charData, const ItemDatabase& items,
                            const SpellDatabase& spells, JavaRandom& globalRng);

    static int ActiveAilmentCount(const PlayerState& p);
    // Clears one uniformly-random currently-active ailment bit.
    static void CureRandomAilment(PlayerState& p, JavaRandom& globalRng);

    // Gates the "Learn" inventory-item menu option: true for a
    // not-already-known spell scroll (category 12) whose required skill
    // the player has at least 1 point in.
    static bool CanLearnSpell(const PlayerState& p, const ItemDatabase& items, const SpellDatabase& spells, int slot);
    // Learns the spell encoded on a scroll-type item in `slot`, then
    // consumes the scroll (via PlayerInventory::RemoveSlot).
    static bool LearnSpellFromScroll(PlayerState& p, const ItemDatabase& items, int slot);

    // "Spell name" strings for every known spell (ascending bit-index
    // order), prefixing the currently-selected one with "R: ".
    static std::vector<std::string> KnownSpellsSummary(const PlayerState& p, const SpellDatabase& spells);
    // The `index`-th known spell's 0-based Spell.all[]/bit index (NOT a
    // 1-based spellId -- see the class doc comment above), matching
    // KnownSpellsSummary's ordering; -1 if `index` is out of range.
    static int NthKnownSpellId(const PlayerState& p, const SpellDatabase& spells, int index);
    // Cycles to the next known spell after selectedSpellId (wrapping),
    // or the first known spell if the current selection isn't valid;
    // returns the new 1-based spellId to assign to selectedSpellId, or 0
    // if no spells are known. Does not mutate `p` itself, matching the
    // original (GameCanvas.cycleSelectedSpell() assigns the result back).
    static int CycleSelectedSpell(const PlayerState& p, const SpellDatabase& spells);
    // `index` is the same 0-based bit index NthKnownSpellId() returns.
    static std::string SpellTooltip(const CharacterData& charData, const SpellDatabase& spells, int index);
};

}  // namespace dawnstar
