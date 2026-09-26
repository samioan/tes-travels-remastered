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

Keys
----
Arrows or 2/4/6/8 move, Enter or 5 attacks/acts, Z / F1 = left soft key (menu),
X / F2 = right soft key (inventory), 7 / 9 quick health / magicka potion,
3 toggles weapon/spell (rebindable under Help > Controls). Esc quits.

Saves
-----
%APPDATA%\OblivionPort\oblivion.eso, in the original's record format
(override with --save FILE).

Developer flags
---------------
--level /l01_1.scr   start straight in a level
--dump out.ppm [--run-ms N] [--keys up,fire,...]   render headless
PageUp / PageDown    jump between levels
