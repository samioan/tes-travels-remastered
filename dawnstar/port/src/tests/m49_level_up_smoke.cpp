// M49 smoke test: LevelUpMenu (ui/level_up_menu.h) -- ESGame.newLevelUpUI()
// + the secondaryParam 39 branch, Player.availableAttributeIncreases() and
// Player.levelUp().
//
// No JVM ground truth. Expectations are hand-derived from ../../src/
// ESGame.java (newLevelUpUI, the 39 branch), ../../src/Player.java
// (availableAttributeIncreases, levelUp, gainSkillExp, recalcMaxStats) and
// ../../src/Shop.java (reset), using a real character and the real
// character data. The end-to-end case drives the real GainSkillExp into the
// menu.
//
// Usage: level_up_smoke [assetRoot]   (default: ../../extracted)
#include <cstdio>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/item_database.h"
#include "npc/shop_interaction.h"
#include "player/player_combat_stats.h"
#include "player/player_creation.h"
#include "ui/level_up_menu.h"
#include "util/java_random.h"

namespace {

using dawnstar::LevelUpAction;
using dawnstar::LevelUpMenu;
using dawnstar::PlayerState;
using dawnstar::ShopState;

bool g_ok = true;

// The level-up screen as ESGame.newLevelUpUI() builds it, rendered
// independently: a prompt list with `prompt` over `items` and no Cancel.
// Compared by pixels so the wrapped prompt text is checked in full.
bool ShowsPrompt(const LevelUpMenu& m, const std::string& prompt, const std::vector<std::string>& items,
                 int selected) {
    dawnstar::Screen expected(dawnstar::ScreenMode::PromptList);
    expected.SetupPromptList("Level Up", prompt, items);
    expected.RemoveCommand(dawnstar::CommandId::Cancel);
    expected.SetSelectedIndex(selected);
    dawnstar::Backbuffer a, b;
    m.Render(a);
    expected.Paint(b);
    for (int i = 0; i < dawnstar::Backbuffer::kWidth * dawnstar::Backbuffer::kHeight; i++) {
        if (a.Data()[i] != b.Data()[i]) return false;
    }
    return true;
}

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

}  // namespace

int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : "../../extracted";
    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        dawnstar::CharacterData charData = dawnstar::CharacterData::Load(archive);
        dawnstar::JavaRandom rng(49);
        const auto& names = charData.attributeNames;

        // --- no flag set: nothing to open (Java would NPE) ---
        {
            PlayerState p = dawnstar::PlayerCreation::CreateCharacter(0, "Lv", charData, items, rng);
            p.attributeIncreaseFlags = 0;
            LevelUpMenu m;
            Check(LevelUpMenu::AvailableAttributeIncreases(p, charData).empty(), "no flags -> no attributes");
            Check(!m.Open(p, charData), "Open() refuses when no attribute is eligible");
        }

        // --- three picks: +3 / +2 / +1 on chosen entries ---
        {
            PlayerState p = dawnstar::PlayerCreation::CreateCharacter(0, "Lv", charData, items, rng);
            ShopState shop = ShopState::Reset();
            shop.firstVisit[2] = false;
            shop.rewardsGiven[1] = 5;
            shop.questState1[0] = 3;
            shop.showDeathGreeting = true;
            p.attributeIncreaseFlags = (1 << 0) | (1 << 2) | (1 << 5);
            p.coreStats[1] = 12;
            const std::array<int16_t, 16> before = p.attributes;

            std::vector<std::string> avail = LevelUpMenu::AvailableAttributeIncreases(p, charData);
            Check(avail.size() == 3 && avail[0] == names[0] && avail[1] == names[4] && avail[2] == names[10],
                  "bits 0,2,5 -> attribute slots 0,4,10 in bit order");

            LevelUpMenu m;
            Check(m.Open(p, charData) && m.Step() == 0, "Open() shows step 1");
            Check(m.Current().Title() == "Level Up" &&
                      ShowsPrompt(m, "Select an attribute to increase 3 points:", avail, 0),
                  "step 1: title, 3-point prompt, the eligible names");

            // Pick the 3rd entry (slot 10) for +3.
            m.OnDown();
            m.OnDown();
            Check(m.OnSelect(p, shop, charData) == LevelUpAction::None && m.Step() == 1, "first pick advances");
            Check(ShowsPrompt(m, "Select an attribute to increase 2 points:", avail, 0),
                  "step 2: 2-point prompt, same list, selection back at the top");
            m.OnDown();  // slot 4 for +2
            Check(m.OnSelect(p, shop, charData) == LevelUpAction::None && m.Step() == 2, "second pick advances");
            Check(ShowsPrompt(m, "Select an attribute to increase 1 point:", avail, 0),
                  "step 3: 1-point prompt (singular), selection reset");
            // Slot 0 for +1 (selection is already at the top).
            Check(p.attributes == before, "nothing is applied before the third pick");
            Check(m.OnSelect(p, shop, charData) == LevelUpAction::ReturnToGame, "third pick returns to the game");

            Check(p.attributes[10] == before[10] + 3 && p.attributes[4] == before[4] + 2 &&
                      p.attributes[0] == before[0] + 1,
                  "+3/+2/+1 landed on the picked attributes");
            for (size_t i = 0; i < 16; i++) {
                if (i != 10 && i != 4 && i != 0) Check(p.attributes[i] == before[i], "no other attribute changed");
            }
            Check(p.coreStats[3] == (p.attributes[0] + p.attributes[10]) / 2 &&
                      p.coreStats[7] == p.attributes[0] + p.attributes[4] + p.attributes[6] + p.attributes[10],
                  "max HP/Fatigue recalculated from the new attributes");
            Check(p.attributeIncreaseFlags == 0, "levelUp() cleared attributeIncreaseFlags");
            Check(p.coreStats[1] == 2, "levelUp() consumed 10 level-exp (12 -> 2)");
            const ShopState fresh = ShopState::Reset();
            Check(shop.firstVisit == fresh.firstVisit && shop.rewardsGiven == fresh.rewardsGiven &&
                      shop.questState1 == fresh.questState1 && !shop.showDeathGreeting,
                  "levelUp() ran Shop.reset() (restock/quest state cleared)");
        }

        // --- one eligible attribute picked three times: +6 ---
        {
            PlayerState p = dawnstar::PlayerCreation::CreateCharacter(1, "Lv", charData, items, rng);
            ShopState shop = ShopState::Reset();
            p.attributeIncreaseFlags = 1 << 3;
            p.coreStats[1] = 10;
            int16_t before = p.attributes[6];
            LevelUpMenu m;
            Check(m.Open(p, charData) && m.Current().Items().size() == 1 && m.Current().Items()[0] == names[6],
                  "a single flag offers a single attribute");
            m.OnSelect(p, shop, charData);
            m.OnSelect(p, shop, charData);
            Check(m.OnSelect(p, shop, charData) == LevelUpAction::ReturnToGame, "same attribute three times");
            Check(p.attributes[6] == before + 6, "the list is not consumed: +3 +2 +1 = +6 on one attribute");
        }

        // --- end to end: real GainSkillExp -> pending -> menu ---
        {
            PlayerState p = dawnstar::PlayerCreation::CreateCharacter(2, "Lv", charData, items, rng);
            ShopState shop = ShopState::Reset();
            p.attributeIncreaseFlags = 0;
            p.levelUpPending = false;
            p.coreStats[1] = 9;
            p.skills[3][2] = 10;
            int level = p.coreStats[0];
            dawnstar::PlayerCombatStats::GainSkillExp(p, charData, 3, 1);
            Check(p.levelUpPending && p.coreStats[0] == level + 1 && p.coreStats[1] == 10,
                  "GainSkillExp's rank-up reaches 10 level-exp: level+1 and levelUpPending");
            int bit = charData.skillAttributeIndex[3] / 2;
            Check(p.attributeIncreaseFlags == (1 << bit), "the ranked skill flagged its governing attribute");
            LevelUpMenu m;
            Check(m.Open(p, charData) && m.Current().Items().size() == 1 &&
                      m.Current().Items()[0] == names[static_cast<size_t>(bit * 2)],
                  "the menu offers exactly that attribute");
            m.OnSelect(p, shop, charData);
            m.OnSelect(p, shop, charData);
            m.OnSelect(p, shop, charData);
            Check(p.coreStats[1] == 0 && p.attributeIncreaseFlags == 0, "after the picks: exp consumed, flags cleared");
        }
    } catch (const std::exception& e) {
        std::printf("level_up_smoke: exception: %s\n", e.what());
        return 2;
    }

    if (!g_ok) {
        std::printf("level_up_smoke: FAILED\n");
        return 1;
    }
    std::printf("level_up_smoke: all checks passed\n");
    return 0;
}
