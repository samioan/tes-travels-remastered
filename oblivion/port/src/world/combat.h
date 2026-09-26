#pragma once
#include <string>

#include "script/interpreter.h"
#include "world/actor.h"
#include "world/projectiles.h"

namespace oblivion {

// What combat needs from the game around it (the Game.* calls that
// ActorSystem / ProjectileManager make). World implements it.
class CombatHost {
public:
    virtual ~CombatHost() = default;
    virtual Actor* ActorSlot(int index) = 0;  // 0..24, nullptr when empty
    virtual int SlotCount() const = 0;
    virtual int Random() = 0;  // Random.nextInt(): any signed 32-bit value
    virtual ScriptInterpreter& Script() = 0;
    virtual Projectiles& Fx() = 0;
    virtual std::string GetString(int id) = 0;
    virtual void ShowMessage(const std::string& text, int seconds, int color, int style) = 0;
    virtual void RemoveActorSlot(int index) = 0;  // Game.removeActor
    // Game.spawnActor: an actor in the highest free slot.
    virtual Actor* SpawnFreeActor(const std::string& cml, int x, int y, const int* row) = 0;
    virtual void SpawnItem(int itemId, bool fromScript, int cellX, int cellY) = 0;
    // The map's collision grid and its first visual layer (Game.layers[0]; 0 = void),
    // for teleporting monsters.
    virtual Grid CurrentGrid() = 0;
    virtual const std::vector<uint8_t>* GroundLayer() = 0;
};

// The combat half of ActorSystem: AI, damage, XP and levelling, specials.
namespace Combat {

// ActorSystem.distance: the octagonal approximation of the world distance.
int Distance(const int a[2], const int b[2]);

// ActorSystem.update: everything per-frame (animation, walking, damage over
// time, regeneration, AI attacks, floating text, buffs, corpse removal).
// `aiAttacksPlayer` is Game's `!dialogueOpen && inputEnabled`.
void Update(Actor& a, int dtMs, bool aiAttacksPlayer, CombatHost& host);

// attack: a melee/ranged hit from `a` to `victim` (or a special from an AI actor).
// True when the victim died.
bool Attack(Actor& a, Actor& victim, bool melee, CombatHost& host);

// checkZoneTiles, the fire key: a zone id if the player stands on one,
// otherwise -1 after the melee / special attack fall-through.
int CheckZoneTiles(Actor& a, const std::vector<int8_t>& zones, int gridHeight, CombatHost& host);

// levelUpTo / applyLevelUpBonus (the LEVEL_UP_TO op).
void LevelUpTo(Actor& a, int level, ScriptInterpreter& sc);

// applyPoison / applyMagicHit (also used by specials).
void ApplyPoison(Actor& source, Actor& victim, int damage, int durationMs, CombatHost& host);
void ApplyMagicHit(Actor& source, Actor& victim, int damage, CombatHost& host);

}  // namespace Combat
}  // namespace oblivion
