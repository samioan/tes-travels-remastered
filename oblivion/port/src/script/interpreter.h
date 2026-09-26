#pragma once
#include <string>
#include <vector>

#include "assets/scr.h"
#include "world/actor.h"

namespace oblivion {

// What the script VM needs from the game (the Game / ActorSystem calls made by
// ScriptInterpreter.step). World implements it; operations whose subsystem is
// not ported yet have a default that records the opcode name via
// Unimplemented(), so a run reports exactly what M6/M7 still owe.
class ScriptHost {
public:
    virtual ~ScriptHost() = default;

    virtual Actor* ActorAt(int slot) = 0;
    virtual bool DialogueOpen() const = 0;
    virtual bool ShopOpen() const = 0;  // Game.isShopState()

    virtual void LoadMap(const std::string& jtm, const std::string& tileCml) = 0;
    virtual void LoadLevel(const std::string& scr) = 0;
    virtual void EndLevel(int kind, int arg) = 0;
    virtual void SetScreenSize(int w, int h) = 0;
    virtual void SetState(int state) = 0;
    virtual void SetStateChangesEnabled(bool on) = 0;
    virtual void SetPlayerCollides(bool on) = 0;
    virtual void SetBackgroundColor(int rgb) = 0;
    virtual void SetHudVisible(bool on) = 0;
    virtual void SetInputEnabled(bool on) = 0;
    virtual void SetRespawnPoint(int x, int y) = 0;

    virtual void SetTrigger(int x, int y, int enter, int leave, int zone) = 0;
    virtual void SetTile(int x, int y, int layer, int tile) = 0;
    virtual void SetCollision(int x, int y, bool solid) = 0;
    virtual void ClearLayers() = 0;

    // `monsterRow` is the monster-type table row (21 ints); `cml` its sprite path.
    virtual void SpawnActor(const std::string& name, int slot, const std::string& cml, const int* monsterRow,
                            int x, int y) = 0;
    virtual void RemoveActor(int slot) = 0;
    virtual void CameraTo(int x, int y) = 0;
    virtual void CameraFollow(int slot) = 0;

    virtual void ShowDialogue(const std::string& text) = 0;  // showDialogue + beginDialogue
    virtual void ShowMessage(const std::string& text, int seconds, int color, int style) = 0;
    virtual void HideMessage() = 0;
    virtual void ShowTextScreen(const std::string& text) = 0;
    virtual std::string GetString(int id) = 0;
    virtual int StringId(const std::string& text) = 0;  // Game.stringToId
    virtual void LoadLang(int packIndex) = 0;
    virtual void LoadHudSprites(const std::string& cml) = 0;
    virtual void EvictSprites(const std::string& prefix) = 0;
    virtual void SetSpeaker(const std::string* name) = 0;
    // OPEN_MENU (main/pause menu) and OPEN_SHOP_MENU (between-level menu).
    virtual void OpenMenu(bool shop) = 0;

    // Combat-side ops.
    virtual void PlaceItem(int itemId, int cellX, int cellY) = 0;                 // PLACE_ITEM
    virtual void SpawnProjectile(int type, int x, int y, int durationMs) = 0;     // SPAWN_(TIMED_)PROJECTILE
    virtual void ClearProjectileAt(int x, int y) = 0;
    virtual void LevelUpTo(int slot, int level) = 0;
    // GENERATE_DUNGEON: `group` is the spawn-group row, `spawnIds` the level's item ids.
    virtual void GenerateDungeon(const int* group, const int* spawnIds, int zone, int exitScript) = 0;

    // Subsystems that land in later milestones (default: record + ignore).
    virtual void Unimplemented(const char* what) = 0;
};

// ScriptInterpreter (src/ScriptInterpreter.java): a call stack of program
// counters over the level's bytecode. One instruction per Tick, exactly like
// the original (waits, dialogue and walk cutscenes block the stream).
class ScriptInterpreter {
public:
    explicit ScriptInterpreter(ScriptHost& host) : host_(host) { Reset(); }

    // ScriptInterpreter.load: merges the file's table rows over the current
    // tables, takes its bytecode, and runs script 1.
    void Load(Scr scr);

    // runScript(id): pushes a call frame (ignored when the stack is full).
    void RunScript(int id);
    void Tick(int dtMs);
    // keyPressed: `action` is the mapped game action (3 up, 4 down, 5 left,
    // 6 right, 7 fire); unblocks WAIT_KEY or fires a key-hook script.
    void KeyPressed(int action);

    bool waitingForKey() const { return waitingForKey_; }
    bool Running() const { return depth_ > 0; }

    // Tables (public in the original too).
    ScrTables& tables() { return t_; }
    const ScrTables& tables() const { return t_; }
    // getRow(table, index) for the record tables; nullptr when out of range.
    const int* GetRow(int table, int index) const;
    // A table string field: literal index or 0xF000|id, as getItemName.
    std::string ItemName(int field);
    // findString / findByName / itemCategory / classAllows of the original.
    int FindString(const std::string& text);
    const int* FindByName(const std::string& text);
    int ItemCategory(const std::string& text);  // 0 weapon, 2 consumable, 1 armour, -1 none
    bool ClassAllows(int classId, int type) const;
    // rollLoot: the first loot row whose odds divide `rnd` and that has stock left
    // (uses one up); returns the pickup id or 0.
    int RollLoot(int rnd);

private:
    void Reset();
    void Step(int dtMs);
    std::string Str(const ScrString& s);

    ScriptHost& host_;
    ScrTables t_;
    int monsterBackup_[25][21] = {};
    bool firstLoad_ = true;
    Scr scr_;  // bytecode + script offsets of the loaded level (its tables live in t_)

    int pcStack_[10];
    int scriptStack_[10];
    int keyHooks_[8];  // [3..7]: script id or -1
    int depth_ = 0;
    int waitElapsed_ = 0, waitDuration_ = -1;
    std::vector<int> waitActors_;
    bool hasWaitActors_ = false;
    bool hasWalk_ = false;
    int walkTarget_[2] = {0, 0};
    int walkActor_ = 0, walkAxis_ = 0, walkPhase_ = 0;
    int talkActor_ = -1;
    bool waitingForKey_ = false;
};

}  // namespace oblivion
