#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "assets/asset_root.h"
#include "assets/lang.h"
#include "graphics/backbuffer.h"
#include "script/interpreter.h"
#include "world/actor.h"
#include "world/level_view.h"

namespace oblivion {

// The playing field of Game: the level's actors, trigger layers, camera,
// dialogue/message state and the state number, driven by the script VM
// (which it hosts). Menus, HUD, combat and the dungeon generator are later
// milestones; whatever the scripts ask of them is recorded in
// unimplemented().
class World : public ScriptHost {
public:
    static constexpr int kMaxActors = 25;

    World(const AssetRoot& assets, ImageCache& images);

    // Game.loadLevel: resets the field and runs the level script from
    // script 1 (which loads the map and spawns everything).
    void LoadLevel(const std::string& scrPath) override;

    // The boot sequence: /startup.scr (splash screens, and the big item /
    // monster / class tables every level relies on) then /startup2.scr, which
    // opens the main menu.
    void Boot() { LoadLevel("/startup.scr"); }

    // One Game.run iteration: script, actors, triggers, messages.
    void Tick(int dtMs);
    // A movement / fire action held this frame (3 up, 4 down, 5 left, 6 right, 7 fire),
    // Game.handleInput case 0. 0 = none.
    void HeldAction(int action, int dtMs);
    // A key press (script key hooks, WAIT_KEY, dialogue dismissal).
    void KeyPressed(int action);

    void Draw(Backbuffer& bb);

    int state() const { return state_; }
    Actor* player() { return player_.get(); }
    const std::string& currentLevel() const { return currentLevel_; }
    // The text a HUD would show right now (dialogue, message or text screen).
    std::string Caption() const;
    const std::map<std::string, int>& unimplemented() const { return unimplemented_; }
    const LevelView& view() const { return view_; }
    ScriptInterpreter& script() { return script_; }
    bool inputEnabled() const { return inputEnabled_; }

    // ---- ScriptHost ----
    Actor* ActorAt(int slot) override;
    bool DialogueOpen() const override { return dialogueOpen_; }
    bool ShopOpen() const override { return state_ == 1; }
    void LoadMap(const std::string& jtm, const std::string& tileCml) override;
    void EndLevel(int kind, int arg) override;
    void SetScreenSize(int w, int h) override;
    void SetState(int state) override;
    void SetStateChangesEnabled(bool on) override { stateChangesEnabled_ = on; }
    void SetPlayerCollides(bool on) override { playerCollides_ = on; }
    void SetBackgroundColor(int rgb) override { background_ = rgb; }
    void SetHudVisible(bool on) override { hudVisible_ = on; }
    void SetInputEnabled(bool on) override { inputEnabled_ = on; }
    void SetRespawnPoint(int x, int y) override { respawn_[0] = x; respawn_[1] = y; }
    void SetTrigger(int x, int y, int enter, int leave, int zone) override;
    void SetTile(int x, int y, int layer, int tile) override {
        view_.SetTile(x, y, layer, tile);
        redraw_ = true;
    }
    void SetCollision(int x, int y, bool solid) override { view_.SetCollision(x, y, solid); }
    void ClearLayers() override;
    void SpawnActor(const std::string& name, int slot, const std::string& cml, const int* monsterRow, int x,
                    int y) override;
    void RemoveActor(int slot) override;
    void CameraTo(int x, int y) override;
    void CameraFollow(int slot) override;
    void ShowDialogue(const std::string& text) override;
    void ShowMessage(const std::string& text, int seconds, int color, int style) override;
    void HideMessage() override { message_.clear(); }
    void ShowTextScreen(const std::string& text) override;
    std::string GetString(int id) override { return strings_.Get(id); }
    void LoadLang(int packIndex) override;
    void LoadHudSprites(const std::string& cml) override;
    void EvictSprites(const std::string& prefix) override { images_.Evict(prefix); }
    void SetSpeaker(const std::string* name) override;
    void Unimplemented(const char* what) override { unimplemented_[what]++; }

private:
    const SpriteSet& SpritesFor(const std::string& cml);
    void UpdateCamera();
    void CheckPlayerTriggers(Actor& p, int8_t oldEnter, int8_t oldLeave);
    int8_t ZoneUnder(const Actor& a) const;
    void ResizeLayers();

    const AssetRoot& assets_;
    ImageCache& images_;
    LevelView view_;
    ScriptInterpreter script_;
    Strings strings_;
    std::map<std::string, SpriteSet> spriteCache_;
    SpriteSet hudSprites_;

    std::shared_ptr<Actor> actors_[kMaxActors];
    std::shared_ptr<Actor> player_;  // persists across levels (Game.player)
    int maxActorSlot_ = 0;
    int cameraActor_ = -1;
    bool redraw_ = true;  // Game.redraw: makes updateCamera recentre
    int playerClass_ = 1;  // menuSelection[1] + 1 in the original

    std::vector<int8_t> enterLayer_, leaveLayer_, zoneLayer_;

    int state_ = 6;
    bool stateChangesEnabled_ = true;
    bool inputEnabled_ = true;
    bool playerCollides_ = true;
    bool hudVisible_ = true;
    int background_ = 0;
    int respawn_[2] = {0, 0};
    std::string currentLevel_;

    bool dialogueOpen_ = false;
    int dialogueAgeMs_ = 0;
    std::string dialogueText_, speaker_;
    bool hasSpeaker_ = false;

    std::string message_;
    int messageDurationMs_ = 0, messageElapsedMs_ = 0, messageColor_ = 0, messageStyle_ = 0;

    // Text screens (states 4, 9, 10, 21...) scroll in the original; until the
    // fonts land they simply run a timer.
    std::string textScreen_;
    int textTimerMs_ = 0;

    std::map<std::string, int> unimplemented_;
};

}  // namespace oblivion
