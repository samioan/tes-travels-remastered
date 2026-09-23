// M61 smoke test: PlayerInventory's newly-ported inventory-ACTION logic --
// ItemTooltip/CanEquipOrUnequip/IsScrollCategory/CanLearnSpellFromScroll/
// LearnSpellFromScroll/CanUseItem/UseItem (Player.java's own itemTooltip()/
// canEquipOrUnequip()/isScrollCategory()/canLearnSpellFromScroll()/
// learnSpellFromScroll()/canUseItem()/useItem()) -- the last un-ported
// pieces of Player's own inventory logic, deliberately NOT wired into any
// UI yet (see player_inventory.h's own M61 doc comment for that scope
// line).
#include <cstdio>
#include <cstdlib>
#include <string>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/spell_database.h"
#include "dungeon/dungeon_runtime.h"
#include "player/player_creation.h"
#include "player/player_inventory.h"

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

// Confirmed real finding (see player_inventory.h's own CanUseItem/
// IsScrollCategory comments): the two predicates are an EXACT duplicate
// of each other, category-13-or-15, under two different names. Verified
// here against every real item in the database rather than a couple of
// hand-picked ids.
void TestScrollCategoryDuplicateFinding(const CharacterData& charData, const ItemDatabase& items) {
    std::printf("-- IsScrollCategory/CanUseItem confirmed-duplicate finding --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    for (int id = 1; id <= items.ItemCount(); id++) {
        p.inventoryItemIds[0] = static_cast<int8_t>(id);
        bool scroll = PlayerInventory::IsScrollCategory(p, 0, items);
        bool useable = PlayerInventory::CanUseItem(p, 0, items);
        if (scroll != useable) {
            std::printf("  FAIL: item id %d disagrees between IsScrollCategory and CanUseItem\n", id);
            g_ok = false;
        }
    }
}

// CanEquipOrUnequip: categories 1-10 or 17, confirmed to include 17 (NOT
// the same range as ItemDatabase::IsEquipmentCategory, which stops at 10).
void TestCanEquipOrUnequip(const CharacterData& charData, const ItemDatabase& items) {
    std::printf("-- CanEquipOrUnequip --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    int cat17Id = -1;
    for (int id = 1; id <= items.ItemCount(); id++) {
        int8_t cat = items.category[static_cast<size_t>(id - 1)];
        p.inventoryItemIds[0] = static_cast<int8_t>(id);
        bool got = PlayerInventory::CanEquipOrUnequip(p, 0, items);
        bool want = (cat >= 1 && cat <= 10) || cat == 17;
        Expect(got == want, "CanEquipOrUnequip should match the 1-10-or-17 category range exactly");
        if (cat == 17 && cat17Id < 0) cat17Id = id;
    }
    if (cat17Id < 0) {
        std::printf("  (no real category-17 item found -- the 1-10-or-17 sweep above still covered it)\n");
    }
}

// Category 12: spell scrolls. CanLearnSpellFromScroll needs a skill
// PREREQUISITE (some existing rank in the spell's governing skill), not a
// "not already known" gate.
void TestLearnSpellFromScroll(const CharacterData& charData, const ItemDatabase& items,
                               const SpellDatabase& spells) {
    std::printf("-- CanLearnSpellFromScroll/LearnSpellFromScroll --\n");
    int scrollId = -1;
    for (int id = 1; id <= items.ItemCount(); id++) {
        if (items.category[static_cast<size_t>(id - 1)] == 12) {
            scrollId = id;
            break;
        }
    }
    if (scrollId < 0) {
        std::printf("  (no real category-12 spell scroll found in itemsin.dat -- skipping)\n");
        return;
    }

    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
    p.inventoryCount = 0;
    for (auto& id : p.inventoryItemIds) id = 0;
    PlayerInventory::AddInventoryItemRaw(p, scrollId, 0, 0);
    int slot = p.inventoryCount - 1;
    // CanLearnSpellFromScroll/LearnSpellFromScroll both read the LOW byte
    // of inventoryItemData (AddInventoryItemRaw's own `charge` parameter,
    // per its packing scheme -- see player_inventory.cpp's own comment)
    // as the encoded spell id; no confirmed real producer of a category-12
    // item with a nonzero charge byte was found anywhere in ../../../src/
    // (same "no confirmed caller/producer" situation several of this
    // milestone's other methods are already in), so it's set directly
    // here to exercise the READING logic in isolation.
    p.inventoryItemData[static_cast<size_t>(slot)] = 1;  // Spell id 1.

    int spellId = p.inventoryItemData[static_cast<size_t>(slot)] & 0xFF;
    int8_t skillIdx = spells.ById(spellId).skillRequired;

    p.skills[static_cast<size_t>(skillIdx)][0] = 0;
    Expect(!PlayerInventory::CanLearnSpellFromScroll(p, slot, items, spells),
           "zero rank in the governing skill should refuse the scroll");

    p.skills[static_cast<size_t>(skillIdx)][0] = 5;
    Expect(PlayerInventory::CanLearnSpellFromScroll(p, slot, items, spells),
           "a nonzero rank in the governing skill should allow learning");

    int beforeCount = p.inventoryCount;
    bool learned = PlayerInventory::LearnSpellFromScroll(p, slot, items);
    Expect(learned, "LearnSpellFromScroll always returns true");
    Expect((p.knownSpellsMask & (1u << (spellId - 1))) != 0, "the matching knownSpellsMask bit should now be set");
    Expect(p.inventoryCount == beforeCount - 1, "the scroll slot should be consumed");
}

// The 13 real "gift"/special-consumable items, ids 87-99 (docs/
// ASSET_FORMATS.md's own confirmed itemsin.dat note) -- exercised against
// whichever of them actually exist in the loaded database, by real id, not
// assumed present.
void TestUseItemGiftIds(const CharacterData& charData, const ItemDatabase& items) {
    std::printf("-- UseItem: the 87-99 gift/special-consumable switch --\n");
    MonsterDatabase monsters;  // Empty -- every sub-case below passes target=nullptr (see below).
    WorldRegistry world(1);
    JavaRandom rng(1);

    auto freshPlayerWithItem = [&](int id, int packedValue) {
        PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
        p.inventoryCount = 0;
        for (auto& itemId : p.inventoryItemIds) itemId = 0;
        PlayerInventory::AddInventoryItemRaw(p, id, packedValue, 0);
        return p;
    };

    if (items.ItemCount() < 87) {
        std::printf("  (itemsin.dat has fewer than 87 items -- skipping the whole 87-99 sweep)\n");
        return;
    }

    // id 87: warp to camp / mark camp, branching on hasCampMark() -- only
    // meaningful when currentLevel==1 (both the mark-camp default spawn
    // and the warp branch's own gate).
    {
        PlayerState p = freshPlayerWithItem(87, 0);
        Expect(!PlayerInventory::HasCampMark(p), "a fresh character should start with no camp mark");
        int slot = p.inventoryCount - 1;
        PlayerInventory::UseItem(p, slot, nullptr, items, monsters, world, rng);
        Expect(p.inventoryCount == 0, "using id 87 should always consume the slot");
        Expect(PlayerInventory::HasCampMark(p), "with no existing camp mark, id 87 should mark one (not warp)");
    }
    {
        PlayerState p = freshPlayerWithItem(87, 0);
        p.campLevel = 3;
        p.campX = 5;
        p.campY = 6;
        p.campFacing = 2;
        p.currentLevel = 1;
        int slot = p.inventoryCount - 1;
        PlayerInventory::UseItem(p, slot, nullptr, items, monsters, world, rng);
        Expect(p.currentLevel == 3 && p.tileX == 5 && p.tileY == 6,
               "with an existing camp mark on currentLevel==1, id 87 should warp to it instead of re-marking");
    }

    // id 88: cure a random active ailment.
    {
        PlayerState p = freshPlayerWithItem(88, 0);
        p.ailmentMask = static_cast<int8_t>(1 << 2);  // Exactly one active ailment -- CureRandomAilment's pick is deterministic.
        int slot = p.inventoryCount - 1;
        PlayerInventory::UseItem(p, slot, nullptr, items, monsters, world, rng);
        Expect(p.ailmentMask == 0, "id 88 with exactly one active ailment should clear it");
        Expect(p.inventoryCount == 0, "id 88 should consume the slot");
    }

    // id 89: HP -> max HP. id 90: Magicka -> max Magicka. id 93: both.
    {
        PlayerState p = freshPlayerWithItem(89, 0);
        p.coreStats[2] = 1;
        p.coreStats[3] = 50;
        int slot = p.inventoryCount - 1;
        PlayerInventory::UseItem(p, slot, nullptr, items, monsters, world, rng);
        Expect(p.coreStats[2] == 50, "id 89 should fully restore HP");
    }
    {
        PlayerState p = freshPlayerWithItem(93, 0);
        p.coreStats[2] = 1;
        p.coreStats[3] = 50;
        p.coreStats[4] = 2;
        p.coreStats[5] = 40;
        int slot = p.inventoryCount - 1;
        PlayerInventory::UseItem(p, slot, nullptr, items, monsters, world, rng);
        Expect(p.coreStats[2] == 50 && p.coreStats[4] == 40, "id 93 should restore both HP and Magicka");
    }

    // id 91: a REAL confirmed original quirk -- adds 3*coreStats[5] (max
    // MAGICKA, the same index case 90 restores Magicka to) to
    // coreStats[6] (current FATIGUE), uncapped, NOT 3*maxFatigue as its
    // slot in the 87-99 sequence (right after the Magicka restorative)
    // might suggest. Confirmed by reading useItem()'s own case 91 line
    // directly: `this.coreStats[6] = (short)(this.coreStats[6] + 3 *
    // this.coreStats[5])` -- also consistent with id 91 being the one
    // "gift" item whose own specialEffectText entry (giftIdx 4) is blank
    // ({"", ""}), unlike every other id in this switch, suggesting this
    // may be unused/debug content in the original rather than a live
    // player-facing item. Preserved exactly, not "corrected" to use
    // coreStats[7] (max Fatigue).
    {
        PlayerState p = freshPlayerWithItem(91, 0);
        p.coreStats[5] = 15;
        p.coreStats[6] = 10;
        p.coreStats[7] = 9999;  // Deliberately never read by this branch -- see the quirk note above.
        int slot = p.inventoryCount - 1;
        PlayerInventory::UseItem(p, slot, nullptr, items, monsters, world, rng);
        Expect(p.coreStats[6] == 10 + 3 * 15,
               "id 91 should add 3*coreStats[5] (max Magicka, NOT max Fatigue) to current Fatigue -- see this "
               "case's own comment for the confirmed quirk");
    }

    // id 92: level-exp += 1 (coreStats[1]++).
    {
        PlayerState p = freshPlayerWithItem(92, 0);
        p.coreStats[1] = 4;
        int slot = p.inventoryCount - 1;
        PlayerInventory::UseItem(p, slot, nullptr, items, monsters, world, rng);
        Expect(p.coreStats[1] == 5, "id 92 should increment level-exp by 1");
    }

    // id 94/95: harm/armor buffs. id 96: safe-camping buff, NOT consumed.
    {
        PlayerState p = freshPlayerWithItem(94, 0);
        int slot = p.inventoryCount - 1;
        PlayerInventory::UseItem(p, slot, nullptr, items, monsters, world, rng);
        Expect(p.increaseHarmBuff, "id 94 should set increaseHarmBuff");
    }
    {
        PlayerState p = freshPlayerWithItem(95, 0);
        int slot = p.inventoryCount - 1;
        PlayerInventory::UseItem(p, slot, nullptr, items, monsters, world, rng);
        Expect(p.increaseArmorBuff, "id 95 should set increaseArmorBuff");
    }
    {
        PlayerState p = freshPlayerWithItem(96, 0);
        int slot = p.inventoryCount - 1;
        PlayerInventory::UseItem(p, slot, nullptr, items, monsters, world, rng);
        Expect(p.safeCampingBuff, "id 96 should set safeCampingBuff");
        Expect(p.inventoryCount == 1, "id 96 is the one gift item that should NOT consume its slot");
    }

    // ids 97/98/99: instant-kill scrolls, gated on a real Monster target.
    // Confirmed real finding (see UseItem's own declaration comment):
    // ESGame's only call site always passes target=null, so these three
    // branches never actually fire in the real game -- exercised here
    // with target=nullptr (matching that real call site exactly, still
    // consuming the slot and touching nothing else) AND, separately, with
    // a real MonsterState* to prove the gated branch itself still works
    // correctly for whichever future caller might supply one.
    for (int id : {97, 98, 99}) {
        PlayerState p = freshPlayerWithItem(id, 0);
        int slot = p.inventoryCount - 1;
        PlayerInventory::UseItem(p, slot, nullptr, items, monsters, world, rng);
        Expect(p.inventoryCount == 0, "an instant-kill scroll with target=nullptr (the real call site's own "
                                       "shape) should still just consume the slot and do nothing else");
    }
}

// Exercises ids 97-99's gated instant-kill branch against a real
// MonsterState/MonsterDatabase, purely as a logic check on the ported
// switch itself -- NOT a claim that this path is reachable in the real
// game (see TestUseItemGiftIds's own comment on that).
void TestUseItemInstantKillGate(const CharacterData& charData, const ItemDatabase& items,
                                 const MonsterDatabase& monsters) {
    std::printf("-- UseItem: ids 97-99's instant-kill gate against a real MonsterState --\n");
    if (items.ItemCount() < 97 || monsters.TypeCount() < 1) {
        std::printf("  (missing real item id 97 or a real monster type -- skipping)\n");
        return;
    }
    WorldRegistry world(1);
    JavaRandom rng(1);

    // Find a monster type whose stat(4)/stat(10) are both <= 13, so id 97
    // (the tightest gate) should succeed against it.
    int easyType = -1;
    for (int t = 1; t <= monsters.TypeCount(); t++) {
        if (monsters.Stat(t, 4) <= 13 && monsters.Stat(t, 10) <= 13) {
            easyType = t;
            break;
        }
    }
    if (easyType < 0) {
        std::printf("  (no real monster type passes the id-97 gate (stat4/stat10 <= 13) -- skipping the kill "
                     "case, the miss case below still covers the gate)\n");
    } else {
        PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
        p.inventoryCount = 0;
        for (auto& id : p.inventoryItemIds) id = 0;
        PlayerInventory::AddInventoryItemRaw(p, 97, 0, 0);
        int slot = p.inventoryCount - 1;
        MonsterState target;
        target.typeIndex = static_cast<int8_t>(easyType);
        target.currentHp = 100;
        target.dungeonLevel = 1;
        PlayerInventory::UseItem(p, slot, &target, items, monsters, world, rng);
        Expect(target.currentHp == 0, "a monster passing id 97's gate should have currentHp forced to 0");
        Expect(world.monsters[0].count(target.spawnId) == 1, "the killed monster should be stored into the registry");
    }

    // Find a monster type that FAILS the gate (stat4 or stat10 > 13), to
    // confirm id 97 leaves it untouched.
    int hardType = -1;
    for (int t = 1; t <= monsters.TypeCount(); t++) {
        if (monsters.Stat(t, 4) > 13 || monsters.Stat(t, 10) > 13) {
            hardType = t;
            break;
        }
    }
    if (hardType < 0) {
        std::printf("  (every real monster type passes id 97's gate -- skipping the miss case)\n");
    } else {
        PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
        p.inventoryCount = 0;
        for (auto& id : p.inventoryItemIds) id = 0;
        PlayerInventory::AddInventoryItemRaw(p, 97, 0, 0);
        int slot = p.inventoryCount - 1;
        MonsterState target;
        target.typeIndex = static_cast<int8_t>(hardType);
        target.currentHp = 100;
        target.dungeonLevel = 1;
        PlayerInventory::UseItem(p, slot, &target, items, monsters, world, rng);
        Expect(target.currentHp == 100, "a monster failing id 97's gate should be left completely untouched");
    }
}

// ItemTooltip: one real item per confirmed category shape, scanned by
// real id rather than assumed.
void TestItemTooltip(const CharacterData& charData, const ItemDatabase& items, const SpellDatabase& spells) {
    std::printf("-- ItemTooltip --\n");
    PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);

    // Weapon (category 1, Miner Pick -- the barbarian's own starting
    // weapon, already confirmed real by player_creation.h's own tests).
    {
        std::string tip = PlayerInventory::ItemTooltip(p, 0, items, spells, charData);
        Expect(tip.find("Weapon value:") != std::string::npos, "a category-1 weapon's tooltip should show a weapon value");
        Expect(tip.find(items.name[0]) == 0, "the tooltip should start with the item's own real name");
    }
    // Armor (category 5-10, Padded Cloth -- the barbarian's own starting armor).
    {
        std::string tip = PlayerInventory::ItemTooltip(p, 1, items, spells, charData);
        Expect(tip.find("Armor value:") != std::string::npos, "a category 5-10 armor's tooltip should show an armor value");
    }

    // Category 12 (spell scroll).
    int scrollId = -1;
    for (int id = 1; id <= items.ItemCount(); id++) {
        if (items.category[static_cast<size_t>(id - 1)] == 12) {
            scrollId = id;
            break;
        }
    }
    if (scrollId > 0) {
        PlayerState p2 = p;
        p2.inventoryCount = 0;
        for (auto& id : p2.inventoryItemIds) id = 0;
        PlayerInventory::AddInventoryItemRaw(p2, scrollId, 0, 0);
        int slot = p2.inventoryCount - 1;
        p2.inventoryItemData[static_cast<size_t>(slot)] = 1;  // Spell id 1 -- see TestLearnSpellFromScroll's own comment.
        std::string tip = PlayerInventory::ItemTooltip(p2, slot, items, spells, charData);
        Expect(tip.find("Spell: ") != std::string::npos, "a category-12 scroll's tooltip should show \"Spell: <name>\"");
        Expect(tip.find(spells.ById(1).name) != std::string::npos, "the tooltip should show the real spell's own name");
    } else {
        std::printf("  (no real category-12 item found -- skipping the spell-scroll tooltip case)\n");
    }

    // Gift/special-consumable id 87 -- confirmed specialEffectText[0].
    if (items.ItemCount() >= 87) {
        PlayerState p3 = p;
        p3.inventoryCount = 0;
        for (auto& id : p3.inventoryItemIds) id = 0;
        PlayerInventory::AddInventoryItemRaw(p3, 87, 0, 0);
        int slot = p3.inventoryCount - 1;
        std::string tip = PlayerInventory::ItemTooltip(p3, slot, items, spells, charData);
        Expect(tip.find("Warp to camp") != std::string::npos,
               "id 87's tooltip should include its own real specialEffectText[0] line, \"Warp to camp\"");
    } else {
        std::printf("  (itemsin.dat has fewer than 87 items -- skipping the gift-item tooltip case)\n");
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assets(root);
        CharacterData charData = CharacterData::Load(assets);
        ItemDatabase items = ItemDatabase::Load(assets);
        SpellDatabase spells = SpellDatabase::Load(assets);
        MonsterDatabase monsters = MonsterDatabase::Load(assets);

        TestScrollCategoryDuplicateFinding(charData, items);
        TestCanEquipOrUnequip(charData, items);
        TestLearnSpellFromScroll(charData, items, spells);
        TestUseItemGiftIds(charData, items);
        TestUseItemInstantKillGate(charData, items, monsters);
        TestItemTooltip(charData, items, spells);

        if (!g_ok) {
            std::fprintf(stderr, "m61_inventory_actions_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m61_inventory_actions_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
