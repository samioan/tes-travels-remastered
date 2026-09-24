#pragma once
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "player/player_state.h"
#include "world/shop_state.h"

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
// **M54: `Shop.clearQuestTurnInState()`** -- a confirmed real cross-system
// coupling `consumeLevelExp()` triggers on every rank-up (see
// ../../../src/Player.java's own header comment, and M13/M14's "what's
// next" notes that first flagged this exact gap) -- is now wired for
// real via `world/shop_state.h`'s `Shop::ClearQuestTurnInState` (M53's
// own `ShopState`, with its corrected, faithful `Shop.reset()` defaults).
// `ConsumeLevelExp`/`ApplyLevelUpAttributeChoices` below both take a new
// `ShopState&` to do it. M77: live -- main.cpp calls TryRankUpSkills
// every tick (GameCanvas.run()'s own position, right after
// tickDeathAndRegen) and ui/level_up_menu.h's three-pick screen calls
// ApplyLevelUpAttributeChoices.
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

    // Player.consumeLevelExp(): calls Shop.clearQuestTurnInState() (M54,
    // see class comment) then spends 10 level-exp (coreStats[1] -= 10),
    // matching the original's own statement order exactly.
    static void ConsumeLevelExp(PlayerState& p, ShopState& shop) {
        Shop::ClearQuestTurnInState(shop);
        p.coreStats[1] = static_cast<int16_t>(p.coreStats[1] - 10);
    }

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
    static void ApplyLevelUpAttributeChoices(PlayerState& p, int firstChoice, int secondChoice, int thirdChoice,
                                              ShopState& shop);

    // M63: Player.skillSummaryList() -- "<name>: <rank>" for every one of
    // the 14 skills whose rank (skills[i][0]) is > 0 (i.e. actually
    // trained at least once), in skill-index order.
    static std::vector<std::string> SkillSummaryList(const PlayerState& p, const CharacterData& charData);

    // Player.nthLearnedSkillIndex(n): the n-th trained skill (rank > 0),
    // as a 0-based skill index -- same "row in the summary list ->
    // underlying index" role SpellCasting::NthKnownSpellId plays for
    // spells. -1 if out of range.
    static int NthLearnedSkillIndex(const PlayerState& p, int n);

    // Player.skillTooltip(skillIndex): name/rank/exp-toward-next-rank,
    // "<n>/10" (exp toward rank-up, matching TryRankUpSkills's own
    // "exp >= 10" threshold).
    static std::string SkillTooltip(const PlayerState& p, const CharacterData& charData, int skillIndex);
};

}  // namespace stormhold
