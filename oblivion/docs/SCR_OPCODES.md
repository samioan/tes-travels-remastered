# .scr bytecode opcodes

From `ScriptInterpreter.step` (`e.b(long)`). Operand key: `b` = 1 byte, `s` = 16-bit
big-endian, `i` = 24-bit big-endian, `str` = a 16-bit length whose high nibble 0xF
(0xF000+id) means "localized string id" instead of a literal string of that length.
Opcodes 30, 31 and anything above 78 do nothing. Names are descriptive, derived from
the code each opcode runs; operands marked `?` are read but not yet understood.

| op | name | operands |
|---|---|---|
| 0 | `INVALID0` | - |
| 1 | `INVALID1` | - |
| 2 | `RETURN` | end of script: pops the call stack |
| 3 | `SAY` | str|0xF000+id  -> dialogue box, waits for key |
| 4 | `SET_SCREEN_SIZE` | b w, b h |
| 5 | `NOP_B2` | b |
| 6 | `NOP_B3` | b |
| 7 | `SET_PLAYER_COLLIDES` | b flag |
| 8 | `LOAD_MAP` | b len,str .jtm path; b len,str tile sprite (.cml) path |
| 9 | `SKIP_STRING` | b len,str (ignored) |
| 10 | `END_LEVEL` | b kind, i arg |
| 11 | `WAIT` | s ms |
| 12 | `SET_STATE_PLAYING` | - |
| 13 | `NOP_B4` | b |
| 14 | `SET_KEY_HOOK` | b key(0-4), script id (b) run when that key is pressed |
| 15 | `SPAWN_ACTOR` | s name(str|0xF000+id, 0=none), b slot, b monsterType, s x, s y |
| 16 | `SET_TRIGGER` | b x, b y, b enter script, b leave script, b zone |
| 17 | `MOVE_ACTOR_TO` | b slot, s x, s y |
| 18 | `SET_TILE` | b x, b y, b layer, b tile |
| 19 | `SET_INPUT_ENABLED` | b flag |
| 20 | `REMOVE_ACTOR` | b slot |
| 21 | `WAIT_ACTORS_STOP` | b n, n x b slot |
| 22 | `SET_COLLISION` | b x, b y, b solid |
| 23 | `CALL` | b script id |
| 24 | `SET_ANIM_STATE` | b slot, b state |
| 25 | `CAMERA_TO` | s x, s y |
| 26 | `CAMERA_FOLLOW` | b slot |
| 27 | `CLEAR_TRIGGER` | b x, b y |
| 28 | `CLEAR_KEY_HOOK` | b key(0-4) |
| 29 | `LOAD_LEVEL` | b len,str .scr path |
| 32 | `SET_DEATH_SCRIPT` | b slot, b arg, b script |
| 33 | `CLEAR_DEATH_SCRIPT` | b slot, b arg |
| 34 | `SET_STAT` | b slot, b stat, value (b or s depending on stat) |
| 35 | `NOP_B5` | b |
| 36 | `SET_POSITION` | b slot, s x, s y |
| 37 | `GIVE_ITEM` | b slot, b category(0 weapon,1 armor,2 consumable), b index |
| 38 | `REMOVE_ITEM` | b slot, b category, b index |
| 39 | `SHOW_MESSAGE` | s text(str|0xF000+id), b seconds, b colour, b style |
| 40 | `HIDE_MESSAGE` | - |
| 41 | `MOVE_ACTOR_X` | b slot, s x |
| 42 | `MOVE_ACTOR_Y` | b slot, s y |
| 43 | `LOAD_HUD_SPRITES` | b len,str .cml path |
| 44 | `OPEN_MENU` | - |
| 45 | `OPEN_SHOP_MENU` | - |
| 46 | `SET_STATUS_ICON` | b slot, b icon |
| 47 | `GENERATE_DUNGEON` | b spawnGroup, b, b |
| 48 | `CLEAR_LAYERS` | - |
| 49 | `PLACE_ITEM` | b item, b x, b y |
| 50 | `SET_TRIGGER_RECT` | b x1, b y1, b x2, b y2, b enter, b leave, b zone |
| 51 | `CLEAR_TRIGGER_RECT` | b x1, b y1, b x2, b y2 |
| 52 | `WALK_CUTSCENE` | b slot, b axis, s x, s y |
| 53 | `TALK` | b slot, b icon, s text(str|0xF000+id) |
| 54 | `NOP54` | - |
| 55 | `NOP55` | - |
| 56 | `LOAD_LANG` | b len,str name, b count |
| 57 | `NOP57` | - |
| 58 | `SCALE_MONSTER` | b monsterType, b level |
| 59 | `SET_DROPS_LOOT` | b slot, b flag |
| 60 | `WAIT_KEY` | - |
| 61 | `SET_STATE_9` | - |
| 62 | `NOP62` | - |
| 63 | `NOP63` | - |
| 64 | `SET_BACKGROUND_COLOR` | i rgb |
| 65 | `LEVEL_UP_TO` | b slot, b level |
| 66 | `SHOW_TEXT_SCREEN` | s text(str|0xF000+id) |
| 67 | `RESTORE_MONSTER_TYPE` | b monsterType |
| 68 | `SPAWN_PROJECTILE` | b kind(0,1,2), s x, s y |
| 69 | `SPAWN_TIMED_PROJECTILE` | b kind, s x, s y, b seconds |
| 70 | `CLEAR_PROJECTILE_AT` | s x, s y |
| 71 | `SET_POINT` | s, s |
| 72 | `EVICT_SPRITES` | s len,str path prefix |
| 73 | `BEGIN_FADE` | - |
| 74 | `END_FADE` | - |
| 75 | `TOGGLE_INVULNERABLE` | b slot |
| 76 | `SET_HUD_VISIBLE` | b flag |
| 77 | `SET_STATE_4` | - |
| 78 | `SET_AI_ACTIVE` | b slot, b flag |
