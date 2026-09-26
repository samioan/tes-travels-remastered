#!/usr/bin/env python3
"""Parse and disassemble an Oblivion (Superscape) .scr level script.

Format (from ScriptInterpreter.load / step, see docs/CLASS_MAP.md and
docs/SCR_OPCODES.md):

    n                      1 byte, number of script entry points
    n x (id, offset16)     script id -> absolute file offset (big-endian)
    { 30, tag, record, 31 }*   table records, one per row of tables 0-10
    00 01 01               terminator + the marker of script 1 (`00 id 01`)
    bytecode               scripts back to back, each ending in RETURN and
                           followed by the next script's marker; opcodes are
                           listed in docs/SCR_OPCODES.md

Usage:
    python oblivion/tools/parse_scr.py FILE.scr [--tables] [--code] [--json]
    python oblivion/tools/parse_scr.py --check   # parse every extracted .scr

With no option flag, --tables and --code are both printed.
"""
import json
import struct
import sys
from pathlib import Path

OBL = Path(__file__).resolve().parent.parent

# Operand tokens: b = unsigned byte, s = 16-bit BE, i = 24-bit BE,
# X = string given by a 16-bit value (0xF000|id = localized id, else a length),
# Z = like X but 0 = no string (SPAWN_ACTOR), B = string with 1-byte length,
# S = string with 16-bit length, L = count byte then that many bytes.
OPS = {
    2: ("RETURN", ""), 3: ("SAY", "X"), 4: ("SET_SCREEN_SIZE", "bb"),
    5: ("NOP5", "b"), 6: ("NOP6", "b"), 7: ("SET_PLAYER_COLLIDES", "b"),
    8: ("LOAD_MAP", "BB"), 9: ("SKIP_STRING", "B"), 10: ("END_LEVEL", "bi"),
    11: ("WAIT", "s"), 12: ("SET_STATE_PLAYING", ""), 13: ("NOP13", "b"),
    14: ("SET_KEY_HOOK", "bb"), 15: ("SPAWN_ACTOR", "Zbbss"),
    16: ("SET_TRIGGER", "bbbbb"), 17: ("MOVE_ACTOR_TO", "bss"),
    18: ("SET_TILE", "bbbb"), 19: ("SET_INPUT_ENABLED", "b"),
    20: ("REMOVE_ACTOR", "b"), 21: ("WAIT_ACTORS_STOP", "L"),
    22: ("SET_COLLISION", "bbb"), 23: ("CALL", "b"),
    24: ("SET_ANIM_STATE", "bb"), 25: ("CAMERA_TO", "ss"),
    26: ("CAMERA_FOLLOW", "b"), 27: ("CLEAR_TRIGGER", "bb"),
    28: ("CLEAR_KEY_HOOK", "b"), 29: ("LOAD_LEVEL", "B"),
    32: ("SET_DEATH_SCRIPT", "bbb"), 33: ("CLEAR_DEATH_SCRIPT", "bb"),
    34: ("SET_STAT", "bbV"), 35: ("NOP35", "b"), 36: ("SET_POSITION", "bss"),
    37: ("GIVE_ITEM", "bbb"), 38: ("REMOVE_ITEM", "bbb"),
    39: ("SHOW_MESSAGE", "Xbbb"), 40: ("HIDE_MESSAGE", ""),
    41: ("MOVE_ACTOR_X", "bs"), 42: ("MOVE_ACTOR_Y", "bs"),
    43: ("LOAD_HUD_SPRITES", "B"), 44: ("OPEN_MENU", ""),
    45: ("OPEN_SHOP_MENU", ""), 46: ("SET_STATUS_ICON", "bb"),
    47: ("GENERATE_DUNGEON", "bbb"), 48: ("CLEAR_LAYERS", ""),
    49: ("PLACE_ITEM", "bbb"), 50: ("SET_TRIGGER_RECT", "bbbbbbb"),
    51: ("CLEAR_TRIGGER_RECT", "bbbb"), 52: ("WALK_CUTSCENE", "bbss"),
    53: ("TALK", "bbX"), 54: ("NOP54", ""), 55: ("NOP55", ""),
    56: ("LOAD_LANG", "Bb"), 57: ("NOP57", ""), 58: ("SCALE_MONSTER", "bb"),
    59: ("SET_DROPS_LOOT", "bb"), 60: ("WAIT_KEY", ""),
    61: ("SET_STATE_9", ""), 62: ("NOP62", ""), 63: ("NOP63", ""),
    64: ("SET_BACKGROUND_COLOR", "i"), 65: ("LEVEL_UP_TO", "bb"),
    66: ("SHOW_TEXT_SCREEN", "X"), 67: ("RESTORE_MONSTER_TYPE", "b"),
    68: ("SPAWN_PROJECTILE", "bss"), 69: ("SPAWN_TIMED_PROJECTILE", "bssb"),
    70: ("CLEAR_PROJECTILE_AT", "ss"), 71: ("SET_POINT", "ss"),
    72: ("EVICT_SPRITES", "S"), 73: ("BEGIN_FADE", ""), 74: ("END_FADE", ""),
    75: ("TOGGLE_INVULNERABLE", "b"), 76: ("SET_HUD_VISIBLE", "b"),
    77: ("SET_STATE_4", ""), 78: ("SET_AI_ACTIVE", "bb"),
}
# SET_STAT (ActorSystem.setStat) value width by stat id: one byte for these,
# 16 bits for 7/14/15, no operand for any other id (1, 16, 17, ...).
STAT_BYTE = {2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 18, 19, 20}
STAT_SHORT = {7, 14, 15}


class ScrError(Exception):
    pass


class R:
    def __init__(self, data, pos=0):
        self.d, self.p = data, pos

    def u8(self):
        if self.p >= len(self.d):
            raise ScrError(f"read past end at {self.p}")
        v = self.d[self.p]
        self.p += 1
        return v

    def s8(self):
        v = self.u8()
        return v - 256 if v > 127 else v

    def u16(self):
        return (self.u8() << 8) | self.u8()

    def u24(self):
        return (self.u8() << 16) | (self.u8() << 8) | self.u8()

    def text(self, n):
        if self.p + n > len(self.d):
            raise ScrError(f"string past end at {self.p}")
        s = self.d[self.p:self.p + n].decode("latin1")
        self.p += n
        return s


# ---- table records -------------------------------------------------------
# rules: strf = field ids holding a string-or-localized-id (or plain string),
# shorts / int24s = wide fields, flags = fields with no data byte (value 1),
# lists = {field id: list name}, signed = default byte is signed.
TABLES = {
    0: dict(name="monsterTypes", plain_str={1}, shorts={7, 14, 15}, signed=False),
    1: dict(name="armors", str_or_id={1}, i24={5}, shorts={9}, flags={6}, signed=True),
    2: dict(name="consumables", str_or_id={1}, i24={5}, shorts={13}, flags={4}, signed=False),
    4: dict(name="weapons", str_or_id={1}, shorts={7}, signed=True),
    5: dict(name="classBase", str_or_id={1}, shorts={6, 13, 14}, lists={2: "classItemTypes", 3: "classLists"}, signed=True),
    6: dict(name="table6", shorts={2}, signed=False),
    8: dict(name="specials", str_or_id={1}, shorts={14}, i24={6}, signed=True),
    9: dict(name="spawnGroups", shorts={1, 2}, lists={20: "spawnIds"}, signed=False),
    10: dict(name="lootTable", signed=False),
}


def parse_record(r, tag):
    rule = TABLES[tag]
    rec, lists = {}, {}
    while True:
        fid = r.u8()
        if fid == 31:
            break
        if fid in rule.get("plain_str", ()):
            n = r.u8()
            rec[fid] = r.text(n)
        elif fid in rule.get("str_or_id", ()):
            n = r.d[r.p]
            if n & 0xF0 == 0xF0:
                rec[fid] = ("id", r.u16() & 0xFFF)
            else:
                n = r.u8()
                rec[fid] = r.text(n)
        elif fid in rule.get("shorts", ()):
            rec[fid] = r.u16()
        elif fid in rule.get("i24", ()):
            rec[fid] = r.u24()
        elif fid in rule.get("flags", ()):
            rec[fid] = 1
        elif fid in rule.get("lists", {}):
            lists.setdefault(rule["lists"][fid], []).append(r.u8() if tag == 9 else r.s8())
        else:
            rec[fid] = r.s8() if rule["signed"] else r.u8()
    return rec, lists


def parse_pairs(r):
    out = []
    while r.d[r.p] != 31:
        out.append((r.s8(), r.s8()))
    r.p += 1
    return out


def parse(data):
    r = R(data)
    n = r.u8()
    entries = {}
    for _ in range(n):
        sid = r.u8()
        entries[sid] = r.u16()
    tables = {}
    while r.u8() == 30:
        tag = r.u8()
        if tag == 7:
            tables.setdefault("pairTable", []).extend(parse_pairs(r))
            continue
        if tag not in TABLES:
            raise ScrError(f"unknown table tag {tag} at {r.p - 1}")
        rec, lists = parse_record(r, tag)
        name = TABLES[tag]["name"]
        tables.setdefault(name, {})[rec.get(0, 0)] = dict(rec, **{f"_{k}": v for k, v in lists.items()})
    # the loader consumes the non-30 byte just read, then skips 2 more
    code_start = r.p + 2
    return dict(entries=entries, tables=tables, code_start=code_start)


# ---- disassembly ---------------------------------------------------------
def operand(r, tok, ctx):
    if tok == "b":
        return r.u8()
    if tok == "s":
        return r.u16()
    if tok == "i":
        return r.u24()
    if tok == "X":
        v = r.u16()
        return ("id", v & 0xFFF) if v & 0xF000 == 0xF000 else r.text(v)
    if tok == "Z":
        v = r.u16()
        if v == 0:
            return None
        return ("id", v & 0xFFF) if v & 0xF000 == 0xF000 else r.text(v)
    if tok == "B":
        return r.text(r.u8())
    if tok == "S":
        return r.text(r.u16())
    if tok == "L":
        return [r.u8() for _ in range(r.u8())]
    if tok == "V":  # SET_STAT value; ctx = stat id
        stat = ctx
        if stat in STAT_SHORT:
            return r.u16()
        return r.u8() if stat in STAT_BYTE else None
    raise AssertionError(tok)


def disassemble_script(data, start, end):
    """Disassemble one script: (ops, footer bytes).

    There are no jumps, so a script is a straight run of opcodes ending at
    RETURN; the bytes after it up to the next entry point are the next
    script's 3-byte marker `00 <its id> 01` (never executed). The first
    script's marker is the terminator byte + 2 bytes the loader skips.
    """
    r = R(data, start)
    ops = []
    while r.p < end:
        off = r.p
        op = r.u8()
        if op not in OPS:
            raise ScrError(f"unknown opcode {op} at code offset {off}")
        name, toks = OPS[op]
        args = []
        for t in toks:
            args.append(operand(r, t, args[1] if t == "V" else None))
        ops.append((off, name, args))
        if name == "RETURN":
            break
    return ops, bytes(data[r.p:end])


def scripts(data, info):
    """Yield (script id, ops, footer) in file order."""
    cs = info["code_start"]
    bounds = sorted((v, k) for k, v in info["entries"].items() if v)
    for n, (off, sid) in enumerate(bounds):
        end = bounds[n + 1][0] if n + 1 < len(bounds) else len(data)
        ops, footer = disassemble_script(data, off, end)
        yield sid, ops, footer


def fmt(v):
    if isinstance(v, tuple):
        return f"str#{v[1]}"
    if isinstance(v, str):
        return json.dumps(v)
    if isinstance(v, list):
        return "[" + ",".join(map(str, v)) + "]"
    return str(v)


def dump(path, want_tables, want_code, as_json):
    data = Path(path).read_bytes()
    info = parse(data)
    code_start = info["code_start"]
    if as_json:
        out = {str(sid): [dict(off=o - code_start, op=n, args=a) for o, n, a in ops]
               for sid, ops, _ in scripts(data, info)}
        print(json.dumps(dict(info, scripts=out), indent=1, default=str))
        return
    print(f"; {path}: {len(data)} bytes, {len(info['entries'])} entry points, code at {code_start}")
    if want_tables:
        for name, rows in info["tables"].items():
            print(f"[{name}]")
            rows = rows.items() if isinstance(rows, dict) else enumerate(rows)
            for k, row in rows:
                print(f"  {k}: {row}")
    if want_code:
        for sid, ops, footer in scripts(data, info):
            print(f"\nscript_{sid}:")
            for off, name, args in ops:
                print(f"  {off - code_start:5d}  {name} " + " ".join(fmt(a) for a in args))
            if footer:
                print(f"        ; next-script marker {footer.hex()}")


def check_all():
    bad = 0
    files = sorted((OBL / "extracted").glob("*.scr"))
    for f in files:
        data = f.read_bytes()
        try:
            info = parse(data)
            status = "ok"
            order = [k for _, k in sorted((v, k) for k, v in info["entries"].items() if v)]
            for n, (sid, ops, footer) in enumerate(scripts(data, info)):
                want = bytes([0, order[n + 1], 1]) if n + 1 < len(order) else b""
                # l04_4b.scr has one orphaned (unreferenced) script between two
                # scripts, so accept extra bytes as long as the footer is
                # bracketed by the expected marker.
                if footer != want and not (want and footer.startswith(want) and footer.endswith(want)):
                    status = f"script {sid}: footer {footer.hex()} (want {want.hex()})"
                    break
        except ScrError as e:
            status = f"ERROR {e}"
        if status != "ok":
            bad += 1
        print(f"{f.name:20s} {status}")
    print(f"{len(files) - bad}/{len(files)} clean")
    return bad


def main():
    args = sys.argv[1:]
    if not args:
        sys.exit(__doc__)
    if "--check" in args:
        sys.exit(1 if check_all() else 0)
    path = next(a for a in args if not a.startswith("--"))
    t, c = "--tables" in args, "--code" in args
    if not t and not c:
        t = c = True
    dump(path, t, c, "--json" in args)


if __name__ == "__main__":
    main()
