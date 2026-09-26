#include "world/game_world.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "graphics/text.h"

namespace oblivion {

namespace {

constexpr int kScreenW = Backbuffer::kWidth;
constexpr int kScreenH = Backbuffer::kHeight;

// ActorSystem.revive (the parts that exist so far; hp/mp refill included).
void Revive(Actor& a) {
    a.sortCell[0] = a.sortCell[1] = 0;
    a.enterScript = a.leaveScript = a.zoneId = -1;
    a.dead = 0;
    a.statusIcon = -1;
    a.animState = 0;
    a.moveTarget[0] = a.moveTarget[1] = -1;
    a.hp = a.maxHp = a.level * 4 + (a.strength + a.buffStrength) * 2 + a.endurance * 2 + a.bonusMaxHp;
    a.mp = a.maxMp = a.level * 4 + a.intelligence * 2 + a.bonusMaxMp;
    ActorSystem::UpdateCells(a);
}

}  // namespace

World::World(const AssetRoot& assets, ImageCache& images)
    : assets_(assets), images_(images), view_(assets, images), script_(*this) {
    strings_.pack0.Parse(assets.Read("/lang_0.txt"));
}

const SpriteSet& World::SpritesFor(const std::string& cml) {
    auto it = spriteCache_.find(cml);
    if (it == spriteCache_.end()) it = spriteCache_.emplace(cml, ParseCml(assets_.Read(cml), images_)).first;
    return it->second;
}

// ---- level loading -------------------------------------------------------

void World::LoadLevel(const std::string& scrPath) {
    if (onLoadLevel) onLoadLevel();
    SetState(6);
    currentLevel_ = scrPath;
    view_.Unload();
    enterLayer_.clear();
    leaveLayer_.clear();
    zoneLayer_.clear();
    inputEnabled_ = true;
    playerCollides_ = true;
    maxActorSlot_ = 0;
    dialogue.open = false;
    message = Message{};
    cameraActor_ = -1;
    for (auto& a : actors_) a.reset();
    script_.Load(ParseScr(assets_.Read(scrPath)));
}

void World::LoadMap(const std::string& jtm, const std::string& tileCml) {
    view_.LoadMap(jtm, tileCml);
    redraw_ = true;
    const size_t n = static_cast<size_t>(view_.map().cellCount());
    enterLayer_.assign(n, -1);
    leaveLayer_.assign(n, -1);
    zoneLayer_.assign(n, -1);
    // Game.finishMapLoad: only the player survives a map load.
    for (int i = 1; i < kMaxActors; i++) actors_[i].reset();
    if (actors_[0]) {
        Revive(*actors_[0]);
    }
}

void World::SetTrigger(int x, int y, int enter, int leave, int zone) {
    if (enterLayer_.empty()) return;
    const size_t i = static_cast<size_t>(x * view_.map().height + y);
    if (x < 0 || y < 0 || i >= enterLayer_.size()) return;
    enterLayer_[i] = static_cast<int8_t>(enter);
    leaveLayer_[i] = static_cast<int8_t>(leave);
    zoneLayer_[i] = static_cast<int8_t>(zone);
}

void World::ClearLayers() {
    view_.ClearVisualLayers();
    redraw_ = true;
    if (actors_[0]) Revive(*actors_[0]);
}

void World::SetScreenSize(int, int) {
    // The port runs a fixed 176x208 virtual screen.
}

void World::SetState(int state) {
    if (!stateChangesEnabled_ || state_ == 12) return;
    const int old = state_;
    state_ = state;
    if (onStateChange) onStateChange(old, state);
}

void World::EndLevel(int kind, int arg) {
    cutsceneSprite = kind;
    cutsceneColor = arg;
    SetState(kind == 4 ? 21 : 8);
}

// ---- actors --------------------------------------------------------------

Actor* World::ActorAt(int slot) {
    return slot >= 0 && slot < kMaxActors ? actors_[slot].get() : nullptr;
}

void World::SpawnActor(const std::string& name, int slot, const std::string& cml, const int* row, int x, int y) {
    if (slot < 0 || slot >= kMaxActors) return;
    if (slot == 0 && player_) {
        actors_[0] = player_;
        Revive(*player_);
    } else {
        auto a = std::make_shared<Actor>();
        ActorSystem::Init(*a, cml, static_cast<int8_t>(slot + 1), SpritesFor(cml));
        // ActorSystem.initFromTemplate (the stat part; specials/items come with M7).
        a->level = static_cast<int8_t>(row[2]);
        if (slot == 0) {
            // setClass: the player's attributes come from the class table.
            const int* c = script_.GetRow(5, playerClass_);
            a->classId = static_cast<int8_t>(playerClass_);
            if (c) {
                a->speed = c[6];
                a->strength = c[7];
                a->intelligence = c[8];
                a->willpower = c[9];
                a->agility = c[10];
                a->endurance = c[11];
                a->personality = c[12];
                a->attackRange = c[13];
                a->sightRange = c[14];
            }
        } else {
            a->strength = row[3];
            a->intelligence = row[4];
            a->willpower = row[5];
            a->agility = row[6];
            a->speed = row[7];
            a->endurance = row[8];
            a->personality = row[9];
            a->sightRange = row[14];
            a->attackRange = row[15];
            a->weapon = static_cast<int8_t>(row[10]);
            a->aiType = static_cast<int8_t>(row[18]);
            a->ranged = a->aiType == 4 ? 1 : 0;
            if (row[20] > 0) a->attackInterval = row[20] * 1000;
        }
        a->team = static_cast<int8_t>(row[13]);
        ActorSystem::SetStat(*a, 2, row[2]);  // recomputes max hp/mp and default ranges
        a->hp = a->maxHp;
        a->mp = a->maxMp;
        actors_[slot] = a;
        if (slot == 0) player_ = a;
    }
    Actor& a = *actors_[slot];
    a.name = name;
    ActorSystem::SetPosition(a, x, y);
    if (slot == 0) {
        a.collides = playerCollides_ ? 1 : 0;
        a.dropsLoot = 0;
        a.deathScript = -1;
        cameraActor_ = 0;
        redraw_ = true;
        view_.SetCamera(kScreenW / 2 - a.screenPos[0], kScreenH / 2 - a.screenPos[1]);
    }
    if (slot > maxActorSlot_) maxActorSlot_ = slot;
}

void World::RemoveActor(int slot) {
    if (slot < 0 || slot >= kMaxActors || !actors_[slot]) return;
    if (slot == 0) {
        // The player "dies": the original goes to the continue screen (state 11)
        // and respawns at the respawn point.
        SetState(11);
        Revive(*actors_[0]);
        ActorSystem::SetPosition(*actors_[0], respawn_[0], respawn_[1]);
        message = Message{};
        return;
    }
    if (slot == cameraActor_) SetSpeaker(nullptr);
    actors_[slot].reset();
    if (slot == maxActorSlot_) {
        while (slot > 0 && !actors_[slot]) slot--;
        maxActorSlot_ = slot;
    }
}

// ---- camera --------------------------------------------------------------

void World::CameraTo(int x, int y) {
    // Game.worldToIso, centred on the screen.
    const int sx = (x - y) >> 3, sy = (x + y) >> 4;
    view_.SetCamera(kScreenW / 2 - sx, kScreenH / 2 - sy);
    redraw_ = true;
    cameraActor_ = -1;
    SetSpeaker(nullptr);
}

void World::CameraFollow(int slot) {
    Actor* a = ActorAt(slot);
    if (!a) return;
    cameraActor_ = slot;
    redraw_ = true;
    view_.SetCamera(kScreenW / 2 - a->screenPos[0], kScreenH / 2 - a->screenPos[1]);
    SetSpeaker(a->name.empty() ? nullptr : &a->name);
}

// Game.updateCamera: recentre on the followed actor when it leaves the
// screen, or when something (a tile edit, a new actor) asked for a redraw.
void World::UpdateCamera() {
    Actor* a = ActorAt(cameraActor_);
    if (a && a->dead == 0) {
        const int camX = view_.camX(), camY = view_.camY();
        if (a->screenPos[1] - ActorSystem::SpriteHeight(*a) + camY < 0) redraw_ = true;
        else if (a->screenPos[1] + camY > kScreenH) redraw_ = true;
        else if (a->screenPos[0] + camX < 0) redraw_ = true;
        else if (a->screenPos[0] + ActorSystem::SpriteWidth(*a) + camX > kScreenW) redraw_ = true;
        if (redraw_)
            view_.SetCamera(kScreenW / 2 - a->screenPos[0],
                            kScreenH / 2 - a->screenPos[1] + ActorSystem::SpriteHeight(*a));
    }
    redraw_ = false;
}

// ---- dialogue / messages / text --------------------------------------------

void World::SetSpeaker(const std::string* name) {
    hasSpeaker_ = name != nullptr;
    speaker_ = name ? *name : std::string();
}

// Game.wrapText.
std::vector<std::string> World::WrapText(std::string text, int width) {
    using Text::Face;
    auto replace = [&](const char* key, const std::string& value) {
        size_t p = text.find(key);
        if (p != std::string::npos) text = text.substr(0, p) + value + text.substr(p + std::strlen(key));
    };
    // Key-name placeholders are filled from the key labels (Game.keyLabels);
    // the desktop port shows fixed labels for now.
    replace("ACTION_KEY", "5");
    replace("TOGGLE_WEAPON_KEY", "3");
    replace("QUICK_HEALTH_KEY", "7");
    replace("QUICK_MAGIKA_KEY", "9");
    if (hasSpeaker_) {
        text = speaker_ + ": " + text;
    } else if (text.find(':') != std::string::npos) {
        speaker_ = text.substr(0, text.find(':'));
        hasSpeaker_ = true;
    }
    std::vector<std::string> lines;
    size_t start = 0, lastSpace = 0, i = 0;
    for (i = 0; i + 1 < text.size(); i++) {
        if (text[i] == ' ') lastSpace = i;
        const int w = Text::SubstringWidth(text, start, i - start + 1, Face::SmallBold);
        if (w >= width && lastSpace > 0) {
            lines.push_back(text.substr(start, lastSpace - start));
            start = i = lastSpace + 1;
            lastSpace = 0;
        }
    }
    if (i > start) lines.push_back(text.substr(start, i + 1 - start));
    return lines;
}

void World::ShowDialogue(const std::string& text) {
    // Game.showDialogue: box geometry comes from the HUD sprites.
    const int capW = hudSprites_.Width(54);
    dialogue.right = kScreenW - 10;
    dialogue.textWidth = dialogue.right - capW - 13;
    dialogue.height = std::min(kScreenH >> 1, hudSprites_.Height(51)) - 4;
    dialogue.lines = WrapText(text, dialogue.textWidth);
    // beginDialogue
    dialogue.scroll = -1;
    dialogue.atEnd = true;
    dialogue.open = true;
    dialogue.ageMs = 0;
}

void World::ShowMessage(const std::string& text, int seconds, int color, int style) {
    Message m;
    m.text = text;
    m.durationMs = seconds * 1000;
    m.style = style;
    static const int kColors[] = {0x000000, 0xFFFFFF, 0xFF0000, 0x0000FF, 0xFFFF00, 0x00FF00};
    m.color = color >= 0 && color <= 5 ? kColors[color] : 0;
    message = m;
}

void World::ShowTextScreen(const std::string& text) {
    textScreenText = text;
    SetState(10);
}

void World::LoadLang(int packIndex) {
    char path[32];
    std::snprintf(path, sizeof(path), "/lang_%d.txt", packIndex);
    (packIndex == 0 ? strings_.pack0 : strings_.pack1).Parse(assets_.Read(path));
}

void World::LoadHudSprites(const std::string& cml) { hudSprites_ = SpritesFor(cml); }

// ---- per-frame -----------------------------------------------------------

int8_t World::ZoneUnder(const Actor& a) const {
    if (zoneLayer_.empty()) return -1;
    const int h = view_.map().height;
    const int idx[3] = {a.cell[0] * h + a.cell[1], a.footBCell[0] * h + a.footBCell[1], a.footCCell[0] * h + a.footCCell[1]};
    for (int i : idx) {
        if (i < 0 || i >= static_cast<int>(zoneLayer_.size())) return -1;
        const int8_t z = zoneLayer_[static_cast<size_t>(i)];
        if (z >= 0) return z;  // 255 is stored as -1: no zone
    }
    return -1;
}

void World::CheckPlayerTriggers(Actor& p, int8_t oldEnter, int8_t oldLeave) {
    const int8_t now = ActorSystem::CheckTriggerTiles(p, enterLayer_, leaveLayer_, view_.map().height);
    if (now == oldEnter) return;
    if (oldLeave != 0 && oldLeave != -1 && oldLeave != -2) script_.RunScript(static_cast<uint8_t>(oldLeave));
    if (now != 0 && now != -1 && now != -2) script_.RunScript(static_cast<uint8_t>(now));
    else if (now == -2) ShowMessage(strings_.Get(24), 60, 4, 0);
}

void World::Tick(int dtMs) {
    if (state_ == 12) return;
    if (dialogue.open) dialogue.ageMs += dtMs;

    if (state_ != 3 && state_ != 10 && state_ != 9 && state_ != 13) script_.Tick(dtMs);

    UpdateMessage(dtMs);

    if (state_ == 0) {
        for (int slot = 0; slot <= maxActorSlot_; slot++) {
            Actor* a = actors_[slot].get();
            if (!a) continue;
            const int8_t oldEnter = a->enterScript, oldLeave = a->leaveScript;
            ActorSystem::Update(*a, dtMs);
            if (slot == 0 && actors_[0]) CheckPlayerTriggers(*a, oldEnter, oldLeave);
        }
        UpdateCamera();
    }
}

void World::HeldAction(int action, int dtMs) {
    if (state_ != 0 || dialogue.open || !inputEnabled_ || !player_ || player_->dead != 0) return;
    Actor& p = *player_;
    const int oldZone = p.zoneId;
    switch (action) {
        case 3: ActorSystem::SetAnimState(p, 1); ActorSystem::MoveDir(p, view_.grid(), 2, dtMs); break;
        case 4: ActorSystem::SetAnimState(p, 1); ActorSystem::MoveDir(p, view_.grid(), 1, dtMs); break;
        case 5: ActorSystem::SetAnimState(p, 1); ActorSystem::MoveDir(p, view_.grid(), 4, dtMs); break;
        case 6: ActorSystem::SetAnimState(p, 1); ActorSystem::MoveDir(p, view_.grid(), 3, dtMs); break;
        case 7: p.zoneId = ZoneUnder(p); break;  // checkZoneTiles (the melee fall-through is M7)
        default: break;
    }
    if (oldZone != p.zoneId && p.zoneId != 0 && p.zoneId != -1 && p.zoneId != -2)
        script_.RunScript(static_cast<uint8_t>(p.zoneId));
}

void World::KeyPressed(int action) {
    script_.KeyPressed(action);
    HandleDialogueKey(action);
}

// Game.handleDialogueKey.
void World::HandleDialogueKey(int action) {
    if (!dialogue.open) return;
    if (action == 3 && dialogue.scroll > -1) {
        dialogue.scroll -= 4;
    } else if (action == 4 && !dialogue.atEnd) {
        dialogue.scroll += 4;
    } else if (action == 7 && dialogue.ageMs >= 1000) {
        dialogue.open = false;
        if (player_) player_->zoneId = 0;
    }
}

// Game.updateMessage.
void World::UpdateMessage(int dt) {
    Message& m = message;
    if (m.text.empty()) return;
    if (m.style == 1) {
        if (m.blinkTimerMs >= 500) {
            m.blank = !m.blank;
            m.blinkTimerMs = 0;
        }
        m.blinkTimerMs += dt;
    } else if (m.style == 2 || m.style == 3) {
        if (m.scrollTimerMs >= 50) {
            m.x += m.style == 2 ? 2 : -2;
            if (m.x == -1) m.x += m.style == 2 ? 1 : -1;
            m.scrollTimerMs = 0;
        }
        m.scrollTimerMs += dt;
    }
    if (m.elapsedMs > m.durationMs) {
        m = Message{};
        return;
    }
    m.elapsedMs += dt;
}

void World::DrawField(Backbuffer& bb) {
    bb.Fill(static_cast<uint32_t>(background_));
    if (!view_.loaded()) return;
    std::vector<Actor*> list;
    for (int i = 0; i <= maxActorSlot_; i++)
        if (actors_[i]) list.push_back(actors_[i].get());
    view_.Draw(bb, list);
}

}  // namespace oblivion
