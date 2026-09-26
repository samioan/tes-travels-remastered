#include "world/combat.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "world/items.h"

namespace oblivion {
namespace Combat {

namespace {

// ActorSystem.xpForLevel / xpReward.
const int kXpForLevel[] = {0,    0,    100,  210,  340,   500,   700,   950,   1260,  1640,  2100, 2650, 3300,
                           4060, 4940, 5950, 7100, 8400,  9860,  11490, 13300, 15300, 17500, 19910, 22540, 25400};
const int kXpReward[] = {0,  10,  12,  15,  19,  24,  30,  37,  45,  54,  64,  75,  87,
                         100, 114, 129, 145, 162, 180, 199, 219, 240, 262, 285, 309, 334};

int Sign(int v) { return v < 0 ? -v : v; }

// Recompute max hp/mp from the attributes (the tail shared by level-ups).
void RecomputeMax(Actor& a) {
    a.maxHp = a.level * 4 + (a.strength + a.buffStrength) * 2 + a.endurance * 2 + a.bonusMaxHp;
    a.hpRegenInterval = 40000 / std::max(1, a.maxHp);
    a.maxMp = a.level * 4 + a.intelligence * 2 + a.bonusMaxMp;
    a.mpRegenInterval = 40000 / std::max(1, a.maxMp);
}

// Class bonuses at levels 5/10/15/20 (applyLevelUpBonus).
void ApplyLevelUpBonus(Actor& a) {
    enum { STR, INT, WIL, AGI, SPD, END };
    struct Bonus { int stat, amount; };
    // classId 1..8, then levels 5, 10, 15, 20.
    static const Bonus kBonus[9][4] = {
        {},
        {{SPD, 25}, {AGI, 1}, {END, 2}, {STR, 2}},
        {{AGI, 1}, {WIL, 1}, {INT, 2}, {AGI, 2}},
        {{STR, 1}, {END, 1}, {END, 2}, {STR, 2}},
        {{SPD, 25}, {AGI, 2}, {STR, 1}, {STR, 2}},
        {{STR, 1}, {END, 1}, {STR, 2}, {END, 2}},
        {{AGI, 1}, {WIL, 1}, {INT, 2}, {WIL, 2}},
        {{INT, 1}, {WIL, 1}, {WIL, 2}, {INT, 2}},
        {{WIL, 1}, {STR, 1}, {INT, 2}, {WIL, 2}},
    };
    if (a.classId < 1 || a.classId > 8) return;
    int idx = a.level == 5 ? 0 : a.level == 10 ? 1 : a.level == 15 ? 2 : a.level == 20 ? 3 : -1;
    if (idx < 0) return;
    const Bonus& b = kBonus[a.classId][idx];
    switch (b.stat) {
        case STR: a.strength += b.amount; break;
        case INT: a.intelligence += b.amount; break;
        case WIL: a.willpower += b.amount; break;
        case AGI: a.agility += b.amount; break;
        case SPD: a.speed += b.amount; break;
        case END: a.endurance += b.amount; break;
    }
}

void RaiseLevel(Actor& a) {
    a.level++;
    a.strength++;
    a.intelligence++;
    a.willpower++;
    a.agility++;
    a.speed++;
    a.endurance++;
    a.personality++;
    ApplyLevelUpBonus(a);
}

Actor* Locked(const std::weak_ptr<Actor>& w, std::shared_ptr<Actor>& hold) {
    hold = w.lock();
    return hold.get();
}

Actor* FindNearestEnemy(Actor& a, CombatHost& host) {
    Actor* best = nullptr;
    int bestDist = 16777215;
    if (a.slot == 1 && a.hasAim) {
        // Aiming: of the enemies in reach, the one nearest the aim direction
        // (distance counts, so a close enemy off to the side can still win).
        float bestScore = 1e30f;
        for (int i = 0; i < host.SlotCount(); i++) {
            Actor* o = host.ActorSlot(i);
            if (!o || o->dead == 1 || o->team == a.team || o->slot == a.slot) continue;
            const int d = Distance(a.pos, o->pos);
            if (d >= a.attackRange) continue;
            const float fx = static_cast<float>(o->pos[0] - a.pos[0]), fy = static_cast<float>(o->pos[1] - a.pos[1]);
            const float len = std::sqrt(fx * fx + fy * fy);
            const float dot = len < 1e-3f ? 1.0f : (fx * a.aimX + fy * a.aimY) / len;
            const float score = static_cast<float>(d) * (1.6f - dot);
            if (score < bestScore) {
                bestScore = score;
                best = o;
            }
        }
        if (best) return best;
    }
    for (int i = 0; i < host.SlotCount(); i++) {
        Actor* o = host.ActorSlot(i);
        if (!o || o->dead == 1 || o->team == a.team || o->slot == a.slot) continue;
        const int d = Distance(a.pos, o->pos);
        if (d < bestDist) {
            bestDist = d;
            best = o;
        }
    }
    return best;
}

// The original calls this stepAwayFrom, but it walks *towards* the enemy: 20
// units along the dominant axis.
void StepToward(Actor& a, const Actor& t) {
    const int dx = a.pos[0] - t.pos[0], dy = a.pos[1] - t.pos[1];
    if (std::abs(dx) > std::abs(dy)) {
        if (dx > 0) ActorSystem::SetMoveTarget(a, a.pos[0] - 20, a.pos[1]);
        else ActorSystem::SetMoveTarget(a, a.pos[0] + 20, a.pos[1]);
    } else if (dy > 0) {
        ActorSystem::SetMoveTarget(a, a.pos[0], a.pos[1] - 20);
    } else {
        ActorSystem::SetMoveTarget(a, a.pos[0], a.pos[1] + 20);
    }
}

void FaceTowards(Actor& a, const Actor& t) {
    if (a.screenPos[0] < t.screenPos[0] && a.screenPos[1] > t.screenPos[1]) a.facing = 2;
    else if (a.screenPos[0] > t.screenPos[0] && a.screenPos[1] < t.screenPos[1]) a.facing = 1;
    else if (a.screenPos[0] < t.screenPos[0] && a.screenPos[1] < t.screenPos[1]) a.facing = 3;
    else if (a.screenPos[0] > t.screenPos[0] && a.screenPos[1] > t.screenPos[1]) a.facing = 4;
}

// aiThink: pick / drop a target. False means "not attacking this frame".
bool AiThink(Actor& a, CombatHost& host) {
    Actor* enemy = FindNearestEnemy(a, host);
    if (enemy) {
        const int d = Distance(a.pos, enemy->pos);
        if (d <= a.sightRange) {
            if (d >= a.attackRange) {
                if (a.slot != 1) {
                    StepToward(a, *enemy);
                    return false;
                }
            } else {
                a.moveTarget[0] = -1;
                a.target = enemy->weak_from_this();
                a.animState = 4;
                FaceTowards(a, *enemy);
            }
        } else if (!a.target.expired() && a.aiType != 2) {
            a.moveTarget[0] = -1;
            a.target.reset();
            a.animState = 0;
        }
    } else if (!a.target.expired()) {
        a.target.reset();
        a.animState = 0;
    }
    return true;
}

void GrantXp(Actor& a, int victimLevel, CombatHost& host) {
    std::shared_ptr<Actor> hold;
    Actor* who = &a;
    if (Actor* o = Locked(a.owner, hold)) who = o;
    if (who->slot != 1) return;
    const int reward = victimLevel >= 0 && victimLevel <= 25 ? kXpReward[victimLevel] : 0;
    who->xp += reward;
    if (who->level < 25 && who->xp >= kXpForLevel[who->level + 1]) {
        const int s = who->strength, i = who->intelligence, w = who->willpower, ag = who->agility,
                  e = who->endurance, p = who->personality;
        RaiseLevel(*who);
        std::string m = host.GetString(41) + " " + std::to_string(who->level) + ": +" +
                        std::to_string(who->strength - s) + " " + host.GetString(415) + ", +" +
                        std::to_string(who->intelligence - i) + " " + host.GetString(416) + ", +" +
                        std::to_string(who->willpower - w) + " " + host.GetString(417) + ", +" +
                        std::to_string(who->agility - ag) + " " + host.GetString(418) + ", +" +
                        std::to_string(who->endurance - e) + " " + host.GetString(419) + ", +" +
                        std::to_string(who->personality - p) + " " + host.GetString(420);
        RecomputeMax(*who);
        Items::RecalcDerivedStats(*who, host.Script());
        host.ShowMessage(m, 30, 4, 3);
        return;
    }
    host.ShowMessage(std::to_string(reward) + " " + host.GetString(42) + "!!!", 3, 4, 1);
}

// applyDamage. `dot` is damage over time (no armour, no dodge, may have no
// source). True when the victim died.
bool ApplyDamage(int damage, Actor& v, Actor* source, bool crit, bool dot, CombatHost& host) {
    if ((source == nullptr && !dot) || v.invulnerable == 1) return false;
    int dodge = v.dodgeChance + (v.dodgeChance >> 1);
    int block = v.blockChance + (v.blockChance >> 1);
    int soak = ((v.agility + v.armor + v.buffArmor) >> 3) + v.buffDefense;
    if (dot) {
        soak = 0;
        block = -1000;
        dodge = -1000;
    }
    const int dmg = damage - soak;
    int rollDodge = Sign(host.Random() % 100);
    int rollBlock = Sign(host.Random() % 100);
    dodge = dodge * v.dodgeScale / 100;
    if (v.target.expired() && source) v.target = source->weak_from_this();

    if (rollDodge <= dodge) {
        v.floatText = host.GetString(471);
        v.floatKind = 1;
        v.floatTextY = 0;
    } else if (rollBlock <= block) {
        v.floatText = host.GetString(470);
        v.floatKind = 2;
        v.floatTextY = 0;
    } else if (dmg > 0) {
        v.floatKind = 0;
        std::shared_ptr<Actor> hold;
        if (v.slot != 1 && !dot)
            if (Actor* t = Locked(v.target, hold)) v.sightRange = std::max(Distance(v.pos, t->pos), v.sightRange);
        if (source && source->ranged == 0) host.Random();
        v.hp -= dmg;
        v.floatText = (crit ? host.GetString(472) : std::string()) + std::to_string(dmg);
        v.floatTextY = 0;
        v.dead = v.hp <= 0 ? 1 : 0;
        if (v.dead == 1) {
            if (source) {
                source->killTimer = 0;
                GrantXp(*source, v.level, host);
            }
            host.Random();
            v.animState = 6;
            ActorSystem::UpdateSortCell(v);
            if (v.deathScript >= 0) host.Script().RunScript(static_cast<uint8_t>(v.deathScript));
            if (v.dropsLoot == 1) {
                const int loot = host.Script().RollLoot(host.Random());
                if (loot != 0) host.SpawnItem(loot, false, v.footBCell[0], v.footBCell[1]);
            }
        }
    }
    return v.dead == 1;
}

// teleportStep (aiType 2): vanish, then reappear near the player.
bool TeleportStep(Actor& a, CombatHost& host) {
    Actor* player = host.ActorSlot(0);
    if (!player) return false;
    static const int8_t kAround[5][2] = {{-1, 0}, {0, -1}, {0, 0}, {0, 1}, {1, 0}};
    if (a.vanished == 1) {
        if (a.teleportTimer <= -1000 && player->dead == 0) {
            const Grid grid = host.CurrentGrid();
            const std::vector<uint8_t>* floor = host.GroundLayer();
            bool ok = false;
            int x = 0, y = 0;
            for (int tries = 0; floor && !ok && tries < 100; tries++) {
                ok = true;
                x = std::abs(player->pos[0] + host.Random() % 500);
                y = std::abs(player->pos[1] + host.Random() % 500);
                const int cx = x >> 7, cy = y >> 7;
                for (const auto& d : kAround) {
                    const int idx = (cx + d[0]) * grid.height + cy + d[1];
                    if (idx >= 0 && idx < static_cast<int>(floor->size())) {
                        if ((grid.collision && (*grid.collision)[static_cast<size_t>(idx)] != 0) ||
                            (*floor)[static_cast<size_t>(idx)] == 0) {
                            ok = false;
                            break;
                        }
                    } else {
                        ok = false;
                    }
                }
            }
            ActorSystem::SetPosition(a, x, y);
            host.Fx().SpawnFixed(8, a.pos[0], a.pos[1]);
            if (ok) {
                a.vanished = 0;
                return true;
            }
        }
    } else if (player->dead == 0) {
        host.Fx().SpawnFixed(8, a.pos[0], a.pos[1]);
        a.moveTarget[0] = -1;
        a.moveTarget[1] = -1;
        ActorSystem::SetPosition(a, -10000, -10000);
        a.vanished = 1;
    }
    return false;
}

void ClearBuffs(Actor& a, CombatHost& host) {
    a.itemBuffElapsed = 0;
    a.itemBuffDuration = 0;
    a.bonusMaxMp = 0;
    a.buffArmor = 0;
    a.buffAttack = 0;
    a.buffDefense = 0;
    a.buffAttack2 = 0;
    a.dodgeScale = 0;
    a.effectIcon = -1;
    host.Fx().Clear(a.buffFxSlot);
    if (a.bonusMaxMp != 0) {  // (unreachable in the original as well: it was just zeroed)
        a.mp = a.maxMp = a.level * 4 + a.intelligence * 2 + a.bonusMaxMp;
        a.mpRegenInterval = 40000 / std::max(1, a.maxMp);
    }
    if (a.buffStrength != 0) {
        a.buffStrength = 0;
        a.maxHp = a.level * 4 + (a.strength + a.buffStrength) * 2 + a.endurance * 2 + a.bonusMaxHp;
        a.hpRegenInterval = 40000 / std::max(1, a.maxHp);
    }
    Items::RecalcDerivedStats(a, host.Script());
}

void UseSpecialAttack(Actor& a, bool checkMana, CombatHost& host) {
    ScriptInterpreter& sc = host.Script();
    if (a.ranged == 1) {
        if (a.slot == 1) {
            a.idleTimer = 500;
            a.animState = 7;
        }
        host.Fx().SpawnFromActor(11, a.facing, a);
        return;
    }
    if (!a.special) return;
    const int* sp = a.special;
    int power;
    if (a.level >= sp[10]) {
        if (a.mp < sp[13] && checkMana) return;
        power = sp[5];
        a.mp -= sp[13];
    } else if (a.level >= sp[9]) {
        if (a.mp < sp[12] && checkMana) return;
        power = sp[4];
        a.mp -= sp[12];
    } else {
        if (a.mp < sp[11] && checkMana) return;
        power = sp[3];
        a.mp -= sp[11];
    }

    auto glow = [&]() {
        host.Fx().Clear(a.buffFxSlot);
        a.buffFxSlot = static_cast<int8_t>(host.Fx().SpawnFromActor(9, 0, a, 5000));
    };
    auto hitEnemiesInRange = [&](auto&& fn) {
        for (int i = 0; i < host.SlotCount(); i++) {
            Actor* o = host.ActorSlot(i);
            if (o && o != &a && o->team != a.team && Distance(a.pos, o->pos) <= sp[14]) fn(*o);
        }
    };

    switch (sp[2]) {
        case 0:  // armour buff
            a.buffTimer = sp[6];
            a.buffArmor = power;
            a.effectIcon = -48;
            glow();
            break;
        case 1:  // attack buff
            a.buffTimer = sp[6];
            a.buffAttack2 = power;
            a.effectIcon = -50;
            glow();
            break;
        case 2:  // summon, then poison like case 4
        case 4:  // poison cloud
            if (sp[2] == 2) {
                if (auto s = a.summon.lock()) host.RemoveActorSlot(s->slot - 1);
                Actor* summon = host.SpawnFreeActor("/oh_scamp.cml", a.pos[0], a.pos[1], a.spawnRow);
                if (summon) {
                    a.summon = summon->weak_from_this();
                    summon->owner = a.weak_from_this();
                    summon->dropsLoot = 0;
                }
            }
            hitEnemiesInRange([&](Actor& o) { ApplyPoison(a, o, power, sp[6], host); });
            break;
        case 3:  // magic: area hit, heal, or a bolt depending on the name id
            if (sp[1] == 61618) {
                hitEnemiesInRange([&](Actor& o) { ApplyMagicHit(a, o, power, host); });
            } else if (sp[1] == 61619) {
                host.Fx().SpawnFromActor(8, 0, a);
                a.hp = std::min(a.maxHp, a.hp + std::abs(power));
            } else {
                host.Fx().SpawnFromActor(0, a.facing, a);
            }
            break;
        case 5:  // dodge buff
            a.buffTimer = sp[6];
            a.dodgeScale = power + 100;
            a.effectIcon = -48;
            glow();
            break;
        case 6:  // cure
            host.Fx().SpawnFromActor(8, 0, a);
            a.dotRemaining = 0;
            a.dotTick = 0;
            a.effectIcon = -1;
            break;
        default: break;
    }
    Items::RecalcDerivedStats(a, sc);
}

}  // namespace

int Distance(const int a[2], const int b[2]) {
    int dx = std::abs(a[0] - b[0]), dy = std::abs(a[1] - b[1]);
    int lo = std::min(dx, dy), hi = std::max(dx, dy);
    int d = hi * 1007 + lo * 441;
    if (hi < (lo << 4)) d -= hi * 40;
    return std::abs((d + 512) >> 10);
}

bool Attack(Actor& a, Actor& victim, bool melee, CombatHost& host) {
    if (a.slot != 1 && (a.special || a.ranged == 1) && melee) {
        UseSpecialAttack(a, false, host);
        if (a.aiType == 3) {
            a.special = nullptr;
            a.attackRange >>= 1;
        } else if (a.aiType == 2 && a.teleportTimer <= 0 && TeleportStep(a, host)) {
            a.teleportTimer = static_cast<int16_t>(std::abs(host.Random()) % 2000 + 2000);
        }
        return false;
    }
    int dmg = ((a.strength + a.buffStrength + a.weaponPower) >> 1) + a.buffAttack + a.buffAttack2;
    const int roll = host.Random() % 16;
    bool crit = false;
    dmg = dmg * a.attackRating / 100;
    if (a.special) {
        if (a.level >= a.special[10]) dmg = a.special[5];
        else if (a.level >= a.special[9]) dmg = a.special[4];
        else dmg = a.special[3];
        if (a.slot != 1 && a.special[2] != 4) dmg >>= 1;
    }
    if (Sign(roll) == 1) {
        dmg = std::max(victim.hp >> 2, dmg << 1);
        crit = true;
    }
    return ApplyDamage(dmg, victim, &a, crit, false, host);
}

void ApplyPoison(Actor& source, Actor& victim, int damage, int durationMs, CombatHost& host) {
    victim.dotDamage = static_cast<int8_t>(damage);
    victim.dotRemaining = durationMs;
    victim.dotSource = source.weak_from_this();
    victim.effectIcon = -47;
    host.Fx().SpawnFromActor(8, 0, victim);
    ApplyDamage(damage, victim, &source, false, true, host);
}

void ApplyMagicHit(Actor& source, Actor& victim, int damage, CombatHost& host) {
    host.Fx().SpawnFromActor(10, 0, victim);
    ApplyDamage(damage, victim, &source, false, false, host);
}

int CheckZoneTiles(Actor& a, const std::vector<int8_t>& zones, int gridHeight, CombatHost& host, bool melee) {
    a.zoneId = -1;
    if (zones.empty()) return a.zoneId;
    const int idx[3] = {a.cell[0] * gridHeight + a.cell[1], a.footBCell[0] * gridHeight + a.footBCell[1],
                        a.footCCell[0] * gridHeight + a.footCCell[1]};
    for (int i : idx) {
        if (i < 0 || i >= static_cast<int>(zones.size())) return -1;
        if (zones[static_cast<size_t>(i)] >= 0) {  // 255 is stored as -1
            a.zoneId = zones[static_cast<size_t>(i)];
            return a.zoneId;
        }
    }
    if (!melee) return -1;
    if (a.slot == 1 && a.hasAim) ActorSystem::FaceWorldDir(a, a.aimX, a.aimY);  // bolts and arrows fly where you aim
    if (a.ranged == 0 && !a.special) {
        a.idleTimer = 500;
        a.animState = 4;
    }
    if (a.attackTimer >= a.attackInterval) {
        a.attackTimer = 0;
        if (!a.special && a.ranged != 1) {
            AiThink(a, host);
            std::shared_ptr<Actor> hold;
            if (Actor* t = Locked(a.target, hold)) {
                FaceTowards(a, *t);
                if (Attack(a, *t, true, host)) {
                    a.target.reset();
                    a.dotSource.reset();
                    a.animState = 0;
                }
            }
        } else {
            UseSpecialAttack(a, true, host);
        }
    }
    return -1;
}

void LevelUpTo(Actor& a, int level, ScriptInterpreter& sc) {
    while (a.level < level) {
        RaiseLevel(a);
        RecomputeMax(a);
        Items::RecalcDerivedStats(a, sc);
    }
}

void Update(Actor& a, int dt, bool aiAttacksPlayer, CombatHost& host) {
    ScriptInterpreter& sc = host.Script();
    a.killTimer += dt;
    a.attackTimer += dt;
    ActorSystem::Update(a, dt);  // animation timer + walking to moveTarget

    if (a.dead != 0) {
        if (a.deathTimer >= 250) {
            host.RemoveActorSlot(a.slot - 1);  // `a` may be gone after this
            return;
        }
        a.deathTimer += dt;
        return;
    }

    if (a.dotRemaining > 0) {
        a.dotRemaining -= dt;
        a.dotTick -= dt;
        if (a.dotTick <= 0) {
            host.Fx().SpawnFromActor(8, 0, a);
            a.dotTick = 1000;
            std::shared_ptr<Actor> hold;
            ApplyDamage(a.dotDamage, a, Locked(a.dotSource, hold), false, true, host);
            if (a.dead != 0) return;
        }
    } else if (a.effectIcon == -47) {
        a.effectIcon = -1;
    }

    if (a.aiType == 2) a.teleportTimer = static_cast<int16_t>(a.teleportTimer - dt);

    if (a.slot == 1) {
        if (a.hp < a.maxHp) {
            a.hpRegenTimer += dt;
            if (a.hpRegenTimer >= a.hpRegenInterval) {
                if (a.hp < a.maxHp) a.hp++;
                a.hpRegenTimer = 0;
                Items::RecalcDerivedStats(a, sc);
            }
        }
        if (a.mp < a.maxMp) {
            a.mpRegenTimer += dt;
            if (a.mpRegenTimer >= a.mpRegenInterval) {
                if (a.mp < a.maxMp) a.mp++;
                a.mpRegenTimer = 0;
                Items::RecalcDerivedStats(a, sc);
            }
        }
        if (a.itemBuffDuration > 0) {
            if (a.itemBuffElapsed >= a.itemBuffDuration) ClearBuffs(a, host);
            a.itemBuffElapsed += dt;
        }
    } else if (a.aiActive == 1 && AiThink(a, host) && !a.target.expired() && a.attackTimer >= a.attackInterval) {
        std::shared_ptr<Actor> hold;
        Actor* t = Locked(a.target, hold);
        if (t && (aiAttacksPlayer || t->slot != 1) && Attack(a, *t, true, host)) {
            a.target.reset();
            a.dotSource.reset();
            a.animState = 0;
        }
        a.attackTimer = 0;
    }

    if (!a.floatText.empty()) {
        a.floatTextTimer += dt;
        if (a.floatTextTimer > 50) {
            a.floatTextY -= 2;
            a.floatTextColor -= a.floatTextShadow;
            if (a.floatTextColor <= 0 || std::abs(a.floatTextStartY - a.floatTextY) > 20) {
                a.floatTextColor = 0;
                a.floatTextY = 0;
                a.floatTextStartY = 0;
                a.floatText.clear();
            }
            a.floatTextTimer = 0;
        }
    }

    if (a.buffTimer > 0) {
        a.buffTimer -= dt;
        if (a.buffTimer <= 0) ClearBuffs(a, host);
    }
}

}  // namespace Combat
}  // namespace oblivion
