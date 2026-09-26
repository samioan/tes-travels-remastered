Oblivion (Superscape J2ME) -- PC port
=====================================

A clean C++17 reimplementation of the mobile game's engine. It loads the
original game's data files; none are included here.

Setup
-----
1. Put the game's resources (the contents of the original .jar: .scr, .jtm,
   .cml, .png, lang_*.txt, start.txt ...) in a folder called "extracted" next
   to oblivion_port.exe, or run:  oblivion_port.exe --assets C:\path\to\data
2. Optional, for the phone's look: put Ceurope.gdr (and Browsereur.gdr) from a
   Nokia 3650 in a folder called "fonts" next to the exe (--fonts DIR). Without
   them a system font is used.

Controls
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

Saves
-----
%APPDATA%\OblivionPort\oblivion.eso, in the original's record format
(override with --save FILE).

Developer flags
---------------
--level /l01_1.scr   start straight in a level
--dump out.ppm [--run-ms N] [--keys up,fire,...]   render headless
PageUp / PageDown    jump between levels
