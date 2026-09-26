#include "assets/scr.h"

#include <cstring>
#include <stdexcept>

namespace oblivion {

namespace {

struct Cursor {
    const std::vector<uint8_t>& d;
    size_t p;
    int Peek() const {
        if (p >= d.size()) throw std::runtime_error("scr: read past end");
        return d[p];
    }
    int U8() { int v = Peek(); p++; return v; }
    int S8() { return static_cast<int8_t>(U8()); }
    int U16() { int h = U8(); return (h << 8) | U8(); }
    int U24() { int h = U8(); int m = U8(); return (h << 16) | (m << 8) | U8(); }
    std::string Text(size_t n) {
        if (p + n > d.size()) throw std::runtime_error("scr: string past end");
        std::string s(d.begin() + p, d.begin() + p + n);
        p += n;
        return s;
    }
};

// How the fields of one table's records are stored (mirrors
// ScriptInterpreter's parseXxxRecord methods).
struct TableRule {
    int cols;       // row width
    int plainStr;   // field holding an always-literal string, or -1
    bool strOrId;   // field 1 is a literal string or a 0xF0xx localized id
    int shorts[3];  // 16-bit fields (-1 = unused slot)
    int i24;        // 24-bit field or -1
    int flag;       // field with no data byte (value 1) or -1
    bool isSigned;  // default one-byte fields are signed
};

const TableRule kRules[11] = {
    /*0 monsterTypes*/ {21, 1, false, {7, 14, 15}, -1, -1, false},
    /*1 armors*/       {10, -1, true, {9, -1, -1}, 5, 6, true},
    /*2 consumables*/  {14, -1, true, {13, -1, -1}, 5, 4, false},
    /*3 (unused)*/     {0, -1, false, {-1, -1, -1}, -1, -1, false},
    /*4 weapons*/      {8, -1, true, {7, -1, -1}, -1, -1, true},
    /*5 classes*/      {15, -1, true, {6, 13, 14}, -1, -1, true},
    /*6 table6*/       {7, -1, false, {2, -1, -1}, -1, -1, false},
    /*7 pairs*/        {0, -1, false, {-1, -1, -1}, -1, -1, true},
    /*8 specials*/     {15, -1, true, {14, -1, -1}, 6, -1, true},
    /*9 spawnGroups*/  {21, -1, false, {1, 2, -1}, -1, -1, false},
    /*10 loot*/        {4, -1, false, {-1, -1, -1}, -1, -1, false},
};

bool IsShort(const TableRule& r, int f) {
    for (int s : r.shorts)
        if (s == f) return true;
    return false;
}

// Parses one record (after its tag byte), consuming through the terminating
// 31. `row` gets the field values; classes (tag 5) also fill two -1-terminated
// lists. Returns the row index (field 0).
int ParseRecord(Cursor& c, int tag, ScrTables& t, int* row, int* list2, int* list3) {
    const TableRule& r = kRules[tag];
    int n2 = 0, n3 = 0, rowIndex = 0;
    while (c.Peek() != 31) {
        int fid = c.U8();
        if (fid >= r.cols && !(tag == 5 && (fid == 2 || fid == 3)) && !(tag == 9 && fid == 20))
            throw std::runtime_error("scr: field id out of range");
        if (fid == r.plainStr) {
            int len = c.U8();
            t.strings.push_back(c.Text(static_cast<size_t>(len)));
            row[fid] = static_cast<int>(t.strings.size()) - 1;
        } else if (fid == 1 && r.strOrId) {
            if ((c.Peek() & 0xF0) == 0xF0) {
                row[fid] = c.U16();
            } else {
                int len = c.U8();
                t.strings.push_back(c.Text(static_cast<size_t>(len)));
                row[fid] = static_cast<int>(t.strings.size()) - 1;
            }
        } else if (fid == r.i24) {
            row[fid] = c.U24();
        } else if (IsShort(r, fid)) {
            row[fid] = c.U16();
        } else if (fid == r.flag) {
            row[fid] = 1;
        } else if (tag == 5 && (fid == 2 || fid == 3)) {
            if (n2 >= 14 || n3 >= 14) throw std::runtime_error("scr: class list overflow");
            (fid == 2 ? list2[n2++] : list3[n3++]) = c.S8();
        } else if (tag == 9 && fid == 20) {
            if (t.spawnIdCount >= 10) throw std::runtime_error("scr: spawn id overflow");
            t.spawnIds[t.spawnIdCount++] = c.U8();
        } else {
            row[fid] = r.isSigned ? c.S8() : c.U8();
            if (fid == 0) rowIndex = row[0];
        }
    }
    c.p++;  // the 31
    if (tag == 5) {
        list2[n2] = -1;
        list3[n3] = -1;
    }
    return rowIndex;
}

template <size_t Rows, size_t Cols>
void PutRow(int (&table)[Rows][Cols], int idx, const int* row) {
    if (idx < 0 || static_cast<size_t>(idx) >= Rows) throw std::runtime_error("scr: table row index out of range");
    std::memcpy(table[idx], row, sizeof(int) * Cols);
}

}  // namespace

Scr ParseScr(const std::vector<uint8_t>& data) {
    Cursor c{data, 0};
    Scr scr;
    int n = c.U8();
    int ids[256], offs[256];
    for (int i = 0; i < n; i++) {
        ids[i] = c.U8();
        offs[i] = c.U16();
    }
    ScrTables& t = scr.tables;
    while (c.U8() == 30) {
        int tag = c.U8();
        if (tag == 7) {
            int i = 0;
            while (c.Peek() != 31) {
                if (i + 2 >= 100) throw std::runtime_error("scr: pair table overflow");
                t.pairTable[i++] = c.S8();
                t.pairTable[i++] = c.S8();
            }
            t.pairTable[i] = -1;
            c.p++;
            continue;
        }
        if (tag > 10 || tag == 3) throw std::runtime_error("scr: unknown table tag");
        int row[21] = {0}, list2[15] = {0}, list3[15] = {0};
        int idx = ParseRecord(c, tag, t, row, list2, list3);
        switch (tag) {
            case 0: PutRow(t.monsterTypes, idx, row); break;
            case 1: PutRow(t.armors, idx, row); break;
            case 2: PutRow(t.consumables, idx, row); break;
            case 4: PutRow(t.weapons, idx, row); break;
            case 5:
                PutRow(t.classBase, idx, row);
                PutRow(t.classItemTypes, idx, list2);
                PutRow(t.classLists, idx, list3);
                break;
            case 6: PutRow(t.table6, idx, row); break;
            case 8: PutRow(t.specials, idx, row); break;
            case 9: PutRow(t.spawnGroups, idx, row); break;
            case 10: PutRow(t.loot, idx, row); break;
        }
    }
    // `c` is one past the first non-30 byte; the loader skips 2 more.
    size_t codeStart = c.p + 2;
    if (codeStart > data.size()) throw std::runtime_error("scr: no code");
    scr.code.assign(data.begin() + static_cast<std::ptrdiff_t>(codeStart), data.end());
    for (int i = 0; i < n; i++)
        if (offs[i]) scr.scriptOffset[ids[i]] = static_cast<int>(offs[i] - codeStart);
    return scr;
}

// ---- instruction decoding -------------------------------------------------

namespace {

// Operand tokens: b/s/i = 1/2/3-byte unsigned, X = string (16-bit: 0xF000|id
// or a length), Z = X where 0 means none, B = byte-length string, S = 16-bit
// length string, L = count + bytes, V = SET_STAT value (width by stat id).
const char* Spec(int op) {
    switch (op) {
        case 2: return "";
        case 3: return "X";
        case 4: return "bb";
        case 5: case 6: case 13: case 35: return "b";
        case 7: return "b";
        case 8: return "BB";
        case 9: return "B";
        case 10: return "bi";
        case 11: return "s";
        case 12: return "";
        case 14: return "bb";
        case 15: return "Zbbss";
        case 16: return "bbbbb";
        case 17: return "bss";
        case 18: return "bbbb";
        case 19: case 20: return "b";
        case 21: return "L";
        case 22: return "bbb";
        case 23: return "b";
        case 24: return "bb";
        case 25: return "ss";
        case 26: return "b";
        case 27: return "bb";
        case 28: return "b";
        case 29: return "B";
        case 32: return "bbb";
        case 33: return "bb";
        case 34: return "bbV";
        case 36: return "bss";
        case 37: case 38: return "bbb";
        case 39: return "Xbbb";
        case 40: return "";
        case 41: case 42: return "bs";
        case 43: return "B";
        case 44: case 45: return "";
        case 46: return "bb";
        case 47: return "bbb";
        case 48: return "";
        case 49: return "bbb";
        case 50: return "bbbbbbb";
        case 51: return "bbbb";
        case 52: return "bbss";
        case 53: return "bbX";
        case 54: case 55: case 57: return "";
        case 56: return "Bb";
        case 58: case 59: return "bb";
        case 60: case 61: case 62: case 63: return "";
        case 64: return "i";
        case 65: return "bb";
        case 66: return "X";
        case 67: return "b";
        case 68: return "bss";
        case 69: return "bssb";
        case 70: case 71: return "ss";
        case 72: return "S";
        case 73: case 74: return "";
        case 75: case 76: return "b";
        case 77: return "";
        case 78: return "bb";
        default: return nullptr;
    }
}

}  // namespace

ScrInsn Scr::Decode(size_t pc) const {
    Cursor c{code, pc};
    ScrInsn in;
    in.offset = pc;
    in.op = c.U8();
    const char* spec = Spec(in.op);
    if (!spec) throw std::runtime_error("scr: unknown opcode");
    for (const char* s = spec; *s; s++) {
        auto literal = [&](size_t len) {
            ScrString v;
            v.text = c.Text(len);
            in.strings.push_back(v);
        };
        switch (*s) {
            case 'b': in.args.push_back(c.U8()); break;
            case 's': in.args.push_back(c.U16()); break;
            case 'i': in.args.push_back(c.U24()); break;
            case 'X':
            case 'Z': {
                int v = c.U16();
                if (*s == 'Z' && v == 0) {
                    in.strings.emplace_back();
                } else if ((v & 0xF000) == 0xF000) {
                    ScrString id;
                    id.isId = true;
                    id.id = v & 0xFFF;
                    in.strings.push_back(id);
                } else {
                    literal(static_cast<size_t>(v));
                }
                break;
            }
            case 'B': literal(static_cast<size_t>(c.U8())); break;
            case 'S': literal(static_cast<size_t>(c.U16())); break;
            case 'L': {
                int cnt = c.U8();
                in.args.push_back(cnt);
                for (int i = 0; i < cnt; i++) in.args.push_back(c.U8());
                break;
            }
            case 'V': {
                int stat = in.args.back();
                static const int kByteStats[] = {2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 18, 19, 20};
                if (stat == 7 || stat == 14 || stat == 15) {
                    in.args.push_back(c.U16());
                } else {
                    for (int b : kByteStats)
                        if (b == stat) in.args.push_back(c.U8());
                }
                break;
            }
        }
    }
    in.length = c.p - pc;
    return in;
}

std::vector<ScrInsn> Scr::Script(int id) const {
    std::vector<ScrInsn> out;
    if (id < 0 || id > 255 || scriptOffset[id] < 0) return out;
    size_t pc = static_cast<size_t>(scriptOffset[id]);
    while (pc < code.size()) {
        ScrInsn in = Decode(pc);
        out.push_back(in);
        pc += in.length;
        if (in.op == OP_RETURN) break;
    }
    return out;
}

}  // namespace oblivion
