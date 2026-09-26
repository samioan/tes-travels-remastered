#include "game/help_pages.h"

#include <cstdlib>

namespace oblivion {

namespace {

std::string N(int v) { return std::to_string(v); }

// ScriptInterpreter.getSkillName: skill ids 0..14 are language strings 523..537.
std::string SkillName(const Strings& s, int id) { return id >= 0 && id <= 14 ? s.Get(523 + id) : std::string(); }

}  // namespace

bool ClassAllows(const ScrTables& t, int classId, int type) {
    if (classId < 0 || classId >= 9) return false;
    for (int v : t.classItemTypes[classId])
        if (v == type) return true;
    return false;
}

HelpPages BuildHelpClasses(ScriptInterpreter& sc, const Strings& s) {
    const ScrTables& t = sc.tables();
    HelpPages pages;
    for (int c = 1; c < 9; c++) {
        std::vector<std::string> p;
        p.push_back(s.Get(481) + sc.ItemName(t.classBase[c][1]));
        const int attr[6][2] = {{415, 7}, {416, 8}, {417, 9}, {418, 10}, {419, 11}, {420, 12}};
        for (const auto& a : attr) p.push_back(s.Get(a[0]) + ": " + N(t.classBase[c][a[1]] * 3));
        p.push_back(s.Get(305) + sc.ItemName(t.weapons[t.classBase[c][4]][1]));
        p.push_back(s.Get(499) + sc.ItemName(t.armors[t.classBase[c][5]][1]));
        p.push_back(s.Get(538));
        for (int i = 0; i < 15 && t.classItemTypes[c][i] != -1; i++) p.push_back("   " + SkillName(s, t.classItemTypes[c][i]));
        p.push_back(s.Get(539));
        for (int i = 0; i < 9; i++) {
            if (t.classLists[c][i] == -1) {
                if (i == 0) p.push_back("   " + s.Get(572));
                break;
            }
            p.push_back("   " + sc.ItemName(t.specials[t.classLists[c][i]][1]));
        }
        pages.push_back(p);
    }
    return pages;
}

HelpPages BuildHelpItems(ScriptInterpreter& sc, const Strings& s) {
    const ScrTables& t = sc.tables();
    HelpPages pages;
    for (int i = 1; i < 11; i++) {
        const int* c = t.consumables[i];
        std::vector<std::string> p;
        p.push_back(s.Get(481) + sc.ItemName(c[1]));
        p.push_back(s.Get(484) + N(c[13]));
        p.push_back(s.Get(485) + N(c[13] >> 2));
        if (c[2] > 0) p.push_back(s.Get(496) + N(c[2]));
        if (c[3] > 0) p.push_back(s.Get(497) + N(c[3]));
        if (c[6] > 0) p.push_back(s.Get(498) + N(c[6]));
        if (c[7] > 0) p.push_back(s.Get(499) + N(c[7]));
        if (c[8] > 0) p.push_back(s.Get(500) + N(c[8]));
        if (c[10] > 0) p.push_back(s.Get(501) + N(c[10]));
        if (c[5] > 0) p.push_back(s.Get(502) + N(c[5] / 1000) + s.Get(521));
        pages.push_back(p);
    }
    return pages;
}

HelpPages BuildHelpSpells(ScriptInterpreter& sc, const Strings& s) {
    const ScrTables& t = sc.tables();
    HelpPages pages;
    for (int i = 1; i < 10; i++) {
        const int* sp = t.specials[i];
        std::string kind, target;
        switch (sp[2]) {
            case 0: case 1: kind = s.Get(437); break;
            case 3: kind = s.Get(439); break;
            case 6: kind = s.Get(503); break;
            case 5: kind = s.Get(433); break;
            case 4: kind = s.Get(504); break;
            case 2: kind = s.Get(505); break;
            default: break;
        }
        if (sp[7] >= 0 && sp[7] <= 4) target = s.Get(506 + sp[7]);
        pages.push_back({s.Get(481) + sc.ItemName(sp[1]),
                         s.Get(511) + kind,
                         s.Get(512),
                         s.Get(513) + N(sp[8]),
                         s.Get(514) + N(sp[9]),
                         s.Get(515) + N(sp[10]),
                         s.Get(516),
                         s.Get(513) + N(std::abs(sp[3])),
                         s.Get(514) + N(std::abs(sp[4])),
                         s.Get(515) + N(std::abs(sp[5])),
                         s.Get(517),
                         s.Get(513) + N(sp[11]),
                         s.Get(514) + N(sp[12]),
                         s.Get(515) + N(sp[13]),
                         s.Get(518) + N(sp[6] / 1000) + s.Get(521),
                         s.Get(519) + target,
                         s.Get(520) + N(sp[14])});
    }
    return pages;
}

HelpPages BuildHelpArmor(ScriptInterpreter& sc, const Strings& s) {
    const ScrTables& t = sc.tables();
    HelpPages pages;
    for (int i = 1; i < 42; i++) {
        const int* a = t.armors[i];
        // Slot names are language strings 28..35 in body-part order 0,1,2,3,4,5,6,7.
        static const int kSlotString[8] = {28, 29, 30, 31, 32, 33, 34, 35};
        const std::string slot = a[3] >= 0 && a[3] <= 7 ? s.Get(kSlotString[a[3]]) : std::string();
        std::string weight;
        int need = 1;
        if (a[2] == 2) { weight = s.Get(487); need = 4; }
        else if (a[2] == 1) { weight = s.Get(488); need = 3; }
        else if (a[2] == 0) { weight = s.Get(489); need = 1; }
        std::vector<std::string> p = {s.Get(481) + sc.ItemName(a[1]), s.Get(482) + weight, s.Get(490) + slot,
                                      s.Get(483) + N(a[4]), s.Get(484) + N(a[9]), s.Get(485) + N(a[9] >> 2),
                                      s.Get(486)};
        for (int c = 0; c < 9; c++)
            if (ClassAllows(t, t.classBase[c][0], need)) p.push_back("   " + sc.ItemName(t.classBase[c][1]));
        pages.push_back(p);
    }
    return pages;
}

HelpPages BuildHelpWeapons(ScriptInterpreter& sc, const Strings& s) {
    const ScrTables& t = sc.tables();
    HelpPages pages;
    for (int i = 1; i < 37; i++) {
        const int* w = t.weapons[i];
        std::string kind;
        int need = 14;
        switch (w[2]) {
            case 0: kind = s.Get(476); need = 14; break;
            case 1: kind = s.Get(477); need = 5; break;
            case 4: kind = s.Get(478); need = 8; break;
            case 2: kind = s.Get(479); need = 6; break;
            case 3: kind = s.Get(480); need = 7; break;
            default: break;
        }
        std::vector<std::string> p = {s.Get(481) + sc.ItemName(w[1]), s.Get(482) + kind, s.Get(483) + N(w[3]),
                                      s.Get(484) + N(w[7]), s.Get(485) + N(w[7] >> 2), s.Get(486)};
        for (int c = 0; c < 9; c++)
            if (ClassAllows(t, t.classBase[c][0], need)) p.push_back("   " + sc.ItemName(t.classBase[c][1]));
        pages.push_back(p);
    }
    return pages;
}

}  // namespace oblivion
