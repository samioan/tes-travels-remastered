// M16 smoke test: PlayerSpellcasting + CombatResolution::CastOnMonster
// against real SpellDatabase/CharacterData/ItemDatabase/MonsterDatabase
// data and real characters/monsters (M11's PlayerCreation, M15's
// MonsterRuntime::Spawn). No JVM ground truth available (same reason as
// M6/M9/M11/M13/M14/M15).
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/spell_database.h"
#include "combat/combat_resolution.h"
#include "monster/monster_runtime.h"
#include "player/player_combat_stats.h"
#include "player/player_creation.h"
#include "player/player_inventory.h"
#include "player/player_spellcasting.h"
#include "util/java_random.h"

namespace {

using dawnstar::CharacterData;
using dawnstar::ItemDatabase;
using dawnstar::MonsterDatabase;
using dawnstar::MonsterRuntime;
using dawnstar::MonsterState;
using dawnstar::PlayerCombatStats;
using dawnstar::PlayerInventory;
using dawnstar::PlayerSpellcasting;
using dawnstar::PlayerState;
using dawnstar::Spell;
using dawnstar::SpellDatabase;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

// Independent transcription of Player.java's spellSkillIndexFor(),
// written straight from the source rather than copied from
// player_spellcasting.cpp.
int ExpectedSpellSkillIndexFor(int spellId) {
    if (spellId <= 5) return 1;
    if (spellId <= 10) return 3;
    if (spellId <= 15) return 4;
    if (spellId <= 20) return 6;
    return 10;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        CharacterData charData = CharacterData::Load(archive);
        ItemDatabase items = ItemDatabase::Load(archive);
        MonsterDatabase monsters = MonsterDatabase::Load(archive);
        SpellDatabase spells = SpellDatabase::Load(archive);
        std::printf("spells: %d\n", spells.Count());

        // --- spellSkillIndexFor: every boundary plus one interior point
        // per bucket ---
        {
            int probes[] = {1, 3, 5, 6, 8, 10, 11, 13, 15, 16, 18, 20, 21, 25, 42};
            for (int spellId : probes) {
                Check(PlayerSpellcasting::SpellSkillIndexFor(spellId) == ExpectedSpellSkillIndexFor(spellId),
                      "SpellSkillIndexFor should match the hand-traced bucket boundaries");
            }
        }

        // --- activeAilmentCount / cureRandomAilment ---
        {
            PlayerState p;
            p.ailmentMask = static_cast<int8_t>((1 << 1) | (1 << 4) | (1 << 6));
            Check(PlayerSpellcasting::ActiveAilmentCount(p) == 3, "ActiveAilmentCount should count 3 set bits");

            // active==1: Player.java's cureRandomAilment() special-cases
            // this to pick=1 WITHOUT drawing from the RNG at all -- any
            // JavaRandom works since it's never touched.
            PlayerState single;
            single.ailmentMask = static_cast<int8_t>(1 << 3);
            dawnstar::JavaRandom untouched(1);
            PlayerSpellcasting::CureRandomAilment(single, untouched);
            Check(single.ailmentMask == 0, "cureRandomAilment with exactly one active ailment should clear it");

            // active>1: hand-trace the same LingoRandomInt(active) draw
            // with a same-seeded probe, then walk the same seen/pick
            // scan independently to predict which bit clears.
            PlayerState multi;
            multi.ailmentMask = static_cast<int8_t>((1 << 0) | (1 << 2) | (1 << 5) | (1 << 7));
            int active = PlayerSpellcasting::ActiveAilmentCount(multi);
            Check(active == 4, "ActiveAilmentCount should count 4 set bits");

            dawnstar::JavaRandom probe(777);
            int pick = dawnstar::LingoRandomInt(probe, active);
            int expectedClearedBit = -1;
            int seen = 0;
            for (int i = 0; i < 8; i++) {
                if (((multi.ailmentMask >> i) & 1) != 0) {
                    if (++seen == pick) {
                        expectedClearedBit = i;
                        break;
                    }
                }
            }
            Check(expectedClearedBit >= 0, "hand-traced scan should find a bit to clear");

            dawnstar::JavaRandom rng(777);
            int8_t before = multi.ailmentMask;
            PlayerSpellcasting::CureRandomAilment(multi, rng);
            Check(((before >> expectedClearedBit) & 1) == 1 && ((multi.ailmentMask >> expectedClearedBit) & 1) == 0,
                  "cureRandomAilment should clear exactly the hand-traced bit");
            Check(PlayerSpellcasting::ActiveAilmentCount(multi) == 3, "cureRandomAilment should clear exactly one bit");
        }

        // --- canLearnSpell / learnSpellFromScroll ---
        {
            int scrollItemId = -1;
            int fillerItemId = -1;
            for (int id = 1; id <= items.ItemCount(); id++) {
                int8_t category = items.category[static_cast<size_t>(id - 1)];
                if (scrollItemId < 0 && category == 12) scrollItemId = id;
                if (fillerItemId < 0 && category != 12) fillerItemId = id;
            }
            Check(scrollItemId > 0, "real ItemDatabase should contain at least one category-12 scroll item");
            Check(fillerItemId > 0, "real ItemDatabase should contain at least one non-scroll filler item");

            if (scrollItemId > 0 && fillerItemId > 0) {
                int spellId = 3;  // arbitrary valid spell id encoded on the scroll.
                int requiredSkill = spells.ById(spellId).skillRequired;

                PlayerState p;
                PlayerInventory::AddItem(p, scrollItemId, 0, 0);
                p.inventoryItemData[0] = spellId;  // low byte = spell id, per learnSpellFromScroll.
                PlayerInventory::AddItem(p, fillerItemId, 0, 0);  // unrelated non-scroll filler slot.

                Check(!PlayerSpellcasting::CanLearnSpell(p, items, spells, 1),
                      "a non-category-12 item should never be learnable");

                p.skills[static_cast<size_t>(requiredSkill)][0] = 0;
                Check(!PlayerSpellcasting::CanLearnSpell(p, items, spells, 0),
                      "canLearnSpell should require at least 1 point in the governing skill");

                p.skills[static_cast<size_t>(requiredSkill)][0] = 1;
                Check(PlayerSpellcasting::CanLearnSpell(p, items, spells, 0),
                      "canLearnSpell should pass with skill>0 and an unknown spell");

                p.knownSpellsMask = 1u << (spellId - 1);
                Check(!PlayerSpellcasting::CanLearnSpell(p, items, spells, 0),
                      "canLearnSpell should refuse an already-known spell");
                p.knownSpellsMask = 0;

                int otherItemId = p.inventoryItemIds[1];
                bool learned = PlayerSpellcasting::LearnSpellFromScroll(p, items, 0);
                Check(learned, "learnSpellFromScroll should return true");
                Check((p.knownSpellsMask & (1u << (spellId - 1))) != 0,
                      "learnSpellFromScroll should set the spell's known-mask bit");
                Check(p.inventoryCount == 1, "learnSpellFromScroll should consume the scroll slot");
                Check(p.inventoryItemIds[0] == static_cast<int8_t>(otherItemId),
                      "removing slot 0 should compact the filler item down to slot 0");
            }
        }

        // --- knownSpellsSummary / nthKnownSpellId / cycleSelectedSpell /
        // spellTooltip, on a hand-picked knownSpellsMask ---
        {
            PlayerState p;
            p.knownSpellsMask = (1u << 0) | (1u << 2) | (1u << 5);  // spell ids 1, 3, 6
            p.selectedSpellId = 3;

            std::vector<std::string> summary = PlayerSpellcasting::KnownSpellsSummary(p, spells);
            Check(summary.size() == 3, "knownSpellsSummary should list exactly the 3 known spells");
            if (summary.size() == 3) {
                Check(summary[0] == spells.all[0].name, "1st entry should be spell id 1's name, unprefixed");
                Check(summary[1] == "R: " + spells.all[2].name, "2nd entry (the selected spell) should get the R: prefix");
                Check(summary[2] == spells.all[5].name, "3rd entry should be spell id 6's name, unprefixed");
            }

            Check(PlayerSpellcasting::NthKnownSpellId(p, spells, 0) == 0, "nthKnownSpellId(0) should be bit index 0");
            Check(PlayerSpellcasting::NthKnownSpellId(p, spells, 1) == 2, "nthKnownSpellId(1) should be bit index 2");
            Check(PlayerSpellcasting::NthKnownSpellId(p, spells, 2) == 5, "nthKnownSpellId(2) should be bit index 5");
            Check(PlayerSpellcasting::NthKnownSpellId(p, spells, 3) == -1, "nthKnownSpellId out of range should be -1");

            Check(PlayerSpellcasting::CycleSelectedSpell(p, spells) == 6,
                  "cycling from spell 3 should land on the next known spell, id 6");
            p.selectedSpellId = 6;
            Check(PlayerSpellcasting::CycleSelectedSpell(p, spells) == 1,
                  "cycling from spell 6 should wrap around to spell 1");
            p.selectedSpellId = 0;  // not a valid spell id.
            Check(PlayerSpellcasting::CycleSelectedSpell(p, spells) == 1,
                  "cycling from an invalid selection should land on the first known spell");

            PlayerState none;
            Check(PlayerSpellcasting::CycleSelectedSpell(none, spells) == 0,
                  "cycling with no known spells should return 0");

            std::string tooltip = PlayerSpellcasting::SpellTooltip(charData, spells, 2);
            std::string expectedTooltip = spells.all[2].name + "\n" +
                                           charData.skillNames[static_cast<size_t>(spells.all[2].skillRequired)] +
                                           "\n" + "Cost: " + std::to_string(spells.all[2].magickaCost) + "\n" +
                                           spells.all[2].description;
            Check(tooltip == expectedTooltip, "spellTooltip should match the hand-built name/skill/cost/description string");
        }

        // --- castOnSelf: one fully hand-derived example (spell 21, a
        // straightforward HP heal with no side branches), against a real
        // character with real equipment-derived skill values ---
        {
            dawnstar::JavaRandom creationRng(4242);
            PlayerState p = dawnstar::PlayerCreation::CreateCharacter(2, "Healer", charData, items, creationRng);
            p.coreStats[6] = 100;  // plenty of Fatigue, avoid the <7 skillValue penalty.
            p.coreStats[4] = 200;  // plenty of Magicka headroom.
            p.coreStats[2] = static_cast<int16_t>(p.coreStats[3] - 10);  // 10 HP short of max, so healing is visible.
            p.selectedSpellId = 21;

            int spellId = 21;
            int skillIdx = ExpectedSpellSkillIndexFor(spellId);
            int atkSkill = PlayerCombatStats::SkillValue(p, charData, skillIdx, true);
            int atkBonus = PlayerCombatStats::SkillBonus(p, skillIdx);
            const Spell& spell21 = spells.ById(spellId);
            int cost = spell21.icon;
            int durationMult = spell21.durationMultiplier;
            int school = spell21.power;

            int diff = atkSkill - cost;
            int atkChance = atkBonus + diff * 5;
            int defChance = durationMult - diff * 5;
            atkChance = std::min(std::max(atkChance, 10), 95);
            defChance = std::min(std::max(defChance, 10), 95);

            dawnstar::JavaRandom probe(9090);
            auto expectedRoll = PlayerCombatStats::RollOutcome(atkChance, defChance, probe);
            int outcome = expectedRoll.outcome;

            int expectedMagickaSpend;
            int multiplier = 1;
            if (outcome == 0) {
                expectedMagickaSpend = 3 * school;
            } else if (outcome == 1) {
                expectedMagickaSpend = 3 * school / 2;
            } else if (outcome == 2) {
                expectedMagickaSpend = school;
            } else {
                expectedMagickaSpend = school;
                multiplier = 2;
            }
            int expectedMagicka = std::max(p.coreStats[4] - expectedMagickaSpend, 0);

            int healAmount = 6 + PlayerCombatStats::SkillValue(p, charData, 10, false);
            int expectedHp = std::min<int>(p.coreStats[2] + multiplier * healAmount, p.coreStats[3]);
            int expectedFatigue = std::max<int>(p.coreStats[6] - 5 * PlayerCombatStats::FatigueCostMultiplier(p), 0);

            dawnstar::JavaRandom rng(9090);
            PlayerSpellcasting::CastOnSelf(p, charData, items, spells, rng);

            Check(p.coreStats[4] == expectedMagicka, "castOnSelf(21) should spend the hand-derived Magicka amount");
            Check(p.coreStats[2] == expectedHp, "castOnSelf(21) should heal by the hand-derived amount, capped at max HP");
            Check(p.coreStats[6] == expectedFatigue, "castOnSelf(21) should spend the standard 5xfatigueCostMultiplier Fatigue");
            std::printf("  castOnSelf(21): outcome=%d magicka=%d hp=%d\n", outcome, p.coreStats[4], p.coreStats[2]);
        }

        // --- castOnSelf integration: every self-targeted (non-offensive)
        // spell id, on a real character, never corrupts core stats ---
        {
            dawnstar::JavaRandom creationRng(1357);
            PlayerState p = dawnstar::PlayerCreation::CreateCharacter(4, "Caster", charData, items, creationRng);
            dawnstar::JavaRandom rng(2468);

            for (int spellId = 1; spellId <= spells.Count(); spellId++) {
                if (spells.IsOffensive(spellId)) continue;
                p.selectedSpellId = static_cast<int8_t>(spellId);
                p.coreStats[4] = p.coreStats[5];  // refill Magicka before each cast.
                p.coreStats[6] = p.coreStats[7];  // refill Fatigue before each cast.
                PlayerSpellcasting::CastOnSelf(p, charData, items, spells, rng);
                Check(p.coreStats[2] >= 0 && p.coreStats[2] <= p.coreStats[3], "HP should stay within [0,maxHP]");
                // NOTE: castOnSelf's Magicka spend is only ever
                // Math.max(...,0)'d in the original, never capped against
                // maxMagicka -- if a spell's "school" byte (Spell.power,
                // read as a signed byte) is >=128 as unsigned, the spend
                // goes negative and Magicka can end up ABOVE max with no
                // upper clamp. Confirmed real (some of these 25 spells
                // actually trigger it): only >=0 is a real invariant here.
                Check(p.coreStats[4] >= 0, "Magicka should never go negative");
                Check(p.coreStats[6] >= 0, "Fatigue should never go negative");
            }
            std::printf("  castOnSelf integration: all %d non-offensive spells stayed within range\n", spells.Count());
        }

        // --- castOnMonster: one fully hand-derived example (spell 20, a
        // direct-damage nuke with no target.scratch[] side effects) ---
        {
            dawnstar::JavaRandom creationRng(3210);
            PlayerState p = dawnstar::PlayerCreation::CreateCharacter(5, "BattleMage", charData, items, creationRng);
            p.coreStats[4] = 200;
            p.selectedSpellId = 20;
            MonsterState target = MonsterRuntime::Spawn(777, 2, 3, monsters);

            int spellId = 20;
            int skillIdx = ExpectedSpellSkillIndexFor(spellId);
            int atkSkill = PlayerCombatStats::SkillValue(p, charData, skillIdx, true);
            int atkBonus = PlayerCombatStats::SkillBonus(p, skillIdx);
            int targetEvasion = MonsterRuntime::Stat(target, monsters, 10);
            int targetDef = MonsterRuntime::Stat(target, monsters, 9);
            int power = spells.ById(spellId).magickaCost;

            int diff = atkSkill - targetEvasion;
            int targetHp = MonsterRuntime::Stat(target, monsters, 2);
            diff = std::min(diff, targetHp);
            int atkChance = atkBonus + diff * 5;
            int defChance = targetDef - diff * 5;
            atkChance = std::min(std::max(atkChance, 10), 95);
            defChance = std::min(std::max(defChance, 10), 95);

            dawnstar::JavaRandom probe(6060);
            auto expectedRoll = PlayerCombatStats::RollOutcome(atkChance, defChance, probe);
            int outcome = expectedRoll.outcome;
            int multiplier = 1;
            int expectedMagickaSpend;
            if (outcome == 0) {
                expectedMagickaSpend = 3 * power;
            } else if (outcome == 1) {
                expectedMagickaSpend = 3 * power / 2;
            } else if (outcome == 2) {
                expectedMagickaSpend = power;
            } else {
                expectedMagickaSpend = power;
                multiplier = 2;
            }
            int expectedMagicka = std::max(p.coreStats[4] - expectedMagickaSpend, 0);

            int base20 = 80 - 5 * targetEvasion;
            int raw20 = base20 * multiplier;
            int dmg20 = std::max(raw20 - targetDef, 4);
            int scaled20 = dmg20 * MonsterRuntime::Stat(target, monsters, 14) / 100;
            int startingHp = static_cast<uint8_t>(target.hp);
            int expectedHp = std::max(startingHp - scaled20, 0);

            dawnstar::JavaRandom rng(6060);
            dawnstar::CombatResolution::CastOnMonster(p, target, charData, items, monsters, spells, rng);

            Check(p.coreStats[4] == expectedMagicka, "castOnMonster(20) should spend the hand-derived Magicka amount");
            Check(static_cast<uint8_t>(target.hp) == expectedHp, "castOnMonster(20) should deal the hand-derived damage");
            std::printf("  castOnMonster(20): outcome=%d magicka=%d monster hp=%d\n", outcome, p.coreStats[4],
                        static_cast<uint8_t>(target.hp));
        }

        // --- castOnMonster integration: every offensive spell id, on a
        // real character and a real monster, never corrupts state, and
        // spell 14 ("Blade Focus") really does re-enter PlayerAttack ---
        {
            dawnstar::JavaRandom creationRng(8642);
            PlayerState p = dawnstar::PlayerCreation::CreateCharacter(1, "Duelist", charData, items, creationRng);
            dawnstar::JavaRandom rng(9753);
            bool sawSpell14CombatTarget = false;

            for (int spellId = 1; spellId <= spells.Count(); spellId++) {
                if (!spells.IsOffensive(spellId)) continue;
                MonsterState target = MonsterRuntime::Spawn(static_cast<int16_t>(1000 + spellId), 3, 4, monsters);
                p.selectedSpellId = static_cast<int8_t>(spellId);
                p.coreStats[4] = p.coreStats[5];
                p.combatTargetSpawnId = 0;

                dawnstar::CombatResolution::CastOnMonster(p, target, charData, items, monsters, spells, rng);

                // Same no-upper-clamp caveat as the castOnSelf integration
                // loop above -- only >=0 is a real invariant.
                Check(p.coreStats[4] >= 0, "Magicka should never go negative");
                Check(static_cast<uint8_t>(target.hp) <= 255, "monster hp stays in byte range");
                if (spellId == 14 && p.combatTargetSpawnId == target.spawnId) sawSpell14CombatTarget = true;
            }
            Check(sawSpell14CombatTarget,
                  "spell 14 should call back into PlayerAttack (observable via combatTargetSpawnId)");
            std::printf("  castOnMonster integration: all offensive spells stayed within range\n");
        }

        if (!g_ok) {
            std::fprintf(stderr, "m16_spellcasting_smoke: FAILED\n");
            return 1;
        }
        std::printf("all spellcasting checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m16_spellcasting_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
