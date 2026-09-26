#!/usr/bin/env python3
"""Parse an Oblivion (Superscape) .jtm tile-map file.

Format (from Game.loadMap, see docs/ASSET_FORMATS.md):

    width, height          1 byte each
    layer* :               width*height cells each, back to back until EOF;
                           cells are stored row by row (y outer, x inner) and
                           run-length encoded: 0xFF count value = `count`
                           copies of `value`, any other byte is one literal
                           cell. A run may cross row boundaries.

Layer 0 is the collision layer (Game.collision: 0 open, 1 solid, 2-5
half-tile diagonals); every later layer is a visual tile layer (Game.layers).
The engine indexes a cell as layer[x * height + y].

Usage:
    python oblivion/tools/parse_jtm.py FILE.jtm [--layer N] [--json]
    python oblivion/tools/parse_jtm.py --check
"""
import json
import sys
from pathlib import Path

OBL = Path(__file__).resolve().parent.parent


class JtmError(Exception):
    pass


def parse(data):
    """Return (width, height, [layer, ...]); layer[y][x] is a list of rows."""
    if len(data) < 2:
        raise JtmError("file too short")
    w, h = data[0], data[1]
    pos, layers = 2, []
    while pos < len(data):
        cells = []
        while len(cells) < w * h:
            if pos >= len(data):
                raise JtmError(f"layer {len(layers)} runs past end of file")
            b = data[pos]
            pos += 1
            if b == 0xFF:
                if pos + 2 > len(data):
                    raise JtmError("truncated run")
                n, v = data[pos], data[pos + 1]
                pos += 2
                if n == 0:
                    n = 1  # the engine still writes one cell for count 0/1
                cells.extend([v] * n)
            else:
                cells.append(b)
        if len(cells) != w * h:
            raise JtmError(f"layer {len(layers)} overshoots: {len(cells)} != {w * h}")
        layers.append([cells[y * w:(y + 1) * w] for y in range(h)])
    return w, h, layers


def show(layer, w, h):
    for row in layer:
        print(" ".join(f"{v:3d}" for v in row))


def check_all():
    bad = 0
    files = sorted((OBL / "extracted").glob("*.jtm"))
    for f in files:
        try:
            w, h, layers = parse(f.read_bytes())
            status = f"ok {w}x{h}, {len(layers)} layers"
        except JtmError as e:
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
    w, h, layers = parse(Path(path).read_bytes())
    if "--json" in args:
        print(json.dumps(dict(width=w, height=h, layers=layers)))
        return
    only = int(args[args.index("--layer") + 1]) if "--layer" in args else None
    print(f"; {path}: {w}x{h}, {len(layers)} layers (0 = collision)")
    for n, layer in enumerate(layers):
        if only is None or only == n:
            print(f"layer {n}:")
            show(layer, w, h)


if __name__ == "__main__":
    main()
