#pragma once
#include <cstdint>
#include <vector>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "player/player_state.h"
#include "util/java_random.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Player.java's two
// serializations: toBytes(true)/fromBytes(data,true), the "full"
// in-progress save format (the actual save/load path); and
// toBytes(false)/fromBytes(data,false), a lightweight "character
// summary" (no position/inventory/status -- likely a high-score/
// leaderboard record given Player.java's own comment on toBytes, though
// ESGame still isn't wired into this port to confirm where it's
// actually called from).
class PlayerSave {
public:
    static std::vector<uint8_t> ToBytes(const PlayerState& p);
    static PlayerState FromBytes(const std::vector<uint8_t>& data);

    // Player.java's toBytes(false): name/classIndex/raceIndex, a
    // normalized copy of coreStats (current=max for HP/Magicka/Fatigue,
    // [8] zeroed -- see normalizeForSummary()), gold, attributes,
    // classMagickaFactor/classUnknownPair, skills, and a freshly
    // recomputed STARTING known-spell mask (not the live
    // knownSpellsMask -- see below). No inventory/position/status
    // fields at all.
    //
    // SURPRISING, faithfully-preserved real behavior: Player.java's
    // toBytes(false) computes that spell mask via
    // `this.computeStartingSpellMask()` -- called on the LIVE character
    // being saved, not a scratch one. That method has a real side
    // effect (it sets selectedSpellId to the class's first starting
    // spell -- see PlayerCreation::ComputeStartingSpellMask's doc
    // comment). So producing a "read-only" character summary actually
    // MUTATES `p` itself: it silently resets whatever spell the player
    // currently has selected back to their class's default, and the
    // written mask reflects only the class's starting spells, silently
    // discarding any spells actually learned since character creation.
    // `p` is therefore taken by non-const reference here too, unlike
    // ToBytes above.
    static std::vector<uint8_t> ToBytesSummary(PlayerState& p, const CharacterData& charData);

    // Player.java's fromBytes(data,false): reads name/classIndex, then
    // (like the original's applyClassTemplate(classIndex)+
    // resetState(false)) rebuilds a fresh character of that class via
    // PlayerCreation::CreateCharacter -- rolling a NEW traitorIndex and
    // re-granting starting items/hub position, since none of that is
    // actually in the summary format -- before overwriting
    // raceIndex/coreStats/gold/attributes/classMagickaFactor/
    // classUnknownPair/skills/knownSpellsMask with the serialized
    // values, exactly matching the original's read-then-overwrite
    // order. `globalRng` is CreateCharacter's, see its own doc comment.
    static PlayerState FromBytesSummary(const std::vector<uint8_t>& data, const CharacterData& charData,
                                         const ItemDatabase& items, JavaRandom& globalRng);
};

}  // namespace dawnstar
