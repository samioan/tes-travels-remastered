#pragma once
#include <string>
#include <vector>

#include "assets/binary_reader.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Shop.java's npcstrings.dat
// loader (Shop.loadDialogue()/Shop.load()/Shop.loadGroup()).
//
// Unlike most of this game's data tables, npcstrings.dat is NOT bundled
// inside datfiles.lmp: Shop.load() reads it via Util.openResource(path)
// (ESGame.getDataInputStream() -- a direct top-level jar resource
// stream), not ESGame.getResource(name) (the datfiles.lmp archive scan
// every other *.dat file goes through). Confirmed by reading Shop.java
// directly, and independently by ../../extracted/ itself
// (npcstrings.dat sits there as its own top-level file, same as
// datfiles.lmp/imgfiles.lmp, not nested inside either) -- this corrects
// ../../docs/ASSET_FORMATS.md's previous claim that it was bundled like
// charin.dat/itemsin.dat/helptext.dat/etc. That's why Load() takes a
// plain file path, unlike every other loader in ../assets/ which takes a
// DatArchive&.
struct ShopDialogue {
    // 10 fixed-size groups (Shop.GROUP_SIZES = {3,3,3,3,14,16,16,16,16,
    // 77}, checked against the file's own per-group count -- a mismatch
    // throws in the original, and here too): 0-3 per-generic-shop-type
    // flavor lines, 4 Jakar's (main quest-giver/rumor-mill NPC), 5-8 the
    // 4 named shopkeepers, 9 the generic/rumor pool (77 entries,
    // <TAG>-templated -- see Util.replace()).
    std::vector<std::vector<std::string>> groups;

    static ShopDialogue Load(const std::string& path);
};

}  // namespace dawnstar
