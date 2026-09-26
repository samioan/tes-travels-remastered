#pragma once
#include <string>
#include <vector>

#include "assets/lang.h"
#include "script/interpreter.h"

namespace oblivion {

// Game.getHelpClasses / Weapons / Armor / Spells / Items: one page (a list of
// lines) per class / weapon / armour / spell / consumable, built from the
// script record tables and the language strings. The original's pages are
// fixed-size arrays that end at the first null; here a page is exactly its
// non-null lines.
using HelpPages = std::vector<std::vector<std::string>>;

HelpPages BuildHelpClasses(ScriptInterpreter& script, const Strings& s);
HelpPages BuildHelpWeapons(ScriptInterpreter& script, const Strings& s);
HelpPages BuildHelpArmor(ScriptInterpreter& script, const Strings& s);
HelpPages BuildHelpSpells(ScriptInterpreter& script, const Strings& s);
HelpPages BuildHelpItems(ScriptInterpreter& script, const Strings& s);

// ScriptInterpreter.classAllows: does class `classId` list item type `type`?
bool ClassAllows(const ScrTables& t, int classId, int type);

}  // namespace oblivion
