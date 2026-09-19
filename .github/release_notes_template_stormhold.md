<!--
The body of every Stormhold GitHub release, with {{TAG}} substituted by
.github/workflows/release-stormhold.yml. Kept as a file rather than inline
YAML so that changing what a release says is a normal edit and not a
workflow edit. Sibling of release_notes_template.md (Dawnstar's own copy).
-->
## Stormhold Remastered {{TAG}}

A PC port of *The Elder Scrolls Travels: Stormhold* (J2ME/mobile Java),
rebuilt from the original game's own decompiled code. Very much a work in
progress -- see below.

### Getting it running

1. Download `StormholdRemastered-{{TAG}}-win64.zip` below and unzip it
   anywhere.
2. Run **Stormhold.exe**.
3. Point it at your own copy of `TEST-Stormhold.jar` -- the launcher
   unpacks it for you.
4. Press **Play**.

### This download does not include the game

It cannot: the game is Bethesda's. You supply your own copy.

### What's playable right now

Movement, combat, and starting a new character (Main Menu -> class
selection -> naming -> intro). Not yet ported: saving/loading a game, the
inventory/skills/spells/options menu, shops and NPCs, and camping. See
`stormhold/docs/PORT_ROADMAP.md` in the repository for the full,
milestone-by-milestone write-up of what's been verified against the
original game's own decompiled code.

### Notes

- Windows 64-bit. No installer and no Visual C++ redistributable -- unzip
  and run.
- Everything stays in the folder you unzipped: your game files are
  unpacked to `data\`, the log goes to `user\`. To uninstall, delete the
  folder. No registry keys, nothing in AppData.
- If something goes wrong, `user\stormhold_port.log` says what the game
  did. Attach it to a bug report.

*The Elder Scrolls* and *Stormhold* are trademarks of ZeniMax Media Inc.
This project is not affiliated with or endorsed by Bethesda Softworks,
ZeniMax or the game's original developer.
