#pragma once
#include <string>
#include <vector>

#include "assets/asset_root.h"
#include "assets/image.h"
#include "graphics/backbuffer.h"
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
    Quick0,     // quick-use health potion (binding 0)
    Quick1,     // quick-use mana potion (binding 1)
    Quick2,     // toggle weapon (binding 2)
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

    void OnKeyDown(Key key);
    // The key currently held (movement, text scrolling); Key::None if none.
    void SetHeldKey(Key key) { held_ = key; }
    void Tick(int dtMs);
    void Draw(Backbuffer& bb);

    bool quit() const { return world_.state() == 12; }
    World& world() { return world_; }
    const World& world() const { return world_; }

private:

    // -- state machine (Game.setState tail, handleInput, run) --
    void OnStateChange(int oldState, int newState);
    void HandleKey(Key key);
    void HandleMenuKey(Key key);
    void HandleTextKey(Key key);
    void ActivateMenuItem();
    void BuildMenus();
    void StartNewGame();
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
    void DrawPrompt(Backbuffer& bb, const std::string& text, bool twoSoftKeys, bool large = true);

    const AssetRoot& assets_;
    ImageCache& images_;
    World world_;
    Key held_ = Key::None;

    // menus
    std::vector<std::vector<std::string>> menus_;
    int menuId_ = -1;
    int menuSelection_[7] = {};
    int menuReturn_ = 0;
    bool stateFlagF_ = false;  // true once the player has left the playing state
    std::shared_ptr<Image> titleImage_;
    bool newGameLocked_ = false;

    // text screens
    std::vector<std::vector<std::string>> textLines_;
    int textScrollY_ = 0, textScrollTimer_ = 0;
    bool textAtEnd_ = false;
    int textEndWaitMs_ = -1;  // >= 0 once the text has scrolled off (Thread.sleep(3000) in the original)
    int helpTitleId_ = 0;

    // misc timers
    int loadProgress_ = -1;
    int blinkTimer_ = -1;
    bool blinkOn_ = true;
    int spinnerTimer_ = 0;
    SpriteSet playerSprites_;  // /oh_pc.cml: group 5 is the "please wait" spinner
};

}  // namespace oblivion
