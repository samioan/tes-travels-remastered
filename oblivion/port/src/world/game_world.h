#pragma once
#include <functional>
#include <map>
#include <array>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "assets/asset_root.h"
#include "assets/lang.h"
#include "graphics/backbuffer.h"
#include "script/interpreter.h"
#include "world/actor.h"
#include "world/combat.h"
#include "world/level_view.h"
#include "world/projectiles.h"

namespace oblivion {

// The playing field of Game: the level's actors, trigger layers, camera,
// dialogue/message state and the state number, driven by the script VM
// (which it hosts). Menus, HUD, combat and the dungeon generator are later
// milestones; whatever the scripts ask of them is recorded in
// unimplemented().
class World : public ScriptHost, public CombatHost {
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
    // The screen width (>= 176) the camera centres on; widescreen shows more of the world.
    void SetScreenWidth(int w) {
        if (w == screenW_) return;
        screenW_ = w;
        view_.SetScreenWidth(w);
        redraw_ = true;
    }
    int screenWidth() const { return screenW_; }
    // ---- modern controls (screen-relative free movement) ----
    // Move the player by a screen-space stick vector (+x right, +y down, length
    // 0..1): straight up the screen is the world diagonal, so all eight
    // directions walk at the same speed, sliding along walls.
    void MoveAnalog(float screenX, float screenY, int dtMs);
    // The direction (screen space) the player is aiming; `valid` false clears it.
    void SetAim(bool valid, float screenX, float screenY);
    // The attack button: zone scripts under the player, else melee / special / arrow.
    void Attack();
    // The interact button: zone scripts and picking items up, never an attack.
    void Interact();
    // The player's body on the screen (for mouse aiming); false without a player.
    bool PlayerScreenPos(int* x, int* y) const;
    // A key press (script key hooks, WAIT_KEY, dialogue dismissal).
    void KeyPressed(int action);

    // The map with its actors (state 0 backdrop); the HUD is GameApp's.
    void DrawField(Backbuffer& bb);

    // Game.dialogue* fields: the script dialogue box (SAY / TALK).
    struct Dialogue {
        bool open = false;
        int ageMs = 0;
        std::vector<std::string> lines;
        int scroll = -1;
        bool atEnd = true;  // set by the painter (Game.drawDialogue)
        int left = 12, top = 7, right = 0, textWidth = 0, height = 0;
    };
    // Game.message*: the one-line message strip above the bottom edge.
    struct Message {
        std::string text;  // empty = none
        int durationMs = 0, elapsedMs = 0, color = 0, style = 0;
        int x = -1, y = -1, blinkTimerMs = 0, scrollTimerMs = 0;
        bool blank = false;
    };
    Dialogue dialogue;
    int gold = 100;  // Game.gold
    bool scriptPaused = false;  // an inventory/shop screen is up: the VM does not tick
    int cutsceneSprite = 0, cutsceneColor = 0;  // END_LEVEL: splash / cutscene image
    std::function<void()> onLoadLevel;
    std::function<void(int)> onOpenMenu;  // 0 = main/pause menu, 4 = shop menu
    const std::string* speaker() const { return hasSpeaker_ ? &speaker_ : nullptr; }  // Game.loadLevel start (text scroll reset)
    Message message;
    // Text of the last SHOW_TEXT_SCREEN.
    std::string textScreenText;
    // Called after every accepted state change (old, new): the UI sets up
    // the per-state screens (Game.setState's tail).
    std::function<void(int, int)> onStateChange;

    int state() const { return state_; }
    bool hudVisible() const { return hudVisible_; }
    int backgroundColor() const { return background_; }
    const SpriteSet& hudSprites() const { return hudSprites_; }
    const SpriteSet& tileSprites() const { return view_.tiles(); }
    ImageCache& images() { return images_; }
    const SpriteSet& SpritesFor(const std::string& cml);
    const Strings& strings() const { return strings_; }
    // Game.wrapText: word-wraps `text` into lines no wider than `width`
    // (SmallBold), prefixing/learning the speaker name like the original.
    std::vector<std::string> WrapText(std::string text, int width);
    // Actor slot by index for HUD/camera code.
    int cameraActor() const { return cameraActor_; }
    int playerClass() const { return playerClass_; }
    void SetPlayerClass(int c) { playerClass_ = c; }
    // Drops the player (a new game starts with no player object).
    void ForgetPlayer() {
        player_.reset();
        for (auto& a : actors_) a.reset();
    }
    // Drops every actor from the table but keeps the player object (end of the epilogue text).
    void ClearActors() {
        for (auto& a : actors_) a.reset();
        maxActorSlot_ = 0;
    }
    Actor* player() { return player_.get(); }
    // ActorSystem.serialize / fromRecord: the player as stored in the save record
    // (gold included). Restore returns false on a truncated record.
    void SerializePlayer(std::vector<uint8_t>& out) const;
    bool RestorePlayer(const std::vector<uint8_t>& data, size_t offset);
    const std::string& currentLevel() const { return currentLevel_; }
    const std::map<std::string, int>& unimplemented() const { return unimplemented_; }
    const LevelView& view() const { return view_; }
    int pickupCount() const { return static_cast<int>(pickups_.size()); }
    ScriptInterpreter& script() { return script_; }
    bool inputEnabled() const { return inputEnabled_; }

    // ---- ScriptHost ----
    Actor* ActorAt(int slot) override;
    bool DialogueOpen() const override { return dialogue.open; }
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
    void HideMessage() override { message = Message{}; }
    void ShowTextScreen(const std::string& text) override;
    std::string GetString(int id) override { return strings_.Get(id); }
    int StringId(const std::string& text) override { return strings_.IdOf(text); }
    void LoadLang(int packIndex) override;
    void LoadHudSprites(const std::string& cml) override;
    void EvictSprites(const std::string& prefix) override { images_.Evict(prefix); }
    void SetSpeaker(const std::string* name) override;
    void OpenMenu(bool shop) override {
        if (onOpenMenu) onOpenMenu(shop ? 4 : 0);
        else SetState(3);
    }
    void PlaceItem(int itemId, int cellX, int cellY) override { SpawnItem(itemId, true, cellX, cellY); }
    void SpawnProjectile(int type, int x, int y, int durationMs) override {
        projectiles_.SpawnFixed(type, x, y, durationMs);
    }
    void ClearProjectileAt(int x, int y) override { projectiles_.ClearAt(x, y); }
    void LevelUpTo(int slot, int level) override {
        if (Actor* a = ActorAt(slot)) Combat::LevelUpTo(*a, level, script_);
    }
    void GenerateDungeon(const int* group, const int* spawnIds, int zone, int exitScript) override;
    void Unimplemented(const char* what) override { unimplemented_[what]++; }

    // ---- CombatHost ----
    Actor* ActorSlot(int index) override { return ActorAt(index); }
    int SlotCount() const override { return kMaxActors; }
    int Random() override { return static_cast<int32_t>(rng_()); }
    ScriptInterpreter& Script() override { return script_; }
    Projectiles& Fx() override { return projectiles_; }
    void RemoveActorSlot(int index) override { RemoveActor(index); }
    Actor* SpawnFreeActor(const std::string& cml, int x, int y, const int* row) override;
    void SpawnItem(int itemId, bool fromScript, int cellX, int cellY) override;
    Grid CurrentGrid() override { return view_.grid(); }
    const std::vector<uint8_t>* GroundLayer() override { return view_.Ground(); }

private:
    void UpdateCamera();
    void HandleDialogueKey(int action);
    void UpdateMessage(int dt);
    void CheckPlayerTriggers(Actor& p, int8_t oldEnter, int8_t oldLeave);
    int8_t ZoneUnder(const Actor& a) const;
    void ResizeLayers();
    void ReviveActor(Actor& a);
    void UpdatePickupPrompt();
    void TryPickup(Actor& p);
    bool PlayerCanAct() const;
    void RunZoneScript(int oldZone);

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
    int screenW_ = Backbuffer::kWidth;
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

    std::string speaker_;
    bool hasSpeaker_ = false;

    std::map<std::string, int> unimplemented_;

    Projectiles projectiles_;
    std::mt19937 rng_{0x0B11}; 
    // Game.pickups: items lying on the map (cell x, cell y, table6 row), at most 24.
    std::vector<std::array<uint8_t, 3>> pickups_;
};

}  // namespace oblivion
