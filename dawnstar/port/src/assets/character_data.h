#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "assets/dat_archive.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Player.java's static
// "character data" tables -- charin.dat (../../docs/ASSET_FORMATS.md),
// loaded by Player.loadCharacterData(). Just the static per-class/
// per-skill template data Player.ensureCharDataLoaded() builds once at
// startup; Player's own runtime instance state (stats, inventory,
// equipment, ...) is gameplay logic for a later milestone.
//
// classNames/raceNames were caught swapped by loading this exact table
// against the real archive (this session): the array now called
// classNames prints Barbarian/Battlemage/Knight/Nightblade/Rogue/
// Sorcerer/Spellsword (7 entries) -- confirmed as the character CLASS
// list by ESGame.java's own character-creation screen, which titles this
// exact list "Select a Class:". The array now called raceNames prints
// Redguard/Nord/Breton/High Elf/Wood Elf/Dark Elf (6 entries) -- the real
// race list; there is no separate gender selection anywhere in this game.
// Player.java's own fields were fixed to match (see its "NOTE" comment
// near classCount).
struct CharacterData {
    std::vector<std::string> statLabels;
    std::vector<std::string> attributeNames;
    std::vector<std::string> classNames;
    std::vector<std::string> raceNames;
    std::vector<std::string> skillNames;

    // skillNames[i]'s governing attribute -- an index into attributes[]
    // (see Player.java's recalcMaxStats/applyClassTemplate).
    std::vector<int16_t> skillAttributeIndex;

    // [classIndex][13 + 2*skillCount] per-class template: base attributes,
    // base skills, starting spell-knowledge thresholds, and (column 1)
    // the race that comes with this class -- see Player.java's
    // applyClassTemplate().
    std::vector<std::vector<int16_t>> classTemplates;

    int ClassCount() const { return static_cast<int>(classNames.size()); }
    int RaceCount() const { return static_cast<int>(raceNames.size()); }
    int SkillCount() const { return static_cast<int>(skillNames.size()); }

    static CharacterData Load(DatArchive& archive);
};

}  // namespace dawnstar
