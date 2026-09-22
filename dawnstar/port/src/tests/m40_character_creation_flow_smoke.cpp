// M40 smoke test: CharacterCreationFlow (ui/character_creation_flow.h)
// -- the real class-selection/"You selected"/name-entry/intro flow,
// built on M37's Screen + the new ui/name_entry.h widget.
//
// No JVM ground truth is available (same reason as every prior
// milestone). Every real target is independently re-derived from
// ../../../src/ESGame.java's own secondaryParam==3/4/5/6/7/101/102
// branches and ../../../src/Player.java's own buildCreationSummary(),
// not read back from character_creation_flow.cpp/player_creation.cpp.
// Text placement is cross-checked against BitmapFont::DrawString used
// as an oracle, the same technique M37/M38/M39's own tests established.
#include <cstdio>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/item_database.h"
#include "assets/shop_dialogue.h"
#include "graphics/backbuffer.h"
#include "graphics/bitmap_font.h"
#include "player/player_combat_stats.h"
#include "player/player_creation.h"
#include "render/message_popup.h"
#include "ui/character_creation_flow.h"
#include "ui/screen.h"
#include "util/java_random.h"

namespace {

using dawnstar::Backbuffer;
namespace BitmapFont = dawnstar::BitmapFont;
using dawnstar::CharacterCreationAction;
using dawnstar::CharacterCreationFlow;
using dawnstar::CharacterData;
using dawnstar::MessagePopup;
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

// Same oracle technique m37_screen_smoke.cpp established -- see its own
// copy of this helper for the full doc comment on why spanW/the pixel-
// select condition below are what they are (M58's real, proportional,
// alpha-blended GDI font).
bool TextRenderedAt(const Backbuffer& bb, int x0, int y0, const std::string& text, uint16_t color) {
    Backbuffer scratch;
    scratch.Fill(0);
    BitmapFont::DrawString(scratch, 0, 0, text, 0xFFFF);
    bool sawOnPixel = false;
    int spanW = BitmapFont::StringWidth(text);
    for (int dy = 0; dy < BitmapFont::kGlyphHeight; dy++) {
        for (int dx = 0; dx < spanW; dx++) {
            if (PixelAt(scratch, dx, dy) != 0xFFFF) continue;
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

// ../../../src/Player.java's own buildCreationSummary(), reconstructed
// independently here (not calling into player_creation.cpp's own
// BuildCreationSummary, which is exactly what's under indirect test via
// CharacterCreationFlow's "See Class Info") using only
// already-independently-verified building blocks
// (PlayerCombatStats::EffectiveStat, M14).
std::string ExpectedCreationSummary(const PlayerState& p, const CharacterData& charData) {
    std::string out = charData.raceNames[static_cast<size_t>(p.raceIndex)] + " " +
                       charData.classNames[static_cast<size_t>(p.classIndex)] + "\n";
    out += charData.statLabels[0] + ": " + std::to_string(p.coreStats[0]) + "\n";
    out += charData.statLabels[2] + ": " + std::to_string(PlayerCombatStats::EffectiveStat(p, charData, 2)) + "\n";
    out += charData.statLabels[4] + ": " + std::to_string(PlayerCombatStats::EffectiveStat(p, charData, 4)) + "\n";
    out += charData.statLabels[6] + ": " + std::to_string(PlayerCombatStats::EffectiveStat(p, charData, 6)) + "\n";
    for (int i = 0; i < 8; i++) {
        int slot = 2 * i;
        out += charData.attributeNames[static_cast<size_t>(slot)] + ": " +
               std::to_string(p.attributes[static_cast<size_t>(slot)]) + "\n";
    }
    for (int i = 0; i < 14; i++) {
        if (p.skills[static_cast<size_t>(i)][0] > 0) {
            out += charData.skillNames[static_cast<size_t>(i)] + ": " +
                   std::to_string(p.skills[static_cast<size_t>(i)][0]) + "\n";
        }
    }
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        CharacterData charData = CharacterData::Load(archive);
        ShopDialogue shopDialogue = ShopDialogue::Load(root + "/npcstrings.dat");

        CharacterCreationFlow flow(charData, items, shopDialogue);
        Backbuffer bb;

        // --- A: initial state is the real class-selection list. ---
        bb.Fill(0);
        flow.Render(bb);
        Check(TitleShownIs(bb, "New Game"), "the flow should start on the real class-selection list");
        Check(TextRenderedAt(bb, 10, 37, charData.classNames[0], kItemTextColor),
              "the class list's own first item should be the real first class name");

        // --- B: selecting class 0 -> "You selected: <class>". ---
        Check(flow.OnSelect() == CharacterCreationAction::None, "selecting a class should not itself be a main.cpp-level action");
        bb.Fill(0);
        flow.Render(bb);
        Check(TitleShownIs(bb, "Character"), "selecting a class should show the real \"Character\" screen");
        Check(flow.SelectedClassIndex() == 0, "SelectedClassIndex should reflect the picked class");

        // --- C: "See Class Info" -> the real creation summary, cross-
        // checked against an independently-reconstructed expectation
        // (a fresh preview PlayerState of the SAME class, built with a
        // DIFFERENT rng seed -- buildCreationSummary reads only
        // deterministic, class-template-derived fields, so the two
        // should match regardless of rng state). ---
        Check(flow.OnSelect() == CharacterCreationAction::None, "\"See Class Info\" should not itself be a main.cpp-level action");
        bb.Fill(0);
        flow.Render(bb);
        Check(TitleShownIs(bb, "Info"), "\"See Class Info\" should show the real creation-summary info screen");
        dawnstar::JavaRandom testRng(999);
        PlayerState testPreview = dawnstar::PlayerCreation::CreateCharacter(0, "", charData, items, testRng);
        std::vector<std::string> expectedSummaryLines =
            MessagePopup::WordWrap(ExpectedCreationSummary(testPreview, charData), Screen::width() - 5 - 5);
        Check(TextRenderedAt(bb, 5, 20, expectedSummaryLines[0], kItemTextColor),
              "the creation-summary screen should match the real, independently-recomputed buildCreationSummary");
        Check(flow.OnSelect() == CharacterCreationAction::None, "Ok on the creation summary is not a main.cpp-level action");
        bb.Fill(0);
        flow.Render(bb);
        Check(TitleShownIs(bb, "Character"), "Ok on the creation summary should return to \"Character\"");

        // --- D: "Create Character" -> "Character Created!" prompt. ---
        flow.OnDown();  // index 0 ("See Class Info") -> 1 ("Create Character")
        Check(flow.OnSelect() == CharacterCreationAction::None, "\"Create Character\" should not itself be a main.cpp-level action");
        bb.Fill(0);
        flow.Render(bb);
        Check(TitleShownIs(bb, "New Character"), "\"Create Character\" should show the real \"Character Created!\" prompt");

        // --- E: Ok -> the real name-entry widget. ---
        Check(flow.OnSelect() == CharacterCreationAction::None, "Ok on \"Character Created!\" is not a main.cpp-level action");
        bb.Fill(0);
        flow.Render(bb);
        Check(flow.EnteredName().empty(), "the name-entry widget should start empty");

        // --- F: an unsupported character and a too-long run are both
        // real, silent no-ops/truncations (see NameEntry's own header
        // doc comment). ---
        flow.OnChar('@');  // unsupported -- ignored
        flow.OnChar('a');  // lowercase 'a' is NOT in NameEntry's own
                            // accepted set (only 'A'-'Z') -- ignored;
                            // main.cpp only ever calls OnChar with an
                            // already-uppercase key character anyway.
        Check(flow.EnteredName().empty(), "an unsupported character should be silently ignored");
        for (char c : std::string("ABCDEFGHIJKL")) flow.OnChar(c);  // 12 chars, only 10 fit
        Check(flow.EnteredName() == "ABCDEFGHIJ", "typed text should truncate at the real TextField's own 10-char max");

        // --- G: a too-short name shows the real length-error Alert,
        // then Ok returns to name entry with the text preserved (the
        // real TextField is never cleared on this error). ---
        CharacterCreationFlow flow2(charData, items, shopDialogue);
        flow2.OnSelect();  // ClassSelect -> CharacterMain
        flow2.OnDown();
        flow2.OnSelect();  // -> "Character Created!"
        flow2.OnSelect();  // -> NameEntry
        flow2.OnChar('A');
        flow2.OnChar('B');
        Check(flow2.OnSelect() == CharacterCreationAction::None, "submitting a too-short name is not a main.cpp-level action");
        bb.Fill(0);
        flow2.Render(bb);
        Check(TitleShownIs(bb, "Error"), "a too-short name should show the real length-error message");
        Check(flow2.OnSelect() == CharacterCreationAction::None, "Ok on the length error is not a main.cpp-level action");
        bb.Fill(0);
        flow2.Render(bb);
        Check(flow2.EnteredName() == "AB", "the too-short name should be preserved (a real TextField isn't cleared on error)");
        flow2.OnBackspace();  // "AB" -> "A"
        Check(flow2.EnteredName() == "A", "Backspace should remove the last character");
        flow2.OnBackspace();
        flow2.OnBackspace();  // already empty -- a real no-op
        Check(flow2.EnteredName().empty(), "Backspace on an empty name should be a real no-op");
        flow2.OnChar('B');
        flow2.OnChar('O');
        flow2.OnChar('B');

        // --- H: a valid name -> Welcome -> Introduction -> a second
        // Introduction screen -> StartGame. ---
        Check(flow2.OnSelect() == CharacterCreationAction::None, "submitting a valid name should not itself be a main.cpp-level action");
        bb.Fill(0);
        flow2.Render(bb);
        Check(TitleShownIs(bb, "Welcome"), "a valid name should show the real \"Welcome\" screen");
        Check(flow2.OnSelect() == CharacterCreationAction::None, "Ok on Welcome is not a main.cpp-level action");
        bb.Fill(0);
        flow2.Render(bb);
        Check(TitleShownIs(bb, "Introduction"), "Ok on Welcome should show the real first Introduction screen");
        std::vector<std::string> expectedIntro1 =
            MessagePopup::WordWrap(shopDialogue.groups[9][3], Screen::width() - 5 - 5);
        Check(TextRenderedAt(bb, 5, 20, expectedIntro1[0], kItemTextColor),
              "the first Introduction screen should match the real npcstrings.dat dialogue[9][3]");
        Check(flow2.OnSelect() == CharacterCreationAction::None, "Ok on the first Introduction screen is not a main.cpp-level action");
        bb.Fill(0);
        flow2.Render(bb);
        std::string expectedIntro2Text = shopDialogue.groups[9][4] + shopDialogue.groups[9][5];
        std::vector<std::string> expectedIntro2 = MessagePopup::WordWrap(expectedIntro2Text, Screen::width() - 5 - 5);
        Check(TextRenderedAt(bb, 5, 20, expectedIntro2[0], kItemTextColor),
              "the second Introduction screen should match dialogue[9][4]+dialogue[9][5]");
        Check(flow2.OnSelect() == CharacterCreationAction::StartGame,
              "Ok on the second Introduction screen should be the real end of character creation");
        Check(flow2.EnteredName() == "BOB", "EnteredName should reflect exactly what was typed");
        Check(flow2.SelectedClassIndex() == 0, "SelectedClassIndex should still reflect the picked class");

        // --- I: Cancel on \"Character\" returns to class selection;
        // Cancel on class selection itself requests the real menu. ---
        CharacterCreationFlow flow3(charData, items, shopDialogue);
        flow3.OnSelect();
        Check(flow3.OnCancel() == CharacterCreationAction::None, "Cancel on \"Character\" is not a main.cpp-level action");
        bb.Fill(0);
        flow3.Render(bb);
        Check(TitleShownIs(bb, "New Game"), "Cancel on \"Character\" should return to class selection");
        Check(flow3.OnCancel() == CharacterCreationAction::CancelToMainMenu,
              "Cancel on class selection should request returning to the main menu");

        if (g_ok) {
            std::printf("all character-creation-flow checks passed\n");
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
