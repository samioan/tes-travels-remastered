#!/usr/bin/env python3
"""Unpack a game's .jar (a plain zip) into <game>/extracted/, unmodified.

Usage: python tools/extract_jar.py <game>
  <game> is one of: dawnstar, oblivion, stormhold
  (matches roms/TEST-<Game>.jar and <game>/extracted/)
"""
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

GAMES = {
    "dawnstar": "TEST-Dawnstar.jar",
    "oblivion": "TEST-Oblivion.jar",
    "stormhold": "TEST-Stormhold.jar",
}


def extract(game: str) -> None:
    jar_name = GAMES[game]
    jar_path = ROOT / "roms" / jar_name
    out_dir = ROOT / game / "extracted"
    out_dir.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(jar_path) as zf:
        zf.extractall(out_dir)
    print(f"{game}: extracted {jar_name} -> {out_dir.relative_to(ROOT)}")


def main() -> None:
    games = sys.argv[1:] or list(GAMES)
    for game in games:
        if game not in GAMES:
            sys.exit(f"unknown game {game!r}, expected one of {list(GAMES)}")
        extract(game)


if __name__ == "__main__":
    main()
