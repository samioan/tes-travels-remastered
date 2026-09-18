Dawnstar Remastered
====================

A PC port of The Elder Scrolls Travels: Dawnstar (J2ME, mobile Java),
rebuilt from the original game's own decompiled code.


WHAT YOU NEED
-------------

This download does NOT include the game. It cannot: the game is
Bethesda's. You supply your own copy.

  1. Your own copy of TEST-Dawnstar.jar (or however your own Dawnstar
     build is named). The launcher unpacks it for you -- you don't need
     to extract it yourself first.


HOW TO PLAY
-----------

Run Dawnstar.exe. Choose your .jar file. Press Play.

Everything you add is copied into this folder, so you can delete or move
your original download afterwards and nothing breaks.


CONTROLS
--------

  Up / Down            walk forward / backward
  Left / Right          turn left / right
  Enter                 confirm / select
  Escape                back / cancel
  0-9                   hotbar (spells, items, actions -- depends on context)
  A                      attack
  C                      cycle selected spell
  I                      interact -- doors, people, chests, anything nearby
  Z                      camp / rest
  O                      options menu (inventory, skills, spells, save/load)
  M                      toggle the zoomed-out minimap

These are the original's own D-pad/numeric-keypad controls mapped onto a
keyboard: the J2ME phone this shipped on had a D-pad and a 0-9 keypad, so
the arrow keys stand in for the D-pad and the letter keys stand in for
whichever numeric keys the original used for each action.

The window size (2x, 3x or 4x) is set in the launcher, not in the game.


WHERE YOUR FILES GO
--------------------

  data\    the game files you supplied (unpacked from your .jar)
  user\    your saved games and the log
  bin\     the game engine itself

To uninstall, delete this folder. Nothing is written anywhere else -- no
registry keys, no AppData, no installer.

To move it to another drive, move the whole folder. To back up your saves,
copy user\.


IF SOMETHING GOES WRONG
------------------------

user\dawnstar_port.log records what the game did, including anything it
could not do. Attach it to a bug report.


CREDITS
-------

The Elder Scrolls and Dawnstar are trademarks of ZeniMax Media Inc. This
project is not affiliated with or endorsed by Bethesda Softworks, ZeniMax
or the game's original developer. See NOTICE.md.
