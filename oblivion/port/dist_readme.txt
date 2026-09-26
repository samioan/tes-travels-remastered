Oblivion Remastered
===================

A PC port of The Elder Scrolls Travels: Oblivion (Superscape J2ME, mobile
Java): a reimplementation of the game's engine that runs the original
game's own data.


WHAT YOU NEED
-------------

This download does NOT include the game. It cannot: the game is
Bethesda's. You supply your own copy.

  1. Your own copy of TEST-Oblivion.jar (or however your own Oblivion
     build is named). The launcher unpacks it for you -- you don't need
     to extract it yourself first.

  2. Optional, but it's how the game really looked: the phone's own font,
     Ceurope.gdr (Browsereur.gdr beside it is picked up too). Oblivion ran
     on Nokia's Series 60 phones (the 3650 and its siblings) and drew its
     text in those phones' built-in fonts. They are Nokia's, so they can't
     be included here either. Without them the game uses stand-in letters
     that look close, but not the same.

     You'll find them in the System\Fonts folder of a Series 60 1st Edition
     phone (Nokia 7650, 3650, 3660, N-Gage), in an EKA2L1 emulator setup
     for one of those phones, or in Nokia's Series 60 MIDP SDK. The
     launcher looks in EKA2L1's and the SDK's usual places by itself.


HOW TO PLAY
-----------

Run Oblivion.exe. Choose your .jar file. Optionally choose Ceurope.gdr.
Pick a window size (2x-4x) or Fullscreen (borderless). Press Play. Everything you add is copied into this folder, so you can
delete or move your original download afterwards and nothing breaks.

The launcher checks GitHub for a newer release each time it opens and
offers to install it (release builds only).


CONTROLS
--------

Keyboard + mouse
  WASD / arrows   walk (screen-relative: W is up the screen; 8 directions)
  Space / J       attack (hold to keep attacking)
  Left click      attack towards the cursor (aims the swing, bolt or arrow)
  E / Enter       interact (talk, open, pick up)   Right click: same
  1 / 2           health / magicka potion
  Tab / R         toggle weapon <-> spell
  I               inventory        Esc: menu / back     Backspace: back
Gamepad (XInput, any Xbox-style pad)
  Left stick / d-pad   walk (stick pushes are analogue)
  Right stick          aim (the swing / spell / arrow goes where you aim)
  A or right trigger   attack      X: interact       Y: toggle weapon/spell
  LB / RB              health / magicka potion
  Back: inventory      Start: menu     B: back
  In menus: d-pad / stick to move, A to choose, B to go back, LB / RB switch tabs.
The old phone keypad keys still work (digits, Z / X soft keys) and can be
rebound under Help > Controls.


DISPLAY SETTINGS
----------------

Settings is in the main menu and the in-game pause menu (or use the keys):
  Resolution (F8)  Original 176x208, Auto (follows the window), 4:3, 16:10,
                   16:9, 21:9. The picture keeps its 208-row height and gets
                   wider, so sprites and the HUD are never stretched -- you
                   just see more of the world at the sides. Menus, text and
                   the inventory stay in their original centred layout.
  Display (F11)    Fullscreen (borderless) or Windowed. Alt+Enter also works.
  Scaling          Fit (any size) or Integer (whole multiples: crisp pixels).
The window can be resized freely. Your choices are remembered (user\display.cfg).


WHERE THINGS LIVE
-----------------

Everything stays in this folder. No installer, no registry keys, nothing in
AppData. To uninstall, delete the folder.

  data\   your unpacked game files (made by the launcher)
  fonts\  the Nokia font, if you chose one
  user\   your save (oblivion.eso) and the game's log (oblivion_port.log)
