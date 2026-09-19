#pragma once
#include <cstdint>
#include <vector>

#include "player/player_state.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/Player.java's persistent
// save-format serialization: `toBytes(true)`/`fromBytes(data, true)` --
// Player.java's own header comment calls this "the complete in-progress
// save" (everything), as opposed to `toBytes(false)`/`fromBytes(data,
// false)`'s "lightweight character summary" format (most likely a
// high-score/leaderboard record, per that same comment).
//
// **Deliberately NOT ported here: the `full=false` lightweight format,**
// same deferral dawnstar's own M12 (`PlayerSave`) already made for its
// own equivalent, and for a related reason: round-tripping it
// meaningfully needs `applyClassTemplate()`+`resetState(classIndex,
// false)`'s "new character" reconstruction path (`fromBytes(...,
// false)`'s own first two lines) run BEFORE the serialized fields
// layer on top of it, overwriting only some of what that reset just
// set (raceIndex/coreStats/attributes/classMagickaFactor/
// classUnknownPair/skills/knownSpellsMask -- inventory/equipment/
// position are left at whatever the fresh reset produced, never
// re-read). Unlike dawnstar's own case (complicated by a hidden
// "traitor index" RNG roll baked into creation), Stormhold's own
// creation pipeline has NO randomness at all (confirmed at M9) and
// already exists as a directly-reusable building block
// (`player/player_creation.h`'s `PlayerCreation::CreateCharacter`) --
// so this is a scope choice, not a blocker, left for whichever later
// milestone actually needs a leaderboard-style summary record rather
// than the real save/load path this milestone unblocks.
//
// Needs `BinaryWriter`/`BinaryReader` (`assets/binary_writer.h`/
// `binary_reader.h`) -- the former added this milestone specifically for
// this (no writer existed anywhere in this port before now). Every field
// `PlayerState` doesn't carry here (`pendingLevel`/`pendingTileX`/
// `pendingTileY`/`pendingFacing`, `prevTileX`/`prevTileY`,
// `crossingLevelBoundary`, `enteredNewLevelZone`/`leftLevelZone`,
// `justMarkedCamp`, `pendingLockedItemFlag`) is confirmed, by reading
// `toBytes(true)`/`fromBytes(data, true)` in full, to genuinely NOT be
// part of the real save format either -- all of them are per-tick/
// per-turn transient scratch state recomputed fresh on the next move,
// not persisted state, matching the original exactly.
class PlayerSave {
public:
    // Player.toBytes(true).
    static std::vector<uint8_t> ToBytes(const PlayerState& p);
    // Player.fromBytes(data, true).
    static PlayerState FromBytes(const std::vector<uint8_t>& data);
};

}  // namespace stormhold
