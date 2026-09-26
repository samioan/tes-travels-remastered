#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "assets/asset_root.h"
#include "assets/image.h"
#include "graphics/backbuffer.h"
#include "game/analog_input.h"
#include "game/dialogue_screen.h"
#include "game/help_pages.h"
#include "world/game_world.h"

namespace oblivion {

// Keys as the game sees them after Game.mapKey (the phone keypad).
enum class Key {
    None,
    Up,         // action 3
    Down,       // action 4
    Left,       // action 5
    Right,      // action 6
    Fire,       // action 7
    SoftLeft,   // keyLeftSoft
    SoftRight,  // keyRightSoft
    Char,       // a keypad character key ('0'-'9', '*', '#'); the code says which
    Other,      // any other key: still releases WAIT_KEY
};

// The Game class of the original minus the playing field (World): the state
// machine, menus, text screens, HUD and per-state painting. States (see
// src/Game.java): 0 playing, 1 shop, 2 inventory, 3 menu, 4 about text,
// 5 controls, 6/7 loading, 8 splash/cutscene image, 9 intro text, 10 level
// text, 11 continue?, 12 exit, 13 saved, 14 load prompt, 15 please wait,
// 16 overwrite, 17 help text, 18 help pages, 19 quit?, 20 key taken,
// 21 credits scroll, 22 pause, 23 overview text.
class GameApp {
public:
    GameApp(const AssetRoot& assets, ImageCache& images);

    // Runs the startup scripts until the main menu opens (or, with `level`,
    // straight into that level like New Game does).
    void Start();
    void StartLevel(const std::string& scrPath);
    // Boots and fast-forwards the splash screens to the main menu (for tests / --keys).
    void StartMenu();

    // Where the save record lives (the original's RecordStore "ESO", record 1).
    // Empty (the default) disables persistence, which keeps tests off real saves.
    void SetSavePath(std::string path) { savePath_ = std::move(path); }
    bool HasSavedGame() const;
    // Game.saveGame / loadGame: key bindings, sound flag, current level and the
    // player. loadGame(false) (start-up) restores settings and player only.
    void SaveGame();
    void LoadGame(bool enterLevel);

    void OnKeyDown(Key key, int code = 0);
    // The key currently held (movement, text scrolling); Key::None if none.
    void SetHeldKey(Key key, int code = 0) {
        held_ = key;
        heldCode_ = code;
    }
    // Modern controls: this frame's walking / aiming / attack (see AnalogInput).
    // Only acts while playing; menus and dialogue use OnKeyDown events instead.
    void SetAnalogInput(const AnalogInput& in) {
        // `interact` is an edge: keep it until a Tick has consumed it.
        const bool pending = analog_.interact;
        analog_ = in;
        analog_.interact = in.interact || pending;
    }
    // Quick-use actions from any device: 0 health potion, 1 magicka potion, 2 toggle weapon/spell.
    void OnQuick(int action);
    // True when the player walks freely (playing, no dialogue, input enabled).
    bool AnalogMode() const { return world_.state() == 0 && !world_.dialogue.open && world_.inputEnabled(); }
    void Tick(int dtMs);
    void Draw(Backbuffer& bb);

    bool quit() const { return world_.state() == 12; }
    World& world() { return world_; }
    const World& world() const { return world_; }

private:

    // -- state machine (Game.setState tail, handleInput, run) --
    void OnStateChange(int oldState, int newState);
    void HandleKey(Key key, int code);
    int MapKey(Key key, int code) const;  // Game.mapKey: 0..2 quick keys, 3..7 directions/fire, -1 none
    void HandleMenuKey(Key key);
    void HandleControlsKey(Key key, int code);
    void HandleHelpKey(Key key);
    void OpenHelp(int titleId);
    void OpenInventory();
    void OpenShop();
    void MenuSelected(DialogueNode& node);
    void ActivateMenuItem();
    void BuildMenus();
    void StartNewGame();
    std::vector<uint8_t> ReadSave() const;
    void ResetMenu();
    void SetTextScreen(const std::string& text);
    std::string S(int id) const { return world_.strings().Get(id); }
    std::string StartLine(int index) const;

    // -- painting --
    void DrawHud(Backbuffer& bb);
    void DrawDialogue(Backbuffer& bb);
    void DrawMessage(Backbuffer& bb);
    void DrawMenu(Backbuffer& bb);
    void DrawTextScreen(Backbuffer& bb);
    void DrawControls(Backbuffer& bb);
    void DrawHelpPage(Backbuffer& bb);
    void DrawPrompt(Backbuffer& bb, const std::string& text, bool twoSoftKeys, bool large = true);

    const AssetRoot& assets_;
    ImageCache& images_;
    World world_;
    AnalogInput analog_;
    Key held_ = Key::None;
    int heldCode_ = 0;

    // menus
    std::vector<std::vector<std::string>> menus_;
    int menuId_ = -1;
    int menuSelection_[7] = {};
    int menuReturn_ = 0;
    bool stateFlagF_ = false;  // true once the player has left the playing state
    std::shared_ptr<Image> titleImage_;
    bool newGameLocked_ = false;
    std::string savePath_;

    // text screens
    std::vector<std::vector<std::string>> textLines_;
    int textScrollY_ = 0, textScrollTimer_ = 0;
    bool textAtEnd_ = false;
    int textEndWaitMs_ = -1;  // >= 0 once the text has scrolled off (Thread.sleep(3000) in the original)
    int helpTitleId_ = 0;
    HelpPages helpPages_;
    int helpPage_ = 0, helpScroll_ = 0;
    bool helpHasMore_ = false;

    // controls screen (state 5)
    int keyBindings_[3] = {55, 57, 51};  // quick health, quick mana, toggle weapon: '7', '9', '3'
    int keyBindingsEdit_[3] = {};
    std::vector<std::string> optionLabels_;
    int optionsCursor_ = 0;
    bool editingKey_ = false;
    std::string KeyLabel(int code) const;

    // misc timers
    int loadProgress_ = -1;
    int blinkTimer_ = -1;
    bool blinkOn_ = true;
    int spinnerTimer_ = 0;
    std::unique_ptr<DialogueScreen> dialogue_;  // inventory / shop screen
    DialogueNode* equippedWeaponNode_ = nullptr;
    DialogueNode* equippedSpellNode_ = nullptr;
    SpriteSet playerSprites_;  // /oh_pc.cml: group 5 is the "please wait" spinner
};

}  // namespace oblivion
