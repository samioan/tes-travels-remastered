Stormhold Remastered
====================

A PC port of The Elder Scrolls Travels: Stormhold (J2ME, mobile Java),
rebuilt from the original game's own decompiled code. Very much a work
in progress -- see below.


WHAT YOU NEED
-------------

This download does NOT include the game. It cannot: the game is
Bethesda's. You supply your own copy.

  1. Your own copy of TEST-Stormhold.jar (or however your own Stormhold
     build is named). The launcher unpacks it for you -- you don't need
     to extract it yourself first.


HOW TO PLAY
-----------

Run Stormhold.exe. Choose your .jar file. Press Play.

Everything you add is copied into this folder, so you can delete or move
your original download afterwards and nothing breaks.

From the Main Menu, choose New Game, pick a class, name your character,
and read through (or skip past) the introduction to start playing.


CONTROLS
--------

  Arrow keys            walk forward/backward, turn left/right
  Space                  attack whatever monster you're facing
  M                      toggle the zoomed-out minimap
  Enter                  confirm / select (menus)
  Escape                 back / cancel (menus)
  A-Z, 0-9, Backspace     type your character's name (menus)

The window size (2x, 3x or 4x) is set in the launcher, not in the game.


WHERE YOUR FILES GO
--------------------

  data\    the game files you supplied (unpacked from your .jar)
  user\    the log (no save/load yet -- see below)
  bin\     the game engine itself

To uninstall, delete this folder. Nothing is written anywhere else -- no
registry keys, no AppData, no installer.

To move it to another drive, move the whole folder.


THIS IS A WORK IN PROGRESS
---------------------------

Movement, combat, and starting a new character all work. Not yet ported:
saving/loading a game, the inventory/skills/spells/options menu, shops
and NPCs, and camping. Progress and a full write-up of what's been
verified against the original game's own decompiled code live at
stormhold/docs/PORT_ROADMAP.md in the project's repository.


IF SOMETHING GOES WRONG
------------------------

user\stormhold_port.log records what the game did, including anything it
could not do. Attach it to a bug report.


CREDITS
-------

The Elder Scrolls and Stormhold are trademarks of ZeniMax Media Inc. This
project is not affiliated with or endorsed by Bethesda Softworks, ZeniMax
or the game's original developer. See NOTICE.md.
