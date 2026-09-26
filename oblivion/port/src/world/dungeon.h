#pragma once
#include <cstdint>
#include <functional>
#include <vector>

#include "assets/jtm.h"

namespace oblivion {

// The random dungeon of GENERATE_DUNGEON (Game.generateDungeon / carvePath /
// carveEndpoints / pickWallTiles). `group` is a spawnGroups row:
//   [1] width, [2] height, [3] rock tile, [4] path tile, [5..12] the wall
//   edge tiles, [13] stairs tile, [14] path width, [15] max branches,
//   [16] branch odds (1 in n per step), [17] monster type, [18] how many
//   branch-point entries may hold a monster (2 per point), [19] odds of a
//   branch point per segment.
struct Dungeon {
    JtmMap map;  // layers: 0 collision, 1 floor, 2 walls, 3 objects (stairs)
    std::vector<int8_t> enter, leave, zone;  // trigger layers, -1 = none
    std::vector<int> branchPoints;           // x, y pairs: where monsters and items go
    int start[2] = {2, 2}, end[2] = {0, 0};  // the two stairs
};

// `rng` is Random.nextInt() (any signed 32-bit value). `zone` goes on the
// entry stairs, `exitScript` on the exit stairs' enter trigger.
Dungeon GenerateDungeon(const int* group, int zone, int exitScript, const std::function<int()>& rng);

}  // namespace oblivion
