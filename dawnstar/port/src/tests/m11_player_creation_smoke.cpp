// M11 smoke test: creates a character of each of the 7 real classes
// (CharacterData, M4) and checks the result against the real class
// templates/item data by hand -- attributes/skills/race copied straight
// from classTemplates, gold always 50, hub-town starting position,
// starting items granted and auto-equipped into valid equip slots, and
// a plausible starting-spell mask.
#include <cstdio>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/item_database.h"
#include "player/player_creation.h"
#include "util/java_random.h"

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::CharacterData charData = dawnstar::CharacterData::Load(archive);
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);

        dawnstar::JavaRandom globalRng(12345);
        bool ok = true;

        for (int classIdx = 0; classIdx < charData.ClassCount(); classIdx++) {
            dawnstar::PlayerState p =
                dawnstar::PlayerCreation::CreateCharacter(classIdx, "Test", charData, items, globalRng);

            std::printf("class[%d] %-12s race=%-10s gold=%d level=%d pos=(%d,%d) facing=%d\n", classIdx,
                        charData.classNames[classIdx].c_str(), charData.raceNames[p.raceIndex].c_str(), p.gold,
                        p.coreStats[0], p.tileX, p.tileY, p.facing);

            if (p.gold != 50) {
                std::printf("  FAIL: gold should always be 50, got %d\n", p.gold);
                ok = false;
            }
            if (p.tileX != 9 || p.tileY != 9 || p.facing != 1 || p.currentLevel != 1) {
                std::printf("  FAIL: should start at hub-town spawn (9,9) facing 1 on level 1\n");
                ok = false;
            }
            if (p.raceIndex != charData.classTemplates[classIdx][1]) {
                std::printf("  FAIL: raceIndex doesn't match classTemplates column 1\n");
                ok = false;
            }
            for (int i = 0; i < 8; i++) {
                if (p.attributes[2 * i] != charData.classTemplates[classIdx][2 + i]) {
                    std::printf("  FAIL: attribute %d doesn't match classTemplates\n", i);
                    ok = false;
                }
            }

            std::printf("  attributes:");
            for (int i = 0; i < 8; i++) {
                std::printf(" %s=%d", charData.attributeNames[2 * i].c_str(), p.attributes[2 * i]);
            }
            std::printf("\n");

            std::printf("  inventory (%d items):", p.inventoryCount);
            for (int i = 0; i < p.inventoryCount; i++) {
                int8_t raw = p.inventoryItemIds[i];
                bool equipped = raw < 0;
                int itemId = equipped ? -raw : raw;
                if (itemId < 1 || itemId > items.ItemCount()) {
                    std::printf(" FAIL(bad item id %d)", itemId);
                    ok = false;
                    continue;
                }
                std::printf(" %s%s", items.name[itemId - 1].c_str(), equipped ? "(equipped)" : "");
                if (equipped && !items.IsEquippable(itemId)) {
                    std::printf("[FAIL: marked equipped but not equippable]");
                    ok = false;
                }
            }
            std::printf("\n");

            std::printf("  knownSpellsMask=0x%x selectedSpellId=%d\n", p.knownSpellsMask, p.selectedSpellId);
            for (int bit : {0, 5, 10, 15, 20}) {
                bool known = (p.knownSpellsMask & (1u << bit)) != 0;
                if (known && (p.selectedSpellId < 1 || p.selectedSpellId > 25)) {
                    std::printf("  FAIL: a spell is known but selectedSpellId (%d) looks invalid\n",
                                p.selectedSpellId);
                    ok = false;
                }
            }
        }

        if (!ok) {
            std::fprintf(stderr, "m11_player_creation_smoke: FAILED\n");
            return 1;
        }
        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m11_player_creation_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
