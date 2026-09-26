#include "script/interpreter.h"

#include <cstdio>
#include <cstring>
#include <stdexcept>

#include "world/items.h"

namespace oblivion {

namespace {

// ScriptInterpreter.archetypeGrowth: per-level attribute growth of a monster
// type, indexed by monsterTypes[t][17].
const int kArchetypeGrowth[3][7] = {{3, 1, 1, 2, 0, 1, 1}, {2, 2, 2, 1, 0, 1, 1}, {1, 3, 3, 1, 0, 2, 3}};

template <size_t Rows, size_t Cols>
void MergeRows(int (&dst)[Rows][Cols], const int (&src)[Rows][Cols], const bool* has) {
    for (size_t r = 0; r < Rows; r++)
        if (has[r]) std::memcpy(dst[r], src[r], sizeof(int) * Cols);
}

}  // namespace

void ScriptInterpreter::Reset() {
    for (int i = 3; i <= 7; i++) keyHooks_[i] = -1;
    keyHooks_[0] = keyHooks_[1] = keyHooks_[2] = -1;
    t_.pairTable[0] = -1;
    depth_ = 0;
    waitElapsed_ = 0;
    waitDuration_ = -1;
    waitActors_.clear();
    hasWaitActors_ = false;
    hasWalk_ = false;
    talkActor_ = -1;
    waitingForKey_ = false;
    scr_.code.clear();
    t_.spawnIdCount = 0;
    std::memset(t_.spawnIds, 0, sizeof(t_.spawnIds));
    for (int i = 0; i < 10; i++) pcStack_[i] = scriptStack_[i] = 0;
    for (int& o : scr_.scriptOffset) o = -1;
    std::memset(t_.loot, 0, sizeof(t_.loot));
}

void ScriptInterpreter::Load(Scr scr) {
    Reset();
    const ScrTables& n = scr.tables;
    // The strings[] array persists across loads and only the counter restarts,
    // so a level's strings overwrite from index 0 and older rows (e.g. the
    // monster types of startup.scr) keep pointing at what they loaded.
    if (t_.strings.size() < n.strings.size()) t_.strings.resize(n.strings.size());
    for (size_t i = 0; i < n.strings.size(); i++) t_.strings[i] = n.strings[i];
    MergeRows(t_.monsterTypes, n.monsterTypes, n.has[0]);
    MergeRows(t_.armors, n.armors, n.has[1]);
    MergeRows(t_.consumables, n.consumables, n.has[2]);
    MergeRows(t_.weapons, n.weapons, n.has[4]);
    MergeRows(t_.classBase, n.classBase, n.has[5]);
    MergeRows(t_.classItemTypes, n.classItemTypes, n.has[5]);
    MergeRows(t_.classLists, n.classLists, n.has[5]);
    MergeRows(t_.table6, n.table6, n.has[6]);
    MergeRows(t_.specials, n.specials, n.has[8]);
    MergeRows(t_.spawnGroups, n.spawnGroups, n.has[9]);
    MergeRows(t_.loot, n.loot, n.has[10]);
    if (n.hasPairs) std::memcpy(t_.pairTable, n.pairTable, sizeof(t_.pairTable));
    t_.spawnIdCount = n.spawnIdCount;
    std::memcpy(t_.spawnIds, n.spawnIds, sizeof(t_.spawnIds));
    if (firstLoad_) {
        std::memcpy(monsterBackup_, t_.monsterTypes, sizeof(monsterBackup_));
        firstLoad_ = false;
    }
    scr_.code = std::move(scr.code);
    std::memcpy(scr_.scriptOffset, scr.scriptOffset, sizeof(scr_.scriptOffset));
    RunScript(1);
}

void ScriptInterpreter::RunScript(int id) {
    if (id < 0 || id > 255 || scr_.scriptOffset[id] < 0) return;
    if (depth_ < 8) {  // stack is 10 deep, the original keeps 2 spare
        scriptStack_[depth_] = id;
        pcStack_[depth_++] = scr_.scriptOffset[id];
    }
}

void ScriptInterpreter::Tick(int dtMs) {
    if (depth_ > 0) Step(dtMs);
}

void ScriptInterpreter::KeyPressed(int action) {
    if (waitingForKey_) {
        waitingForKey_ = false;
    } else if (action >= 3 && action <= 7 && keyHooks_[action] >= 0) {
        int script = keyHooks_[action];
        keyHooks_[action] = -1;
        RunScript(script);
    }
}

const int* ScriptInterpreter::GetRow(int table, int index) const {
    if (index < 0) return nullptr;
    auto in = [&](size_t rows) { return static_cast<size_t>(index) < rows; };
    switch (table) {
        case 0: return in(25) ? t_.monsterTypes[index] : nullptr;
        case 1: return in(42) ? t_.armors[index] : nullptr;
        case 2: return in(11) ? t_.consumables[index] : nullptr;
        case 4: return in(37) ? t_.weapons[index] : nullptr;
        case 5: return in(9) ? t_.classBase[index] : nullptr;
        case 6: return in(25) ? t_.table6[index] : nullptr;
        case 7: return t_.pairTable;
        case 8: return in(10) ? t_.specials[index] : nullptr;
        case 9: return in(10) ? t_.spawnGroups[index] : nullptr;
        case 10: return in(30) ? t_.loot[index] : nullptr;
        default: return nullptr;
    }
}

std::string ScriptInterpreter::ItemName(int field) {
    if ((field & 0xF000) == 0xF000) return host_.GetString(field & 0xFFF);
    if (field >= 0 && static_cast<size_t>(field) < t_.strings.size()) return t_.strings[static_cast<size_t>(field)];
    return "";
}

int ScriptInterpreter::FindString(const std::string& text) {
    for (size_t i = 0; i < t_.strings.size(); i++)
        if (t_.strings[i] == text) return static_cast<int>(i);
    const int id = host_.StringId(text);
    return id == -1 ? -1 : (0xF000 | id);
}

const int* ScriptInterpreter::FindByName(const std::string& text) {
    const int s = FindString(text);
    if (s == -1) return nullptr;
    for (auto& r : t_.weapons) if (r[1] == s && r[0] != 0) return r;
    for (auto& r : t_.consumables) if (r[1] == s && r[0] != 0) return r;
    for (auto& r : t_.armors) if (r[1] == s && r[0] != 0) return r;
    for (auto& r : t_.classBase) if (r[1] == s && r[0] != 0) return r;
    for (auto& r : t_.specials) if (r[1] == s && r[0] != 0) return r;
    return nullptr;
}

int ScriptInterpreter::ItemCategory(const std::string& text) {
    const int s = FindString(text);
    if (s == -1) return -1;
    for (auto& r : t_.weapons) if (r[1] == s && r[0] != 0) return 0;
    for (auto& r : t_.consumables) if (r[1] == s && r[0] != 0) return 2;
    for (auto& r : t_.armors) if (r[1] == s && r[0] != 0) return 1;
    return -1;
}

bool ScriptInterpreter::ClassAllows(int classId, int type) const {
    if (classId < 0 || classId >= 9) return false;
    for (int v : t_.classItemTypes[classId])
        if (v == type) return true;
    return false;
}

int ScriptInterpreter::RollLoot(int rnd) {
    for (int i = 1; i < 30 && t_.loot[i][1] != 0; i++) {
        if (t_.loot[i][2] != 0 && rnd % t_.loot[i][2] == 0 && t_.loot[i][3] > 0) {
            t_.loot[i][3]--;
            return t_.loot[i][1];
        }
    }
    return 0;
}

// SPAWN_PROJECTILE's effect number -> projectile type (0 puff, 1 buff glow, 2 magic hit).
static int ProjectileType(int v) { return v == 0 ? 8 : v == 2 ? 10 : v == 1 ? 9 : v; }

std::string ScriptInterpreter::Str(const ScrString& s) {
    return s.isId ? host_.GetString(s.id) : s.text;
}

void ScriptInterpreter::Step(int dtMs) {
    if (waitingForKey_) return;
    if (host_.DialogueOpen()) return;
    if (talkActor_ >= 0) {
        if (Actor* a = host_.ActorAt(talkActor_)) ActorSystem::SetStatusIcon(*a, 0);
        talkActor_ = -1;
    }
    if (host_.ShopOpen()) return;

    if (waitDuration_ >= 0) {
        waitElapsed_ += dtMs;
        if (waitElapsed_ < waitDuration_) return;
        waitDuration_ = -1;
    }

    if (hasWaitActors_) {
        for (int slot : waitActors_) {
            Actor* a = host_.ActorAt(slot);
            if (a && a->moveTarget[0] != -1) return;
        }
        hasWaitActors_ = false;
        waitActors_.clear();
    }

    if (hasWalk_) {
        // WALK_CUTSCENE: walk along one axis first, then the other, at cutscene speed.
        Actor* a = host_.ActorAt(walkActor_);
        if (!a) {
            hasWalk_ = false;
            return;
        }
        switch (walkPhase_) {
            case 0:
                host_.SetInputEnabled(false);
                host_.CameraFollow(walkActor_);
                if (walkAxis_ != 0 && walkAxis_ != 1) ActorSystem::SetMoveTarget(*a, walkTarget_[0], a->pos[1]);
                else ActorSystem::SetMoveTarget(*a, a->pos[0], walkTarget_[1]);
                ActorSystem::SetAnimState(*a, 2);
                Items::SetStat(*a, 7, 900, *this);
                walkPhase_ = 1;
                return;
            case 1:
                if (a->moveTarget[0] == -1) {
                    ActorSystem::SetAnimState(*a, 3);
                    Items::SetStat(*a, 7, 400, *this);
                    ActorSystem::SetMoveTarget(*a, walkTarget_[0], walkTarget_[1]);
                    walkPhase_ = 2;
                }
                return;
            case 2:
                if (a->moveTarget[0] == -1) {
                    host_.SetInputEnabled(true);
                    hasWalk_ = false;
                }
                return;
        }
        return;
    }

    if (depth_ <= 0) return;
    const size_t pc = static_cast<size_t>(pcStack_[depth_ - 1]);
    if (pc >= scr_.code.size()) {
        std::fprintf(stderr, "script: pc %zu past end of code\n", pc);
        depth_ = 0;
        return;
    }
    const int op = scr_.code[pc];
    ScrInsn in;
    try {
        in = scr_.Decode(pc);
    } catch (const std::exception&) {
        // Opcodes 0, 1, 30, 31 and > 78 are no-ops that consume one byte
        // (0/1 also log an error in the original).
        if (op == 0 || op == 1) std::fprintf(stderr, "script: invalid opcode %d at %zu\n", op, pc);
        pcStack_[depth_ - 1]++;
        return;
    }
    // The original advances the pc before executing; ops that load another
    // level or push a frame rely on that.
    pcStack_[depth_ - 1] += static_cast<int>(in.length);
    const std::vector<int>& a = in.args;

    switch (op) {
        case OP_RETURN:
            depth_--;
            return;
        case OP_SAY:
            host_.SetSpeaker(nullptr);
            host_.ShowDialogue(Str(in.strings[0]));
            return;
        case OP_SET_SCREEN_SIZE: host_.SetScreenSize(a[0], a[1]); return;
        case OP_SET_PLAYER_COLLIDES: host_.SetPlayerCollides(a[0] == 1); return;
        case OP_LOAD_MAP: host_.LoadMap(in.strings[0].text, in.strings[1].text); return;
        case OP_END_LEVEL: host_.EndLevel(a[0], a[1]); return;
        case OP_WAIT:
            waitDuration_ = a[0];
            waitElapsed_ = 0;
            return;
        case OP_SET_STATE_PLAYING: host_.SetState(0); return;
        case OP_SET_KEY_HOOK:
            if (a[0] >= 0 && a[0] <= 4) keyHooks_[3 + a[0]] = a[1];
            return;
        case OP_SPAWN_ACTOR: {
            const int* row = GetRow(0, a[1]);
            if (!row) return;
            std::string name = in.strings[0].isId || !in.strings[0].text.empty() ? Str(in.strings[0]) : "";
            host_.SpawnActor(name, a[0], ItemName(row[1]), row, a[2], a[3]);
            return;
        }
        case OP_SET_TRIGGER: host_.SetTrigger(a[0], a[1], a[2], a[3], a[4]); return;
        case OP_MOVE_ACTOR_TO:
            if (Actor* x = host_.ActorAt(a[0])) ActorSystem::SetMoveTarget(*x, a[1], a[2]);
            return;
        case OP_SET_TILE: host_.SetTile(a[0], a[1], a[2], a[3]); return;
        case OP_SET_INPUT_ENABLED: host_.SetInputEnabled(a[0] == 1); return;
        case OP_REMOVE_ACTOR: host_.RemoveActor(a[0]); return;
        case OP_WAIT_ACTORS_STOP:
            waitActors_.assign(a.begin() + 1, a.end());
            hasWaitActors_ = true;
            return;
        case OP_SET_COLLISION: host_.SetCollision(a[0], a[1], a[2] == 1); return;
        case OP_CALL: RunScript(a[0]); return;
        case OP_SET_ANIM_STATE:
            if (Actor* x = host_.ActorAt(a[0])) ActorSystem::SetAnimState(*x, static_cast<int8_t>(a[1]));
            return;
        case OP_CAMERA_TO: host_.CameraTo(a[0], a[1]); return;
        case OP_CAMERA_FOLLOW: host_.CameraFollow(a[0]); return;
        case OP_CLEAR_TRIGGER: host_.SetTrigger(a[0], a[1], 255, 255, 255); return;
        case OP_CLEAR_KEY_HOOK:
            if (a[0] >= 0 && a[0] <= 4) keyHooks_[3 + a[0]] = -1;
            return;
        case OP_LOAD_LEVEL: host_.LoadLevel(in.strings[0].text); return;
        case OP_SET_DEATH_SCRIPT:
            if (Actor* x = host_.ActorAt(a[0])) x->deathScript = static_cast<int8_t>(a[2]);
            return;
        case OP_CLEAR_DEATH_SCRIPT:
            if (Actor* x = host_.ActorAt(a[0])) x->deathScript = -1;
            return;
        case OP_SET_STAT:
            if (Actor* x = host_.ActorAt(a[0])) Items::SetStat(*x, a[1], a.size() > 2 ? a[2] : 0, *this);
            return;
        case OP_SET_POSITION:
            if (Actor* x = host_.ActorAt(a[0])) ActorSystem::SetPosition(*x, a[1], a[2]);
            return;
        case OP_GIVE_ITEM:
        case OP_REMOVE_ITEM: {
            Actor* x = host_.ActorAt(a[0]);
            static const int kTable[3] = {4, 1, 2};  // weapons, armours, consumables
            if (!x || a[1] < 0 || a[1] > 2) return;
            const int* row = GetRow(kTable[a[1]], a[2]);
            if (op == OP_GIVE_ITEM) Items::AddItem(*x, a[1], row, *this);
            else Items::RemoveItem(*x, a[1], row, *this);
            return;
        }
        case OP_SHOW_MESSAGE: host_.ShowMessage(Str(in.strings[0]), a[0], a[1], a[2]); return;
        case OP_HIDE_MESSAGE: host_.HideMessage(); return;
        case OP_MOVE_ACTOR_X:
            if (Actor* x = host_.ActorAt(a[0])) ActorSystem::SetMoveTarget(*x, a[1], x->pos[1]);
            return;
        case OP_MOVE_ACTOR_Y:
            if (Actor* x = host_.ActorAt(a[0])) ActorSystem::SetMoveTarget(*x, x->pos[0], a[1]);
            return;
        case OP_LOAD_HUD_SPRITES: host_.LoadHudSprites(in.strings[0].text); return;
        case OP_OPEN_MENU: host_.OpenMenu(false); return;
        case OP_OPEN_SHOP_MENU: host_.OpenMenu(true); return;
        case OP_SET_STATUS_ICON:
            if (Actor* x = host_.ActorAt(a[0])) ActorSystem::SetStatusIcon(*x, a[1]);
            return;
        case OP_GENERATE_DUNGEON: host_.GenerateDungeon(GetRow(9, a[0]), t_.spawnIds, a[1], a[2]); return;
        case OP_CLEAR_LAYERS: host_.ClearLayers(); return;
        case OP_PLACE_ITEM: host_.PlaceItem(a[0], a[1], a[2]); return;
        case OP_SET_TRIGGER_RECT:
            for (int x = a[0]; x <= a[2]; x++)
                for (int y = a[1]; y <= a[3]; y++) host_.SetTrigger(x, y, a[4], a[5], a[6]);
            return;
        case OP_CLEAR_TRIGGER_RECT:
            for (int x = a[0]; x <= a[2]; x++)
                for (int y = a[1]; y <= a[3]; y++) host_.SetTrigger(x, y, 255, 255, 255);
            return;
        case OP_WALK_CUTSCENE:
            walkActor_ = a[0];
            walkAxis_ = a[1];
            walkTarget_[0] = a[2];
            walkTarget_[1] = a[3];
            walkPhase_ = 0;
            hasWalk_ = true;
            return;
        case OP_TALK:
            talkActor_ = a[0];
            host_.CameraFollow(talkActor_);
            if (Actor* x = host_.ActorAt(talkActor_)) ActorSystem::SetStatusIcon(*x, a[1]);
            host_.ShowDialogue(Str(in.strings[0]));
            return;
        case OP_LOAD_LANG: host_.LoadLang(a[0]); return;  // count 0 means pack 0
        case OP_SCALE_MONSTER: {
            if (a[0] < 0 || a[0] >= 25) return;
            int* row = t_.monsterTypes[a[0]];
            row[2] = a[1];
            const int arch = row[17] >= 0 && row[17] < 3 ? row[17] : 0;
            for (int i = 3; i <= 9; i++) row[i] += a[1] * kArchetypeGrowth[arch][i - 3];
            return;
        }
        case OP_SET_DROPS_LOOT:
            if (Actor* x = host_.ActorAt(a[0])) x->dropsLoot = a[1] == 1;
            return;
        case OP_WAIT_KEY: waitingForKey_ = true; return;
        case OP_SET_STATE_9: host_.SetState(9); return;
        case OP_SET_BACKGROUND_COLOR: host_.SetBackgroundColor(a[0]); return;
        case OP_LEVEL_UP_TO: host_.LevelUpTo(a[0], a[1]); return;
        case OP_SHOW_TEXT_SCREEN: host_.ShowTextScreen(Str(in.strings[0])); return;
        case OP_RESTORE_MONSTER_TYPE:
            if (a[0] >= 0 && a[0] < 25)
                std::memcpy(t_.monsterTypes[a[0]], monsterBackup_[a[0]], sizeof(t_.monsterTypes[0]));
            return;
        case OP_SPAWN_PROJECTILE: host_.SpawnProjectile(ProjectileType(a[0]), a[1], a[2], 0); return;
        case OP_SPAWN_TIMED_PROJECTILE: host_.SpawnProjectile(ProjectileType(a[0]), a[1], a[2], a[3] * 1000); return;
        case OP_CLEAR_PROJECTILE_AT: host_.ClearProjectileAt(a[0], a[1]); return;
        case OP_SET_RESPAWN_POINT: host_.SetRespawnPoint(a[0], a[1]); return;
        case OP_EVICT_SPRITES: host_.EvictSprites(in.strings[0].text); return;
        case OP_BEGIN_FADE:
            host_.SetState(15);
            host_.SetStateChangesEnabled(false);
            return;
        case OP_END_FADE: host_.SetStateChangesEnabled(true); return;
        case OP_TOGGLE_INVULNERABLE:
            if (Actor* x = host_.ActorAt(a[0])) x->invulnerable = x->invulnerable == 1 ? 0 : 1;
            return;
        case OP_SET_HUD_VISIBLE: host_.SetHudVisible(a[0] == 1); return;
        case OP_SET_STATE_4: host_.SetState(4); return;
        case OP_SET_AI_ACTIVE:
            if (Actor* x = host_.ActorAt(a[0])) x->aiActive = static_cast<int8_t>(a[1]);
            return;
        default:
            return;  // the NOP_* opcodes
    }
}

}  // namespace oblivion
