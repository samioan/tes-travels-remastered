#!/usr/bin/env python3
"""Re-decompile oblivion/extracted/*.class with docs/rename.map applied.

The original obfuscator reused one letter across fields (and methods) of
different JVM descriptors, which plain Vineflower output cannot express as
valid Java. tools/MapRenamer.java is a Vineflower identifier renamer that
keys every rename on (class, name, descriptor), so it can tell them apart.

Usage: python oblivion/tools/decompile_renamed.py <outdir> [--uniquify]
  --uniquify  also give every UNMAPPED field a descriptor suffix (a_aBy,
              a_In, ...) so the output reads unambiguously.
Requires JDK 21 as JAVA21_HOME (or java on PATH) and tools/vineflower.jar.
"""
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
OBL = HERE.parent
ROOT = OBL.parent
VINEFLOWER = ROOT / "tools" / "vineflower.jar"


def main() -> None:
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    out = Path(sys.argv[1])
    java_home = os.environ.get("JAVA21_HOME")
    java = str(Path(java_home) / "bin" / "java") if java_home else "java"
    javac = str(Path(java_home) / "bin" / "javac") if java_home else "javac"

    cls = Path(tempfile.mkdtemp())
    subprocess.run([javac, "-cp", str(VINEFLOWER), "-d", str(cls), str(HERE / "MapRenamer.java")], check=True)
    inp = Path(tempfile.mkdtemp())
    for f in (OBL / "extracted").glob("*.class"):
        shutil.copy(f, inp)
    out.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, RENAME_MAP=str(OBL / "docs" / "rename.map"))
    if "--uniquify" in sys.argv:
        env["RENAME_UNIQUIFY"] = "1"
    sep = ";" if os.name == "nt" else ":"
    subprocess.run(
        [java, "-cp", f"{VINEFLOWER}{sep}{cls}",
         "org.jetbrains.java.decompiler.main.decompiler.ConsoleDecompiler",
         "-ren=1", "-urc=MapRenamer", "-log=WARN", str(inp), str(out)],
        check=True, env=env)
    shutil.rmtree(cls, ignore_errors=True)
    shutil.rmtree(inp, ignore_errors=True)


if __name__ == "__main__":
    main()
