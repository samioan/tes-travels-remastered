#include "ui/level_up_menu.h"

#include "player/player_creation.h"

namespace dawnstar {

std::vector<std::string> LevelUpMenu::AvailableAttributeIncreases(const PlayerState& p,
                                                                   const CharacterData& charData) {
    std::vector<std::string> out;
    for (int i = 0; i < 8; i++) {
        if ((p.attributeIncreaseFlags & (1 << i)) != 0) {
            out.push_back(charData.attributeNames[static_cast<size_t>(i * 2)]);
        }
    }
    return out;
}

void LevelUpMenu::ApplyLevelUp(PlayerState& p, ShopState& shop) {
    p.attributeIncreaseFlags = 0;
    shop = ShopState::Reset();
    p.coreStats[1] = static_cast<int16_t>(p.coreStats[1] - 10);
}

void LevelUpMenu::BuildStep(const PlayerState& p, const CharacterData& charData, int step) {
    static const char* const kPrompts[3] = {"Select an attribute to increase 3 points:",
                                            "Select an attribute to increase 2 points:",
                                            "Select an attribute to increase 1 point:"};
    step_ = step;
    screen_ = Screen(ScreenMode::PromptList);
    screen_.SetupPromptList("Level Up", kPrompts[step], AvailableAttributeIncreases(p, charData));
    screen_.RemoveCommand(CommandId::Cancel);
}

bool LevelUpMenu::Open(const PlayerState& p, const CharacterData& charData) {
    if (AvailableAttributeIncreases(p, charData).empty()) return false;
    BuildStep(p, charData, 0);
    return true;
}

LevelUpAction LevelUpMenu::OnSelect(PlayerState& p, ShopState& shop, const CharacterData& charData) {
    // attribIncr[contextIndex] = the FIRST attribute-name slot whose text
    // equals the selected item (-1 if none, which can't happen: the list is
    // built from those same names).
    const std::string& picked = screen_.SelectedItemText();
    int slot = -1;
    for (size_t i = 0; i < charData.attributeNames.size(); i++) {
        if (charData.attributeNames[i] == picked) {
            slot = static_cast<int>(i);
            break;
        }
    }
    attribIncr_[static_cast<size_t>(step_)] = slot;

    if (step_ < 2) {
        BuildStep(p, charData, step_ + 1);
        return LevelUpAction::None;
    }

    p.attributes[static_cast<size_t>(attribIncr_[0])] = static_cast<int16_t>(p.attributes[static_cast<size_t>(attribIncr_[0])] + 3);
    p.attributes[static_cast<size_t>(attribIncr_[1])] = static_cast<int16_t>(p.attributes[static_cast<size_t>(attribIncr_[1])] + 2);
    p.attributes[static_cast<size_t>(attribIncr_[2])]++;
    PlayerCreation::RecalcMaxStats(p);
    ApplyLevelUp(p, shop);
    return LevelUpAction::ReturnToGame;
}

}  // namespace dawnstar
