// M39 smoke test: OptionsMenu (ui/options_menu.h) -- the real IN-GAME
// options menu, built on M37's Screen + M38's own established patterns.
//
// No JVM ground truth is available (same reason as every prior
// milestone). Every real target is independently re-derived from
// ../../../src/ESGame.java's own secondaryParam==31/32/60/61/203/206/202
// branches, not read back from options_menu.cpp -- including its own
// RUMOR_STRING_OFFSET/UNCONFIRMED_A/UNCONFIRMED_B tables, transcribed
// AGAIN here directly from ../../../src/Shop.java rather than reused
// from options_menu.cpp, so this test can't just be checking its own
// implementation's arithmetic against itself. Clue Log text is checked
// against the real npcstrings.dat (M8's ShopDialogue) content. Text
// placement is cross-checked against BitmapFont::DrawString used as an
// oracle, the same technique M37/M38's own tests established.
#include <cstdio>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/help_text.h"
#include "assets/item_database.h"
#include "assets/shop_dialogue.h"
#include "graphics/backbuffer.h"
#include "graphics/bitmap_font.h"
#include "player/player_combat_stats.h"
#include "player/player_creation.h"
#include "player/player_state.h"
#include "render/message_popup.h"
#include "ui/options_menu.h"
#include "ui/screen.h"
#include "util/java_random.h"

namespace {

using dawnstar::Backbuffer;
namespace BitmapFont = dawnstar::BitmapFont;
using dawnstar::CharacterData;
using dawnstar::MessagePopup;
using dawnstar::OptionsMenu;
using dawnstar::OptionsMenuAction;
using dawnstar::PlayerCombatStats;
using dawnstar::PlayerState;
using dawnstar::Screen;
using dawnstar::ShopDialogue;

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

// Same oracle technique m37/m38's own tests established.
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

// ../../../src/Player.java's own buildCharacterSheet(), reconstructed
// independently here (not calling into options_menu.cpp's own internal
// BuildCharacterSheet, which isn't part of OptionsMenu's public API
// anyway) using only already-independently-verified building blocks
// (PlayerCombatStats::EffectiveStat/HasAilment, M14).
std::string ExpectedCharacterSheet(const PlayerState& p, const CharacterData& charData) {
    const char* ailmentNames[8] = {"Frost Limbs", "Snow Mirage",   "Blind",        "Troll Thirst",
                                    "Glacier Curse", "Grievous Harm", "Terrified", "Winter Worn"};
    std::string out = p.name + "\n" + charData.classNames[static_cast<size_t>(p.classIndex)] + "\n";
    out += "Level " + std::to_string(p.coreStats[0]) + " (" + std::to_string(p.coreStats[1]) + "/10)\n";
    out += "Health: " + std::to_string(PlayerCombatStats::EffectiveStat(p, charData, 2)) + "/" +
           std::to_string(p.coreStats[3]) + "\n";
    out += "Magicka: " + std::to_string(PlayerCombatStats::EffectiveStat(p, charData, 4)) + "/" +
           std::to_string(p.coreStats[5]) + "\n";
    out += "Fatigue: " + std::to_string(PlayerCombatStats::EffectiveStat(p, charData, 6)) + "/" +
           std::to_string(p.coreStats[7]) + "\n  \nStatus ailments: ";
    int count = 0;
    for (int i = 1; i <= 8; i++) {
        if (PlayerCombatStats::HasAilment(p, i)) {
            out += "\n";
            out += ailmentNames[i - 1];
            count++;
        }
    }
    if (count == 0) out += "\nNone";
    out += "\n  \nGift points found: " + std::to_string(p.giftPointsFound) + "\n  \nAttributes:\n";
    for (int i = 0; i < 8; i++) {
        int slot = 2 * i;
        out += charData.attributeNames[static_cast<size_t>(slot)] + ": " + std::to_string(p.attributes[static_cast<size_t>(slot)]) + "\n";
    }
    return out;
}

// ../../../src/Shop.java's own RUMOR_STRING_OFFSET/UNCONFIRMED_A/
// UNCONFIRMED_B, transcribed AGAIN here (independently of
// options_menu.cpp) for this test's own expected-value derivation.
constexpr int8_t kRumorStringOffset[4][6] = {
    {1, 3, 5, 8, 10, 12}, {1, 2, 4, 7, 9, 12}, {2, 3, 6, 7, 10, 11}, {2, 4, 5, 8, 9, 11},
};
constexpr int8_t kUnconfirmedA[24] = {13, 19, 25, 31, 14, 20, 26, 32, 15, 21, 27, 33,
                                       17, 23, 29, 35, 16, 22, 28, 34, 18, 24, 30, 36};
constexpr int8_t kUnconfirmedB[24] = {37, 43, 49, 55, 38, 44, 50, 56, 39, 45, 51, 57,
                                       41, 47, 53, 59, 40, 46, 52, 58, 42, 48, 54, 60};

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        CharacterData charData = CharacterData::Load(archive);
        dawnstar::HelpText helpText = dawnstar::HelpText::Load(archive);
        ShopDialogue shopDialogue = ShopDialogue::Load(root + "/npcstrings.dat");
        Check(shopDialogue.groups.size() == 10 && shopDialogue.groups[9].size() == 77,
              "the real npcstrings.dat should have 10 groups, group 9 with 77 entries (M8)");

        dawnstar::JavaRandom rng(12345);
        PlayerState player = dawnstar::PlayerCreation::CreateCharacter(0, "Traveler", charData, items, rng);

        // Passed BY VALUE (copied, not moved) below -- same convention
        // main.cpp uses for MenuFlow's own HelpText parameter -- so
        // `helpText`/`shopDialogue` stay valid for this test's own
        // expected-value derivation afterward.
        OptionsMenu menu(helpText, shopDialogue);
        Backbuffer bb;

        // --- A: initial state is the real Options list. ---
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Options"), "OptionsMenu should start on the real Options list");
        Check(TextRenderedAt(bb, 15, 20, "Stats", kItemTextColor),
              "the Options list's own first item should render (\"Stats\")");

        // --- B: "Stats" (index 0) -> the real character sheet. ---
        Check(menu.OnSelect(player, charData) == OptionsMenuAction::None,
              "selecting Stats should not itself be a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Stats"), "selecting Stats should show the real character-sheet info screen");
        std::string expectedSheet = ExpectedCharacterSheet(player, charData);
        std::vector<std::string> expectedSheetLines = MessagePopup::WordWrap(expectedSheet, Screen::width() - 5 - 5);
        Check(TextRenderedAt(bb, 5, 20, expectedSheetLines[0], kItemTextColor),
              "the Stats screen should show the real character sheet's own first (word-wrapped) line");
        Check(menu.OnSelect(player, charData) == OptionsMenuAction::None, "Ok on Stats is not a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Options"), "Ok on Stats should return to Options");

        // --- C: deferred no-ops (Inventory/Skills/Spells/Save/Load/
        // Reveal Traitor -- see OptionsMenu's own header doc comment on
        // why each still needs a whole system this port hasn't built
        // yet). Each iteration forces the selection back to a known
        // index (0) first via repeated OnUp() calls, then steps down to
        // the target index -- same "real Screen instances persist their
        // own selectedIndex across visits rather than resetting"
        // precedent M38's own test already established. ---
        for (int idx : {1, 3, 4, 5, 6, 8}) {
            // Navigate Options back to a known index (0) first, then
            // step down to `idx`.
            for (int i = 0; i < 10; i++) menu.OnUp();
            for (int i = 0; i < idx; i++) menu.OnDown();
            Check(menu.OnSelect(player, charData) == OptionsMenuAction::None,
                  "a deferred Options action should be a real, silent no-op");
            bb.Fill(0);
            menu.Render(bb);
            Check(TitleShownIs(bb, "Options"), "a deferred Options action should leave Options showing");
        }

        // --- D: "Clue Log" (index 2) -> a named suspect's own entry,
        // built from the real ShopDialogue + a scripted eventFlags/
        // traitorIndex scenario (see this file's own ExpectedClueLine
        // derivation above/below). ---
        for (int i = 0; i < 10; i++) menu.OnUp();
        menu.OnDown();
        menu.OnDown();  // index 0 -> 1 -> 2 ("Clue Log")
        Check(menu.OnSelect(player, charData) == OptionsMenuAction::None,
              "selecting Clue Log should not itself be a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Clue Log"), "selecting Clue Log should show the real Clue Log prompt list");
        // marginX_=10 (SetupPromptList), items start at y=20+12(the one
        // empty-string-wrapped prompt line)+5=37 -- same formula
        // m37_screen_smoke.cpp's own prompt-list section already
        // established (RenderPromptList/RenderItemRows).
        Check(TextRenderedAt(bb, 10, 37, "Alhavara", kItemTextColor),
              "the Clue Log's own first item should be the real first suspect name");

        // Scenario 1: idx=0 (Alhavara), NOT the traitor -- always the
        // UNCONFIRMED_A branch. eventFlags[0] (row 0, col 0) set;
        // `col(0) >= idx(0)` so bump=1 on this very iteration ->
        // UNCONFIRMED_A[0*4+0+1] = UNCONFIRMED_A[1] = 19 ->
        // dialogue[9][5+19] = dialogue[9][24].
        player.traitorIndex = 2;
        player.eventFlags[0] = true;
        Check(menu.OnSelect(player, charData) == OptionsMenuAction::None,
              "selecting a Clue Log suspect should not itself be a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Alhavara"), "selecting Alhavara should show an info screen titled \"Alhavara\"");
        std::string expected1 = shopDialogue.groups[9][24] + "\n";
        std::vector<std::string> expected1Lines = MessagePopup::WordWrap(expected1, Screen::width() - 5 - 5);
        Check(TextRenderedAt(bb, 5, 20, expected1Lines[0], kItemTextColor),
              "Alhavara's own clue entry should match the real, independently-recomputed UNCONFIRMED_A lookup");

        // Ok on a Clue Log entry returns to Clue Log, not Options.
        Check(menu.OnSelect(player, charData) == OptionsMenuAction::None, "Ok on a Clue Log entry is not a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Clue Log"), "Ok on a Clue Log entry should return to Clue Log, not Options");

        // --- E: "Rumors" (index 4) -- the generic, unnamed-traitor pool
        // via RUMOR_STRING_OFFSET[traitorIndex]. ---
        player.traitorIndex = 1;
        for (auto& f : player.eventFlags) f = false;
        player.eventFlags[90] = true;  // Rumors reveal-step 0.
        for (int i = 0; i < 10; i++) menu.OnUp();
        for (int i = 0; i < 4; i++) menu.OnDown();  // index 0 -> ... -> 4 ("Rumors")
        Check(menu.OnSelect(player, charData) == OptionsMenuAction::None,
              "selecting Rumors should not itself be a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Rumors"), "selecting Rumors should show an info screen titled \"Rumors\"");
        std::string expected2 = shopDialogue.groups[9][5 + kRumorStringOffset[1][0]] + "\n";
        std::vector<std::string> expected2Lines = MessagePopup::WordWrap(expected2, Screen::width() - 5 - 5);
        Check(TextRenderedAt(bb, 5, 20, expected2Lines[0], kItemTextColor),
              "Rumors should match the real, independently-recomputed RUMOR_STRING_OFFSET lookup");

        // --- F: the traitor's own admission (UNCONFIRMED_B), when
        // idx == the player's real traitorIndex AND the extra
        // eventFlags[72+...] confirmation flag is also set. ---
        Check(menu.OnSelect(player, charData) == OptionsMenuAction::None, "Ok on Rumors returns to Clue Log");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Clue Log"), "Ok on Rumors should return to Clue Log");
        menu.OnCancel();  // Clue Log -> Options
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Options"), "Cancel on Clue Log should return to Options");
        player.traitorIndex = 0;
        for (auto& f : player.eventFlags) f = false;
        player.eventFlags[0] = true;   // row 0, col 0 (base = 18*0 = 0)
        player.eventFlags[72] = true;  // the "traitor's own admission" flag for the same slot
        for (int i = 0; i < 10; i++) menu.OnUp();
        menu.OnDown();
        menu.OnDown();  // Options -> Clue Log
        Check(menu.OnSelect(player, charData) == OptionsMenuAction::None, "re-entering Clue Log should be a no-op action");
        // clueLog_'s own selectedIndex_ is still 4 ("Rumors", from
        // scenario E above) -- real Screen state persisting across
        // visits, same M38 precedent section C's own comment already
        // cites -- so it's forced back to 0 ("Alhavara") first.
        for (int i = 0; i < 10; i++) menu.OnUp();
        Check(menu.OnSelect(player, charData) == OptionsMenuAction::None, "selecting Alhavara again should be a no-op action");
        bb.Fill(0);
        menu.Render(bb);
        // Same (row=0,col=0,bump=1) slot as scenario 1 above, but now
        // isTraitor is true and eventFlags[72] is set, so this reads
        // UNCONFIRMED_B[1] = 43 -> dialogue[9][5+43] = dialogue[9][48],
        // NOT the UNCONFIRMED_A[1]=19 -> dialogue[9][24] scenario 1 used.
        std::string expected3 = shopDialogue.groups[9][48] + "\n";
        std::vector<std::string> expected3Lines = MessagePopup::WordWrap(expected3, Screen::width() - 5 - 5);
        Check(TextRenderedAt(bb, 5, 20, expected3Lines[0], kItemTextColor),
              "the traitor's own suspect entry should use the UNCONFIRMED_B table, not UNCONFIRMED_A");

        // --- G: Ok back to Clue Log, then Cancel on Clue Log returns to
        // Options. ---
        Check(menu.OnSelect(player, charData) == OptionsMenuAction::None, "Ok on the traitor's own entry returns to Clue Log");
        menu.OnCancel();
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Options"), "Cancel on Clue Log should reach Options");

        // --- H: "Help" (index 7) -> the real Help topic list (M8's
        // HelpText) -> a topic's own body -> back to Help (not Options)
        // -> Cancel back to Options. ---
        for (int i = 0; i < 10; i++) menu.OnUp();
        for (int i = 0; i < 7; i++) menu.OnDown();
        Check(menu.OnSelect(player, charData) == OptionsMenuAction::None, "selecting Help should not itself be a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Help"), "selecting Help should show the real Help topic list");
        Check(menu.OnSelect(player, charData) == OptionsMenuAction::None, "selecting a Help topic should not itself be a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, helpText.titles[0]), "the info screen should show the SELECTED topic's own real title");
        Check(menu.OnSelect(player, charData) == OptionsMenuAction::None, "Ok on a Help topic's body is not a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Help"), "Ok on a Help topic's body should return to Help, not Options");
        Check(menu.OnCancel() == OptionsMenuAction::None, "Cancel on the Help topic list is not a main.cpp-level action");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Options"), "Cancel on the Help topic list should return to Options");

        // --- I: "Quit Game" (index 9) -> the real "Are you sure?" quit
        // confirmation, same preserved "either answer exits" bug M38's
        // MenuFlow already established. ---
        for (int i = 0; i < 10; i++) menu.OnUp();
        for (int i = 0; i < 9; i++) menu.OnDown();
        Check(menu.OnSelect(player, charData) == OptionsMenuAction::None, "selecting Quit Game should show a confirmation, not exit immediately");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Quit?"), "selecting Quit Game should show the real quit-confirmation prompt");
        Check(menu.OnCancel() == OptionsMenuAction::None, "Cancel on the quit confirmation should be a real no-op");
        bb.Fill(0);
        menu.Render(bb);
        Check(TitleShownIs(bb, "Quit?"), "Cancel on the quit confirmation should be a real no-op (stays showing)");
        menu.OnDown();  // select "No"
        Check(menu.OnSelect(player, charData) == OptionsMenuAction::Exit,
              "selecting \"No\" on the quit confirmation should STILL exit -- the real, preserved bug");

        // --- J: fresh OptionsMenu, Back on Options returns to the game. ---
        OptionsMenu menu2(helpText, shopDialogue);
        Check(menu2.OnCancel() == OptionsMenuAction::ReturnToGame,
              "Back on a fresh Options list should request returning to the game");

        if (g_ok) {
            std::printf("all options-menu checks passed\n");
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
