// M11 smoke test: ShopDialogue against the real npcstrings.dat.
#include <cstdio>
#include <string>

#include "assets/asset_root.h"
#include "assets/shop_dialogue.h"

namespace {

bool Expect(bool cond, const char* what) {
    if (!cond) std::printf("  FAIL: %s\n", what);
    return cond;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        stormhold::AssetRoot assets(root);
        stormhold::ShopDialogue dialogue = stormhold::ShopDialogue::Load(assets);

        bool ok = true;

        ok &= Expect(dialogue.groups.size() == 8, "should load exactly 8 groups");

        const int expectedSizes[8] = {20, 20, 20, 20, 5, 22, 5, 41};
        const char* names[8] = {"Arantamo", "Celegil", "Favela Dralor", "Vander", "Beneca", "Helga", "Varus",
                                 "(rumor pool)"};
        for (int i = 0; i < 8; i++) {
            ok &= Expect(dialogue.groups[static_cast<size_t>(i)].size() == static_cast<size_t>(expectedSizes[i]),
                         "group size should match GROUP_SIZES");
            for (const auto& line : dialogue.groups[static_cast<size_t>(i)]) {
                ok &= Expect(!line.empty(), "every dialogue line should be non-empty");
            }
            std::printf("  group %d (%-14s, %2d lines): \"%s\"\n", i, names[i],
                        static_cast<int>(dialogue.groups[static_cast<size_t>(i)].size()),
                        dialogue.groups[static_cast<size_t>(i)][0].c_str());
        }

        if (!ok) {
            std::fprintf(stderr, "m11_shop_dialogue_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m11_shop_dialogue_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
