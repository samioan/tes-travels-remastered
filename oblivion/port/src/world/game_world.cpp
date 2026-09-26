#include "world/game_world.h"

#include <algorithm>
#include <cstdio>
#include <cmath>
#include <cstring>

#include "graphics/text.h"
#include "world/dungeon.h"
#include "world/items.h"

namespace oblivion {

namespace {

constexpr int kScreenH = Backbuffer::kHeight;

}  // namespace

World::World(const AssetRoot& assets, ImageCache& images)
    : assets_(assets), images_(images), view_(assets, images), script_(*this) {
    strings_.pack0.Parse(assets.Read("/lang_0.txt"));
    projectiles_.SetFrames(SpritesFor("/oh_magic.cml"));
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
    projectiles_.ClearAll();
    pickups_.clear();
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
        ReviveActor(*actors_[0]);
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
    if (actors_[0]) ReviveActor(*actors_[0]);
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
        ReviveActor(*player_);
    } else {
        auto a = std::make_shared<Actor>();
        ActorSystem::Init(*a, cml, static_cast<int8_t>(slot + 1), SpritesFor(cml));
        if (slot == 0) Items::SetClass(*a, playerClass_, false, script_);
        Items::InitFromTemplate(*a, row, script_);
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
        view_.SetCamera(screenW_ / 2 - a.screenPos[0], kScreenH / 2 - a.screenPos[1]);
    }
    if (slot > maxActorSlot_) maxActorSlot_ = slot;
}

void World::RemoveActor(int slot) {
    if (slot < 0 || slot >= kMaxActors || !actors_[slot]) return;
    if (slot == 0) {
        projectiles_.ClearAll();
        // The player "dies": the original goes to the continue screen (state 11)
        // and respawns at the respawn point.
        SetState(11);
        ReviveActor(*actors_[0]);
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
    view_.SetCamera(screenW_ / 2 - sx, kScreenH / 2 - sy);
    redraw_ = true;
    cameraActor_ = -1;
    SetSpeaker(nullptr);
}

void World::CameraFollow(int slot) {
    Actor* a = ActorAt(slot);
    if (!a) return;
    cameraActor_ = slot;
    redraw_ = true;
    view_.SetCamera(screenW_ / 2 - a->screenPos[0], kScreenH / 2 - a->screenPos[1]);
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
        else if (a->screenPos[0] + ActorSystem::SpriteWidth(*a) + camX > screenW_) redraw_ = true;
        if (redraw_)
            view_.SetCamera(screenW_ / 2 - a->screenPos[0],
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
    // Key-name placeholders (Game.keyLabels): the desktop port's controls.
    replace("ACTION_KEY", "Space");
    replace("TOGGLE_WEAPON_KEY", "Tab");
    replace("QUICK_HEALTH_KEY", "1");
    replace("QUICK_MAGIKA_KEY", "2");
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
    dialogue.right = Backbuffer::kWidth - 10;  // the box is a 176-wide column, centred when the screen is wider
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

void World::CheckPlayerTriggers(Actor& p, int8_t oldEnter, int8_t oldLeave) {
    const int8_t now = ActorSystem::CheckTriggerTiles(p, enterLayer_, leaveLayer_, view_.map().height);
    if (now == oldEnter) return;
    if (oldLeave != 0 && oldLeave != -1 && oldLeave != -2) script_.RunScript(static_cast<uint8_t>(oldLeave));
    if (now != 0 && now != -1 && now != -2) script_.RunScript(static_cast<uint8_t>(now));
    else if (now == -2) ShowMessage(strings_.Get(24), 60, 4, 0);
}

// ActorSystem.revive: also drops every actor's target.
void World::ReviveActor(Actor& a) {
    for (auto& o : actors_)
        if (o) o->target.reset();
    Items::Revive(a, script_);
}

Actor* World::SpawnFreeActor(const std::string& cml, int x, int y, const int* row) {
    int slot = kMaxActors - 1;
    while (slot >= 0 && actors_[slot]) slot--;
    if (slot < 0) return nullptr;
    SpawnActor(std::string(), slot, cml, row, x, y);
    return actors_[slot].get();
}

// Game.spawnItem: an item lying on a cell, drawn into the overlay layer.
void World::SpawnItem(int itemId, bool fromScript, int cellX, int cellY) {
    std::vector<uint8_t>* overlay = view_.Overlay();
    const int idx = cellX * view_.map().height + cellY;
    if (!overlay || pickups_.size() >= 24 || idx < 0 || idx >= view_.map().cellCount()) return;
    (*overlay)[static_cast<size_t>(idx)] = fromScript ? 211 : 22;  // -45 / 22
    pickups_.push_back({static_cast<uint8_t>(cellX), static_cast<uint8_t>(cellY), static_cast<uint8_t>(itemId)});
}

// The fire key next to an item picks it up (Game.handleInput case 7).
void World::TryPickup(Actor& p) {
    for (size_t k = 0; k < pickups_.size(); k++) {
        const int at[2] = {pickups_[k][0] << 7, pickups_[k][1] << 7};
        const int* row = script_.GetRow(6, pickups_[k][2]);
        if (Combat::Distance(at, p.pos) >= 350 || !row) continue;
        projectiles_.SpawnFixed(8, at[0], at[1] + 128);
        if (std::vector<uint8_t>* overlay = view_.Overlay()) {
            uint8_t& tile = (*overlay)[static_cast<size_t>(pickups_[k][0] * view_.map().height + pickups_[k][1])];
            tile = tile == 211 ? 212 : 0;  // -45 -> -44 (opened chest), else gone
        }
        pickups_.erase(pickups_.begin() + static_cast<std::ptrdiff_t>(k));
        if (row[2] > 0) {
            ShowMessage(std::to_string(row[2]) + " " + strings_.Get(38), 3, 4, 0);
            gold += row[2];
        } else if (row[4] > 0) {
            if (const int* item = script_.GetRow(1, row[4])) {
                ShowMessage(script_.ItemName(item[1]), 3, 4, 0);
                Items::AddItem(p, 1, item, script_);
            }
        } else if (row[3] > 0) {
            if (const int* item = script_.GetRow(4, row[3])) {
                ShowMessage(script_.ItemName(item[1]), 3, 4, 0);
                Items::AddItem(p, 0, item, script_);
            }
        } else if (row[5] > 0) {
            if (const int* item = script_.GetRow(2, row[5])) {
                ShowMessage(script_.ItemName(item[1]), 3, 4, 0);
                Items::AddItem(p, 2, item, script_);
            }
        }
        break;
    }
}

// Standing near an item shows the "pick up" prompt (string 363).
void World::UpdatePickupPrompt() {
    const std::string prompt = strings_.Get(363);
    bool shown = false;
    if (Actor* p = player_.get()) {
        for (const auto& pk : pickups_) {
            const int at[2] = {pk[0] << 7, pk[1] << 7};
            if (Combat::Distance(at, p->pos) < 350) {
                ShowMessage(prompt, 60, 4, 0);
                shown = true;
                break;
            }
        }
    }
    if ((message.text.empty() || message.text == prompt) && !shown) HideMessage();
}

// Game.generateDungeon: replaces the map with a random dungeon (tile set stays),
// then scatters monsters (and the level's item ids) on its branch points.
void World::GenerateDungeon(const int* group, const int* spawnIds, int zone, int exitScript) {
    if (!group || static_cast<int8_t>(group[1]) < 6 || static_cast<int8_t>(group[2]) < 6) return;
    Dungeon d = oblivion::GenerateDungeon(group, zone, exitScript, [this]() { return Random(); });
    view_.SetMap(std::move(d.map));
    enterLayer_ = std::move(d.enter);
    leaveLayer_ = std::move(d.leave);
    zoneLayer_ = std::move(d.zone);
    pickups_.clear();
    for (int i = 1; i < kMaxActors; i++) actors_[i].reset();
    maxActorSlot_ = 0;
    if (player_) player_->summon.reset();
    redraw_ = true;
    if (actors_[0]) {
        ReviveActor(*actors_[0]);
        ActorSystem::UpdateCells(*actors_[0]);
    }

    const int* monster = script_.GetRow(0, group[17]);
    int item = 0;
    for (int k = 0; k + 1 < static_cast<int>(d.branchPoints.size()) && k < group[18]; k += 2) {
        const int bx = d.branchPoints[static_cast<size_t>(k)], by = d.branchPoints[static_cast<size_t>(k) + 1];
        if (item < 10 && spawnIds[item] != 0) SpawnItem(spawnIds[item++], false, bx, by);
        if (!monster) continue;
        int slot = 2;
        while (slot < kMaxActors && actors_[slot]) slot++;
        if (slot >= kMaxActors) break;
        SpawnActor(std::string(), slot, script_.ItemName(monster[1]), monster, bx << 7, by << 7);
    }
}

void World::Tick(int dtMs) {
    if (state_ == 12) return;
    if (dialogue.open) dialogue.ageMs += dtMs;

    if (!scriptPaused && state_ != 3 && state_ != 10 && state_ != 9 && state_ != 13) {
        projectiles_.Tick(dtMs, *this);
        script_.Tick(dtMs);
    }

    UpdateMessage(dtMs);

    if (state_ == 0) {
        for (int slot = 0; slot <= maxActorSlot_; slot++) {
            std::shared_ptr<Actor> keep = actors_[slot];  // Update may remove the actor
            Actor* a = keep.get();
            if (!a) continue;
            const int8_t oldEnter = a->enterScript, oldLeave = a->leaveScript;
            Combat::Update(*a, dtMs, !dialogue.open && inputEnabled_, *this);
            if (slot == 0 && actors_[0]) {
                CheckPlayerTriggers(*a, oldEnter, oldLeave);
                UpdatePickupPrompt();
            }
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
        case 7: Combat::CheckZoneTiles(p, zoneLayer_, view_.map().height, *this); break;
        default: break;
    }
    if (oldZone != p.zoneId && p.zoneId != 0 && p.zoneId != -1 && p.zoneId != -2)
        script_.RunScript(static_cast<uint8_t>(p.zoneId));
    if (action == 7 && p.killTimer > 1000) TryPickup(p);
}

// ---- modern controls -------------------------------------------------------

namespace {
// Screen space (x right, y down) to the ground plane of the isometric view:
// +x world is down-right on screen, +y world is down-left.
void ScreenToWorld(float sx, float sy, float* wx, float* wy) {
    const float k = 0.70710678f;
    *wx = (sx + sy) * k;
    *wy = (sy - sx) * k;
}
}  // namespace

bool World::PlayerCanAct() const {
    return state_ == 0 && !dialogue.open && inputEnabled_ && player_ && player_->dead == 0;
}

void World::MoveAnalog(float sx, float sy, int dtMs) {
    if (!PlayerCanAct()) return;
    float mag = std::sqrt(sx * sx + sy * sy);
    if (mag < 0.01f) return;
    if (mag > 1.0f) {
        sx /= mag;
        sy /= mag;
        mag = 1.0f;
    }
    Actor& p = *player_;
    float wx, wy;
    ScreenToWorld(sx, sy, &wx, &wy);
    const float step = static_cast<float>(p.speed) * static_cast<float>(dtMs) / 1000.0f;
    ActorSystem::SetAnimState(p, 1);
    ActorSystem::MoveAnalog(p, view_.grid(), wx * step, wy * step);
    if (p.hasAim) ActorSystem::FaceWorldDir(p, p.aimX, p.aimY);
    else ActorSystem::FaceWorldDir(p, wx, wy);
    p.idleTimer = 120;  // back to the idle pose soon after the stick is released
}

void World::SetAim(bool valid, float sx, float sy) {
    if (!player_) return;
    float wx, wy;
    ScreenToWorld(sx, sy, &wx, &wy);
    const float len = std::sqrt(wx * wx + wy * wy);
    player_->hasAim = valid && len > 1e-3f;
    if (player_->hasAim) {
        player_->aimX = wx / len;
        player_->aimY = wy / len;
    }
}

void World::RunZoneScript(int oldZone) {
    Actor& p = *player_;
    if (oldZone != p.zoneId && p.zoneId != 0 && p.zoneId != -1 && p.zoneId != -2)
        script_.RunScript(static_cast<uint8_t>(p.zoneId));
}

void World::Attack() {
    if (!PlayerCanAct()) return;
    Actor& p = *player_;
    const int oldZone = p.zoneId;
    Combat::CheckZoneTiles(p, zoneLayer_, view_.map().height, *this, true);
    RunZoneScript(oldZone);
}

void World::Interact() {
    if (!PlayerCanAct()) return;
    Actor& p = *player_;
    const int oldZone = p.zoneId;
    Combat::CheckZoneTiles(p, zoneLayer_, view_.map().height, *this, false);
    RunZoneScript(oldZone);
    if (p.killTimer > 1000) TryPickup(p);
}

bool World::PlayerScreenPos(int* x, int* y) const {
    if (!player_) return false;
    const Actor& p = *player_;
    *x = p.screenPos[0] + view_.camX() + (ActorSystem::SpriteWidth(p) >> 1);
    *y = p.screenPos[1] + view_.camY() - (ActorSystem::SpriteHeight(p) >> 1);
    return true;
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
    projectiles_.Draw(bb, images_, view_.camX(), view_.camY());
}

}  // namespace oblivion
