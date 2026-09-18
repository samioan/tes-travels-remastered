#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "assets/asset_root.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/Player.java's static
// "character data" tables -- charin.dat (../../docs/ASSET_FORMATS.md),
// loaded by Player.loadCharacterData(). Just the static per-class/
// per-skill template data Player.ensureCharDataLoaded() builds once at
// startup; Player's own runtime instance state (stats, inventory,
// equipment, ...) is gameplay logic for a later milestone.
//
// Same overall shape as dawnstar's own charin.dat/CharacterData --
// including dawnstar's own class-vs-race naming fix (found there by
// loading this exact table against real data: the "class" array holds
// archetypes like Barbarian/Sorcerer, the "race" array holds actual TES
// races). Phase 1's rename pass here already carried that fix over rather
// than re-discovering it (../docs/ASSET_FORMATS.md's charin.dat section
// says so directly) -- this milestone is confirmation, not a fresh find.
struct CharacterData {
    std::vector<std::string> statLabels;
    std::vector<std::string> attributeNames;
    std::vector<std::string> classNames;
    std::vector<std::string> raceNames;
    std::vector<std::string> skillNames;

    // skillNames[i]'s governing attribute -- an index into attributeNames[]
    // (see Player.java's skillGoverningAttribute usage).
    std::vector<int16_t> skillAttributeIndex;

    // [classIndex][13 + 2*skillCount] per-class template: base attributes,
    // base skills, starting spell-knowledge thresholds, and (column 1) the
    // race that comes with this class -- see Player.java's
    // applyClassTemplate().
    std::vector<std::vector<int16_t>> classTemplates;

    int ClassCount() const { return static_cast<int>(classNames.size()); }
    int RaceCount() const { return static_cast<int>(raceNames.size()); }
    int SkillCount() const { return static_cast<int>(skillNames.size()); }

    static CharacterData Load(const AssetRoot& assets);
};

}  // namespace stormhold
