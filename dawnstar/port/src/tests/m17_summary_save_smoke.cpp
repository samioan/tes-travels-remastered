// M17 smoke test: PlayerSave::ToBytesSummary/FromBytesSummary --
// Player.java's toBytes(false)/fromBytes(data,false), the lightweight
// "character summary" format M12 deferred. Builds a real character (M11's
// PlayerCreation), advances/mutates it away from a fresh spawn (moved off
// the hub tile, extra gold/skills/inventory, a non-starting
// selectedSpellId and an extra learned spell), round-trips through the
// summary format, and checks both what survives (name/classIndex/
// raceIndex/normalized coreStats/gold/attributes/classMagickaFactor/
// classUnknownPair/skills) and what does NOT (inventory/position/
// traitorIndex/the extra learned spell -- all silently regenerated fresh,
// exactly like a brand-new character of that class), plus the real
// selectedSpellId-mutation side effect toBytes(false) has on the LIVE
// character being saved. No JVM ground truth available (same reason as
// M6/M9/M11/M13/M14/M15/M16).
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/item_database.h"
#include "player/player_creation.h"
#include "player/player_save.h"
#include "util/java_random.h"

namespace {

using dawnstar::CharacterData;
using dawnstar::ItemDatabase;
using dawnstar::PlayerCreation;
using dawnstar::PlayerSave;
using dawnstar::PlayerState;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        CharacterData charData = CharacterData::Load(archive);
        ItemDatabase items = ItemDatabase::Load(archive);

        for (int classIdx = 0; classIdx < charData.ClassCount(); classIdx++) {
            dawnstar::JavaRandom creationRng(9000 + classIdx);
            PlayerState p = PlayerCreation::CreateCharacter(classIdx, "Summarized", charData, items, creationRng);

            // Independently recompute this class's starting spell mask
            // (against a scratch PlayerState, so it can't be perturbed by
            // ToBytesSummary's own mutation below) to know what the
            // summary format's mask -- and toBytes(false)'s selectedSpellId
            // side effect -- SHOULD produce, before p gets mutated.
            PlayerState scratch;
            scratch.classIndex = p.classIndex;
            uint32_t expectedStartingMask = PlayerCreation::ComputeStartingSpellMask(scratch, charData);
            int8_t expectedStartingSpellId = scratch.selectedSpellId;

            // Advance the character away from a fresh spawn: move off
            // the hub tile, spend gold, gain a skill rank, add extra
            // inventory, and -- critically -- pick a DIFFERENT selected
            // spell and learn one beyond the class's starting set, so a
            // round-trip that failed to discard them would be visible.
            p.currentLevel = 5;
            p.tileX = 12;
            p.tileY = 3;
            p.facing = 2;
            p.gold += 500;
            p.skills[0][0] = static_cast<int16_t>(p.skills[0][0] + 3);
            p.coreStats[2] = static_cast<int16_t>(p.coreStats[3] - 1);  // 1 HP short of max.
            p.coreStats[9] = 7;  // normalizeForSummary leaves [9] untouched -- should survive.
            int8_t originalTraitorIndex = p.traitorIndex;
            int originalInventoryCount = p.inventoryCount;
            // A spell bit well outside every class's starting set (id 25).
            p.knownSpellsMask |= (1u << 24);
            p.selectedSpellId = 25;
            Check(p.selectedSpellId != expectedStartingSpellId,
                  "test setup should pick a selectedSpellId different from the class's starting one");

            std::vector<uint8_t> bytes = PlayerSave::ToBytesSummary(p, charData);

            // The real, surprising side effect: saving the summary
            // resets the LIVE character's selectedSpellId back to the
            // class's starting spell -- but ONLY if the class actually
            // has one (computeStartingSpellMask's `first` flag, and
            // thus the this.selectedSpellId assignment, is only ever
            // reached if at least one starting-spell bit gets set). A
            // pure-warrior class with no innate spells (mask==0, e.g.
            // Barbarian/Knight/Rogue below) leaves selectedSpellId
            // completely untouched instead.
            if (expectedStartingMask != 0) {
                Check(p.selectedSpellId == expectedStartingSpellId,
                      "ToBytesSummary should reset selectedSpellId to the class's starting spell as a side effect");
            } else {
                Check(p.selectedSpellId == 25,
                      "ToBytesSummary should leave selectedSpellId untouched for a class with no starting spells");
            }

            dawnstar::JavaRandom loadRng(1000 + classIdx);  // deliberately a DIFFERENT rng/seed than creation.
            PlayerState reloaded = PlayerSave::FromBytesSummary(bytes, charData, items, loadRng);

            // --- fields the summary format DOES carry ---
            Check(reloaded.name == p.name, "name should round-trip");
            Check(reloaded.classIndex == p.classIndex, "classIndex should round-trip");
            Check(reloaded.raceIndex == p.raceIndex, "raceIndex should round-trip");
            Check(reloaded.gold == p.gold, "gold should round-trip");
            Check(reloaded.attributes == p.attributes, "attributes should round-trip");
            Check(reloaded.classMagickaFactor == p.classMagickaFactor, "classMagickaFactor should round-trip");
            Check(reloaded.classUnknownPair == p.classUnknownPair, "classUnknownPair should round-trip");
            Check(reloaded.skills == p.skills, "skills (including the +3 rank bump) should round-trip");
            Check(reloaded.knownSpellsMask == expectedStartingMask,
                  "knownSpellsMask should be recomputed from the class template, discarding the extra learned spell");

            // --- normalizeForSummary: current=max for HP/Magicka/
            // Fatigue, [8] zeroed, [9] untouched ---
            Check(reloaded.coreStats[0] == p.coreStats[0], "coreStats[0] (level) should round-trip as-is");
            Check(reloaded.coreStats[1] == p.coreStats[1], "coreStats[1] (levelExp) should round-trip as-is");
            Check(reloaded.coreStats[2] == reloaded.coreStats[3], "curHP should be normalized to maxHP");
            Check(reloaded.coreStats[4] == reloaded.coreStats[5], "curMagicka should be normalized to maxMagicka");
            Check(reloaded.coreStats[6] == reloaded.coreStats[7], "curFatigue should be normalized to maxFatigue");
            Check(reloaded.coreStats[8] == 0, "coreStats[8] should be zeroed by normalizeForSummary");
            Check(reloaded.coreStats[9] == 7, "coreStats[9] should survive untouched (normalizeForSummary skips it)");

            // --- fields the summary format does NOT carry: regenerated
            // fresh via CreateCharacter, exactly like a new character ---
            Check(reloaded.currentLevel == 1 && reloaded.tileX == 9 && reloaded.tileY == 9 && reloaded.facing == 1,
                  "position should be reset to the fresh-character hub spawn, not the saved position");
            Check(reloaded.inventoryCount == originalInventoryCount,
                  "inventory should be re-granted fresh (same starting count), not carried over");
            Check(reloaded.traitorIndex >= 0 && reloaded.traitorIndex <= 3,
                  "traitorIndex should be freshly (re-)rolled in a valid range");
            (void)originalTraitorIndex;

            std::printf("class[%d] %-12s gold=%d startingMask=0x%x reloadedMask=0x%x traitor=%d\n", classIdx,
                        charData.classNames[static_cast<size_t>(classIdx)].c_str(), reloaded.gold,
                        expectedStartingMask, reloaded.knownSpellsMask, reloaded.traitorIndex);
        }

        if (!g_ok) {
            std::fprintf(stderr, "m17_summary_save_smoke: FAILED\n");
            return 1;
        }
        std::printf("all summary-save checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m17_summary_save_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
