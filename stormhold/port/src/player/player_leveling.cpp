#include "player/player_leveling.h"

#include "player/player_creation.h"

namespace stormhold {

bool PlayerLeveling::TryRankUpSkills(PlayerState& p, const CharacterData& charData) {
    for (int i = 0; i < 14; i++) {
        if (p.skills[static_cast<size_t>(i)][2] >= 10) {
            p.skills[static_cast<size_t>(i)][2] = static_cast<int16_t>(p.skills[static_cast<size_t>(i)][2] - 10);
            p.skills[static_cast<size_t>(i)][0]++;

            int attr = charData.skillAttributeIndex[static_cast<size_t>(i)];
            int bit = attr / 2;
            p.levelUpAttributeFlags = static_cast<int8_t>(p.levelUpAttributeFlags | (1 << bit));
            p.coreStats[1]++;
        }
    }

    if (p.coreStats[1] >= 10) {
        p.coreStats[0]++;
        return true;
    }
    return false;
}

std::vector<std::string> PlayerLeveling::PendingLevelUpAttributeNames(const PlayerState& p,
                                                                        const CharacterData& charData) {
    std::vector<std::string> out;
    for (int i = 0; i < 8; i++) {
        if (p.levelUpAttributeFlags & (1 << i)) {
            out.push_back(charData.attributeNames[static_cast<size_t>(i * 2)]);
        }
    }
    return out;
}

void PlayerLeveling::ApplyLevelUpAttributeChoices(PlayerState& p, int firstChoice, int secondChoice, int thirdChoice,
                                                   ShopState& shop) {
    p.attributes[static_cast<size_t>(firstChoice)] =
        static_cast<int16_t>(p.attributes[static_cast<size_t>(firstChoice)] + 3);
    p.attributes[static_cast<size_t>(secondChoice)] =
        static_cast<int16_t>(p.attributes[static_cast<size_t>(secondChoice)] + 2);
    p.attributes[static_cast<size_t>(thirdChoice)]++;

    PlayerCreation::ComputeDerivedStats(p);
    ConsumeLevelExp(p, shop);
}

std::vector<std::string> PlayerLeveling::SkillSummaryList(const PlayerState& p, const CharacterData& charData) {
    std::vector<std::string> out;
    for (int i = 0; i < 14; i++) {
        if (p.skills[static_cast<size_t>(i)][0] <= 0) continue;
        out.push_back(charData.skillNames[static_cast<size_t>(i)] + ": " +
                      std::to_string(p.skills[static_cast<size_t>(i)][0]));
    }
    return out;
}

int PlayerLeveling::NthLearnedSkillIndex(const PlayerState& p, int n) {
    int seen = 0;
    for (int i = 0; i < 14; i++) {
        if (p.skills[static_cast<size_t>(i)][0] > 0) {
            if (seen == n) return i;
            seen++;
        }
    }
    return -1;
}

std::string PlayerLeveling::SkillTooltip(const PlayerState& p, const CharacterData& charData, int skillIndex) {
    return charData.skillNames[static_cast<size_t>(skillIndex)] + '\n' +
           "Rank: " + std::to_string(p.skills[static_cast<size_t>(skillIndex)][0]) + '\n' +
           "Exp: " + std::to_string(p.skills[static_cast<size_t>(skillIndex)][2]) + "/10";
}

}  // namespace stormhold
