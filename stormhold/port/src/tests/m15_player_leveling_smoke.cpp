// M15 smoke test: PlayerLeveling against a real created character, plus
// an integration check that combat/combat_resolution.h's PlayerAttack/
// MonsterTick now actually award the skill exp M13/M14 both had to defer.
#include <array>
#include <cstdio>
#include <string>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "combat/combat_resolution.h"
#include "monster/monster_runtime.h"
#include "player/player_creation.h"
#include "player/player_leveling.h"
#include "world/shop_state.h"

namespace {

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

void TestGainSkillExpGuard() {
    std::printf("-- GainSkillExp's skillIndex>=0 guard --\n");
    stormhold::PlayerState p;
    p.skills[0][2] = 5;
    stormhold::PlayerLeveling::GainSkillExp(p, -1, 1);
    Expect(p.skills[0][2] == 5, "a negative skillIndex (unarmed/unequipped) should be a no-op");
    stormhold::PlayerLeveling::GainSkillExp(p, 0, 3);
    Expect(p.skills[0][2] == 8, "a non-negative skillIndex should add to skills[i][2]");
}

void TestTryRankUpSkills(const stormhold::CharacterData& charData, const stormhold::ItemDatabase& items) {
    std::printf("-- TryRankUpSkills against a real created character --\n");
    stormhold::PlayerState p = stormhold::PlayerCreation::CreateCharacter(0, "Leveler", 1, charData, items);
    int rank0Before = p.skills[0][0];
    int rank3Before = p.skills[3][0];
    int rank5Before = p.skills[5][0];
    int levelExpBefore = p.coreStats[1];
    int8_t flagsBefore = p.levelUpAttributeFlags;

    p.skills[0][2] = 10;   // exactly at threshold
    p.skills[3][2] = 15;   // over threshold -- excess is NOT carried, just dropped (matches -= 10 exactly)
    p.skills[5][2] = 9;    // just under -- should NOT rank up

    bool leveledUp = stormhold::PlayerLeveling::TryRankUpSkills(p, charData);

    Expect(p.skills[0][0] == rank0Before + 1, "skill 0 should rank up (exp was exactly 10)");
    Expect(p.skills[0][2] == 0, "skill 0's exp should drop by exactly 10, not reset to 0 blindly");
    Expect(p.skills[3][0] == rank3Before + 1, "skill 3 should rank up (exp was over 10)");
    Expect(p.skills[3][2] == 5, "skill 3's excess exp (15-10=5) should be preserved, not discarded");
    Expect(p.skills[5][0] == rank5Before, "skill 5 should NOT rank up (exp was 9, under threshold)");
    Expect(p.skills[5][2] == 9, "skill 5's exp should be untouched");
    Expect(p.coreStats[1] == levelExpBefore + 2, "2 rank-ups this call should grant exactly 2 level-exp");

    int attr0 = charData.skillAttributeIndex[0];
    int attr3 = charData.skillAttributeIndex[3];
    Expect((p.levelUpAttributeFlags & (1 << (attr0 / 2))) != 0, "skill 0's governing-attribute bit should be set");
    Expect((p.levelUpAttributeFlags & (1 << (attr3 / 2))) != 0, "skill 3's governing-attribute bit should be set");
    Expect((p.levelUpAttributeFlags & ~flagsBefore) != 0, "at least one NEW flag bit should have been set by this call");

    Expect(!leveledUp, "2 level-exp (starting from 0) should not reach the 10-point character-level threshold yet");

    // Drive coreStats[1] up to the threshold directly to check the
    // level-up-triggered branch in isolation.
    p.coreStats[1] = 9;
    p.skills[7][2] = 10;
    int levelBefore = p.coreStats[0];
    bool leveledUp2 = stormhold::PlayerLeveling::TryRankUpSkills(p, charData);
    Expect(leveledUp2, "reaching exactly 10 level-exp should report a level-up");
    Expect(p.coreStats[0] == levelBefore + 1, "a reported level-up should increment coreStats[0]");
}

void TestConsumeLevelExpAndPendingNames(const stormhold::CharacterData& charData, const stormhold::ItemDatabase& items) {
    std::printf("-- ConsumeLevelExp + PendingLevelUpAttributeNames --\n");
    stormhold::PlayerState p = stormhold::PlayerCreation::CreateCharacter(0, "Leveler2", 1, charData, items);
    p.coreStats[1] = 12;

    // M54: ConsumeLevelExp now calls Shop::ClearQuestTurnInState for
    // real -- give it dirty questState1/2 first so the call is actually
    // observable, not just "still zero from a fresh ShopState".
    stormhold::ShopState shop;
    shop.questState1 = {1, 2, 3, 4};
    shop.questState2 = {5, 6, 7, 8};

    stormhold::PlayerLeveling::ConsumeLevelExp(p, shop);
    Expect(p.coreStats[1] == 2, "ConsumeLevelExp should subtract exactly 10");
    Expect(shop.questState1 == std::array<int8_t, 4>{0, 0, 0, 0},
           "ConsumeLevelExp should clear questState1 for shops 0-3 via Shop::ClearQuestTurnInState");
    Expect(shop.questState2 == std::array<int8_t, 4>{0, 0, 0, 0},
           "ConsumeLevelExp should clear questState2 for shops 0-3 via Shop::ClearQuestTurnInState");

    p.levelUpAttributeFlags = 0;
    Expect(stormhold::PlayerLeveling::PendingLevelUpAttributeNames(p, charData).empty(),
           "no flagged bits should yield an empty (not null-shaped) vector");

    p.levelUpAttributeFlags = static_cast<int8_t>((1 << 0) | (1 << 3));
    std::vector<std::string> names = stormhold::PlayerLeveling::PendingLevelUpAttributeNames(p, charData);
    Expect(names.size() == 2, "2 flagged bits should yield exactly 2 names");
    Expect(names[0] == charData.attributeNames[0], "bit 0 should map to attributeNames[0]");
    Expect(names[1] == charData.attributeNames[6], "bit 3 should map to attributeNames[3*2]=attributeNames[6]");
}

void TestApplyLevelUpAttributeChoices(const stormhold::CharacterData& charData, const stormhold::ItemDatabase& items) {
    std::printf("-- ApplyLevelUpAttributeChoices' +3/+2/+1 weighting + derived-stat recompute --\n");
    stormhold::PlayerState p = stormhold::PlayerCreation::CreateCharacter(0, "Leveler3", 1, charData, items);
    p.coreStats[1] = 10;

    int16_t a0Before = p.attributes[0];
    int16_t a10Before = p.attributes[10];
    int16_t a2Before = p.attributes[2];
    int16_t maxHpBefore = p.coreStats[3];
    int16_t maxMagickaBefore = p.coreStats[5];

    // Choose attribute index 0 (feeds coreStats[3]/maxHP), 2 (feeds
    // coreStats[5]/maxMagicka), and 10 (feeds coreStats[3] too) as
    // first/second/third picks -- deliberately overlapping so the +3/+2
    // weighting is independently checkable per attribute.
    stormhold::ShopState shop;
    shop.questState1[0] = 9;  // M54: dirtied so ConsumeLevelExp's own clear is observable below.
    stormhold::PlayerLeveling::ApplyLevelUpAttributeChoices(p, 0, 2, 10, shop);

    Expect(p.attributes[0] == a0Before + 3, "the FIRST choice should get +3");
    Expect(p.attributes[2] == a2Before + 2, "the SECOND choice should get +2");
    Expect(p.attributes[10] == a10Before + 1, "the THIRD choice should get +1");

    int16_t expectedMaxHp = static_cast<int16_t>((p.attributes[0] + p.attributes[10]) / 2);
    Expect(p.coreStats[3] == expectedMaxHp, "maxHP should be recomputed from the NEW attribute values");
    Expect(p.coreStats[3] != maxHpBefore, "maxHP should actually have changed given attributes[0]/[10] both moved");

    int16_t expectedMaxMagicka = static_cast<int16_t>(p.classMagickaFactor * p.attributes[2] / 4);
    Expect(p.coreStats[5] == expectedMaxMagicka, "maxMagicka should be recomputed from the new attributes[2]");
    Expect(p.coreStats[5] != maxMagickaBefore || p.classMagickaFactor == 0,
           "maxMagicka should change given attributes[2] moved (unless this class has 0 magicka factor)");

    Expect(p.coreStats[1] == 0, "ApplyLevelUpAttributeChoices should consume the 10 level-exp");
    Expect(shop.questState1[0] == 0,
           "ApplyLevelUpAttributeChoices's own ConsumeLevelExp call should clear questState1 too (M54)");
}

void TestCombatAwardsSkillExp(const stormhold::CharacterData& charData, const stormhold::ItemDatabase& items,
                               const stormhold::MonsterDatabase& monsters) {
    std::printf("-- combat/combat_resolution.h now awards skill exp on a strong hit --\n");
    // PlayerAttack: run enough seeds that at least one produces a tier>=2
    // hit (the exp-award branch) against a real Barbarian (equips an
    // Axe -> ActiveWeaponSkillIndex()==0, confirmed in M13's own test).
    bool sawPlayerAttackExpGain = false;
    for (int64_t seed = 0; seed < 200 && !sawPlayerAttackExpGain; seed++) {
        stormhold::PlayerState player = stormhold::PlayerCreation::CreateCharacter(0, "ExpTest", 1, charData, items);
        stormhold::MonsterState target = stormhold::MonsterRuntime::Spawn(1, 1, 2, monsters);
        stormhold::WorldRegistry world(37);
        int16_t expBefore = player.skills[0][2];
        stormhold::JavaRandom rng(seed);
        stormhold::CombatResolution::PlayerAttack(player, target, charData, items, monsters, rng, world);
        if (player.skills[0][2] > expBefore) sawPlayerAttackExpGain = true;
    }
    Expect(sawPlayerAttackExpGain, "at least one of 200 seeds should produce a tier>=2 PlayerAttack hit that awards skill exp");

    // MonsterTick: same idea, watching the player's DEFENSE skill (7,
    // since a Barbarian's equipped armor here isn't a shield -- see
    // M13's own DefenseSkillIndex test) for a successful-block exp award.
    stormhold::GeneratedLevel level;
    level.number = 2;
    level.width = level.height = 35;
    level.tiles.assign(35, std::vector<uint8_t>(35, 0));

    bool sawMonsterTickExpGain = false;
    for (int64_t seed = 0; seed < 200 && !sawMonsterTickExpGain; seed++) {
        stormhold::PlayerState player = stormhold::PlayerCreation::CreateCharacter(0, "ExpTest2", 1, charData, items);
        stormhold::MonsterState m = stormhold::MonsterRuntime::Spawn(2, 1, 2, monsters);
        stormhold::WorldRegistry world(37);
        int16_t expBefore = player.skills[7][2];
        stormhold::JavaRandom rng(seed);
        stormhold::JavaRandom ambushRng(seed + 500);
        int16_t spawnIdCounter = 0;
        stormhold::CombatResolution::MonsterTick(m, player, charData, items, monsters, 0, rng, level, world,
                                                  ambushRng, spawnIdCounter);
        if (player.skills[7][2] > expBefore) sawMonsterTickExpGain = true;
    }
    Expect(sawMonsterTickExpGain, "at least one of 200 seeds should produce a MonsterTick block that awards defense skill exp");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        stormhold::AssetRoot assets(root);
        stormhold::CharacterData charData = stormhold::CharacterData::Load(assets);
        stormhold::ItemDatabase items = stormhold::ItemDatabase::Load(assets);
        stormhold::MonsterDatabase monsters = stormhold::MonsterDatabase::Load(assets);

        TestGainSkillExpGuard();
        TestTryRankUpSkills(charData, items);
        TestConsumeLevelExpAndPendingNames(charData, items);
        TestApplyLevelUpAttributeChoices(charData, items);
        TestCombatAwardsSkillExp(charData, items, monsters);

        if (!g_ok) {
            std::fprintf(stderr, "m15_player_leveling_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m15_player_leveling_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
