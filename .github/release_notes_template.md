<!--
The body of every Dawnstar GitHub release, with {{TAG}} substituted by
.github/workflows/release.yml. Kept as a file rather than inline YAML so
that changing what a release says is a normal edit and not a workflow edit.
-->
## Dawnstar Remastered {{TAG}}

A PC port of *The Elder Scrolls Travels: Dawnstar* (J2ME/mobile Java),
rebuilt from the original game's own decompiled code.

### Getting it running

1. Download `DawnstarRemastered-{{TAG}}-win64.zip` below and unzip it
   anywhere.
2. Run **Dawnstar.exe**.
3. Point it at your own copy of `TEST-Dawnstar.jar` -- the launcher
   unpacks it for you.
4. Press **Play**.

### This download does not include the game

It cannot: the game is Bethesda's. You supply your own copy.

### Notes

- Windows 64-bit. No installer and no Visual C++ redistributable -- unzip
  and run.
- Everything stays in the folder you unzipped: your game files are
  unpacked to `data\`, saves and settings go to `user\`. To uninstall,
  delete the folder. No registry keys, nothing in AppData.
- If something goes wrong, `user\dawnstar_port.log` says what the game
  did. Attach it to a bug report.

*The Elder Scrolls* and *Dawnstar* are trademarks of ZeniMax Media Inc.
This project is not affiliated with or endorsed by Bethesda Softworks,
ZeniMax or the game's original developer.
