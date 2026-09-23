#include "combat/spell_casting.h"

#include <algorithm>

#include "combat/combat_resolution.h"
#include "monster/monster_runtime.h"
#include "player/player_combat_stats.h"
#include "player/player_inventory.h"
#include "player/player_leveling.h"

namespace stormhold {

namespace {

// Player.nthKnownSpellId(n): the original's own body computes a local
// `spellId = i + 1` inside the loop but never returns it -- only `i`
// (0-based) is ever returned, confirmed by reading the whole method
// directly. A genuine dead local in the original, elided here rather than
// reproduced literally (an unused local would trip this port's own /W4
// warnings bar) -- same "write out the equivalent behavior, don't
// mechanically reproduce a provably dead expression" treatment
// GameCanvas.java's own tickPlayerAction header comment documents for its
// analogous dead `|| this.v` tautology term.
int NthKnownSpellId(const PlayerState& p, const SpellDatabase& spells, int n) {
    int seen = 0;
    for (int i = 0; i < spells.Count(); i++) {
        if ((p.knownSpellsMask & (1u << i)) != 0) {
            if (seen == n) return i;
            seen++;
        }
    }
    return -1;
}

}  // namespace

int SpellCasting::SpellSkillIndexFor(int spellId) {
    if (spellId <= 5) return 1;
    if (spellId <= 10) return 3;
    if (spellId <= 15) return 4;
    return spellId <= 20 ? 6 : 10;
}

void SpellCasting::CastOnSelf(PlayerState& p, int spellId, const SpellDatabase& spells, const CharacterData& charData,
                               const ItemDatabase& items, JavaRandom& rng) {
    int skillIdx = SpellSkillIndexFor(spellId);
    int skillVal = PlayerCombatStats::SkillValue(p, charData, skillIdx, true);
    int skillBns = PlayerCombatStats::SkillBonus(p, skillIdx);
    const Spell& spell = spells.ById(spellId);
    int8_t icon = spell.icon;
    int8_t durationMultiplier = spell.durationMultiplier;
    int8_t magickaCost = spell.magickaCost;
    int8_t power = spell.power;

    int diff = skillVal - icon;
    int defendChance = skillBns + diff * 5;
    int attackChance = durationMultiplier - diff * 5;
    defendChance = std::min(std::max(defendChance, 10), 95);
    attackChance = std::min(std::max(attackChance, 10), 95);
    // defendChance FIRST -- see this file's own header comment's NAMING
    // WARNING.
    int tier = PlayerCombatStats::RollOutcome(rng, defendChance, attackChance);

    int8_t mult = 1;
    if (tier == 0) {
        p.coreStats[4] = static_cast<int16_t>(p.coreStats[4] - 3 * magickaCost);
    } else if (tier == 1) {
        p.coreStats[4] = static_cast<int16_t>(p.coreStats[4] - 3 * magickaCost / 2);
    } else if (tier == 2) {
        p.coreStats[4] = static_cast<int16_t>(p.coreStats[4] - magickaCost);
    } else if (tier == 3) {
        p.coreStats[4] = static_cast<int16_t>(p.coreStats[4] - magickaCost);
        mult = 2;
    }

    p.coreStats[4] = std::max(p.coreStats[4], int16_t{0});
    if (tier >= 2) {
        PlayerLeveling::GainSkillExp(p, skillIdx, 1);
    }

    switch (spellId) {
        case 1:
        case 2:
        case 3:
        case 5:
            p.effectDurations[static_cast<size_t>(spellId - 1)] = static_cast<int8_t>(power * mult);
            break;  // falls into the empty 4/7-20/22/default block below in
                    // the original -- a provable no-op, written as `break`
                    // here (see CastOnMonster's own identical case-4
                    // fallthrough for the same treatment).
        case 4:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 22:
        default:
            break;
        case 6:
            // Grants item 109 (see combat/spell_casting.h's own class
            // comment for what auto-equipping it means) and auto-equips it;
            // only marks the buff active on BOTH succeeding.
            if (PlayerInventory::AddInventoryItemRaw(p, 109, 0, 0) &&
                PlayerInventory::EquipLastPickedUpItem(p, true, items)) {
                p.effectDurations[static_cast<size_t>(spellId - 1)] = static_cast<int8_t>(power * mult);
            }
            break;
        case 21: {
            int heal = 6 + PlayerCombatStats::SkillValue(p, charData, 10, false);
            p.coreStats[2] = static_cast<int16_t>(p.coreStats[2] + mult * heal);
            p.coreStats[2] = std::min(p.coreStats[2], p.coreStats[3]);
            break;
        }
        case 23:
            p.effectDurations[static_cast<size_t>(spellId - 1)] = -2;
            break;
        case 24:
            p.effectDurations[static_cast<size_t>(spellId - 1)] = -4;
            break;
        case 25:
            for (int i = 1; i <= mult; i++) {
                PlayerCombatStats::CureRandomAilment(p, rng);
            }
            break;
    }

    // Player.fatigueCostMultiplier(): 3 while ailment 1 is active, else 1
    // -- inlined directly rather than a named helper, matching
    // combat/combat_resolution.cpp's own PlayerAttack, which already
    // inlines this exact same one-line formula rather than adding a
    // PlayerCombatStats method for it.
    int fatigueCostMultiplier = (p.ailmentMask & 1) ? 3 : 1;
    p.coreStats[6] = static_cast<int16_t>(p.coreStats[6] - 5 * fatigueCostMultiplier);
    p.coreStats[6] = std::max(p.coreStats[6], int16_t{0});
    if (PlayerCombatStats::HasAilment(p, 6)) {
        int selfDmg = 2 * p.coreStats[3] / 100;
        if (selfDmg < 1) selfDmg = 1;
        p.coreStats[2] = static_cast<int16_t>(p.coreStats[2] - selfDmg);
    }
}

void SpellCasting::CastOnMonster(PlayerState& p, int spellId, MonsterState& target, const SpellDatabase& spells,
                                  const CharacterData& charData, const ItemDatabase& items,
                                  const MonsterDatabase& monsterDb, JavaRandom& rng, WorldRegistry& world) {
    int skillIdx = SpellSkillIndexFor(spellId);
    int skillVal = PlayerCombatStats::SkillValue(p, charData, skillIdx, true);
    int skillBns = PlayerCombatStats::SkillBonus(p, skillIdx);
    int targetOffense = MonsterRuntime::Stat(target, monsterDb, 10);
    int targetDefense = MonsterRuntime::Stat(target, monsterDb, 9);
    const Spell& spell = spells.ById(spellId);
    int8_t magickaCost = spell.magickaCost;
    // Unlike CastOnSelf, the original's own local `power = Spell.byId(
    // spellId).power` is read here but confirmed NEVER used anywhere in
    // castOnMonster's whole switch (every damage case derives its own
    // magnitude from skill values/targetOffense instead) -- a genuine dead
    // read in the original, elided here rather than reproduced literally
    // (an unused local would trip this port's own /W4 warnings bar), same
    // treatment as this file's own NthKnownSpellId helper.

    int diff = skillVal - targetOffense;
    int maxHp = MonsterRuntime::Stat(target, monsterDb, 2);
    diff = std::min(diff, maxHp);
    int attackChance = skillBns + diff * 5;
    int defendChance = targetDefense - diff * 5;
    attackChance = std::min(std::max(attackChance, 10), 95);
    defendChance = std::min(std::max(defendChance, 10), 95);
    int tier = PlayerCombatStats::RollOutcome(rng, attackChance, defendChance);

    int8_t mult = 1;
    if (tier == 0) {
        p.coreStats[4] = static_cast<int16_t>(p.coreStats[4] - 3 * magickaCost);
    } else if (tier == 1) {
        p.coreStats[4] = static_cast<int16_t>(p.coreStats[4] - 3 * magickaCost / 2);
    } else if (tier == 2) {
        p.coreStats[4] = static_cast<int16_t>(p.coreStats[4] - magickaCost);
    } else if (tier == 3) {
        p.coreStats[4] = static_cast<int16_t>(p.coreStats[4] - magickaCost);
        mult = 2;
    }

    p.coreStats[4] = std::max(p.coreStats[4], int16_t{0});
    if (tier >= 2) {
        PlayerLeveling::GainSkillExp(p, skillIdx, 1);
    }

    switch (spellId) {
        case 4:
            target.scratch[9] = -2;
            DungeonRuntime::StoreMonster(world, target);
            break;  // falls into the empty 5/6/default block below in the
                    // original -- a provable no-op (see this file's own
                    // header comment for why this is written as `break`).
        case 5:
        case 6:
        default:
            break;
        case 7: {
            int dmg7 = 10 + PlayerCombatStats::SkillValue(p, charData, 3, false);
            target.scratch[1] = static_cast<int8_t>(dmg7);
            break;
        }
        case 8: {
            int amt8 = PlayerCombatStats::SkillValue(p, charData, 3, false);
            int hp8 = 12 + 2 * amt8;
            MonsterRuntime::TakeDamage(target, hp8);
            p.coreStats[6] = static_cast<int16_t>(p.coreStats[6] + amt8);
            p.coreStats[6] = std::min(p.coreStats[6], p.coreStats[7]);
            p.coreStats[2] = static_cast<int16_t>(p.coreStats[2] + amt8);
            p.coreStats[2] = std::min(p.coreStats[2], p.coreStats[3]);
            p.coreStats[4] = static_cast<int16_t>(p.coreStats[4] + 12);
            p.coreStats[4] = std::min(p.coreStats[4], p.coreStats[5]);
            break;
        }
        case 9:
            if (MonsterRuntime::IsUndead(target)) {
                int base9 = 60 * mult;
                int dmg9 = base9 - MonsterRuntime::Stat(target, monsterDb, 8);
                dmg9 = std::max(dmg9, 4);
                int scaled9 = dmg9 * static_cast<int>(MonsterRuntime::Stat(target, monsterDb, 14)) / 100;
                MonsterRuntime::TakeDamage(target, scaled9);
            }
            break;
        case 10:
            // NOT followed by target.store() in the original -- see this
            // file's own header comment.
            p.effectDurations[static_cast<size_t>(spellId - 1)] = -2;
            target.scratch[8] = static_cast<int8_t>(2 * mult);
            break;
        case 11: {
            int base11 = 25 + PlayerCombatStats::SkillValue(p, charData, 4, false);
            int scaled11a = base11 * mult;
            int dmg11 = scaled11a - MonsterRuntime::Stat(target, monsterDb, 8);
            dmg11 = std::max(dmg11, 4);
            int scaled11 = dmg11 * static_cast<int>(MonsterRuntime::Stat(target, monsterDb, 14)) / 100;
            MonsterRuntime::TakeDamage(target, scaled11);
            DungeonRuntime::StoreMonster(world, target);
            break;
        }
        case 12: {
            p.effectDurations[static_cast<size_t>(spellId - 1)] = -2;
            int v12 = mult * (10 + PlayerCombatStats::SkillValue(p, charData, 4, false));
            v12 = std::min(v12, 255);
            target.scratch[4] = static_cast<int8_t>(v12);
            DungeonRuntime::StoreMonster(world, target);
            break;
        }
        case 13: {
            p.effectDurations[static_cast<size_t>(spellId - 1)] = -2;
            int v13 = mult * (10 + PlayerCombatStats::SkillValue(p, charData, 4, false));
            v13 = std::min(v13, 255);
            target.scratch[5] = static_cast<int8_t>(v13);
            DungeonRuntime::StoreMonster(world, target);
            break;
        }
        case 14:
            // "Cast a spell that's just a normal weapon attack" -- see this
            // file's own header comment.
            p.effectDurations[static_cast<size_t>(spellId - 1)] = -1;
            CombatResolution::PlayerAttack(p, target, charData, items, monsterDb, rng, world);
            p.effectDurations[static_cast<size_t>(spellId - 1)] = 0;
            break;
        case 15:
            p.effectDurations[static_cast<size_t>(spellId - 1)] = -2;
            target.scratch[2] = 1;
            DungeonRuntime::StoreMonster(world, target);
            break;
        case 16: {
            // NOT followed by target.store() in the original -- see this
            // file's own header comment.
            int room16 = 10 - targetOffense;
            if (room16 > 0) {
                room16 = mult * room16;
                p.effectDurations[static_cast<size_t>(spellId - 1)] = static_cast<int8_t>(room16);
                target.scratch[6] = 1;
            }
            break;
        }
        case 17:
            p.effectDurations[static_cast<size_t>(spellId - 1)] = -2;
            p.spellArmorBonus = static_cast<int16_t>(10 + PlayerCombatStats::SkillValue(p, charData, 6, false));
            break;
        case 18:
            // NOT followed by target.store() in the original -- see this
            // file's own header comment.
            p.effectDurations[static_cast<size_t>(spellId - 1)] = -2;
            target.scratch[0] = static_cast<int8_t>(3 * mult);
            break;
        case 19: {
            // NOT followed by target.store() in the original -- see this
            // file's own header comment.
            p.effectDurations[static_cast<size_t>(spellId - 1)] = -2;
            int chance19 = mult * (60 - 5 * targetOffense);
            chance19 = std::min(std::max(chance19, 0), 100);
            target.scratch[3] = static_cast<int8_t>(chance19);
            break;
        }
        case 20: {
            int base20 = 80 - 5 * targetOffense;
            int scaled20a = base20 * mult;
            int dmg20 = scaled20a - MonsterRuntime::Stat(target, monsterDb, 8);
            dmg20 = std::max(dmg20, 4);
            int scaled20 = dmg20 * static_cast<int>(MonsterRuntime::Stat(target, monsterDb, 14)) / 100;
            MonsterRuntime::TakeDamage(target, scaled20);
            break;
        }
    }

    int fatigueCostMultiplier = (p.ailmentMask & 1) ? 3 : 1;
    p.coreStats[6] = static_cast<int16_t>(p.coreStats[6] - 5 * fatigueCostMultiplier);
    p.coreStats[6] = std::max(p.coreStats[6], int16_t{0});
    if (PlayerCombatStats::HasAilment(p, 6)) {
        int selfDmg = 2 * p.coreStats[3] / 100;
        if (selfDmg < 1) selfDmg = 1;
        p.coreStats[2] = static_cast<int16_t>(p.coreStats[2] - selfDmg);
    }
}

int SpellCasting::CycleSelectedSpell(const PlayerState& p, const SpellDatabase& spells) {
    if (!spells.IsValidId(p.selectedSpellId)) {
        int first = NthKnownSpellId(p, spells, 0);
        return first < 0 ? 0 : first + 1;
    }

    int cur = p.selectedSpellId - 1;
    int next = cur + 1;
    if (next == spells.Count()) next = 0;

    while (next != cur) {
        if ((p.knownSpellsMask & (1u << next)) != 0) return next + 1;
        if (++next == spells.Count()) next = 0;
    }

    return p.selectedSpellId;
}

SpellCasting::Result SpellCasting::ResolveSpellCastInput(PlayerState& player, std::optional<MonsterState>& target,
                                                           bool& castRequested, int64_t now,
                                                           int64_t& lastSpellCastTimeMs, const SpellDatabase& spells,
                                                           const CharacterData& charData, const ItemDatabase& items,
                                                           const MonsterDatabase& monsterDb, JavaRandom& rng,
                                                           WorldRegistry& world) {
    int spellId = player.selectedSpellId;
    if (!spells.IsValidId(spellId)) {
        castRequested = false;
        return Result::InvalidSpell;
    }

    Result result;
    if (spells.ById(spellId).magickaCost > PlayerCombatStats::EffectiveStat(player, charData, 4)) {
        result = Result::NotEnoughMagicka;
    } else if (now - lastSpellCastTimeMs >= 500) {
        // The original re-checks Spell.isValidId(spellId) here too -- a
        // provably dead re-check (spellId/its validity are unchanged since
        // the guard above), not reproduced, same "don't mechanically repeat
        // a provably redundant check" treatment as elsewhere in this port.
        if (spells.IsOffensive(spellId)) {
            if (!target.has_value()) {
                result = Result::NoMonster;
            } else {
                CastOnMonster(player, spellId, *target, spells, charData, items, monsterDb, rng, world);
                result = Result::CastOnMonster;
            }
        } else {
            CastOnSelf(player, spellId, spells, charData, items, rng);
            result = Result::CastOnSelf;
        }
        lastSpellCastTimeMs = now;
    } else {
        result = Result::OnCooldown;
    }

    castRequested = false;
    return result;
}

int SpellCasting::ResolveSpellCycleInput(PlayerState& player, bool& cycleRequested, const SpellDatabase& spells) {
    int spellId = CycleSelectedSpell(player, spells);
    if (spellId != 0) {
        player.selectedSpellId = static_cast<int8_t>(spellId);
    }

    cycleRequested = false;
    return spellId;
}

}  // namespace stormhold
