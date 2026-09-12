#pragma once
#include <cstdint>
#include <vector>

#include "player/player_state.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Player.java's toBytes(true)
// / fromBytes(data, true) -- the "full" in-progress save format (used by
// the actual save/load path, as opposed to the lightweight "character
// summary" format toBytes(false)/fromBytes(false) produce, which is used
// somewhere ESGame hasn't been wired into this port yet -- most likely a
// high-score/leaderboard record given Player.java's own comment on
// toBytes. That summary format is deferred to whichever later milestone
// actually needs it, since round-tripping it meaningfully requires
// re-deriving the class-template reconstruction path
// (applyClassTemplate+resetState) fromBytes(..., false) leans on, which
// this milestone doesn't otherwise need. See docs/PORT_ROADMAP.md's M12
// entry.
class PlayerSave {
public:
    static std::vector<uint8_t> ToBytes(const PlayerState& p);
    static PlayerState FromBytes(const std::vector<uint8_t>& data);
};

}  // namespace dawnstar
