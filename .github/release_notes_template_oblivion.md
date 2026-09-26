<!--
The body of every Oblivion GitHub release, with {{TAG}} substituted by
.github/workflows/release-oblivion.yml. Kept as a file rather than inline
YAML so that changing what a release says is a normal edit and not a
workflow edit. Sibling of release_notes_template_stormhold.md.
-->
## Oblivion Remastered {{TAG}}

A PC port of *The Elder Scrolls Travels: Oblivion* (Superscape J2ME/mobile
Java): a from-scratch reimplementation of the game's engine that runs the
original game's own data.

### Getting it running

1. Download `OblivionRemastered-{{TAG}}-win64.zip` below and unzip it
   anywhere.
2. Run **Oblivion.exe**.
3. Point it at your own copy of `TEST-Oblivion.jar` -- the launcher
   unpacks it for you.
4. Press **Play**.

The launcher updates itself: when a newer release is published it offers to
download and install it.

### This download does not include the game

It cannot: the game is Bethesda's. You supply your own copy. The phone's own
fonts (`Ceurope.gdr`) are optional and also yours to supply.

### What's playable

All twelve levels: the boot menus, class choice, scripted cutscenes and
dialogue, real-time combat and monster AI, XP and levelling, spells and
special attacks, inventory, shop and item logic, pickups and loot, the
random dungeons, and save/load. Controls are modern -- WASD or a gamepad,
mouse or right-stick aiming (see `README.txt`).

### Notes

- Windows 64-bit. No installer and no Visual C++ redistributable -- unzip
  and run.
- Everything stays in the folder you unzipped: your game files are
  unpacked to `data\`, saves and the log go to `user\`. To uninstall,
  delete the folder. No registry keys, nothing in AppData.
- If something goes wrong, `user\oblivion_port.log` says what the game
  did. Attach it to a bug report.

*The Elder Scrolls* and *Oblivion* are trademarks of ZeniMax Media Inc.
This project is not affiliated with or endorsed by Bethesda Softworks,
ZeniMax or the game's original developer.
