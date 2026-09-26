#include "world/items.h"

#include <algorithm>
#include <initializer_list>
#include <utility>

namespace oblivion {
namespace Items {

namespace {

const int kZeroRow[21] = {};

const int* Row(ScriptInterpreter& sc, int table, int index) {
    const int* r = sc.GetRow(table, index);
    return r ? r : kZeroRow;
}

// The class rating tables set a value only when the level is exactly a step's
// level (the values persist between levels, so the net effect is a step curve).
using Step = std::pair<int, int>;
void At(int level, int& field, std::initializer_list<Step> steps) {
    for (const Step& s : steps)
        if (level == s.first) field = s.second;
}

void Dodge(Actor& a) { At(a.level, a.dodgeChance, {{1, 5}, {7, 10}, {15, 15}}); }
void Block(Actor& a) { At(a.level, a.blockChance, {{1, 0}, {5, 3}, {17, 10}}); }
void DefenseA(Actor& a) { At(a.level, a.defenseRating, {{1, 100}, {10, 115}, {20, 130}}); }
void DefenseB(Actor& a) { At(a.level, a.defenseRating, {{1, 100}, {7, 115}, {17, 130}}); }
void Attack8(Actor& a) { At(a.level, a.attackRating, {{1, 100}, {8, 110}, {18, 125}}); }
void Attack7(Actor& a) { At(a.level, a.attackRating, {{1, 100}, {7, 110}, {16, 125}}); }
void Attack6(Actor& a) { At(a.level, a.attackRating, {{1, 100}, {6, 110}, {15, 125}}); }
void Attack5(Actor& a) { At(a.level, a.attackRating, {{1, 100}, {5, 110}, {15, 125}}); }

int CategoryBase(int category) { return category << 8; }

}  // namespace

void RecalcDerivedStats(Actor& a, ScriptInterpreter& sc) {
    const int* w = Row(sc, 4, a.weapon);
    const int type = w[2];
    a.weaponPower = w[3];
    a.armor = 0;
    for (int worn : a.wornArmor)
        if (worn != -1) a.armor += Row(sc, 1, worn)[4];

    switch (a.classId) {
        case 1:
            Dodge(a);
            if (type == 4) {
                Attack7(a);
            } else if (a.weapon == 0) {
                Attack5(a);
                At(a.level, a.defenseRating, {{1, 110}, {5, 125}, {15, 140}});
            }
            break;
        case 2:
            Dodge(a);
            DefenseA(a);
            if (type == 2) Attack8(a);
            else if (type == 3) Attack6(a);
            break;
        case 3:
            Block(a);
            DefenseA(a);
            if (type == 1 || type == 2) Attack8(a);
            else if (type == 3) Attack6(a);
            else if (a.weapon == 0) Attack5(a);
            else if (type == 0) Attack8(a);
            break;
        case 4:
            DefenseA(a);
            if (type == 1) Attack8(a);
            else if (type == 4) Attack7(a);
            break;
        case 5:
            Block(a);
            DefenseA(a);
            DefenseB(a);  // a second set of defence steps with its own levels
            if (type == 1 || type == 2 || type == 0) Attack8(a);
            break;
        case 6:
            Block(a);
            DefenseA(a);
            if (type == 2) Attack8(a);
            else if (type == 4) Attack7(a);
            break;
        case 7:
            Dodge(a);
            DefenseA(a);
            break;
        case 8:
            DefenseA(a);
            DefenseB(a);
            if (type == 1 || type == 2 || type == 0) Attack8(a);
            break;
        default:
            break;
    }
}

bool CanUseItem(const Actor& a, int category, const int* row, ScriptInterpreter& sc) {
    if (!row || a.classId == -1) return false;
    if (category == 0) {
        switch (row[2]) {
            case 1: return sc.ClassAllows(a.classId, 5);
            case 2: return sc.ClassAllows(a.classId, 6);
            case 3: return sc.ClassAllows(a.classId, 7);
            case 4: return sc.ClassAllows(a.classId, 8);
            case 0: return sc.ClassAllows(a.classId, 14);
            default: break;
        }
    } else if (category == 1) {
        switch (row[2]) {
            case 2: return sc.ClassAllows(a.classId, 4);
            case 1: return sc.ClassAllows(a.classId, 3);
            case 0: return sc.ClassAllows(a.classId, 1);
            default: break;
        }
    }
    return true;
}

void EquipArmor(Actor& a, const int* row, ScriptInterpreter& sc) {
    if (row && CanUseItem(a, 1, row, sc) && row[3] >= 0 && row[3] < 8) {
        a.wornArmor[row[3]] = row[0];
        RecalcDerivedStats(a, sc);
    }
}

bool HasArmor(const Actor& a, int armorId) {
    for (int w : a.wornArmor)
        if (w == armorId) return true;
    return false;
}

bool IsWeaponRowEquipped(const Actor& a, const int* row, bool spell) {
    if (a.altSpecial && spell) return row[0] == a.altSpecial[0];
    return !spell ? a.weapon == row[0] : false;
}

void AddItem(Actor& a, int category, const int* row, ScriptInterpreter& sc, bool forced) {
    int slot = 0;
    const int n = 255;
    while (slot < n && a.inventory[slot] != 0) slot++;
    if (slot >= n || !row) return;
    switch (category) {
        case 0:
            a.inventory[slot] = CategoryBase(0) | row[0];
            if ((a.weapon == 0 && a.special == nullptr && CanUseItem(a, 0, row, sc)) || forced) a.weapon = static_cast<int8_t>(row[0]);
            break;
        case 1:
            if (a.wornArmor[row[3]] == -1 || forced) EquipArmor(a, row, sc);
            a.inventory[slot] = CategoryBase(1) | row[0];
            return;
        case 2:
            a.inventory[slot] = CategoryBase(2) | row[0];
            if (row[5] == 0) {
                if (!a.hpPotion && row[2] > 0) {
                    a.hpPotion = row;
                    return;
                }
                if (!a.mpPotion && row[3] > 0) {
                    a.mpPotion = row;
                    return;
                }
            }
            break;
        default:
            break;
    }
}

static void ReselectBestWeapon(Actor& a, ScriptInterpreter& sc) {
    const int* best = nullptr;
    for (int i = 0; i < 255 && a.inventory[i] != 0; i++) {
        if (a.inventory[i] <= 255) {
            const int* w = Row(sc, 4, a.inventory[i] & 0xFF);
            if (CanUseItem(a, 0, w, sc) && (!best || w[3] > best[3])) best = w;
        }
    }
    if (best) {
        a.weapon = static_cast<int8_t>(best[0]);
        a.ranged = best[2] == 4 ? 1 : 0;
    }
}

void RemoveItem(Actor& a, int category, const int* row, ScriptInterpreter& sc) {
    if (!row) return;
    bool reselect = false;
    const int packed = CategoryBase(category) | row[0];
    if (category == 0 && a.weapon == row[0]) {
        a.weapon = 0;
        a.ranged = 0;
        reselect = true;
    }
    for (int i = 0; i < 255 && a.inventory[i] != 0; i++) {
        if (a.inventory[i] == packed) {
            for (int j = i; j < 254; j++) a.inventory[j] = a.inventory[j + 1];
            a.inventory[254] = 0;
            break;
        }
    }
    if (reselect) ReselectBestWeapon(a, sc);
    RecalcDerivedStats(a, sc);
}

void UseConsumable(Actor& a, const int* row, ScriptInterpreter& sc, const Strings& s) {
    if (!row) return;
    if (row[5] == 0) {
        if (sc.ItemName(row[1]) == s.Get(158)) {
            a.hp = std::min(a.maxHp, a.hp + row[2]);
            a.mp = std::min(a.maxMp, a.mp + row[3]);
            RemoveItem(a, 2, row, sc);
            RecalcDerivedStats(a, sc);
        }
        if (row[2] > 0) {
            a.hpPotion = row;
            return;
        }
        if (row[3] > 0) {
            a.mpPotion = row;
            return;
        }
        if (row[4] > 0) {
            a.dotRemaining = 0;
            a.dotTick = 0;
            a.effectIcon = -1;
            RemoveItem(a, 2, row, sc);
            RecalcDerivedStats(a, sc);
            return;
        }
    } else {
        if (row[3] > 0) {
            a.bonusMaxMp = row[3];
            a.mp = a.maxMp = a.level * 4 + a.intelligence * 2 + a.bonusMaxMp;
            a.mpRegenInterval = 40000 / std::max(1, a.maxMp);
        }
        if (row[6] > 0) a.buffAttack = row[6];
        if (row[7] > 0) a.buffArmor = row[7];
        if (row[8] > 0) a.buffDefense = row[8];
        if (row[10] > 0) a.buffAttack2 = row[10];
        if (row[11] > 0) {
            a.buffStrength = row[11];
            a.maxHp = a.level * 4 + (a.strength + a.buffStrength) * 2 + a.endurance * 2 + a.bonusMaxHp;
            a.hpRegenInterval = 40000 / std::max(1, a.maxHp);
        }
        if (row[4] > 0) {
            a.dotRemaining = 0;
            a.dotTick = 0;
            a.effectIcon = -1;
        }
        a.itemBuffElapsed = 0;
        a.itemBuffDuration = row[5];
        RemoveItem(a, 2, row, sc);
        RecalcDerivedStats(a, sc);
    }
}

void QuaffPotion(Actor& a, bool health, ScriptInterpreter& sc) {
    const int* pot = health ? a.hpPotion : a.mpPotion;
    if (!pot) return;
    if (health ? a.hp >= a.maxHp : a.mp >= a.maxMp) return;
    RemoveItem(a, 2, pot, sc);
    if (health) a.hp = std::min(a.maxHp, a.hp + pot[2]);
    else a.mp = std::min(a.maxMp, a.mp + pot[3]);
    RecalcDerivedStats(a, sc);
    (health ? a.hpPotion : a.mpPotion) = nullptr;
    // Pick the next potion of the kind out of the inventory.
    for (int i = 0; i < 255; i++) {
        const int cat = (a.inventory[i] >> 8) & 0xFF, id = a.inventory[i] & 0xFF;
        if (cat != 2) continue;
        const int* r = Row(sc, 2, id);
        if (r[health ? 2 : 3] > 0) (health ? a.hpPotion : a.mpPotion) = r;
    }
}

bool EquipFromString(Actor& a, const std::string& textIn, ScriptInterpreter& sc, const Strings& s) {
    std::string text = textIn;
    const std::string spell = s.Get(304), bow = s.Get(400), weapon = s.Get(305);
    if (text.compare(0, spell.size(), spell) == 0) {
        text = text.substr(spell.size());
        a.altSpecial = sc.FindByName(text);
        a.ranged = 0;
        if (a.special) {
            a.special = a.altSpecial;
            UpdateSpecialIcon(a);
        }
        return true;
    }
    if (text.compare(0, bow.size(), bow) == 0) {
        text = text.substr(bow.size());
        a.ranged = 1;
    } else {
        text = text.substr(std::min(text.size(), weapon.size()));
        a.ranged = 0;
    }
    if (const int* w = sc.FindByName(text)) a.weapon = static_cast<int8_t>(w[0]);
    return false;
}

void UpdateSpecialIcon(Actor& a) {
    if (!a.special) {
        a.attackIcon = -45;
        return;
    }
    switch (a.special[2]) {
        case 0: a.attackIcon = -48; break;
        case 1: a.attackIcon = -50; break;
        case 2: a.attackIcon = -46; break;
        case 3:
            if (a.special[1] == 61618) a.attackIcon = -44;
            else if (a.special[1] == 61619) a.attackIcon = -43;
            else a.attackIcon = -50;
            break;
        case 4: a.attackIcon = -47; break;
        case 5: a.attackIcon = -48; break;
        case 6: a.attackIcon = -43; break;
        default: break;
    }
}

void SetClass(Actor& a, int classId, bool fromSave, ScriptInterpreter& sc) {
    a.classId = static_cast<int8_t>(classId);
    if (classId == 4) a.ranged = 1;
    a.classRow = sc.GetRow(5, classId);
    a.classList = sc.tables().classLists[classId];
    if (!fromSave && a.classRow) {
        AddItem(a, 0, sc.GetRow(4, a.classRow[4]), sc);
        AddItem(a, 1, sc.GetRow(1, a.classRow[5]), sc);
        const int* c = a.classRow;
        a.strength = c[7];
        a.intelligence = c[8];
        a.willpower = c[9];
        a.agility = c[10];
        a.speed = c[6];
        a.endurance = c[11];
        a.personality = c[12];
        a.attackRange = c[13];
        a.sightRange = c[14];
    }
    RecalcDerivedStats(a, sc);
}

void InitFromTemplate(Actor& a, const int* row, ScriptInterpreter& sc) {
    if (!row) return;
    a.spawnRow = row;
    a.level = static_cast<int8_t>(row[2]);
    if (a.slot != 1) {  // monsters and NPCs; the player's attributes come from its class
        a.strength = row[3];
        a.intelligence = row[4];
        a.willpower = row[5];
        a.agility = row[6];
        a.speed = row[7];
        a.endurance = row[8];
        a.personality = row[9];
        a.sightRange = row[14];
        a.attackRange = row[15];
        a.weapon = static_cast<int8_t>(row[10]);
        a.aiType = static_cast<int8_t>(row[18]);
        const int armorId = row[11];
        a.special = row[19] >= 0 && row[19] < 10 ? sc.tables().specials[row[19]] : nullptr;
        a.ranged = a.aiType == 4 ? 1 : 0;
        if (row[20] > 0) a.attackInterval = row[20] * 1000;
        if (a.ranged == 1 || a.aiType == 0) a.special = nullptr;
        if (a.weapon > 0) AddItem(a, 0, sc.GetRow(4, a.weapon), sc);
        if (armorId > 0) AddItem(a, 1, sc.GetRow(1, armorId), sc);
    }
    a.team = static_cast<int8_t>(row[13]);
    a.hp = a.maxHp = a.level * 4 + (a.strength + a.buffStrength) * 2 + a.endurance * 2 + a.bonusMaxHp;
    a.hpRegenInterval = 40000 / std::max(1, a.maxHp);
    a.mp = a.maxMp = a.level * 4 + a.intelligence * 2 + a.bonusMaxMp;
    a.mpRegenInterval = 40000 / std::max(1, a.maxMp);
    if (a.sightRange == 0) a.sightRange = 300;
    if (a.attackRange == 0) a.attackRange = 200;
    RecalcDerivedStats(a, sc);
}

void SetStat(Actor& a, int stat, int value, ScriptInterpreter& sc) {
    ActorSystem::SetStat(a, stat, value);
    if (stat == 19 && value >= 0 && value < 10) a.special = sc.tables().specials[value];
    if (stat == 18 && (a.ranged == 1 || a.aiType == 0)) a.special = nullptr;
    a.hpRegenInterval = 40000 / std::max(1, a.maxHp);
    a.mpRegenInterval = 40000 / std::max(1, a.maxMp);
    if (a.weapon > 0) a.weaponPower = Row(sc, 4, a.weapon)[3];
    RecalcDerivedStats(a, sc);
}

void Revive(Actor& a, ScriptInterpreter& sc) {
    a.sortCell[0] = a.sortCell[1] = 0;
    a.enterScript = a.leaveScript = a.zoneId = -1;
    a.dead = 0;
    a.statusIcon = -1;
    a.target.reset();
    a.deathTimer = 0;
    a.floatText.clear();
    a.floatTextY = 0;
    a.floatTextColor = 0xFF0000;
    a.animState = 0;
    a.moveTarget[0] = a.moveTarget[1] = -1;
    a.dotRemaining = 0;
    a.dotTick = 0;
    a.dotSource.reset();
    a.hp = a.maxHp = a.level * 4 + (a.strength + a.buffStrength) * 2 + a.endurance * 2 + a.bonusMaxHp;
    a.hpRegenInterval = 40000 / std::max(1, a.maxHp);
    a.mp = a.maxMp = a.level * 4 + a.intelligence * 2 + a.bonusMaxMp;
    a.mpRegenInterval = 40000 / std::max(1, a.maxMp);
    RecalcDerivedStats(a, sc);
    ActorSystem::UpdateSortCell(a);
    ActorSystem::UpdateCells(a);
}

}  // namespace Items
}  // namespace oblivion
