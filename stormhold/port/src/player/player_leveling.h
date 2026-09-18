#pragma once
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "player/player_state.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/Player.java's skill-exp/
// level-up pipeline: gainSkillExp()/tryRankUpSkills()/consumeLevelExp()/
// pendingLevelUpAttributeNames(), plus the attribute-point-allocation
// step ESGame.java's own level-up-confirm UI handler performs inline
// (screenGroup 39's 3-step "choose an attribute" flow, lines ~1046-1069)
// rather than a named Player method -- bundled here as
// ApplyLevelUpAttributeChoices since this port has no UI screens at all
// (see ../../docs/PORT_ROADMAP.md) and the 3 chosen attribute indices are
// a real player DECISION, not derivable data -- same "caller supplies
// the missing decision" pattern as M9's traitor-index roll/M13's
// interactionCount.
//
// NOT ported here: `Shop.clearQuestTurnInState()`, a confirmed real
// cross-system coupling `consumeLevelExp()` triggers on every rank-up
// (see ../../../src/Player.java's own header comment, and M13/M14's
// "what's next" notes flagging this exact gap) -- no live `Shop` state
// exists yet (M11 only ported its dialogue TEXT), so `ConsumeLevelExp`
// below skips it, flagged at the skip site rather than silently dropped.
class PlayerLeveling {
public:
    // Player.gainSkillExp(skillIndex, amount): adds to skills[i][2] (exp
    // toward the next rank) -- only if skillIndex >= 0 (callers like
    // combat/combat_resolution.h's PlayerAttack pass
    // ActiveWeaponSkillIndex()'s -1-if-unarmed result straight through
    // without checking it themselves, so this guard is load-bearing, not
    // defensive dead code).
    static void GainSkillExp(PlayerState& p, int skillIndex, int amount) {
        if (skillIndex >= 0) {
            p.skills[static_cast<size_t>(skillIndex)][2] =
                static_cast<int16_t>(p.skills[static_cast<size_t>(skillIndex)][2] + amount);
        }
    }

    // Player.tryRankUpSkills(): scans all 14 skills for exp >= 10,
    // ranking each one up (exp -= 10, rank++), marking
    // levelUpAttributeFlags's governing-attribute bit
    // (CharacterData::skillAttributeIndex[i]/2), and granting 1 level-exp
    // (coreStats[1]) per rank-up -- MULTIPLE skills can rank up in the
    // same call, each contributing its own level-exp point. Returns true
    // (and increments coreStats[0], the character level) once
    // coreStats[1] reaches 10; the caller is then expected to eventually
    // call ApplyLevelUpAttributeChoices once it has 3 attribute choices
    // to spend that level-exp on.
    static bool TryRankUpSkills(PlayerState& p, const CharacterData& charData);

    // Player.consumeLevelExp(): spends 10 level-exp (coreStats[1] -= 10).
    // SIMPLIFIED: skips Shop.clearQuestTurnInState() -- see class comment.
    static void ConsumeLevelExp(PlayerState& p) { p.coreStats[1] = static_cast<int16_t>(p.coreStats[1] - 10); }

    // Player.pendingLevelUpAttributeNames(): the names of every attribute
    // pair (attributeNames[2*i]) whose bit is set in levelUpAttributeFlags
    // -- returns an EMPTY vector where the original returns null (no
    // rank-up pending), rather than reproducing the null/array
    // distinction as a separate flag.
    static std::vector<std::string> PendingLevelUpAttributeNames(const PlayerState& p, const CharacterData& charData);

    // ESGame's level-up-confirm handler (see class comment): spends the
    // pending level-exp by boosting 3 caller-chosen attribute indices
    // (+3/+2/+1, weighted by pick order, matching the real UI's 3-step
    // flow exactly), recomputes derived stats
    // (PlayerCreation::ComputeDerivedStats), and calls ConsumeLevelExp.
    // Does NOT clear/validate levelUpAttributeFlags itself -- the real
    // game doesn't either; flags accumulate across rank-ups and are only
    // ever read by PendingLevelUpAttributeNames, never reset by this
    // method or any other confirmed call site in Player.java.
    static void ApplyLevelUpAttributeChoices(PlayerState& p, int firstChoice, int secondChoice, int thirdChoice);
};

}  // namespace stormhold
