#!/usr/bin/env python3
"""Decompile a game's extracted .class files to Java source with Vineflower.

Run tools/extract_jar.py first. This writes <game>/decompiled/**/*.java and
deletes everything Vineflower also copies over from the input directory
(images, .dat/.lmp/.cus/.scr/... game data) since that's a duplicate of
<game>/extracted/ -- decompiled/ should contain source only.

Usage: python tools/decompile.py <game>
  <game> is one of: dawnstar, oblivion, stormhold
"""
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VINEFLOWER = ROOT / "tools" / "vineflower.jar"
GAMES = ("dawnstar", "oblivion", "stormhold")


def decompile(game: str) -> None:
    src = ROOT / game / "extracted"
    if not src.exists():
        sys.exit(f"{src} does not exist -- run tools/extract_jar.py {game} first")
    out = ROOT / game / "decompiled"
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)

    subprocess.run(
        ["java", "-jar", str(VINEFLOWER), "-log=WARN", str(src), str(out)],
        check=True,
    )

    # Vineflower mirrors non-.class input files into the output tree too;
    # strip everything but the decompiled sources.
    for path in sorted(out.rglob("*")):
        if path.is_file() and path.suffix != ".java":
            path.unlink()
    for path in sorted(out.rglob("*"), reverse=True):
        if path.is_dir() and not any(path.iterdir()):
            path.rmdir()

    n = sum(1 for _ in out.rglob("*.java"))
    print(f"{game}: decompiled {n} source files -> {out.relative_to(ROOT)}")


def main() -> None:
    if not VINEFLOWER.exists():
        sys.exit(
            f"{VINEFLOWER} not found -- download it from "
            "https://github.com/Vineflower/vineflower/releases (vineflower-<ver>.jar)"
        )
    games = sys.argv[1:] or list(GAMES)
    for game in games:
        if game not in GAMES:
            sys.exit(f"unknown game {game!r}, expected one of {GAMES}")
        decompile(game)


if __name__ == "__main__":
    main()
