// M4 smoke test: loads the real charin.dat (CharacterData) via AssetRoot
// and prints enough to check by hand against ../../src/Player.java's own
// tables and docs/ASSET_FORMATS.md.
//
// classNames below should print class archetypes, and raceNames should
// print actual TES races -- confirming phase 1's rename pass correctly
// carried over dawnstar's own class/race naming fix rather than
// reintroducing the swap it found there.
#include <cstdio>

#include "assets/asset_root.h"
#include "assets/character_data.h"

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        stormhold::AssetRoot assets(root);
        stormhold::CharacterData chars = stormhold::CharacterData::Load(assets);

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
