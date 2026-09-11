# tools/

Shared tooling for all three games. Nothing here is project source -- it's
either fetched (`vineflower.jar`) or regenerates output already tracked
elsewhere in the repo, so only the scripts themselves are tracked (see
`.gitignore`).

- **`vineflower.jar`** -- [Vineflower](https://github.com/Vineflower/vineflower)
  1.12.0, a maintained fork of Fernflower. Not tracked in git (~1.6MB
  binary, trivially refetched). Get it with:
  ```
  curl -L -o tools/vineflower.jar https://github.com/Vineflower/vineflower/releases/download/1.12.0/vineflower-1.12.0.jar
  ```
- **`extract_jar.py`** -- unpacks `roms/TEST-<Game>.jar` (a plain zip) into
  `<game>/extracted/`, unmodified. Run with no args to do all three games.
- **`decompile.py`** -- runs Vineflower over `<game>/extracted/` and writes
  Java source to `<game>/decompiled/`. Requires a JDK on `PATH` (tested with
  Temurin 21) and `vineflower.jar` present. Run with no args to do all
  three games.

## Why Vineflower and not Ghidra

Unlike shadowkey (a Symbian ARM native binary -- see
`../shadowkey-decomp` sibling project, no public disassembler support,
required a from-scratch E32Image loader), these three games are J2ME
MIDlets: plain JVM class files with `MIDP-1.0`/`CLDC-1.0` manifests. A
Java decompiler reconstructs real, close-to-original Java directly --
there is no ARM/native step at all. The obfuscation here is just
minifier-style single/double-letter class and member names (`a.class`,
`b.class`, ...); the actual control flow and types come back intact.

```
python tools/extract_jar.py
python tools/decompile.py
```

regenerates `<game>/extracted/` and `<game>/decompiled/` for all three
games from the `roms/*.jar` files.
