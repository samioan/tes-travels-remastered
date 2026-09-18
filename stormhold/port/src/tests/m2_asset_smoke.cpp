// M2 smoke test: loads the real itemsin.dat/droppeditemsin.dat/spellsin.dat
// (via AssetRoot -- no archive here, unlike dawnstar) into ItemDatabase/
// SpellDatabase, then prints counts and a handful of known rows to check by
// hand against ../../src/Item.java's/Spell.java's own data
// (docs/ASSET_FORMATS.md).
#include <cstdio>

#include "assets/asset_root.h"
#include "assets/item_database.h"
#include "assets/spell_database.h"

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        stormhold::AssetRoot assets(root);

        stormhold::ItemDatabase items = stormhold::ItemDatabase::Load(assets);
        std::printf("items: %d, categories: %zu, lootTable rows: %zu\n", items.ItemCount(),
                    items.categoryNames.size(), items.lootTable.size());
        for (int i = 0; i < 3 && i < items.ItemCount(); ++i) {
            std::printf("  item[%d]: %-24s cat=%d subtype=%d buy=%d sell=%d equipSlot=%d\n", i,
                        items.name[i].c_str(), items.category[i], items.subtype[i],
                        items.buyPrice[i], items.sellPrice[i], items.equipSlot[i]);
        }

        stormhold::SpellDatabase spells = stormhold::SpellDatabase::Load(assets);
        std::printf("spells: %d\n", spells.Count());
        for (int i = 0; i < 3 && i < spells.Count(); ++i) {
            const stormhold::Spell& s = spells.all[i];
            std::printf("  spell[%d]: %-20s school=%d cost=%d power=%d -- %s\n", i,
                        s.name.c_str(), s.school, s.magickaCost, s.power, s.description.c_str());
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m2_asset_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
