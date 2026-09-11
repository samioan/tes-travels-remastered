// M4 smoke test: loads the real charin.dat (CharacterData) and prints
// enough to check by hand against ../../src/Player.java's own tables and
// docs/ASSET_FORMATS.md.
//
// This is also the regression check for a real bug this milestone found:
// the class/race arrays were swapped in the original rename pass (a
// "genderNames" that has 6 entries and no "Male"/"Female" anywhere in the
// corpus was the tell). classNames below should print class archetypes
// (Barbarian, Battlemage, ...), and raceNames should print actual TES
// races (Redguard, Nord, ...) -- if that's ever flipped again, this
// output flips with it.
#include <cstdio>

#include "assets/character_data.h"
#include "assets/dat_archive.h"

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";
    std::string datPath = root + "/datfiles.lmp";

    try {
        dawnstar::DatArchive archive(datPath);
        dawnstar::CharacterData chars = dawnstar::CharacterData::Load(archive);

        std::printf("classes: %d, races: %d, skills: %d\n", chars.ClassCount(), chars.RaceCount(),
                    chars.SkillCount());

        std::printf("stat labels:");
        for (const auto& s : chars.statLabels) std::printf(" [%s]", s.c_str());
        std::printf("\n");

        std::printf("attributes:");
        for (const auto& s : chars.attributeNames) std::printf(" [%s]", s.c_str());
        std::printf("\n");

        std::printf("races:");
        for (const auto& s : chars.raceNames) std::printf(" [%s]", s.c_str());
        std::printf("\n");

        for (int c = 0; c < chars.ClassCount(); ++c) {
            std::printf("  class[%d]: %-12s template[0..2]=%d,%d,%d\n", c, chars.classNames[c].c_str(),
                        chars.classTemplates[c][0], chars.classTemplates[c][1], chars.classTemplates[c][2]);
        }

        for (int s = 0; s < chars.SkillCount(); ++s) {
            std::printf("  skill[%d]: %-16s governingAttr=%d\n", s, chars.skillNames[s].c_str(),
                        chars.skillAttributeIndex[s]);
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m4_character_data_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
