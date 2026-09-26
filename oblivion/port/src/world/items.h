#pragma once
#include <string>

#include "assets/lang.h"
#include "script/interpreter.h"
#include "world/actor.h"

namespace oblivion {

// The equipment / consumable / class half of ActorSystem (src/ActorSystem.java):
// inventory slots, equipping, potions, derived combat numbers. Item `category`
// is 0 weapon, 1 armour, 2 consumable; `row` is the item's script-table row.
// Combat, AI and levelling arrive with the rest of M7.
namespace Items {

// recalcDerivedStats: weapon power, worn armour total, and the class rating
// steps (dodge / block / attack / defence, which change only at set levels).
void RecalcDerivedStats(Actor& a, ScriptInterpreter& sc);

// canUseItem: does the actor's class allow this weapon (type 0..4) / armour (weight 0..2)?
bool CanUseItem(const Actor& a, int category, const int* row, ScriptInterpreter& sc);

void AddItem(Actor& a, int category, const int* row, ScriptInterpreter& sc, bool forced = false);
void RemoveItem(Actor& a, int category, const int* row, ScriptInterpreter& sc);
void EquipArmor(Actor& a, const int* row, ScriptInterpreter& sc);
bool HasArmor(const Actor& a, int armorId);
// isWeaponRowEquipped: `spell` selects the alternative-special slot.
bool IsWeaponRowEquipped(const Actor& a, const int* row, bool spell);

// useConsumable (potions go to the quick slots; buffs apply and are used up).
void UseConsumable(Actor& a, const int* row, ScriptInterpreter& sc, const Strings& s);
// quaffPotion: drink the quick health (true) or magicka potion.
void QuaffPotion(Actor& a, bool health, ScriptInterpreter& sc);
// equipFromString: an inventory line ("Weapon: X" / "Bow: X" / "Spell: X"); true for a spell.
bool EquipFromString(Actor& a, const std::string& text, ScriptInterpreter& sc, const Strings& s);

// setClass (player): class row/lists, starting kit and attributes.
void SetClass(Actor& a, int classId, bool fromSave, ScriptInterpreter& sc);
// initFromTemplate: level, attributes, weapon/armour of a monster-type row.
void InitFromTemplate(Actor& a, const int* row, ScriptInterpreter& sc);
// setStat: ActorSystem::SetStat plus specials, weapon power and derived stats.
void SetStat(Actor& a, int stat, int value, ScriptInterpreter& sc);
// updateSpecialIcon: HUD attack icon for the active special.
void UpdateSpecialIcon(Actor& a);
// The character-sheet refill used by revive.
void Revive(Actor& a, ScriptInterpreter& sc);

}  // namespace Items
}  // namespace oblivion
