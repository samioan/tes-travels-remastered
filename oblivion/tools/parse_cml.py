#!/usr/bin/env python3
"""Parse an Oblivion (Superscape) .cml sprite/scene description.

Format (from SpriteRenderer.load, see docs/ASSET_FORMATS.md):

    prefixLen, prefix           path prefix prepended to names not starting '/'
    record* until EOF:
      nameId, nameLen, name     image path (group id defaults to nameId)
      <flagged record>          group record (see below)
      colorKeyCount, n x (rgb24 from, rgb24 to)
      frameCount                0 = one static image, else that many frames:
        frame: <flagged record>, subframeCount, n x <flagged record>

A flagged record is a 16-bit big-endian presence mask (bits 0x200 ... 0x001
select fields 0..9 in that order) followed by the selected fields:
0 = 1 byte (group/frame id), 1/2 = 16-bit (x/y), 3/4 = 1 byte unsigned
(width/height), 5..9 = 1 byte signed (draw dx, dy, hold flag, is-sprite, spare).

Usage:
    python oblivion/tools/parse_cml.py FILE.cml [--json]
    python oblivion/tools/parse_cml.py --check
"""
import json
import sys
from pathlib import Path

OBL = Path(__file__).resolve().parent.parent
FIELDS = ["id", "x", "y", "w", "h", "dx", "dy", "hold", "sprite", "spare"]
MASK_BITS = [0x200, 0x100, 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1]
WIDTH = [1, 2, 2, 1, 1, 1, 1, 1, 1, 1]
SIGNED = [False] * 5 + [True] * 5


class CmlError(Exception):
    pass


class R:
    def __init__(self, d):
        self.d, self.p = d, 0

    def need(self, n):
        if self.p + n > len(self.d):
            raise CmlError(f"read past end at {self.p}")

    def u8(self):
        self.need(1)
        self.p += 1
        return self.d[self.p - 1]

    def u16(self):
        return (self.u8() << 8) | self.u8()

    def rgb(self):
        return (self.u8() << 16) | (self.u8() << 8) | self.u8()

    def text(self, n):
        self.need(n)
        self.p += n
        return self.d[self.p - n:self.p].decode("latin1")

    def record(self):
        mask = self.u16()
        rec = {}
        for i, bit in enumerate(MASK_BITS):
            if mask & bit:
                v = self.u16() if WIDTH[i] == 2 else self.u8()
                if SIGNED[i] and v > 127:
                    v -= 256
                rec[FIELDS[i]] = v
        return rec


def parse(data):
    r = R(data)
    prefix = r.text(r.u8())
    groups = []
    while r.p != len(data):
        name_id = r.u8()
        name = r.text(r.u8())
        if not name.startswith("/"):
            name = prefix + name
        rec = r.record()
        rec.setdefault("id", name_id)
        if rec["id"] == 0:
            rec["id"] = name_id
        keys = [(r.rgb(), r.rgb()) for _ in range(r.u8())]
        frames = []
        for _ in range(r.u8()):
            frame = r.record()
            frame["subframes"] = [r.record() for _ in range(r.u8())]
            frames.append(frame)
        groups.append(dict(name=name, record=rec, colorKeys=keys, frames=frames))
    return prefix, groups


def check_all():
    bad = 0
    files = sorted((OBL / "extracted").glob("*.cml"))
    for f in files:
        try:
            _, groups = parse(f.read_bytes())
            status = f"ok {len(groups)} groups, {sum(len(g['frames']) for g in groups)} frames"
        except CmlError as e:
            status = f"ERROR {e}"
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
    prefix, groups = parse(Path(path).read_bytes())
    if "--json" in args:
        print(json.dumps(dict(prefix=prefix, groups=groups), indent=1))
        return
    print(f"; {path}: prefix {prefix!r}, {len(groups)} groups")
    for g in groups:
        print(f"{g['name']}  {g['record']}" + (f"  colorKeys={g['colorKeys']}" if g["colorKeys"] else ""))
        for i, fr in enumerate(g["frames"]):
            subs = fr["subframes"]
            print(f"    frame {i}: { {k: v for k, v in fr.items() if k != 'subframes'} }")
            for s in subs:
                print(f"        sub {s}")


if __name__ == "__main__":
    main()
