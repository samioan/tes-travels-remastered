// M8 smoke test: loads the real npcstrings.dat (ShopDialogue, a
// standalone file -- see shop_dialogue.h) and helptext.dat (HelpText,
// bundled inside datfiles.lmp), and prints enough to check by hand
// against ../../src/Shop.java/ESGame.java and docs/ASSET_FORMATS.md.
#include <cstdio>

#include "assets/dat_archive.h"
#include "assets/help_text.h"
#include "assets/shop_dialogue.h"

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::ShopDialogue dlg = dawnstar::ShopDialogue::Load(root + "/npcstrings.dat");
        std::printf("dialogue groups: %zu\n", dlg.groups.size());
        for (size_t g = 0; g < dlg.groups.size(); g++) {
            std::printf("  group %zu (%zu strings): \"%s\"%s\n", g, dlg.groups[g].size(),
                        dlg.groups[g].empty() ? "" : dlg.groups[g][0].c_str(),
                        dlg.groups[g].size() > 1 ? " ..." : "");
        }

        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::HelpText help = dawnstar::HelpText::Load(archive);
        std::printf("help topics: %zu\n", help.titles.size());
        for (size_t i = 0; i < help.titles.size(); i++) {
            std::printf("  [%zu] %-24s %zu chars\n", i, help.titles[i].c_str(), help.bodies[i].size());
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m8_dialogue_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
