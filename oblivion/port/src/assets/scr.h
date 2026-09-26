#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace oblivion {

// Opcodes of the .scr bytecode, see docs/SCR_OPCODES.md (== the OP_* constants
// in src/ScriptInterpreter.java).
enum ScrOp : int {
    OP_RETURN = 2, OP_SAY = 3, OP_SET_SCREEN_SIZE = 4, OP_SET_PLAYER_COLLIDES = 7,
    OP_LOAD_MAP = 8, OP_SKIP_STRING = 9, OP_END_LEVEL = 10, OP_WAIT = 11,
    OP_SET_STATE_PLAYING = 12, OP_SET_KEY_HOOK = 14, OP_SPAWN_ACTOR = 15,
    OP_SET_TRIGGER = 16, OP_MOVE_ACTOR_TO = 17, OP_SET_TILE = 18,
    OP_SET_INPUT_ENABLED = 19, OP_REMOVE_ACTOR = 20, OP_WAIT_ACTORS_STOP = 21,
    OP_SET_COLLISION = 22, OP_CALL = 23, OP_SET_ANIM_STATE = 24, OP_CAMERA_TO = 25,
    OP_CAMERA_FOLLOW = 26, OP_CLEAR_TRIGGER = 27, OP_CLEAR_KEY_HOOK = 28,
    OP_LOAD_LEVEL = 29, OP_SET_DEATH_SCRIPT = 32, OP_CLEAR_DEATH_SCRIPT = 33,
    OP_SET_STAT = 34, OP_SET_POSITION = 36, OP_GIVE_ITEM = 37, OP_REMOVE_ITEM = 38,
    OP_SHOW_MESSAGE = 39, OP_HIDE_MESSAGE = 40, OP_MOVE_ACTOR_X = 41,
    OP_MOVE_ACTOR_Y = 42, OP_LOAD_HUD_SPRITES = 43, OP_OPEN_MENU = 44,
    OP_OPEN_SHOP_MENU = 45, OP_SET_STATUS_ICON = 46, OP_GENERATE_DUNGEON = 47,
    OP_CLEAR_LAYERS = 48, OP_PLACE_ITEM = 49, OP_SET_TRIGGER_RECT = 50,
    OP_CLEAR_TRIGGER_RECT = 51, OP_WALK_CUTSCENE = 52, OP_TALK = 53,
    OP_LOAD_LANG = 56, OP_SCALE_MONSTER = 58, OP_SET_DROPS_LOOT = 59,
    OP_WAIT_KEY = 60, OP_SET_STATE_9 = 61, OP_SET_BACKGROUND_COLOR = 64,
    OP_LEVEL_UP_TO = 65, OP_SHOW_TEXT_SCREEN = 66, OP_RESTORE_MONSTER_TYPE = 67,
    OP_SPAWN_PROJECTILE = 68, OP_SPAWN_TIMED_PROJECTILE = 69,
    OP_CLEAR_PROJECTILE_AT = 70, OP_SET_RESPAWN_POINT = 71, OP_EVICT_SPRITES = 72,
    OP_BEGIN_FADE = 73, OP_END_FADE = 74, OP_TOGGLE_INVULNERABLE = 75,
    OP_SET_HUD_VISIBLE = 76, OP_SET_STATE_4 = 77, OP_SET_AI_ACTIVE = 78,
};

// A string operand: either a literal or a localized id (0xF000 | id in the
// stream, the id here).
struct ScrString {
    bool isId = false;
    int id = 0;
    std::string text;
};

// One decoded instruction (for tools and for finding things in a script; the
// interpreter itself will read operands straight from the bytes like the
// original). `args` holds the numeric operands in stream order; string
// operands go to `strings`.
struct ScrInsn {
    int op = 0;
    size_t offset = 0;  // code-relative
    size_t length = 0;
    std::vector<int> args;
    std::vector<ScrString> strings;
};

// The record tables a .scr loads (ScriptInterpreter.load). Dimensions and row
// meanings are documented in docs/CLASS_MAP.md. String fields hold an index
// into `strings` (literal) or 0xF000|id (localized id).
struct ScrTables {
    int monsterTypes[25][21] = {};
    int armors[42][10] = {};
    int consumables[11][14] = {};
    int weapons[37][8] = {};
    int classBase[9][15] = {};
    int classItemTypes[9][15] = {};
    int classLists[9][15] = {};
    int table6[25][7] = {};
    int specials[10][15] = {};
    int spawnGroups[10][21] = {};
    int loot[30][4] = {};
    int pairTable[100] = {-1};
    int spawnIds[10] = {};
    int spawnIdCount = 0;
    std::vector<std::string> strings;
};

struct Scr {
    ScrTables tables;
    std::vector<uint8_t> code;   // bytecode; script offsets index into it
    int scriptOffset[256];       // code-relative; -1 = no such script

    Scr() { for (int& o : scriptOffset) o = -1; }

    // Decodes the instruction at `pc` (code-relative). Throws on an unknown
    // opcode or a truncated operand.
    ScrInsn Decode(size_t pc) const;

    // Instructions of script `id`, up to and including its RETURN.
    std::vector<ScrInsn> Script(int id) const;
};

// Parses a whole .scr file. Throws std::runtime_error on malformed data.
Scr ParseScr(const std::vector<uint8_t>& data);

}  // namespace oblivion
