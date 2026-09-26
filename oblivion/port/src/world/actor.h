#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "assets/cml.h"
#include "assets/image.h"
#include "graphics/backbuffer.h"

namespace oblivion {

// Collision + trigger data of the loaded level, as the engine's static
// Game.gridWidth/gridHeight/collision. `collision` is jtm layer 0, indexed
// [x * height + y]; null means "no map" (nothing blocks).
struct Grid {
    int width = 0, height = 0;
    const std::vector<uint8_t>* collision = nullptr;
};

// The movement / collision / animation / drawing subset of Actor + ActorSystem
// (src/Actor.java, src/ActorSystem.java). Combat, stats, AI and items arrive
// with M7; the fields they need are added then.
struct Actor : std::enable_shared_from_this<Actor> {
    // Sub-tile world points (128 per grid cell): pos, and the two other feet
    // of the sprite's isometric footprint (footB half, footC full sprite width).
    int pos[2] = {0, 0}, footB[2] = {0, 0}, footC[2] = {0, 0}, prevPos[2] = {0, 0};
    int screenPos[2] = {0, 0};
    int8_t cell[2] = {0, 0}, footBCell[2] = {0, 0}, footCCell[2] = {0, 0};
    int8_t sortCell[2] = {0, 0};
    int spriteWidth = 0, spriteHalfWidth = 0;

    int8_t slot = 0;       // 1-based index in the actor table (1 = player)
    int8_t facing = 2;     // 1 +y, 2 -y, 3 +x, 4 -x
    int8_t animState = 0;  // see animStateOffset
    int8_t collides = 1;
    int8_t dead = 0;
    int8_t team = 1;
    int speed = 0;          // world units per second
    int moveTimer = 0;      // ms
    int idleTimer = 0;      // ms until animState returns to 0 (player only)
    int animTimer = 0;      // ms since last frame advance
    int moveTarget[2] = {-1, -1};  // [0] == -1: none

    // Trigger tiles under the actor (checkTriggerTiles).
    int8_t enterScript = -1, leaveScript = -1;
    int8_t zoneId = -1;

    // Script-visible state (Actor.java). Derived stats and combat arrive with M7.
    int8_t classId = -1, level = 0, aiType = -1, aiActive = 1, dropsLoot = 1, invulnerable = 0;
    int8_t attackIcon = -45, effectIcon = -1;  // HUD icon frames (hud sprite groups)
    int8_t statusIcon = -1, deathScript = -1, ranged = 0, weapon = 0;
    int strength = 0, intelligence = 0, willpower = 0, agility = 0, endurance = 0, personality = 0;
    int sightRange = 0, attackRange = 0, attackInterval = 1000;
    int hp = 1, maxHp = 100, mp = 1, maxMp = 100, bonusMaxHp = 0, bonusMaxMp = 0, buffStrength = 0;
    int hpRegenInterval = 400, mpRegenInterval = 400;  // ms per point: 40000 / max
    int xp = 0;

    // Equipment and derived combat numbers (ActorSystem.recalcDerivedStats).
    int weaponPower = 0, armor = 0, dodgeChance = 0, blockChance = 0, defenseRating = 100, attackRating = 100;
    int dodgeScale = 100, buffAttack = 0, buffArmor = 0, buffDefense = 0, buffAttack2 = 0;
    int itemBuffElapsed = 0, itemBuffDuration = 0, dotRemaining = 0, dotTick = 0;
    // 255 packed slots (category << 8) | id: 0 weapon, 1 armour, 2 consumable; 0 = empty.
    int inventory[255] = {};
    int wornArmor[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
    // Rows of the script tables (stable arrays owned by the interpreter).
    const int* hpPotion = nullptr;
    const int* mpPotion = nullptr;
    const int* special = nullptr;
    const int* altSpecial = nullptr;
    const int* classRow = nullptr;
    const int* classList = nullptr;
    std::string name;

    // Combat state (Actor.java). Actor links are weak: the world's actor table
    // owns the actors, and a removed actor simply reads as "no target".
    int attackTimer = 0, killTimer = 0, deathTimer = 0, hpRegenTimer = 0, mpRegenTimer = 0;
    int buffTimer = 0;
    int8_t buffFxSlot = -1;  // projectile pool slot of the buff glow
    int16_t teleportTimer = 0;
    int8_t vanished = 0, dotDamage = 0;
    std::weak_ptr<Actor> target, dotSource, summon, owner;
    const int* spawnRow = nullptr;
    std::string floatText;  // empty = none
    int8_t floatKind = 0;  // 0 damage (red), 1 dodged (green), 2 blocked (blue)
    int floatTextTimer = 0, floatTextY = 0, floatTextStartY = 0;
    int floatTextColor = 0xFF0000, floatTextShadow = 0;

    std::string cmlPath;
    SpriteSet sprite;
};

namespace ActorSystem {

// animStateOffset[state] + facing = sprite group id.
int GroupOf(const Actor& a, int state);

// spriteHeight / spriteWidth of the current animation frame (0 when dead).
int SpriteHeight(const Actor& a);
int SpriteWidth(const Actor& a);

// createFromCml: loads the sprite set and measures the footprint width from group 1.
void Init(Actor& a, const std::string& cmlPath, int8_t slot, const SpriteSet& sprite);

// setPosition / updateCells / updateSortCell / undoMove.
void SetPosition(Actor& a, int x, int y);
void UpdateCells(Actor& a);
void UpdateSortCell(Actor& a);
void UndoMove(Actor& a);

// isBlocked: is any of the three footprint points inside a blocking cell or
// out of the grid.
bool IsBlocked(const Actor& a, const Grid& grid);

// moveDir(dir 1..4): one movement step, gated by the 50 ms move timer; undone
// when it ends up blocked. `dtMs` is the time since the last call.
void MoveDir(Actor& a, const Grid& grid, int dir, int dtMs);

void SetMoveTarget(Actor& a, int x, int y);

// setStat for the plain numeric stats (ids 2..9, 13, 14, 15, 18, 20 -- see
// SCR_OPCODES SET_STAT) plus the max hp/mp recompute; specials (19), weapon
// power and recalcDerivedStats come with M7.
void SetStat(Actor& a, int stat, int value);
// setStatusIcon: 0 none, 1..3 icons, 4 = "-2" (blank frame).
void SetStatusIcon(Actor& a, int icon);

// checkTriggerTiles: looks the actor's three footprint cells up in the enter
// layer (0 / -1 = none) and stores the enter/leave scripts it finds. Returns
// the enter script or -1. Layers are indexed [x * gridHeight + y].
int8_t CheckTriggerTiles(Actor& a, const std::vector<int8_t>& enter, const std::vector<int8_t>& leave,
                         int gridHeight);
void SetAnimState(Actor& a, int8_t state);

// The per-frame part of ActorSystem.update that concerns animation and
// auto-walking to moveTarget (regen, dot, AI, floating text come with M7).
void Update(Actor& a, int dtMs);

// ActorSystem.draw: shadow, sprite, and (dead) corpse group. `cam` is the
// camera offset. Health bar, floating text and status icon come with M7.
void Draw(Actor& a, Backbuffer& bb, ImageCache& images, int camX, int camY);

}  // namespace ActorSystem
}  // namespace oblivion
