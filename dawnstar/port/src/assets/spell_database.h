#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "assets/dat_archive.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Spell.java -- spellsin.dat
// (../../docs/ASSET_FORMATS.md).
struct Spell {
    std::string name;
    int8_t skillRequired = 0;
    int8_t magickaCost = 0;
    int8_t power = 0;
    int8_t school = 0;
    int8_t durationMultiplier = 0;
    int8_t icon = 0;
    std::string description;
};

struct SpellDatabase {
    std::vector<Spell> all;

    int Count() const { return static_cast<int>(all.size()); }

    // spellId is 1-based, matching Spell.java's index0(spellId).
    const Spell& ById(int spellId) const { return all[spellId - 1]; }

    // school == 2 marks an offensive (monster-targeted) spell -- see
    // Spell.java's isOffensive().
    bool IsOffensive(int spellId) const { return ById(spellId).school == 2; }

    static SpellDatabase Load(DatArchive& archive);
};

}  // namespace dawnstar
