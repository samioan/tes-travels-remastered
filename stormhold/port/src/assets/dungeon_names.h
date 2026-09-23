#pragma once
#include <array>
#include <string>

#include "assets/asset_root.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/Dungeon.java's dungnamesin.dat
// loader (Dungeon.loadNames()/displayNames()) -- new this milestone
// (player/death_sequence.h's own respawn-message logic is the first, and so
// far only, confirmed caller of displayNames() anywhere in this port).
//
// 37 levels x 2 UTF-8 strings each, with NO per-entry count prefix at all --
// unlike every other table this port loads through BinaryReader (M2's
// ItemDatabase/SpellDatabase, M11's ShopDialogue), confirmed by reading
// loadNames() directly: it's just 74 straight readUTF() calls in a fixed
// double loop, nothing else to validate against. The 2nd column's exact
// meaning is still unconfirmed -- Dungeon.java's own header comment: "never
// indexed [0] vs [1] separately in what's been traced here" -- both are
// loaded regardless (matching the original, which loads the full 37x2 table
// unconditionally too), even though DisplayNames() below only ever returns
// the whole row, and this port's own sole confirmed caller only reads [0].
struct DungeonNames {
    // Indexed levelNumber-1, matching every other per-level table in this
    // port (DungeonGeometry::rows, WorldRegistry's own level vector, ...).
    std::array<std::array<std::string, 2>, 37> names;

    static DungeonNames Load(const AssetRoot& assets);

    // Dungeon.displayNames() -- levelNumber is 1-based.
    const std::array<std::string, 2>& DisplayNames(int levelNumber) const {
        return names[static_cast<size_t>(levelNumber - 1)];
    }
};

}  // namespace stormhold
