#include "ui/character_creation_flow.h"

#include <utility>

#include "player/player_creation.h"

namespace dawnstar {

CharacterCreationFlow::CharacterCreationFlow(const CharacterData& charData, const ItemDatabase& items,
                                              ShopDialogue shopDialogue)
    : charData_(charData),
      items_(items),
      shopDialogue_(std::move(shopDialogue)),
      classSelect_(ScreenMode::PromptList),
      characterMain_(ScreenMode::PromptListWithFooter),
      info_(ScreenMode::PlainList) {
    // `this.newGameUI = new Screen(this, 5, 3); ...setupPromptList("New
    // Game", "Select a Class:", Player.classNames);` -- `newGameUI.
    // backTarget = mainMenuUI` is this class's own `CancelToMainMenu`
    // (see OnCancel below), not a stored `Screen::backTarget` (M37's own
    // deferred field, same reasoning M38/M39 already gave).
    classSelect_.SetupPromptList("New Game", "Select a Class:", charData_.classNames);
    // `this.characterMainUI = new Screen(this, 6, 4); ...setupPromptList(
    // "Character", "You selected:", "", {"See Class Info", "Create
    // Character"});` -- the footer (column 1) starts empty, filled in via
    // SetTextColumn once a class is actually picked (see OnSelect below).
    characterMain_.SetupPromptList("Character", "You selected:", "", {"See Class Info", "Create Character"});
}

void CharacterCreationFlow::OnUp() {
    switch (active_) {
        case Active::ClassSelect:
            classSelect_.MoveSelectionUp();
            break;
        case Active::CharacterMain:
            characterMain_.MoveSelectionUp();
            break;
        case Active::ClassInfo:
        case Active::CreatedPrompt:
        case Active::NameTooShort:
        case Active::Welcome:
        case Active::Intro1:
        case Active::Intro2:
            // GenericInfoUI is a real scrollable mode-4 list -- long
            // text (buildCreationSummary/the intro dialogue) can need
            // this.
            info_.MoveSelectionUp();
            break;
        case Active::NameEntry:
            // A real TextField doesn't respond to Up/Down at all.
            break;
    }
}

void CharacterCreationFlow::OnDown() {
    switch (active_) {
        case Active::ClassSelect:
            classSelect_.MoveSelectionDown();
            break;
        case Active::CharacterMain:
            characterMain_.MoveSelectionDown();
            break;
        case Active::ClassInfo:
        case Active::CreatedPrompt:
        case Active::NameTooShort:
        case Active::Welcome:
        case Active::Intro1:
        case Active::Intro2:
            info_.MoveSelectionDown();
            break;
        case Active::NameEntry:
            break;
    }
}

CharacterCreationAction CharacterCreationFlow::OnSelect() {
    switch (active_) {
        case Active::ClassSelect: {
            // secondaryParam==3's own Select branch: `this.character =
            // new Player(this); this.character.applyClassTemplate(idx);
            // this.characterMainUI.setTextColumn(1, className); this.
            // setCurrentDisplay(this.characterMainUI);` -- the real,
            // live-but-not-yet-`resetState`d preview character, modeled
            // here by `previewPlayer_`/`previewRng_` (see this class's
            // own header doc comment on why the REAL character isn't
            // built yet at this point).
            int idx = classSelect_.SelectedIndexOrMinusOne();
            if (idx < 0 || idx >= charData_.ClassCount()) return CharacterCreationAction::None;
            selectedClassIndex_ = idx;
            previewPlayer_ = PlayerCreation::CreateCharacter(idx, "", charData_, items_, previewRng_);
            characterMain_.SetTextColumn(1, charData_.classNames[static_cast<size_t>(idx)]);
            active_ = Active::CharacterMain;
            return CharacterCreationAction::None;
        }
        case Active::CharacterMain: {
            // secondaryParam==4's own Select branch.
            int idx = characterMain_.SelectedIndexOrMinusOne();
            if (idx == 0) {
                // "See Class Info": `character.buildCreationSummary();
                // GenericInfoUI.setSecondaryParam(5); setupMessage(
                // "Info", summary);`
                info_.SetupMessage("Info", PlayerCreation::BuildCreationSummary(previewPlayer_, charData_));
                active_ = Active::ClassInfo;
            } else {
                // "Create Character": `GenericInfoUI.setSecondaryParam(
                // 6); setupMessage("New Character", "Character Created!
                // ...");`
                info_.SetupMessage("New Character", "Character Created!\n \nPress 'Ok' to enter a name");
                active_ = Active::CreatedPrompt;
            }
            return CharacterCreationAction::None;
        }
        case Active::ClassInfo:
            // secondaryParam==5: `this.setCurrentDisplay(this.
            // characterMainUI);`
            active_ = Active::CharacterMain;
            return CharacterCreationAction::None;
        case Active::CreatedPrompt:
            // secondaryParam==6: `this.setCurrentDisplay(this.
            // charNameTextForm);`
            active_ = Active::NameEntry;
            return CharacterCreationAction::None;
        case Active::NameEntry:
            // The real `charNameTextForm`'s own Ok command: `if
            // (name.length() < 3) { show the Alert; } else { character.
            // name = name; ... (createGameUI's own instant transition,
            // see this class's own doc comment) ... }`
            if (nameEntry_.Text().size() < 3) {
                info_.SetupMessage(
                    "Error", "Your character name must be at least 3 letters");
                active_ = Active::NameTooShort;
            } else {
                info_.SetupMessage("Welcome", "Welcome to The Elder Scrolls Travels!");
                active_ = Active::Welcome;
            }
            return CharacterCreationAction::None;
        case Active::NameTooShort:
            // The Alert's own (and only) real behavior: back to
            // NameEntry to retry.
            active_ = Active::NameEntry;
            return CharacterCreationAction::None;
        case Active::Welcome:
            // secondaryParam==7: `GenericInfoUI.setSecondaryParam(101);
            // setupMessage("Introduction", Shop.dialogue[9][3]);`
            info_.SetupMessage("Introduction", shopDialogue_.groups[9][3]);
            active_ = Active::Intro1;
            return CharacterCreationAction::None;
        case Active::Intro1:
            // secondaryParam==101: `GenericInfoUI.setSecondaryParam(
            // 102); setupMessage("Introduction", dialogue[9][4] +
            // dialogue[9][5]);`
            info_.SetupMessage("Introduction", shopDialogue_.groups[9][4] + shopDialogue_.groups[9][5]);
            active_ = Active::Intro2;
            return CharacterCreationAction::None;
        case Active::Intro2:
            // secondaryParam==102's own Ok branch -- the real end of
            // this whole flow (see this class's own header doc comment
            // and CharacterCreationAction::StartGame's own doc comment).
            return CharacterCreationAction::StartGame;
    }
    return CharacterCreationAction::None;
}

CharacterCreationAction CharacterCreationFlow::OnCancel() {
    switch (active_) {
        case Active::ClassSelect:
            // `newGameUI.backTarget = mainMenuUI` -- main.cpp's own job
            // to actually switch back to MenuFlow.
            return CharacterCreationAction::CancelToMainMenu;
        case Active::CharacterMain:
            // `characterMainUI.backTarget = newGameUI`.
            active_ = Active::ClassSelect;
            return CharacterCreationAction::None;
        case Active::ClassInfo:
        case Active::CreatedPrompt:
        case Active::NameEntry:
        case Active::NameTooShort:
        case Active::Welcome:
        case Active::Intro1:
        case Active::Intro2:
            // None of GenericInfoUI's own uses here, the raw
            // charNameTextForm, or the Alert have a Cancel/Back command
            // at all in the original -- a real no-op.
            return CharacterCreationAction::None;
    }
    return CharacterCreationAction::None;
}

void CharacterCreationFlow::OnChar(char c) {
    if (active_ == Active::NameEntry) nameEntry_.OnChar(c);
}

void CharacterCreationFlow::OnBackspace() {
    if (active_ == Active::NameEntry) nameEntry_.OnBackspace();
}

void CharacterCreationFlow::Render(Backbuffer& bb) const {
    switch (active_) {
        case Active::ClassSelect:
            classSelect_.Paint(bb);
            break;
        case Active::CharacterMain:
            characterMain_.Paint(bb);
            break;
        case Active::ClassInfo:
        case Active::CreatedPrompt:
        case Active::NameTooShort:
        case Active::Welcome:
        case Active::Intro1:
        case Active::Intro2:
            info_.Paint(bb);
            break;
        case Active::NameEntry:
            nameEntry_.Render(bb);
            break;
    }
}

}  // namespace dawnstar
