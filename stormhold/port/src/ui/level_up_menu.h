#pragma once
#include <array>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "graphics/backbuffer.h"
#include "player/player_state.h"
#include "world/shop_state.h"

namespace stormhold {

// `../../../src/ESGame.java`'s own level-up flow: `newLevelUpUI(step)`
// (`new UIScreen(this, 5, 39)`, a "Level Up" prompt list over
// `player.pendingLevelUpAttributeNames()`, cmdBack removed) plus
// commandAction()'s own screenGroup 39 handler. Opened by GameCanvas.run()
// the tick `player.tryRankUpSkills()` returns true (main.cpp), which also
// pauses the game until the third pick is confirmed.
//
// Three picks in a row -- step 1 "+3 points", step 2 "+2", step 3 "+1" --
// the same attribute can be picked more than once, exactly like the
// original (each step is a fresh list over the same names). Each pick is
// resolved back to an attribute index the way screenGroup 39 does it:
// the FIRST `CharacterData::attributeNames[i]` equal to the chosen label.
// After the third, PlayerLeveling::ApplyLevelUpAttributeChoices applies
// +3/+2/+1, recomputes derived stats and spends the 10 level-exp.
//
// No Cancel: the original removes cmdBack from this screen, so Escape
// does nothing here either.
struct LevelUpMenuState {
    bool active = false;
    int step = 0;  // 0..2 -- ESGame's own ui.contextIndex.
    int selectedIndex = 0;
    std::vector<std::string> names;
    std::array<int, 3> choices{-1, -1, -1};  // ESGame.levelUpAttributeChoices.
};

class LevelUpMenu {
public:
    static void Open(LevelUpMenuState& state, const PlayerState& p, const CharacterData& charData);
    static void MoveSelection(LevelUpMenuState& state, int delta);
    // Returns true once the third pick has been applied and the screen
    // closed (the caller resumes gameplay).
    static bool Confirm(LevelUpMenuState& state, PlayerState& p, const CharacterData& charData, ShopState& shop);
    static void Render(Backbuffer& bb, const LevelUpMenuState& state);
};

}  // namespace stormhold
