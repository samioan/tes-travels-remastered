// M43 smoke test: the real "Reveal Traitor" mini-quiz (ui/options_menu.h)
// -- secondaryParam 31's own case 8, then the 68/65/66/67 chain
// ESGame.commandAction1 dispatches, plus Player.java's own
// grantStarFrostItem() (player/player_inventory.h).
//
// No JVM ground truth is available (same reason as every prior
// milestone). Every real target is independently re-derived from
// ../../../src/ESGame.java's own lines 1098-1101/1219-1256/2272-2289 and
// ../../../src/Player.java's own lines 2174-2211, not read back from
// options_menu.cpp/player_inventory.cpp -- including the suspect-name
// list (transcribed AGAIN here from ESGame.java's own newRevealWhomUI()
// literals rather than shared with options_menu.cpp's own table) and
// Util.replace's first-occurrence-only semantics (re-implemented here
// from Util.java directly). Text is checked against the real
// npcstrings.dat (M8's ShopDialogue), the StarFrost grant/eviction
// arithmetic against the real itemsin.dat sell prices, and text
// placement against the BitmapFont oracle the same way M37-M39's own
// tests established.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/help_text.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/shop_dialogue.h"
#include "assets/spell_database.h"
#include "dungeon/dungeon_runtime.h"
#include "graphics/backbuffer.h"
#include "graphics/bitmap_font.h"
#include "player/player_combat_stats.h"
#include "player/player_creation.h"
#include "player/player_inventory.h"
#include "player/player_save.h"
#include "player/player_state.h"
#include "render/message_popup.h"
#include "ui/options_menu.h"
#include "ui/screen.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::Backbuffer;
namespace BitmapFont = dawnstar::BitmapFont;
using dawnstar::CharacterData;
using dawnstar::GeneratedLevel;
using dawnstar::ItemDatabase;
using dawnstar::MessagePopup;
using dawnstar::OptionsMenu;
using dawnstar::OptionsMenuAction;
using dawnstar::PlayerCombatStats;
using dawnstar::PlayerCreation;
using dawnstar::PlayerInventory;
using dawnstar::PlayerSave;
using dawnstar::PlayerState;
using dawnstar::Screen;
using dawnstar::ShopDialogue;
using dawnstar::WorldRegistry;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

uint16_t PixelAt(const Backbuffer& bb, int x, int y) {
    return bb.Data()[static_cast<size_t>(y) * Backbuffer::kWidth + x];
}

// Same oracle technique m37/m38/m39's own tests established.
bool TextRenderedAt(const Backbuffer& bb, int x0, int y0, const std::string& text, uint16_t color) {
    Backbuffer scratch;
    scratch.Fill(0);
    BitmapFont::DrawString(scratch, 0, 0, text, 0xFFFF);
    bool sawOnPixel = false;
    int spanW = static_cast<int>(text.size()) * BitmapFont::kAdvance;
    for (int dy = 0; dy < BitmapFont::kGlyphHeight; dy++) {
        for (int dx = 0; dx < spanW; dx++) {
            if (PixelAt(scratch, dx, dy) == 0) continue;
            sawOnPixel = true;
            int px = x0 + dx;
            int py = y0 + dy;
            if (px < 0 || px >= Backbuffer::kWidth || py < 0 || py >= Backbuffer::kHeight) return false;
            if (PixelAt(bb, px, py) != color) return false;
        }
    }
    return sawOnPixel;
}

constexpr uint16_t kTitleTextColor = dawnstar::PackRGB565(255, 255, 255);
constexpr uint16_t kItemTextColor = dawnstar::PackRGB565(255, 255, 0);

int CenteredTitleX(const std::string& title) { return Screen::width() / 2 - BitmapFont::StringWidth(title) / 2; }

bool TitleShownIs(const Backbuffer& bb, const std::string& title) {
    return TextRenderedAt(bb, CenteredTitleX(title), 0, title, kTitleTextColor);
}

// ESGame.java's own newRevealWhomUI()/newClueLogUI() literals
// (== Shop.java's NAMES[5..8]), transcribed AGAIN here independently of
// options_menu.cpp's own kSuspectNames table, so this test can't just be
// checking the implementation's own table against itself.
const char* const kQuizNames[4] = {"Alhavara", "Beatrice", "Chung", "Delacroix"};

// Util.java's own replace(source, tag, value), re-implemented here
// directly from Util.java's own lines 70-91 (first occurrence only --
// Util.java's own doc comment) rather than shared with options_menu.cpp's
// own ReplaceFirstTag helper.
std::string QuizReplaceTag(const std::string& source, const std::string& tag, const std::string& value) {
    size_t at = source.find(tag);
    if (at == std::string::npos) return source;
    return source.substr(0, at) + value + source.substr(at + tag.size());
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        ItemDatabase items = ItemDatabase::Load(archive);
        dawnstar::MonsterDatabase monsterDb = dawnstar::MonsterDatabase::Load(archive);
        dawnstar::SpellDatabase spells = dawnstar::SpellDatabase::Load(archive);
        CharacterData charData = CharacterData::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);
        dawnstar::HelpText helpText = dawnstar::HelpText::Load(archive);
        ShopDialogue shopDialogue = ShopDialogue::Load(root + "/npcstrings.dat");
        Check(shopDialogue.groups.size() == 10 && shopDialogue.groups[9].size() == 77,
              "the real npcstrings.dat should have 10 groups, group 9 with 77 entries (M8)");
        // Player.java's grantStarFrostItem() adds item id 100 (and
        // dropInventoryItem special-cases 101), so the real itemsin.dat
        // must carry rows through 101 -- a sanity check against the real
        // data, not the implementation.
        Check(items.ItemCount() >= 101, "the real itemsin.dat must carry item ids up through 101 (StarFrost)");

        std::vector<GeneratedLevel> levels;
        WorldRegistry world(geometry.rows.size());
        for (size_t i = 0; i < geometry.rows.size(); i++) {
            int levelNumber = static_cast<int>(i) + 1;
            levels.push_back(levelNumber == 1
                                  ? dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[0])
                                  : dawnstar::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[i], items,
                                                                              monsterDb));
            dawnstar::DungeonRuntime::RegisterGeneratedSpawns(levels.back(), world);
        }

        dawnstar::JavaRandom rng(424242);
        PlayerState player = PlayerCreation::CreateCharacter(0, "Traveler", charData, items, rng);
        int correctIdx = player.traitorIndex;
        Check(correctIdx >= 0 && correctIdx <= 3, "a fresh character's rolled traitorIndex should be 0..3");

        // M43's OnSelect-grown `nextItemSpawnId` parameter -- this test's
        // own stand-in for main.cpp's nextDropSpawnId, started at the same
        // 1 so the spawn ids handed out below are independently predictable.
        int16_t nextItemSpawnId = 1;
        OptionsMenu menu(helpText, shopDialogue);
        Backbuffer bb;

        // The full Options -> intro (68) -> confirm (65) -> "Yes" ->
        // whom (66) walk. The Options list persists its own selectedIndex
        // across visits (a real Screen, M39's own scenario-E precedent),
        // so this always forces it back to index 0 first via repeated
        // OnUp() calls, then steps down to index 8 ("Reveal Traitor").
        // The confirm screen is built FRESH on every entry in the
        // original (`this.RevealUI = this.newRevealUI()`), so it always
        // starts selected on "Yes" (index 0) -- no OnDown needed, which
        // also proves those fresh-selection semantics along the way.
        auto walkToWhom = [&]() {
            for (int i = 0; i < 10; i++) menu.OnUp();
            for (int i = 0; i < 8; i++) menu.OnDown();
            menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId);  // -> intro (68)
            menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId);  // -> confirm (65)
            menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId);  // "Yes" -> whom (66)
        };

        // --- A: the real screen chain. Options case 8 -> the intro info
        // screen; Ok -> the Yes/No confirm; "No" declines back to Options;
        // re-enter + "Yes" -> the 4-name whom screen; Cancel on whom ->
        // Options (the top-level cancel/backTarget check). ---
        for (int i = 0; i < 10; i++) menu.OnUp();
        for (int i = 0; i < 8; i++) menu.OnDown();
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::None,
              "selecting Reveal Traitor should show the intro info screen");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Reveal Traitor"), "the intro screen should be titled \"Reveal Traitor\"");
        std::vector<std::string> introLines = MessagePopup::WordWrap(shopDialogue.groups[9][66], Screen::width() - 5 - 5);
        Check(TextRenderedAt(bb, 5, 20, introLines[0], kItemTextColor),
              "the intro should show the real dialogue[9][66]'s first (word-wrapped) line");
        Check(menu.OnCancel() == OptionsMenuAction::None,
              "Cancel on the intro (Ok-only mode-4 screen) should be a real no-op");
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::None,
              "Ok on the intro should show the Yes/No confirm");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Reveal Traitor"), "the confirm screen is titled \"Reveal Traitor\" too");
        // Prompt-list layout (m37/m39's own formula): the prompt's own
        // wrapped lines render from y=20 at 12px each (wrap width
        // 176-10-10), then the items start 5px below at x=10, one row per
        // 13px (kLineHeight + RenderItemRows' own trailing cursorY++).
        int confirmPromptLines =
            static_cast<int>(MessagePopup::WordWrap(shopDialogue.groups[9][67], Screen::width() - 10 - 10).size());
        int confirmItemY = 20 + 12 * confirmPromptLines + 5;
        Check(TextRenderedAt(bb, 10, confirmItemY, "Yes", kItemTextColor),
              "the confirm's own first item should be \"Yes\"");
        Check(TextRenderedAt(bb, 10, confirmItemY + 13, "No", kItemTextColor),
              "the confirm's own second item should be \"No\"");
        Check(menu.OnCancel() == OptionsMenuAction::None,
              "Cancel on the confirm should be a real no-op (newRevealUI() removed its own Cancel command)");
        menu.OnDown();  // "No"
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::None,
              "selecting No on the confirm should not itself be a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Options"), "selecting No on the confirm should decline back to Options");

        walkToWhom();
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Reveal Traitor"), "the whom screen is titled \"Reveal Traitor\" too");
        // "Who is the Traitor?" fits one wrapped line, so the items start
        // at y=20+12+5=37, one row per 13px (kLineHeight +
        // RenderItemRows' own trailing cursorY++) -- the same formula
        // m39's own Clue Log check used for its first item.
        Check(TextRenderedAt(bb, 10, 37, kQuizNames[0], kItemTextColor),
              "the whom screen's first item should be the first suspect name");
        Check(TextRenderedAt(bb, 10, 37 + 13, kQuizNames[1], kItemTextColor),
              "the whom screen's second item should be the second suspect name");
        Check(menu.OnCancel() == OptionsMenuAction::None,
              "Cancel on the whom screen should not itself be a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Options"), "Cancel on the whom screen should return to Options (its backTarget)");

        // --- B: a WRONG guess. secondaryParam==66 builds
        // dialogue[9][68]+"\n"+dialogue[9][69]+"\n"+<TAG>-substituted
        // dialogue[9][72] (naming the REAL traitor -- the quiz tells you
        // who it actually was), sets NOTHING, and STILL teleports the
        // player to the hub: resetToHubPosition(false) runs OUTSIDE the
        // if/else, a real quirk preserved as found. ---
        int wrongIdx = (correctIdx + 1) % 4;
        player.currentLevel = 3;
        player.tileX = 4;
        player.tileY = 4;
        player.facing = 3;
        int invCountBefore = player.inventoryCount;
        walkToWhom();
        for (int i = 0; i < wrongIdx; i++) menu.OnDown();
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::None,
              "submitting a wrong guess should not itself be a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Reveal Traitor"), "the result screen should be titled \"Reveal Traitor\"");
        std::string expectedWrong = shopDialogue.groups[9][68] + "\n" + shopDialogue.groups[9][69] + "\n" +
                                    QuizReplaceTag(shopDialogue.groups[9][72], "<TAG>", kQuizNames[correctIdx]);
        std::vector<std::string> wrongLines = MessagePopup::WordWrap(expectedWrong, Screen::width() - 5 - 5);
        Check(TextRenderedAt(bb, 5, 20, wrongLines[0], kItemTextColor),
              "the wrong-guess result should open with the real dialogue[9][68]'s first (word-wrapped) line");
        Check(!player.newGamePlus, "a wrong guess must NOT set newGamePlus");
        Check(!player.starFrostBonusActive, "a wrong guess must NOT set starFrostBonusActive");
        Check(player.ambushTimer == -1 && !player.specialEncounterResolved,
              "a wrong guess must not arm the ambush/special-encounter state (that's the 67 Ok's own job)");
        Check(player.inventoryCount == invCountBefore, "a wrong guess must not grant any item");
        bool wrongGrantedStarFrost = false;
        for (int i = 0; i < player.inventoryCount; i++) {
            wrongGrantedStarFrost = wrongGrantedStarFrost || player.inventoryItemIds[static_cast<size_t>(i)] == 100;
        }
        Check(!wrongGrantedStarFrost, "a wrong guess must not grant the StarFrost item");
        Check(player.currentLevel == 1 && player.tileX == 9 && player.tileY == 9 && player.facing == 1,
              "even a WRONG guess teleports the player back to the hub -- the real resetToHubPosition-runs-either-way quirk");
        Check(menu.OnCancel() == OptionsMenuAction::None,
              "Cancel on the result (Ok-only mode-4 screen) should be a real no-op");
        // The secondaryParam==67 tail: Ok arms the ambush counter, flags
        // the special encounter resolved, and returns to the game.
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::ReturnToGame,
              "Ok on the result should request returning to the game");
        Check(player.ambushTimer == 1,
              "Ok on the result should set ambushTimer to 1 (its ONLY assignment anywhere in the game)");
        Check(player.specialEncounterResolved, "Ok on the result should set specialEncounterResolved");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Options"),
              "after returning to the game, the Options list should be what reopens next");

        // --- C: the CORRECT guess: newGamePlus, grantStarFrostItem (the
        // StarFrost lands in the first free slot with (spawnId<<16)+0
        // data, its +4 already live via M14's own SkillValue reader), the
        // dialogue[9][70] third line, the hub teleport, and the same 67
        // tail. The quiz is freely re-runnable in the original -- this is
        // a second, full, correct run after section B's wrong one. ---
        player.ambushTimer = -1;
        player.currentLevel = 3;
        player.tileX = 4;
        player.tileY = 4;
        player.facing = 3;
        int grantSlot = player.inventoryCount;
        int knownSkill = PlayerCombatStats::NthKnownSkillIndex(player, 0);
        Check(knownSkill >= 0, "a fresh character should have at least one rank>0 skill (the SkillValue probe needs it)");
        int skillBefore = PlayerCombatStats::SkillValue(player, charData, knownSkill, false);
        walkToWhom();
        for (int i = 0; i < correctIdx; i++) menu.OnDown();
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::None,
              "submitting the correct guess should not itself be a main.cpp-level action");
        Check(player.newGamePlus, "a correct guess should set newGamePlus");
        Check(player.starFrostBonusActive, "a correct guess should set starFrostBonusActive (grantStarFrostItem's own first line)");
        Check(PlayerCombatStats::SkillValue(player, charData, knownSkill, false) == skillBefore + 4,
              "the StarFrost bonus should be live immediately as skillValue()'s flat +4");
        Check(player.inventoryCount == grantSlot + 1, "the StarFrost should be appended to the first free slot");
        Check(player.inventoryItemIds[static_cast<size_t>(grantSlot)] == 100, "the granted item should be id 100 (StarFrost)");
        Check(player.inventoryItemData[static_cast<size_t>(grantSlot)] == (1 << 16),
              "the granted StarFrost's packed data should be (spawnId<<16)+0 with spawnId = the counter's hand-out");
        Check(nextItemSpawnId == 2, "the grant should have advanced the shared item spawn-id counter (1 -> 2)");
        Check(player.currentLevel == 1 && player.tileX == 9 && player.tileY == 9 && player.facing == 1,
              "a correct guess should also teleport the player back to the hub");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Reveal Traitor"), "the correct-guess result screen should be titled \"Reveal Traitor\"");
        std::string expectedRight =
            shopDialogue.groups[9][68] + "\n" + shopDialogue.groups[9][69] + "\n" + shopDialogue.groups[9][70];
        std::vector<std::string> rightLines = MessagePopup::WordWrap(expectedRight, Screen::width() - 5 - 5);
        Check(TextRenderedAt(bb, 5, 20, rightLines[0], kItemTextColor),
              "the correct-guess result should open with the same dialogue[9][68]'s first (word-wrapped) line");
        Check(menu.OnSelect(player, charData, items, spells, levels, world, nextItemSpawnId) == OptionsMenuAction::ReturnToGame,
              "Ok on the correct-guess result should also return to the game");
        Check(player.ambushTimer == 1, "Ok on the correct-guess result should also arm the ambush counter");

        // --- D: the transient-fields quirk. newGamePlus/ambushTimer/
        // starFrostBonusActive are NOT in either save format (Player.java
        // never serializes them) -- a full save/load cycle silently loses
        // all three, while the StarFrost item itself (part of the full
        // inventory block) survives. specialEncounterResolved and
        // traitorIndex, by contrast, ARE serialized (the packed traitor
        // byte, M12) -- checked here with traitorSuspicionCount==0, where
        // that byte's own << precedence bug (M12's finding) is lossless. ---
        std::vector<uint8_t> saved = PlayerSave::ToBytes(player);
        PlayerState loaded = PlayerSave::FromBytes(saved);
        Check(!loaded.newGamePlus, "newGamePlus is transient: a full save/load cycle must lose it");
        Check(!loaded.starFrostBonusActive, "starFrostBonusActive is transient: a save/load cycle must lose the +4 with it");
        Check(loaded.ambushTimer == -1, "ambushTimer is transient: a save/load cycle must reset it to -1");
        Check(loaded.specialEncounterResolved, "specialEncounterResolved IS serialized (the packed traitor byte's bit 16)");
        Check(loaded.traitorIndex == player.traitorIndex,
              "traitorIndex itself should round-trip (traitorSuspicionCount==0 keeps the packed byte lossless)");
        bool starFrostSurvived = false;
        for (int i = 0; i < loaded.inventoryCount; i++) {
            starFrostSurvived = starFrostSurvived || loaded.inventoryItemIds[static_cast<size_t>(i)] == 100;
        }
        Check(starFrostSurvived && loaded.inventoryCount == player.inventoryCount,
              "the StarFrost item itself IS serialized (the full inventory block), unlike the bonus flag");

        // --- E: grantStarFrostItem's own full-inventory eviction math,
        // driven directly (PlayerInventory::GrantStarFrostItem) on scratch
        // PlayerStates, with every expected value re-derived from
        // Player.java's own loop against the real itemsin.dat sell
        // prices. ---
        {
            // E1: a free slot exists -- plain append, nothing evicted.
            PlayerState p;
            for (int i = 0; i < 23; i++) {
                p.inventoryItemIds[static_cast<size_t>(i)] = static_cast<int8_t>(i + 1);
                p.inventoryItemData[static_cast<size_t>(i)] = 0;
            }
            p.inventoryCount = 23;
            int16_t counter = 1;
            PlayerInventory::GrantStarFrostItem(p, items, counter);
            Check(p.starFrostBonusActive, "GrantStarFrostItem sets starFrostBonusActive even on a plain free-slot append");
            Check(p.inventoryCount == 24 && p.inventoryItemIds[23] == 100 && p.inventoryItemData[23] == (1 << 16),
                  "with a free slot the StarFrost appends at the end with (spawnId<<16)+0 data");
            Check(counter == 2, "the grant takes exactly one spawn id from the counter");
            bool e1Intact = true;
            for (int i = 0; i < 23; i++) {
                e1Intact = e1Intact && p.inventoryItemIds[static_cast<size_t>(i)] == static_cast<int8_t>(i + 1);
            }
            Check(e1Intact, "no existing slot is touched when a free slot exists");
        }
        {
            // E2: full inventory -- the lowest-positive-sell-price
            // non-equipped slot is evicted (re-derived independently
            // below; all ids positive here, so the isEquipped branch
            // can't fire and neither can the 87 break).
            PlayerState p;
            for (int i = 0; i < 24; i++) {
                p.inventoryItemIds[static_cast<size_t>(i)] = static_cast<int8_t>(i + 1);  // ids 1..24
                p.inventoryItemData[static_cast<size_t>(i)] = 0;
            }
            p.inventoryCount = 24;
            int lowestValue = 10000;
            int evictSlot = -1;
            for (int slot = 0; slot < 24; slot++) {
                int itemId = std::abs(static_cast<int>(p.inventoryItemIds[static_cast<size_t>(slot)]));
                if (itemId == 87) {
                    evictSlot = slot;
                    break;
                }
                int value = items.sellPrice[static_cast<size_t>(itemId - 1)];
                if (value < lowestValue && value > 0) {
                    evictSlot = slot;
                    lowestValue = value;
                }
            }
            Check(evictSlot >= 0, "ids 1..24 should contain at least one positive-sell-price item");
            int8_t evictedId = p.inventoryItemIds[static_cast<size_t>(evictSlot)];
            int16_t counter = 5;
            PlayerInventory::GrantStarFrostItem(p, items, counter);
            Check(p.inventoryCount == 24, "the eviction keeps the inventory full (24)");
            Check(p.inventoryItemIds[23] == 100, "the StarFrost lands in the tail slot the eviction freed");
            bool evictedGone = true;
            for (int i = 0; i < p.inventoryCount; i++) {
                evictedGone = evictedGone && p.inventoryItemIds[static_cast<size_t>(i)] != evictedId;
            }
            Check(evictedGone, "the independently-derived lowest-value slot's item should be the one evicted");
        }
        {
            // E3: an id-87 item present -- Player.java's loop breaks on the
            // FIRST one it sees, so it wins outright (re-derived
            // independently below: the scan stops there too, before any
            // cheaper later item is even considered).
            PlayerState p;
            for (int i = 0; i < 24; i++) {
                p.inventoryItemIds[static_cast<size_t>(i)] = static_cast<int8_t>(i + 1);
                p.inventoryItemData[static_cast<size_t>(i)] = 0;
            }
            p.inventoryItemIds[10] = 87;  // replaces id 11
            p.inventoryCount = 24;
            int lowestValue = 10000;
            int evictSlot = -1;
            for (int slot = 0; slot < 24; slot++) {
                int itemId = std::abs(static_cast<int>(p.inventoryItemIds[static_cast<size_t>(slot)]));
                if (itemId == 87) {
                    evictSlot = slot;
                    break;
                }
                int value = items.sellPrice[static_cast<size_t>(itemId - 1)];
                if (value < lowestValue && value > 0) {
                    evictSlot = slot;
                    lowestValue = value;
                }
            }
            Check(evictSlot == 10, "the independent derivation should pick the id-87 slot (the break)");
            int16_t counter = 9;
            PlayerInventory::GrantStarFrostItem(p, items, counter);
            bool no87Left = true;
            for (int i = 0; i < p.inventoryCount; i++) {
                no87Left = no87Left && std::abs(static_cast<int>(p.inventoryItemIds[static_cast<size_t>(i)])) != 87;
            }
            Check(no87Left && p.inventoryCount == 24 && p.inventoryItemIds[23] == 100,
                  "the id-87 slot should be evicted outright (Player.java's own break), with StarFrost in the freed tail slot");
        }
        {
            // E4: every slot equipped -- Player.java's evictSlot stays -1
            // and its removeInventorySlot(-1) would throw
            // ArrayIndexOutOfBoundsException (unreachable in practice: at
            // most ~7 of 24 slots can ever be equipped). The port guards
            // it defensively instead (player_inventory.h's own doc
            // comment): no crash, no eviction, no grant -- but
            // starFrostBonusActive is still set (it precedes the first
            // add attempt) and the spawn id is still consumed.
            int equipId = -1;
            for (int id = 1; id <= items.ItemCount() && equipId == -1; id++) {
                if (items.IsEquippable(id)) equipId = id;
            }
            Check(equipId != -1, "the real itemsin.dat should contain at least one equippable item");
            PlayerState p;
            for (int i = 0; i < 24; i++) {
                p.inventoryItemIds[static_cast<size_t>(i)] = static_cast<int8_t>(-equipId);
                p.inventoryItemData[static_cast<size_t>(i)] = 0;
            }
            p.inventoryCount = 24;
            int16_t counter = 1;
            PlayerInventory::GrantStarFrostItem(p, items, counter);
            Check(p.starFrostBonusActive,
                  "starFrostBonusActive is set before the first add attempt, even in the guarded no-grant edge");
            Check(p.inventoryCount == 24, "the guarded edge must not evict anything");
            bool e4NoStarFrost = true;
            for (int i = 0; i < p.inventoryCount; i++) {
                e4NoStarFrost = e4NoStarFrost && p.inventoryItemIds[static_cast<size_t>(i)] != 100;
            }
            Check(e4NoStarFrost, "the guarded edge grants no StarFrost (the retry add fails on a still-full inventory)");
            Check(counter == 2, "the spawn id is consumed before the first add attempt either way (Player.java's own ordering)");
        }

        if (g_ok) {
            std::printf("all reveal-traitor checks passed\n");
            return 0;
        } else {
            std::printf("SOME CHECKS FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("EXCEPTION: %s\n", e.what());
        return 1;
    }
}
