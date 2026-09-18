#pragma once
#include <string>
#include <vector>

#include "assets/asset_root.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/Shop.java's npcstrings.dat
// loader (Shop.loadDialogue()/Shop.load()/Shop.loadGroup()).
//
// Unlike dawnstar, there's no "is it bundled inside an archive or not"
// question to answer here at all -- Stormhold has no datfiles.lmp/
// imgfiles.lmp indirection for ANY resource (see docs/ASSET_FORMATS.md's
// header note, and AssetRoot's own doc comment), so npcstrings.dat loads
// through the exact same AssetRoot::OpenFile() every other table here
// does, not a special-cased plain-path loader the way dawnstar's
// ShopDialogue needed.
struct ShopDialogue {
    // 8 fixed-size groups (Shop.GROUP_SIZES = {20, 20, 20, 20, 5, 22, 5,
    // 41}, checked against the file's own per-group count -- a mismatch
    // throws in the original, and here too): groups 0-3 are the 4
    // quest-turn-in shopkeepers (Arantamo/Celegil/Favela Dralor/Vander),
    // 4 is Beneca, 5 is Helga, 6 is Varus (the Warden-tied NPC, see M8's
    // WardenState), and 7 is a separate 41-entry generic/rumor string
    // pool (Shop.rumorFor()'s own dialogue[7][1]/dialogue[7][2], <TAG>-
    // templated the same way as dawnstar's own rumor pool -- see
    // Util.replace()). Group index IS shop index for groups 0-6, directly
    // matching Shop.java's own `dialogue[shopId][...]` indexing -- unlike
    // dawnstar's own 10-group layout (4 generic + Jakar + 4 named shops +
    // rumor pool), Stormhold's groups map onto its 7-NPC roster 1:1 plus
    // the one shared rumor pool, no separate "generic flavor line" groups
    // at all.
    std::vector<std::vector<std::string>> groups;

    static ShopDialogue Load(const AssetRoot& assets);
};

}  // namespace stormhold
