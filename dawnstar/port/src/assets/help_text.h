#pragma once
#include <string>
#include <vector>

#include "assets/dat_archive.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/ESGame.java's
// loadHelpStrings(): helptext.dat is a flat pool of 35 text fragments,
// bundled inside datfiles.lmp (read via ESGame.getResource(), same as
// itemsin.dat/spellsin.dat/etc. -- unlike ShopDialogue's npcstrings.dat,
// see shop_dialogue.h). This build's *code*, not the file itself,
// decides how the 35 fragments group into 12 help topics, via two
// hardcoded index lists: which single fragment is a topic's title, and
// which contiguous range of fragments concatenate into its body. Every
// one of the 35 fragments is used exactly once (12 as titles, 23 across
// the 12 bodies).
struct HelpText {
    std::vector<std::string> titles;  // 12 entries
    std::vector<std::string> bodies;  // 12 entries, same order as titles

    static HelpText Load(DatArchive& archive);
};

}  // namespace dawnstar
