#pragma once
#include <array>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "graphics/backbuffer.h"
#include "npc/shop_interaction.h"
#include "player/player_state.h"
#include "ui/screen.h"

namespace dawnstar {

enum class LevelUpAction { None, ReturnToGame };

// M49: the level-up flow -- ESGame.newLevelUpUI() + the secondaryParam 39
// branch of commandAction1(), GameCanvas.run()'s `levelUpPending` hand-off
// (main.cpp opens this), and Player.levelUp()/availableAttributeIncreases().
//
// Three consecutive "Select an attribute to increase N point(s):" prompt
// lists (3, then 2, then 1 point), each built fresh (selection back at the
// top) from the attributes whose bit is set in `attributeIncreaseFlags` --
// i.e. the attributes governing skills that ranked up since the last level.
// The list is the SAME every time (a chosen attribute is not removed), so
// picking one attribute three times gives it +6. There is no Cancel (the
// original removes the command and points `backTarget` at itself).
// After the third pick: attributes += 3/2/1, recalcMaxStats(), levelUp()
// (flags cleared, Shop.reset(), levelExp -= 10), back to the game.
//
// Player.availableAttributeIncreases() returns null when no flag is set,
// which the original then feeds to setupPromptList() (a NullPointerException
// on `items.length`). The port cannot crash that way: Open() returns false
// and the caller simply doesn't enter the menu.
class LevelUpMenu {
public:
    // Player.availableAttributeIncreases(): the name of attribute slot i*2
    // for every set flag bit i (0-7), in bit order.
    static std::vector<std::string> AvailableAttributeIncreases(const PlayerState& p, const CharacterData& charData);

    // Player.levelUp(): flags = 0; Shop.reset(); levelExp -= 10.
    static void ApplyLevelUp(PlayerState& p, ShopState& shop);

    // newLevelUpUI(1). False (and nothing opened) when no attribute is eligible.
    bool Open(const PlayerState& p, const CharacterData& charData);

    void OnUp() { screen_.MoveSelectionUp(); }
    void OnDown() { screen_.MoveSelectionDown(); }
    LevelUpAction OnSelect(PlayerState& p, ShopState& shop, const CharacterData& charData);
    void Render(Backbuffer& bb) const { screen_.Paint(bb); }

    const Screen& Current() const { return screen_; }
    // Which of the three picks (0-2) the screen is showing.
    int Step() const { return step_; }

private:
    void BuildStep(const PlayerState& p, const CharacterData& charData, int step);

    Screen screen_{ScreenMode::PromptList};
    int step_ = 0;
    // ESGame.attribIncr[3]: the attribute slot picked at each step.
    std::array<int, 3> attribIncr_{};
};

}  // namespace dawnstar
