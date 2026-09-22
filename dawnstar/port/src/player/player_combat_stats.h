#pragma once
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "player/player_state.h"
#include "util/java_random.h"

namespace dawnstar {

// Renamed-source counterpart of ../../../src/Player.java's combat/skill
// stat math: skillValue()/skillBonus()/weaponSkillValue()/baseEvasion()/
// bestArmorSkillIndex()/activeWeaponSkillIndex()/offhandSkillIndex()/
// attackPower()/attackAccuracy()/weaponDamage()/armorValue()/
// isEffectActive()/clearEffect()/hasAilment()/gainSkillExp(), plus the
// static rollOutcome() hit-tier roll. Deliberately NOT attack() itself
// (see docs/PORT_ROADMAP.md's M14 entry) -- attack() needs a live
// Monster target (stat(), takeDamage(), store()), and there is no
// Monster runtime port yet, only M3/M6's static MonsterDatabase/
// generation-time spawn list. Everything here is entirely self-contained
// in Player + ItemDatabase, so it's ported first, on its own.
class PlayerCombatStats {
public:
    // Skill rank, optionally plus 1/3 of the governing attribute's bonus
    // component (CharacterData::skillAttributeIndex says which
    // attribute), plus a 2-handed/shield synergy bonus (skillIndex==11
    // while effect 3 is active), minus 1 if Fatigue is critically low
    // (<7), plus 4 if the StarFrost bonus is active.
    static int SkillValue(const PlayerState& p, const CharacterData& charData, int skillIndex,
                          bool withAttributeBonus);
    static int SkillBonus(const PlayerState& p, int skillIndex) { return p.skills[static_cast<size_t>(skillIndex)][1]; }

    // Dispatches to SkillValue(5,...)/SkillValue(7,...) by the equipped
    // offhand/shield item's category; 0 with nothing equipped.
    static int WeaponSkillValue(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items,
                                bool withAttributeBonus);
    // Base evasion-ish stat opposite a monster's detection roll; 20 with
    // nothing equipped in the offhand slot.
    static int BaseEvasion(const PlayerState& p, const ItemDatabase& items);
    // Picks whichever of the 4 armor-category skills (0/2/8/12) is
    // currently highest-ranked.
    static int BestArmorSkillIndex(const PlayerState& p, const CharacterData& charData);
    // Which skill index currently governs offense/defense: a shield buff
    // (effect 6) picks BestArmorSkillIndex; otherwise the equipped
    // weapon's category maps to skill 0/2/8/12; -1 with nothing equipped.
    static int ActiveWeaponSkillIndex(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items);
    // Which skill index governs the offhand/shield item (5 or 7); -1
    // with nothing equipped.
    static int OffhandSkillIndex(const PlayerState& p, const ItemDatabase& items);

    // Attack-power/accuracy components for rollOutcome, and weapon
    // damage -- all from a buff (effect 14 or the shield buff, effect 6)
    // or the equipped weapon, plus assorted effect bonuses.
    static int AttackPower(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items,
                           bool withAttributeBonus);
    static int AttackAccuracy(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items);
    static int WeaponDamage(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items);
    // Total armor value: weighted sum of equip slots 1-5's raw magnitude
    // column (slot 6 is NOT included -- ported exactly as the original,
    // not "fixed"), divided by 10, plus assorted effect/buff bonuses.
    static int ArmorValue(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items);

    // effectDurations[id-1]==-1: active until cured; ==-2: active while
    // combatTargetSpawnId != 0 (i.e. while in combat); >0: still
    // counting down.
    static bool IsEffectActive(const PlayerState& p, int effectId);
    // Player.java's effectiveStat(index): the HP/Magicka/Fatigue values
    // GameCanvas's status-bar HUD (render/hud_renderer.h, M21) actually
    // displays -- coreStats[index] as-is, unless the "Regeneration"-style
    // buff (effect 23) is active AND index is 2 (HP)/4 (Magicka)/6
    // (Fatigue), in which case skillValue(10, false) is added back in,
    // capped at the matching max stat (coreStats[index+1]). Every other
    // index (and every other caller reading coreStats directly instead
    // of through this) intentionally sees the buff-less raw value --
    // ported exactly, not generalized.
    static int EffectiveStat(const PlayerState& p, const CharacterData& charData, int index);
    static void ClearEffect(PlayerState& p, int effectId) { p.effectDurations[static_cast<size_t>(effectId - 1)] = 0; }
    static bool HasAilment(const PlayerState& p, int ailmentNumber) {
        return (p.ailmentMask & (1 << (ailmentNumber - 1))) != 0;
    }
    // Player.java's fatigueCostMultiplier(): 3x while "Frost Limbs"
    // (ailment bit 0) is active, else 1x. Public (unlike the other
    // methods here that are internal helpers) since combat/
    // combat_resolution.h's PlayerAttack needs it too --
    // player/player_movement.cpp keeps its own small pre-existing copy
    // (predates this method's move to a public API) rather than being
    // churned to call this one, the same kind of harmless single-
    // formula duplication as world/dungeon_generator.h's
    // MonsterTypeForTierBucket doc comment discusses.
    static int FatigueCostMultiplier(const PlayerState& p) { return (p.ailmentMask & 1) == 1 ? 3 : 1; }

    // Player.java's tickFatigueRegen(elapsedMs): passive Fatigue regen
    // over the elapsed time, scaled by the average of attributes[10]/[11]
    // (Endurance-ish), capped at max. GameCanvas.processIdleTick() calls
    // this only `if (!actionTakenThisTick)` -- main.cpp's own call site
    // carries that gate (see docs/PORT_ROADMAP.md's M51 entry for why
    // this was left for a follow-up milestone).
    static void TickFatigueRegen(PlayerState& p, int64_t elapsedMs) {
        int gain = static_cast<int>(elapsedMs * (p.attributes[10] + p.attributes[11]) / 2000);
        p.coreStats[6] = static_cast<int16_t>(p.coreStats[6] + gain);
        if (p.coreStats[6] > p.coreStats[7]) p.coreStats[6] = p.coreStats[7];
    }

    // Adds `amount` skill exp, rolling every full 10 points into +1 skill
    // rank (each rank-up also flags the governing attribute for its next
    // level-up increase, via attributeIncreaseFlags) and +1 "level exp"
    // point (coreStats[1]); reaching 10 level-exp points levels the
    // character up (coreStats[0]++) and sets p.levelUpPending.
    static void GainSkillExp(PlayerState& p, const CharacterData& charData, int skillIndex, int amount);

    // "Skill: rank" strings for every skill with rank > 0 -- Player.java's
    // knownSkillsSummary(), the real "Skills" Options-menu list (M41).
    static std::vector<std::string> KnownSkillsSummary(const PlayerState& p, const CharacterData& charData);
    // The `index`-th skill with rank > 0 (matching KnownSkillsSummary's
    // ordering), or -1 -- Player.java's nthKnownSkillIndex().
    static int NthKnownSkillIndex(const PlayerState& p, int index);
    // "SkillName\nRank: n\nExp: n/10" -- Player.java's skillTooltip().
    static std::string SkillTooltip(const PlayerState& p, const CharacterData& charData, int skillIndex);

    struct RollResult {
        int outcome = 0;  // 0=miss/parry, 1=hit, 2=hit(favored), 3=critical
        bool crit = false;
    };
    // Player.java's static rollOutcome()/Util.randomInt(100) (=
    // ESGame.lingoRandomInt, a 1-100 inclusive roll -- see
    // util/java_random.h's LingoRandomInt). `globalRng` models ESGame.r,
    // same convention as player_creation.h's traitor-index roll.
    static RollResult RollOutcome(int atkChance, int defChance, JavaRandom& globalRng);
};

}  // namespace dawnstar
