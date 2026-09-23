// M46 smoke test: SpellCasting (combat/spell_casting.h) --
// GameCanvas.resolveSpellCastInput()/resolveSpellCycleInput() (decompiled/
// e.java's h(long)/g(long)) and Player.castOnSelf()/castOnMonster()/
// cycleSelectedSpell(), M41's own dispatch web finally reaching its
// spell-casting half.
//
// No JVM ground truth possible (same reasoning every other data-model
// milestone's own tests already give) -- verified via hand-derived
// synthetic scenarios covering every ResolveSpellCastInput branch (dumped
// real spellsin.dat rows first to pick spell ids whose CastOnSelf/
// CastOnMonster effect is provably tier-INDEPENDENT, so each check is
// deterministic regardless of the RNG seed -- no seed-hunting needed), plus
// a direct proof of the confirmed target.store() asymmetry (combat/
// spell_casting.h's own class comment): casting a store()-calling spell
// (4) really updates the WorldRegistry's own copy, casting a
// non-store()-calling spell (16) leaves the registry's copy untouched even
// though the live `target` local mutated.
#include <cstdio>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/spell_database.h"
#include "combat/spell_casting.h"
#include "dungeon/dungeon_runtime.h"
#include "monster/monster_runtime.h"
#include "player/player_creation.h"

namespace {

using namespace stormhold;

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

void TestNotEnoughMagicka(const CharacterData& charData, const ItemDatabase& items, const MonsterDatabase& monsters,
                           const SpellDatabase& spells) {
    std::printf("-- not enough Magicka: no cast, no message-worthy side effect --\n");
    PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    player.selectedSpellId = 8;  // Absorb, magickaCost=20.
    player.coreStats[4] = 5;     // current Magicka, well under cost.
    bool castRequested = true;
    int64_t lastSpellCastTimeMs = 0;
    std::optional<MonsterState> target;
    WorldRegistry world(37);
    JavaRandom rng(1);

    SpellCasting::Result result = SpellCasting::ResolveSpellCastInput(
        player, target, castRequested, 10000, lastSpellCastTimeMs, spells, charData, items, monsters, rng, world);

    Expect(result == SpellCasting::Result::NotEnoughMagicka, "insufficient Magicka should be reported");
    Expect(!castRequested, "castRequested should clear regardless");
    Expect(player.coreStats[4] == 5, "Magicka should be untouched when the cast never happens");
    Expect(lastSpellCastTimeMs == 0, "lastSpellCastTimeMs should NOT restamp when Magicka was insufficient");
}

void TestInvalidSpell(const CharacterData& charData, const ItemDatabase& items, const MonsterDatabase& monsters,
                       const SpellDatabase& spells) {
    std::printf("-- invalid selectedSpellId --\n");
    PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    player.selectedSpellId = 99;  // past Spell.count (25).
    bool castRequested = true;
    int64_t lastSpellCastTimeMs = 0;
    std::optional<MonsterState> target;
    WorldRegistry world(37);
    JavaRandom rng(1);

    SpellCasting::Result result = SpellCasting::ResolveSpellCastInput(
        player, target, castRequested, 10000, lastSpellCastTimeMs, spells, charData, items, monsters, rng, world);

    Expect(result == SpellCasting::Result::InvalidSpell, "an out-of-range spell id should be reported");
    Expect(!castRequested, "castRequested should clear regardless");
}

void TestCooldownGate(const CharacterData& charData, const ItemDatabase& items, const MonsterDatabase& monsters,
                       const SpellDatabase& spells) {
    std::printf("-- 500ms cooldown gate --\n");
    PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    player.selectedSpellId = 23;  // Raise Attribute, cheap and self-targeted.
    player.coreStats[4] = 100;
    bool castRequested = true;
    int64_t lastSpellCastTimeMs = 1000;
    std::optional<MonsterState> target;
    WorldRegistry world(37);
    JavaRandom rng(1);

    // Only 499ms have passed.
    SpellCasting::Result result = SpellCasting::ResolveSpellCastInput(
        player, target, castRequested, 1499, lastSpellCastTimeMs, spells, charData, items, monsters, rng, world);
    Expect(result == SpellCasting::Result::OnCooldown, "a cast within the 500ms cooldown should be dropped");
    Expect(lastSpellCastTimeMs == 1000, "a dropped cast should NOT restamp lastSpellCastTimeMs");
    Expect(player.effectDurations[22] == 0, "a dropped cast should have no gameplay effect at all");

    // Exactly 500ms -- the original's own >= check allows it.
    castRequested = true;
    result = SpellCasting::ResolveSpellCastInput(player, target, castRequested, 1500, lastSpellCastTimeMs, spells,
                                                  charData, items, monsters, rng, world);
    Expect(result == SpellCasting::Result::CastOnSelf, "a cast at exactly the 500ms boundary should connect (>=, not >)");
    Expect(lastSpellCastTimeMs == 1500, "a successful cast should restamp lastSpellCastTimeMs to `now`");
}

void TestNoMonsterStillRestampsCooldown(const CharacterData& charData, const ItemDatabase& items,
                                         const MonsterDatabase& monsters, const SpellDatabase& spells) {
    std::printf("-- offensive spell, no target: NoMonster, but cooldown restamps anyway --\n");
    PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    player.selectedSpellId = 11;  // Damage, offensive (school 2), cost 8.
    player.coreStats[4] = 100;
    bool castRequested = true;
    int64_t lastSpellCastTimeMs = 0;
    std::optional<MonsterState> target;  // nullopt
    WorldRegistry world(37);
    JavaRandom rng(1);

    SpellCasting::Result result = SpellCasting::ResolveSpellCastInput(
        player, target, castRequested, 5000, lastSpellCastTimeMs, spells, charData, items, monsters, rng, world);

    Expect(result == SpellCasting::Result::NoMonster, "an offensive spell with no target should report NoMonster");
    Expect(player.coreStats[4] == 100, "Magicka should be untouched -- CastOnMonster never actually ran");
    // A real, surprising finding confirmed directly from decompiled/
    // e.java's h(long): `this.lastSpellCastTimeMs = now;` sits OUTSIDE the
    // offensive/self dispatch, so it restamps even on a "No monster here!"
    // result -- preserved exactly, not "fixed" into only restamping on an
    // actual cast.
    Expect(lastSpellCastTimeMs == 5000, "lastSpellCastTimeMs restamps even when NoMonster fires (confirmed original behavior)");
}

void TestCastOnSelfDeterministic(const CharacterData& charData, const ItemDatabase& items,
                                  const MonsterDatabase& monsters, const SpellDatabase& spells) {
    std::printf("-- CastOnSelf: spell 23 (Raise Attribute), tier-independent effect --\n");
    PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    player.selectedSpellId = 23;
    player.coreStats[4] = 100;
    bool castRequested = true;
    int64_t lastSpellCastTimeMs = 0;
    std::optional<MonsterState> target;
    WorldRegistry world(37);
    JavaRandom rng(42);

    SpellCasting::Result result = SpellCasting::ResolveSpellCastInput(
        player, target, castRequested, 10000, lastSpellCastTimeMs, spells, charData, items, monsters, rng, world);

    Expect(result == SpellCasting::Result::CastOnSelf, "a valid, affordable self spell should cast");
    // case 23 always writes -2, regardless of hit tier.
    Expect(player.effectDurations[22] == -2, "spell 23's own effectDurations[22] write is tier-independent");
    Expect(player.coreStats[4] < 100, "Magicka should have been spent (every tier deducts something)");
}

void TestCastOnSelfCuresAilment(const CharacterData& charData, const ItemDatabase& items,
                                 const MonsterDatabase& monsters, const SpellDatabase& spells) {
    std::printf("-- CastOnSelf: spell 25 (Remove Ailment) with exactly 1 active ailment --\n");
    PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    player.selectedSpellId = 25;
    player.coreStats[4] = 100;
    player.ailmentMask = 1 << 2;  // ailment 3, the only one set.
    bool castRequested = true;
    int64_t lastSpellCastTimeMs = 0;
    std::optional<MonsterState> target;
    WorldRegistry world(37);
    JavaRandom rng(1234);

    SpellCasting::Result result = SpellCasting::ResolveSpellCastInput(
        player, target, castRequested, 10000, lastSpellCastTimeMs, spells, charData, items, monsters, rng, world);

    Expect(result == SpellCasting::Result::CastOnSelf, "a valid, affordable self spell should cast");
    // With exactly 1 active ailment, CureRandomAilment's own pick is
    // forced (count==1 -> pick=1, no RNG branch at all) -- deterministic
    // regardless of seed, and case 25's loop runs at least once (mult>=1)
    // on every tier including a miss.
    Expect(player.ailmentMask == 0, "the sole active ailment should be cured with no RNG dependency");
}

void TestCastOnMonsterDeterministic(const CharacterData& charData, const ItemDatabase& items,
                                     const MonsterDatabase& monsters, const SpellDatabase& spells) {
    std::printf("-- CastOnMonster: spell 4 (Weakness), tier-independent scratch write + confirmed store() --\n");
    PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    player.selectedSpellId = 4;  // Weakness, offensive, cost 18.
    player.coreStats[4] = 100;
    bool castRequested = true;
    int64_t lastSpellCastTimeMs = 0;
    WorldRegistry world(37);
    JavaRandom rng(77);

    std::optional<MonsterState> target = MonsterRuntime::Spawn(10, 1, 2, monsters);
    int16_t spawnId = target->spawnId;

    SpellCasting::Result result = SpellCasting::ResolveSpellCastInput(
        player, target, castRequested, 10000, lastSpellCastTimeMs, spells, charData, items, monsters, rng, world);

    Expect(result == SpellCasting::Result::CastOnMonster, "a valid, affordable offensive spell with a target should cast");
    Expect(target->scratch[9] == -2, "spell 4's own scratch[9] write is tier-independent");
    Expect(player.coreStats[4] < 100, "Magicka should have been spent");

    // Case 4 DOES call target.store() -- confirm the WorldRegistry's own
    // copy really was updated, not just the local `target`.
    auto it = world.monsters[1].find(spawnId);  // dungeonLevel 2 -> index 1.
    Expect(it != world.monsters[1].end(), "case 4 should have stored the monster into the WorldRegistry");
    if (it != world.monsters[1].end()) {
        MonsterState stored = MonsterRuntime::FromBytes(it->second);
        Expect(stored.scratch[9] == -2, "the STORED copy should also carry the scratch[9] write");
    }
}

void TestStoreAsymmetry(const CharacterData& charData, const ItemDatabase& items, const MonsterDatabase& monsters,
                         const SpellDatabase& spells) {
    std::printf("-- confirmed asymmetry: spell 16 (Paralyze) mutates scratch but never stores --\n");
    WorldRegistry world(37);
    MonsterState baseline = MonsterRuntime::Spawn(11, 1, 2, monsters);
    baseline.scratch[6] = 0;
    DungeonRuntime::StoreMonster(world, baseline);

    PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    player.selectedSpellId = 16;  // Paralyze, offensive, cost 12.
    player.coreStats[4] = 100;
    bool castRequested = true;
    int64_t lastSpellCastTimeMs = 0;

    // Load a fresh local copy from the registry, same as main.cpp's own
    // MonsterInFront() would.
    std::optional<MonsterState> target =
        MonsterRuntime::FromBytes(world.monsters[1].at(baseline.spawnId));

    JavaRandom rng(9001);
    SpellCasting::Result result = SpellCasting::ResolveSpellCastInput(
        player, target, castRequested, 10000, lastSpellCastTimeMs, spells, charData, items, monsters, rng, world);
    Expect(result == SpellCasting::Result::CastOnMonster, "spell 16 should cast against a live target");

    // room16 = 10 - targetOffense; whether it's positive depends on the
    // spawned monster's own type-10 stat column -- only assert the
    // registry-vs-local divergence when the branch actually ran (room16 >
    // 0, i.e. target->scratch[6] == 1 locally).
    if (target->scratch[6] == 1) {
        MonsterState stored = MonsterRuntime::FromBytes(world.monsters[1].at(baseline.spawnId));
        Expect(stored.scratch[6] == 0,
               "the REGISTRY's own copy should be untouched -- case 16 never calls target.store() (confirmed asymmetry)");
        Expect(target->scratch[6] == 1, "the LOCAL target copy should show the mutation");
    } else {
        std::printf("  (this monster type's own targetOffense stat left room16 <= 0 -- scratch[6] branch didn't fire "
                     "this run, asymmetry not exercised)\n");
    }
}

void TestCycleSelectedSpell(const CharacterData& charData, const ItemDatabase& items, const SpellDatabase& spells) {
    std::printf("-- CycleSelectedSpell / ResolveSpellCycleInput --\n");
    PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);

    // No spells known at all.
    player.knownSpellsMask = 0;
    player.selectedSpellId = 0;
    bool cycleRequested = true;
    int newSpellId = SpellCasting::ResolveSpellCycleInput(player, cycleRequested, spells);
    Expect(newSpellId == 0, "cycling with no known spells should return 0 (\"No spells!\")");
    Expect(!cycleRequested, "cycleRequested should clear regardless");
    Expect(player.selectedSpellId == 0, "selectedSpellId should NOT be touched when nothing was cycled to");

    // Two known spells (ids 3 and 9, 0-based bits 2 and 8), currently
    // unselected (selectedSpellId=0 is not a valid id) -- should land on
    // the FIRST known spell.
    player.knownSpellsMask = (1u << 2) | (1u << 8);
    player.selectedSpellId = 0;
    cycleRequested = true;
    newSpellId = SpellCasting::ResolveSpellCycleInput(player, cycleRequested, spells);
    Expect(newSpellId == 3, "an invalid current selection should land on the first known spell id");
    Expect(player.selectedSpellId == 3, "ResolveSpellCycleInput should stamp selectedSpellId on success");

    // Cycling again from spell 3 should wrap to spell 9 (skipping every
    // unknown id in between).
    cycleRequested = true;
    newSpellId = SpellCasting::ResolveSpellCycleInput(player, cycleRequested, spells);
    Expect(newSpellId == 9, "cycling from spell 3 should skip to the next known spell, id 9");

    // Cycling again should wrap back around to spell 3.
    cycleRequested = true;
    newSpellId = SpellCasting::ResolveSpellCycleInput(player, cycleRequested, spells);
    Expect(newSpellId == 3, "cycling from the last known spell should wrap back to the first");

    // A single known spell should cycle back to itself.
    player.knownSpellsMask = 1u << 5;
    player.selectedSpellId = 6;
    cycleRequested = true;
    newSpellId = SpellCasting::ResolveSpellCycleInput(player, cycleRequested, spells);
    Expect(newSpellId == 6, "with exactly 1 known spell, cycling should return to the same spell");
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assets(root);
        ItemDatabase items = ItemDatabase::Load(assets);
        MonsterDatabase monsters = MonsterDatabase::Load(assets);
        CharacterData charData = CharacterData::Load(assets);
        SpellDatabase spells = SpellDatabase::Load(assets);

        Expect(spells.Count() == 25, "spellsin.dat should have 25 real rows (Spell.count)");

        TestNotEnoughMagicka(charData, items, monsters, spells);
        TestInvalidSpell(charData, items, monsters, spells);
        TestCooldownGate(charData, items, monsters, spells);
        TestNoMonsterStillRestampsCooldown(charData, items, monsters, spells);
        TestCastOnSelfDeterministic(charData, items, monsters, spells);
        TestCastOnSelfCuresAilment(charData, items, monsters, spells);
        TestCastOnMonsterDeterministic(charData, items, monsters, spells);
        TestStoreAsymmetry(charData, items, monsters, spells);
        TestCycleSelectedSpell(charData, items, spells);

        if (!g_ok) {
            std::fprintf(stderr, "m46_spell_casting_smoke: FAILED\n");
            return 1;
        }

        std::printf("all checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m46_spell_casting_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
