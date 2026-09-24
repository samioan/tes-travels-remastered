// M77 smoke test: ui/level_up_menu.h -- ESGame's own newLevelUpUI(step)/
// screenGroup 39 three-pick flow, driven from a real created character
// that has just crossed the 10-level-exp threshold.
#include <cstdio>
#include <string>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/item_database.h"
#include "graphics/backbuffer.h"
#include "player/player_creation.h"
#include "player/player_leveling.h"
#include "ui/level_up_menu.h"
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

using namespace stormhold;

int AttributeIndexOf(const CharacterData& charData, const std::string& name) {
    for (size_t i = 0; i < charData.attributeNames.size(); i++) {
        if (charData.attributeNames[i] == name) return static_cast<int>(i);
    }
    return -1;
}

void TestThreePickFlow(const CharacterData& charData, const ItemDatabase& items) {
    std::printf("-- LevelUpMenu: three picks apply +3/+2/+1 and spend 10 level-exp --\n");

    PlayerState p = PlayerCreation::CreateCharacter(0, "Leveler", 1, charData, items);
    p.coreStats[1] = 9;
    p.skills[0][2] = 10;
    p.skills[3][2] = 10;
    bool leveled = PlayerLeveling::TryRankUpSkills(p, charData);
    Expect(leveled, "setup: TryRankUpSkills should report a level-up");

    LevelUpMenuState state;
    LevelUpMenu::Open(state, p, charData);
    Expect(state.active, "Open should activate the screen");
    Expect(!state.names.empty(), "Open should list the pending attribute names");
    Expect(state.step == 0, "Open starts at step 0 (+3)");

    ShopState shop;
    shop.questState1[0] = 7;

    // Pick the last listed name twice (same attribute may repeat, like the
    // original), then the first.
    int last = static_cast<int>(state.names.size()) - 1;
    const std::string firstPick = state.names[static_cast<size_t>(last)];
    const std::string thirdPick = state.names[0];
    int firstAttr = AttributeIndexOf(charData, firstPick);
    int thirdAttr = AttributeIndexOf(charData, thirdPick);
    Expect(firstAttr >= 0 && thirdAttr >= 0, "listed names resolve back to attribute indices");

    int16_t firstBefore = p.attributes[static_cast<size_t>(firstAttr)];
    int16_t thirdBefore = p.attributes[static_cast<size_t>(thirdAttr)];
    int16_t levelExpBefore = p.coreStats[1];

    LevelUpMenu::MoveSelection(state, -1);  // wraps from 0 to the last row
    Expect(state.selectedIndex == last, "MoveSelection(-1) from row 0 wraps to the last row");
    Expect(!LevelUpMenu::Confirm(state, p, charData, shop), "first pick does not finish the flow");
    Expect(state.step == 1 && state.selectedIndex == 0, "first pick advances to step 1 and resets the cursor");

    LevelUpMenu::MoveSelection(state, -1);
    Expect(!LevelUpMenu::Confirm(state, p, charData, shop), "second pick does not finish the flow");
    Expect(state.step == 2, "second pick advances to step 2");
    Expect(p.attributes[static_cast<size_t>(firstAttr)] == firstBefore, "nothing is applied before the third pick");

    Expect(LevelUpMenu::Confirm(state, p, charData, shop), "third pick finishes the flow");
    Expect(!state.active, "the screen closes after the third pick");

    if (firstAttr == thirdAttr) {
        Expect(p.attributes[static_cast<size_t>(firstAttr)] == firstBefore + 6, "+3 +2 +1 all on one attribute");
    } else {
        Expect(p.attributes[static_cast<size_t>(firstAttr)] == firstBefore + 5,
               "the attribute picked first and second gains 3+2");
        Expect(p.attributes[static_cast<size_t>(thirdAttr)] == thirdBefore + 1, "the third pick gains 1");
    }
    Expect(p.coreStats[1] == levelExpBefore - 10, "ConsumeLevelExp spends exactly 10 level-exp");
    Expect(shop.questState1[0] == 0, "ConsumeLevelExp clears quest turn-in state (M54)");
}

void TestRenderDoesNotCrash(const CharacterData& charData, const ItemDatabase& items) {
    std::printf("-- LevelUpMenu::Render paints every step --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Leveler", 1, charData, items);
    p.levelUpAttributeFlags = static_cast<int8_t>(0x7F);
    LevelUpMenuState state;
    LevelUpMenu::Open(state, p, charData);
    Backbuffer bb;
    for (int step = 0; step < 3; step++) {
        state.step = step;
        LevelUpMenu::Render(bb, state);
    }
    Expect(true, "rendered");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assets(root);
        CharacterData charData = CharacterData::Load(assets);
        ItemDatabase items = ItemDatabase::Load(assets);

        TestThreePickFlow(charData, items);
        TestRenderDoesNotCrash(charData, items);

        if (!g_ok) {
            std::fprintf(stderr, "m77_level_up_menu_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m77_level_up_menu_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
