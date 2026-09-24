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

  2. Optional, but it's how the game really looked: the phone's own
     fonts, Ceurope.gdr and Browsereur.gdr. Dawnstar ran on Nokia's
     Series 60 phones (the 3650 and its siblings), and drew all of its
     text in those phones' built-in fonts. They are Nokia's, so they
     can't be included here either. Without them the game uses stand-in
     letters that look close, but not the same.

     You'll find them in the System\Fonts folder of any Series 60 1st
     Edition phone (Nokia 7650, 3650, 3660, N-Gage), in an EKA2L1
     emulator setup for one of those phones, or in Nokia's own Series 60
     MIDP SDK. The launcher looks in EKA2L1's and the SDK's usual places
     by itself; otherwise pick Ceurope.gdr and it takes Browsereur.gdr
     from the same folder.


HOW TO PLAY
-----------

Run Dawnstar.exe. Choose your .jar file. Optionally, choose
Ceurope.gdr for the phone's own fonts. Press Play.

Everything you add is copied into this folder, so you can delete or move
your original download afterwards and nothing breaks.


CONTROLS
--------

  W / S / Up / Down     walk forward / backward
  A / D                 strafe left / right
  Q / E / Left / Right  turn left / right
  Space                  attack
  F                      cast selected spell
  C                      cycle selected spell
  R                      interact -- doors, people, chests, anything nearby
  Z                      rest / camp
  Tab                    options menu (inventory, skills, spells, save/load)
  M                      toggle the zoomed-out minimap
  Enter                  confirm / select
  Escape                 back / cancel

A WASD + strafe layout, the same shape most first-person dungeon crawlers
use today (Legend of Grimrock and its own genre-mates), not the original
phone's own D-pad-and-numeric-keypad scheme. Q/E sit right next to WASD
so you can turn without moving your hand; the arrow keys still turn in
place too, exactly like they always did. Strafing (A/D) is genuinely
new -- the original had it too (its own numeric '4'/'6' keys), it just
never had a PC-keyboard binding here until now.

The window size (2x, 3x or 4x) is set in the launcher, not in the game.


WHERE YOUR FILES GO
--------------------

  data\    the game files you supplied (unpacked from your .jar)
  fonts\   the phone fonts, if you supplied them
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
