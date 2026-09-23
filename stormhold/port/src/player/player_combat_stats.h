#pragma once
#include <cstdint>

#include "assets/character_data.h"
#include "assets/item_database.h"
#include "player/player_state.h"
#include "util/java_random.h"

namespace stormhold {

// Renamed-source counterpart of ../../../src/Player.java's combat/skill
// stat math: skillValue()/skillBonus()/defenseSkillValue()/baseEvasion()/
// bestArmorSkillIndex()/activeWeaponSkillIndex()/defenseSkillIndex()/
// attackPower()/attackAccuracy()/weaponDamage()/armorValue()/
// isEffectActive()/clearEffect()/hasAilment()/effectiveStat(), plus the
// static rollOutcome() hit-tier roll and rollShopOutcome(). Deliberately
// NOT `attack(Monster)` itself, and not `gainSkillExp`/the rank-up/
// level-up cascade `consumeLevelExp()` triggers (a real, separate cross-
// system coupling with `Shop.clearQuestTurnInState()`, per this file's
// own class header comment) -- `attack()` needs a live Monster target
// (`stat()`/`takeDamage()`/`store()`), and there is no Monster runtime
// port yet, only M3/M6's static `MonsterDatabase`/generation-time spawn
// list; leveling is its own separate concern nothing here actually
// depends on. Everything below is entirely self-contained in
// `PlayerState` + `CharacterData` + `ItemDatabase`, so it's ported first,
// on its own -- same scoping dawnstar's own equivalent milestone used.
class PlayerCombatStats {
public:
    // Skill rank (skills[i][0]), plus 1/3 of the governing attribute's
    // bonus component when `includeBonus` (CharacterData::
    // skillAttributeIndex says which attribute), plus skill 1's rank
    // when skillIndex==11 and effect 3 is active, minus 1 when Fatigue
    // is critically low (< 7).
    static int SkillValue(const PlayerState& p, const CharacterData& charData, int skillIndex, bool includeBonus);
    static int SkillBonus(const PlayerState& p, int skillIndex) {
        return p.skills[static_cast<size_t>(skillIndex)][1];
    }

    // Defense-context skill value keyed off the offhand/shield equip slot
    // (equippedItems[1]): category 5 -> skill 5, else skill 7; 0 with
    // nothing equipped.
    static int DefenseSkillValue(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items,
                                  bool includeBonus);
    // Same category dispatch as DefenseSkillValue, but SkillBonus instead
    // of SkillValue; 20 with nothing equipped (a flat baseline, not 0).
    static int BaseEvasion(const PlayerState& p, const ItemDatabase& items);
    // Picks whichever of the 4 armor-category skills (0/2/8/12) is
    // currently highest-ranked (unbonused); returns the WINNING skill's
    // index, not its value.
    static int BestArmorSkillIndex(const PlayerState& p, const CharacterData& charData);
    // Effect 6 ("shield buff") overrides to BestArmorSkillIndex();
    // otherwise the equipped weapon's category maps to skill 0/2/8/12;
    // -1 if unarmed.
    static int ActiveWeaponSkillIndex(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items);
    // Offhand/shield-slot skill INDEX (5 or 7), -1 if unequipped -- the
    // index-returning counterpart of DefenseSkillValue's value.
    static int DefenseSkillIndex(const PlayerState& p, const ItemDatabase& items);

    // attackPower(includeBonus): effect 14 -> spell-power path (5 +
    // SkillValue(4, false)); effect 6 -> BestArmorSkillIndex()'s
    // SkillValue; else the equipped weapon's category-mapped SkillValue;
    // +SkillValue(1, false) when effect 5 is active.
    static int AttackPower(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items,
                            bool includeBonus);
    // The bonus-only counterpart of AttackPower, used as the base
    // offensive chance in attack() -- always SkillBonus, never SkillValue
    // (though it still needs CharacterData transitively, via
    // BestArmorSkillIndex's own skill-rank comparison).
    static int AttackAccuracy(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items);
    // effect 14 -> spell-power-shaped path (5 + SkillValue(4)); effect 6
    // -> 20 + SkillValue(3); else the equipped weapon's raw magnitude
    // column; +10 + SkillValue(1) when effect 1 is active; +25 flat when
    // increaseHarmBuff is set.
    static int WeaponDamage(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items);
    // Weighted sum of equip slots 1-5's magnitude column (4/2/2/1/1,
    // /10) -- slot 6 (index 6, equippedItems[6]) is NOT included, ported
    // exactly as the original, not "fixed". +10 + SkillValue(1) when
    // effect 2 is active; +spellArmorBonus when effect 17 is active; +15
    // flat when increaseArmorBuff is set.
    static int ArmorValue(const PlayerState& p, const CharacterData& charData, const ItemDatabase& items);

    // effectDurations[id-1] == -1: always active. == -2: **NOT**
    // "conditional on giftPointsFound" despite that reading naturally --
    // conditional on `lastCombatTargetId != 0` (i.e. "has attacked
    // something at least once"). Confirmed directly from
    // ../../../src/Player.java's own header comment on this exact
    // method: a genuinely ambiguous decompiled field reference (`t`
    // could plausibly have meant either field) that phase 1 resolved by
    // reading the bytecode, not guessed -- preserved exactly, flagged
    // again here since it's easy to misremember which field it really
    // is. > 0: still counting down.
    static bool IsEffectActive(const PlayerState& p, int effectId);
    static void ClearEffect(PlayerState& p, int effectId) { p.effectDurations[static_cast<size_t>(effectId - 1)] = 0; }
    static bool HasAilment(const PlayerState& p, int ailmentId) {
        return (p.ailmentMask & (1 << (ailmentId - 1))) != 0;
    }
    // Player.activeAilmentCount(): how many of the 8 ailmentMask bits are
    // currently set -- CureRandomAilment's own helper, ported separately
    // since the original itself is a separate named method.
    static int ActiveAilmentCount(const PlayerState& p);
    // Player.cureRandomAilment() (combat/spell_casting.h's own CastOnSelf,
    // spell 25's repeated call): picks ONE currently-active ailment
    // uniformly at random and clears it -- a genuinely DIFFERENT mechanic
    // from ApplyRestRecovery's own per-bit 25%-independent-chance loop
    // above, not a duplicate of it. The original draws its pick from
    // `Util.randomInt(count)`, the SHARED-static-RNG one-arg overload
    // (`ESGame.nextInt`) -- same "caller supplies/owns the RNG stream"
    // divergence this port's RollOutcome/RollShopOutcome/ApplyRestRecovery
    // all already establish, not reproduced bit-exact against a single
    // shared generator.
    static void CureRandomAilment(PlayerState& p, JavaRandom& rng);

    // coreStats[statIndex], plus a SkillValue(10, false)-scaled bonus
    // (clamped to the matching max stat) when effect 23
    // ("Regeneration"-shaped) is active AND statIndex is 2 (HP), 4
    // (Magicka), or 6 (Fatigue) -- every other statIndex, and every
    // other caller reading coreStats directly instead of through this,
    // intentionally sees the buff-less raw value.
    static int EffectiveStat(const PlayerState& p, const CharacterData& charData, int statIndex);

    // Player.rollOutcome(atkChance, defChance): two independent
    // percentile rolls (ESGame.nextInt(100), the confirmed 1-BASED
    // (1..100) formula per M5's own naming-swap finding -- NOT the
    // 0-based lingoRandomInt), returns 0=miss, 1/2=graze (loser/winner by
    // margin), 3=clean hit. `rng` models the shared game-wide RNG this
    // draws from (ESGame's own, distinct from DungeonGenerator's
    // per-level seeded one) -- same "caller supplies/seeds it" pattern
    // M9's PlayerCreation documented for its own now-moot traitor-index
    // roll parameter. The original's `lastDefenseRollHit` static field is
    // set then read within this SAME call and never observed elsewhere
    // (Player.java's own header comment already flags this) -- kept as a
    // plain local here rather than reproduced as a static, matching this
    // port's established "don't reproduce dead staleness" precedent
    // (M6/M9).
    static int RollOutcome(JavaRandom& rng, int atkChance, int defChance);

    // Player.rollShopOutcome(shopId, action): SkillValue(13, true) (+3 if
    // action==3) vs. a quest shop's own interactionCount as a threshold,
    // feeding RollOutcome(). `interactionCount` is `Shop.
    // interactionCount[shopId]`'s current value, passed in explicitly
    // rather than read from a ported `Shop` static-state object -- no
    // live `Shop` state exists in this port yet (M11 only ported its
    // dialogue TEXT), same "caller supplies/owns world state" pattern
    // M8's WardenState and M10/M12's world-registry gaps already
    // established.
    static int RollShopOutcome(const PlayerState& p, const CharacterData& charData, int action, int interactionCount,
                                JavaRandom& rng);

    // GameCanvas.tickStatusCountdowns(deltaMs) (M35, phase-3 port):
    // per-tick ailment-timer countdowns. While each of 3 specific
    // ailments (`HasAilment(id)`, NOT `IsEffectActive(id)` -- a
    // similarly-shaped but genuinely different check, easy to confuse;
    // confirmed directly from decompiled/e.java, which reads
    // `Player.hasAilment` here, not `isEffectActive`) is currently
    // active, its own dedicated millisecond timer (`vampirismTimer`/
    // `manaBurnTimer`/`terrifiedTimer` -- Monster.tick()'s own confirmed
    // ailment-4/5 appliers, ../../../src/Monster.java) counts down by
    // `deltaMs`; once it drops below 0, it's clamped to 0 and the
    // ailment bit clears (bit index = ailmentId - 1, same convention
    // `HasAilment` itself already uses).
    //
    // **A real, surprising coupling, preserved rather than smoothed
    // over:** ailment 7's own timer additionally requires
    // `monsterRenderedThisFrame` -- GameCanvas's own `unconfirmed_A`,
    // the SAME flag `render/visible_object_renderer.h`'s
    // `VisibleObjectRenderer::RenderMonsters` (M28) already returns,
    // set true only when it actually draws a real monster sprite that
    // same frame (never for the Warden). So ailment 7's countdown only
    // progresses on a tick where a monster was ALSO just rendered -- a
    // real dependency between this port's paint and tick passes, not
    // independently confirmed anywhere else. The caller (`main.cpp`)
    // supplies whatever `RenderMonsters` most recently returned, same
    // "caller supplies/owns state" pattern this port uses throughout.
    static void TickStatusCountdowns(PlayerState& p, int64_t deltaMs, bool monsterRenderedThisFrame);

    // GameCanvas.tickPerSecond() (M36, phase-3 port): 3 independent
    // per-real-second mechanics, transcribed byte-for-byte --
    //  1. EVERY currently counting-down `effectDurations[]` slot (all
    //     25, not just the 3 specific ailments `TickStatusCountdowns`'s
    //     own millisecond timers separately track) decrements by 1.
    //     When slot 5 (effect id 6) reaches exactly zero THIS call,
    //     item 109 ("daedric weapon", per the original's own debug
    //     println) is located by `PlayerInventory::
    //     FindEquippedSlotForItem` and removed outright via
    //     `PlayerInventory::RemoveInventorySlot` -- NOT the drop-into-
    //     the-world path `DropInventoryItem` would use; the weapon
    //     simply vanishes when its own temporary effect wears off.
    //  2. `HasAilment(4)` ("vampirism") drains 2% of maxHP from HP.
    //  3. `HasAilment(5)` ("mana burn") regenerates 10% of maxMagicka
    //     into Magicka; the moment Magicka reaches or exceeds its own
    //     max, it resets to EXACTLY ZERO (not clamped to max) and HP
    //     takes a 10%-of-maxMagicka hit instead -- a real, punishing
    //     overflow-and-burn mechanic, not a clamp bug.
    //
    // **NOT reproduced here, a confirmed but provably inert original
    // dead write (see ../../../src/GameCanvas.java's own header
    // comment on `tickPerSecond()` for the full writeup):** a 4th
    // piece in the original iterates every monster registered on the
    // player's current level, mutating a byte pair on a throwaway
    // DECODED COPY of each record (`Monster.fromBytesShared()` copies
    // bytes rather than aliasing the registry's own stored array) and
    // never stores the result back -- confirmed to have zero observable
    // effect anywhere, so mechanically reproducing it here would just
    // be dead code, same "document, don't mechanically port, a
    // provably dead branch" discipline this port already used for
    // world/dungeon_generator.h's own dead chest-record byte.
    static void TickPerSecond(PlayerState& p, const ItemDatabase& items);

    // Player.applyRestRecovery(fullyRested) (M42, phase-3 port): restores
    // a fraction of missing HP/Magicka/Fatigue -- 2/3 when disturbed
    // (fullyRested==false), the full amount when fullyRested, further
    // reduced to 3/4 of THAT when HasAilment(8) is active (the two
    // reductions compound: 2/3 * 3/4 = 1/2 when both apply) -- clears the
    // 3 temporary combat buffs (increaseHarmBuff/increaseArmorBuff/
    // safeCampingBuff), rolls a flat 10% chance to remove item id 96 from
    // the FIRST inventory slot that holds it (not necessarily equipped,
    // unlike TickPerSecond's own item-109 removal above -- a different
    // mechanic, this item's real identity not yet confirmed beyond its
    // numeric id), then independently rolls a 25% chance to cure EACH of
    // the 6 non-vampirism/non-mana-burn ailments (ailments 4 and 5 are
    // skipped entirely, matching the original's own `ailmentId != 4 &&
    // ailmentId != 5` guard -- those two clear only via
    // TickStatusCountdowns's own timers). Confirmed sole caller:
    // CampState::Tick (player/camp_state.h) -- was GameCanvas's own
    // camp-interrupted/camp-complete branches in run().
    static void ApplyRestRecovery(PlayerState& p, bool fullyRested, const ItemDatabase& items, JavaRandom& rng);
};

}  // namespace stormhold
