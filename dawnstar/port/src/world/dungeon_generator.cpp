#include "world/dungeon_generator.h"

#include <algorithm>
#include <stdexcept>

namespace dawnstar {

namespace {

// Dungeon.java's DIFFICULTY_TIER_LOOKUP: level index (level number - 2,
// i.e. levels 2..37) -> a "difficulty tier" in [1,36], NOT the level
// number itself (see the Java source's own doc comment on this table).
const int kDifficultyTierLookup[36] = {1,  5,  9,  13, 14, 15, 22, 23, 24, 2,  6,  10, 19, 20, 21, 31, 32,
                                        33, 3,  7,  11, 16, 17, 18, 28, 29, 30, 4,  8,  12, 25, 26, 27, 34,
                                        35, 36};

// Dungeon.java's MONSTER_TABLE: [tierIndex][difficultyBucket 0-3] ->
// monster type id. tierIndex is tier - 1 (the permuted tier value).
const int kMonsterTable[37][4] = {
    {1, 2, 1, 3},    {6, 7, 8, 6},    {1, 2, 3, 1},    {6, 7, 8, 7},    {3, 4, 11, 12},  {8, 9, 11, 12},
    {3, 4, 12, 13},  {8, 9, 12, 13},  {4, 5, 12, 13},  {9, 10, 12, 13}, {4, 5, 13, 14},  {9, 10, 13, 14},
    {12, 13, 4, 5},  {12, 13, 9, 10}, {13, 14, 15, 16}, {14, 15, 16, 17}, {15, 16, 17, 18}, {16, 17, 18, 21},
    {17, 18, 19, 26}, {18, 19, 20, 21}, {19, 20, 26, 27}, {21, 22, 26, 27}, {21, 22, 27, 28}, {22, 23, 27, 28},
    {22, 23, 28, 29}, {23, 24, 28, 29}, {23, 24, 29, 30}, {24, 25, 29, 30}, {26, 27, 28, 31}, {27, 28, 31, 32},
    {28, 29, 32, 33}, {29, 30, 33, 34}, {31, 32, 34, 35}, {32, 33, 36, 37}, {34, 35, 37, 38}, {38, 39, 40, 35},
    {38, 39, 40, 35}};

// DungeonGenerator.java's ZONE_TIER_CLASS: coarse 1/3/5 difficulty class
// per tier (indexed by tier - 1) -- reused as placeChests's guaranteed
// first chest's gift-item subtype selector.
const int kZoneTierClass[36] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 1, 1, 3, 1, 3, 3,
                                 1, 3, 3, 3, 3, 5, 3, 5, 5, 3, 5, 5, 5, 5, 5, 5, 5, 5};

// The 5 fixed hub-town shop tiles Dungeon.java's hub-grid constructor
// marks special (bit 32) -- Shop.SHOP_X[0..4]/SHOP_Y[0..4] in the Java
// source (this port has no Shop class yet, so these are inlined here
// with their real values rather than a cross-reference).
const int kHubShopX[5] = {12, 6, 7, 12, 12};
const int kHubShopY[5] = {12, 11, 7, 8, 6};

int32_t AbsThenMod(JavaRandom& rng, int32_t bound) { return JavaAbs(rng.NextInt()) % bound; }

int32_t PackCoord(int x, int y) { return (x << 16) | (y & 0xFFFF); }
void UnpackCoord(int32_t packed, int& x, int& y) {
    x = (packed >> 16) & 0xFFFF;
    y = packed & 0xFFFF;
}

// Clears the wall bit over a w x h rectangle at (x, y), preserving the
// bit-8/bit-32 special-room markers.
void CarveRect(std::vector<std::vector<uint8_t>>& tiles, int x, int w, int y, int h) {
    for (int i = x; i < x + w; i++) {
        for (int j = y; j < y + h; j++) {
            if (tiles[i][j] != 8 && tiles[i][j] != 32) tiles[i][j] = 0;
        }
    }
}

struct RoomRect {
    int x0, y0, x1, y1, doorX, doorY;
};

// Random room in [3,31]x[3,31] with width/height in [2,5] and a random
// interior "door" point.
RoomRect RandomRoomRect(JavaRandom& rng) {
    const int origin = 3;
    const int max = 31;
    const int sizeRange = 4;
    RoomRect rect;
    int w = 2 + AbsThenMod(rng, sizeRange);
    int h = 2 + AbsThenMod(rng, sizeRange);
    int xSpan = max - origin + 1 - (w - 1);
    int ySpan = max - origin + 1 - (h - 1);
    rect.x0 = origin + AbsThenMod(rng, xSpan);
    rect.y0 = origin + AbsThenMod(rng, ySpan);
    rect.x1 = rect.x0 + (w - 1);
    rect.y1 = rect.y0 + (h - 1);
    rect.doorX = rect.x0 + AbsThenMod(rng, w);
    rect.doorY = rect.y0 + AbsThenMod(rng, h);
    return rect;
}

// Rejects (returns false) if any tile in `rect`, expanded by a 1-tile
// margin, is already carved (non-wall). Otherwise carves it and records
// it in roomList.
bool TryPlaceRoom(const RoomRect& rect, std::vector<std::vector<uint8_t>>& tiles,
                   std::vector<RoomRect>& roomList) {
    int x0 = std::max(rect.x0 - 1, 0);
    int x1 = std::min(rect.x1 + 1, 34);
    int y0 = std::max(rect.y0 - 1, 0);
    int y1 = std::min(rect.y1 + 1, 34);

    for (int x = x0; x <= x1; x++) {
        for (int y = y0; y <= y1; y++) {
            if (tiles[x][y] == 0) return false;
        }
    }

    CarveRect(tiles, rect.x0, rect.x1 - rect.x0 + 1, rect.y0, rect.y1 - rect.y0 + 1);
    if (rect.x1 != rect.x0 && rect.y1 != rect.y0) {
        roomList.push_back(rect);
    }
    return true;
}

// Carves the fixed-position stairway corridor for compass direction dir
// (1=N,2=E,3=S,4=W) and returns the packed coordinate of its inner
// endpoint, for room-connection purposes.
int CarveStairwayCorridor(std::vector<std::vector<uint8_t>>& tiles, int dir) {
    if (dir == 1) {
        CarveRect(tiles, 17, 1, 0, 5);
        return PackCoord(17, 4);
    }
    if (dir == 3) {
        CarveRect(tiles, 17, 1, 30, 5);
        return PackCoord(17, 30);
    }
    if (dir == 4) {
        CarveRect(tiles, 0, 5, 17, 1);
        return PackCoord(4, 17);
    }
    if (dir == 2) {
        CarveRect(tiles, 30, 5, 17, 1);
        return PackCoord(30, 17);
    }
    return -1;
}

bool IsCardinalDirection(int dir) { return dir >= 1 && dir <= 4; }

void RegisterRoom(int32_t packedCoord, std::vector<int32_t>& connectedSet, std::vector<int32_t>& unconnectedSet) {
    connectedSet.push_back(packedCoord);
    unconnectedSet.push_back(packedCoord);
}

int SquaredDistance(int32_t packedA, int32_t packedB) {
    int ax, ay, bx, by;
    UnpackCoord(packedA, ax, ay);
    UnpackCoord(packedB, bx, by);
    return (bx - ax) * (bx - ax) + (by - ay) * (by - ay);
}

// L-shaped corridor between two packed coordinates, orientation
// (horizontal-then-vertical vs. vertical-then-horizontal) chosen
// randomly.
void CarveCorridorBetween(std::vector<std::vector<uint8_t>>& tiles, JavaRandom& rng, int32_t fromPacked,
                           int32_t toPacked) {
    int fx, fy, tx, ty;
    UnpackCoord(fromPacked, fx, fy);
    UnpackCoord(toPacked, tx, ty);
    int orientation = RandomIntBelow(rng, 2);

    if (orientation == 0) {
        if (tx > fx) {
            CarveRect(tiles, fx, tx - fx + 1, fy, 1);
        } else {
            CarveRect(tiles, tx, fx - tx + 1, fy, 1);
        }
        if (ty > fy) {
            CarveRect(tiles, tx, 1, fy, ty - fy + 1);
        } else {
            CarveRect(tiles, tx, 1, ty, fy - ty + 1);
        }
    } else {
        if (ty > fy) {
            CarveRect(tiles, fx, 1, fy, ty - fy + 1);
        } else {
            CarveRect(tiles, fx, 1, ty, fy - ty + 1);
        }
        if (tx > fx) {
            CarveRect(tiles, fx, tx - fx + 1, ty, 1);
        } else {
            CarveRect(tiles, tx, fx - tx + 1, ty, 1);
        }
    }
}

// Nearest-neighbor-chain corridor connection: for each registered room
// (in registration order), finds its closest still-unconnected room and
// carves a corridor to it. Not a true MST.
void ConnectRooms(std::vector<std::vector<uint8_t>>& tiles, JavaRandom& rng,
                   const std::vector<int32_t>& connectedSet, std::vector<int32_t>& unconnectedSet) {
    int total = static_cast<int>(connectedSet.size());

    for (int i = 0; i < total; i++) {
        int32_t from = connectedSet[i];
        int poolSize = static_cast<int>(unconnectedSet.size());
        int best = INT32_MAX;
        int32_t nearest = 0;
        bool haveNearest = false;
        int selfIndex = -1;

        for (int j = 0; j < poolSize; j++) {
            int32_t candidate = unconnectedSet[j];
            if (candidate != from) {
                int dist = SquaredDistance(from, candidate);
                if (dist < best) {
                    best = dist;
                    nearest = candidate;
                    haveNearest = true;
                }
            } else {
                selfIndex = j;
            }
        }

        if (haveNearest) {
            CarveCorridorBetween(tiles, rng, from, nearest);
        }
        if (selfIndex != -1) {
            unconnectedSet.erase(unconnectedSet.begin() + selfIndex);
        }
    }
}

// Places exactly 5 chests, one in each of the 5 highest-(random-)
// weighted rooms. The first is guaranteed to contain a "gift"-category
// item of subtype ZONE_TIER_CLASS[tier-1]; the rest get a regular 2-roll
// loot pick.
void PlaceChests(const std::vector<int>& topRooms, const std::vector<RoomRect>& roomList, int tier,
                  JavaRandom& rng, const ItemDatabase& items, GeneratedLevel& level) {
    int subtype = kZoneTierClass[tier - 1];
    bool first = true;

    for (int i = 0; i < 5; i++) {
        const RoomRect& room = roomList[topRooms[i]];
        int itemId = first ? items.RandomGiftItemOfSubtype(rng, subtype) : items.RollLoot(rng, tier, 2);

        int w = room.x1 - room.x0 + 1;
        int h = room.y1 - room.y0 + 1;
        int x = room.x0 + RandomIntBelow(rng, w);
        int y;
        // Re-rolls while landing on a tile that has BOTH bit 8 and bit
        // 32 set -- preserved exactly as in the original, though no tile
        // ever has both bits set simultaneously (the special-room
        // marking always sets at most one), so this loop body never
        // actually re-rolls in practice. Likely meant `||` in the
        // original.
        for (y = room.y0 + RandomIntBelow(rng, h);
             (level.tiles[x][y] & 8) != 0 && (level.tiles[x][y] & 32) != 0;
             y = room.y0 + RandomIntBelow(rng, h)) {
            x = room.x0 + RandomIntBelow(rng, w);
        }

        GeneratedChestSpawn chest;
        chest.x = x;
        chest.y = y;
        chest.guaranteedGift = first;
        // Packed tier byte + a 2-bit random roll -- DungeonGenerator.java
        // writes `(rollOf3 << 6) | tier` into the chest record's byte 3;
        // no confirmed read site for that top 2 bits anywhere in
        // ../../../src/ (see CLASS_MAP.md), preserved here as `tier`
        // alone since the random component has no observable effect yet.
        RandomIntBelow(rng, 3);
        chest.itemId = itemId;
        // Item.nextSpawnId() is a pure monotonic counter (no RNG
        // involved, so it can't affect anything else generated here) --
        // but it's genuinely global across all 37 levels in the real
        // game (one shared Item.nextSpawnId counter, incremented as
        // DungeonGenerator's constructor runs every level in sequence).
        // A single level generated in isolation, as this function does,
        // has no way to know that running total, so this is a
        // per-level-local counter (1-5) instead of the real cross-level
        // spawn id -- fine for now since nothing reads it yet, but
        // revisit once a whole-game bootstrap (all 37 levels generated
        // in order, sharing one counter) exists.
        chest.spawnId = i + 1;
        level.chests.push_back(chest);
        level.tiles[x][y] |= 16;

        first = false;
    }
}

}  // namespace

int DungeonGenerator::InitTier(int levelNumber) {
    if (levelNumber >= 2 && levelNumber <= 37) return kDifficultyTierLookup[levelNumber - 2];
    return 1;
}

GeneratedLevel DungeonGenerator::BuildHubLevel(const DungeonGeomRow& geomRow) {
    GeneratedLevel level;
    level.number = 1;
    level.tier = InitTier(1);
    level.width = 19;
    level.height = 19;
    level.tiles.assign(19, std::vector<uint8_t>(19, 1));

    for (int x = 0; x < 19; x++) level.tiles[x][9] = 0;
    for (int y = 0; y < 19; y++) level.tiles[9][y] = 0;

    for (int i = 4; i < 15; i++) {
        level.tiles[4][i] = 0;
        level.tiles[14][i] = 0;
    }
    for (int i = 4; i < 15; i++) {
        level.tiles[i][4] = 0;
        level.tiles[i][14] = 0;
    }

    level.tiles[5][6] = 0;
    level.tiles[6][6] = 0;
    level.tiles[7][6] = 0;
    level.tiles[7][5] = 0;
    level.tiles[7][7] = 0;
    level.tiles[12][6] = 0;
    level.tiles[13][6] = 0;
    level.tiles[12][8] = 0;
    level.tiles[8][8] = 0;
    level.tiles[10][8] = 0;
    level.tiles[8][10] = 0;
    level.tiles[10][10] = 0;
    level.tiles[11][12] = 0;
    level.tiles[11][13] = 0;
    level.tiles[12][12] = 0;
    level.tiles[5][11] = 0;
    level.tiles[6][11] = 0;

    for (int i = 0; i < 5; i++) {
        level.tiles[kHubShopX[i]][kHubShopY[i]] |= 32;
    }

    level.neighborNorth = geomRow.north;
    level.neighborEast = geomRow.east;
    level.neighborSouth = geomRow.south;
    level.neighborWest = geomRow.west;
    level.stairsUpDir = geomRow.stairsUpDir;
    level.stairsDownDir = geomRow.stairsDownDir;
    return level;
}

GeneratedLevel DungeonGenerator::PopulateLevel(int levelNumber, const DungeonGeomRow& geomRow,
                                                const ItemDatabase& items, const MonsterDatabase& monsters) {
    GeneratedLevel level;
    level.number = levelNumber;
    level.tier = InitTier(levelNumber);
    level.width = 35;
    level.height = 35;
    level.tiles.assign(35, std::vector<uint8_t>(35, 1));

    JavaRandom rng(static_cast<int64_t>(levelNumber) * 8000);

    std::vector<int32_t> connectedSet;
    std::vector<int32_t> unconnectedSet;
    std::vector<RoomRect> roomList;

    if (IsCardinalDirection(geomRow.stairsUpDir)) {
        int stairwayRoom = CarveStairwayCorridor(level.tiles, geomRow.stairsUpDir);
        if (stairwayRoom >= 0) RegisterRoom(stairwayRoom, connectedSet, unconnectedSet);
    }
    if (IsCardinalDirection(geomRow.stairsDownDir)) {
        int stairwayRoom = CarveStairwayCorridor(level.tiles, geomRow.stairsDownDir);
        if (stairwayRoom >= 0) RegisterRoom(stairwayRoom, connectedSet, unconnectedSet);
    }

    bool pickedSpecialRoom = false;
    int placed = 0;

    while (placed < 15) {
        RoomRect rect = RandomRoomRect(rng);
        if (TryPlaceRoom(rect, level.tiles, roomList)) {
            placed++;
            int doorCoord = PackCoord(rect.doorX, rect.doorY);
            RegisterRoom(doorCoord, connectedSet, unconnectedSet);

            // The 2nd successfully-placed room becomes this level's one
            // "special" room: on the 4 key levels (3/12/21/30 -- by
            // LEVEL NUMBER, not tier) its center tile is marked bit 32
            // and recorded as that key level's shopkeeper position; on
            // every other level it's just marked bit 8 (no-spawn)
            // instead.
            if (placed >= 2 && !pickedSpecialRoom) {
                int doorX = rect.doorX;
                int doorY = rect.doorY;
                int w = rect.x1 - rect.x0 + 1;
                int h = rect.y1 - rect.y0 + 1;
                if (w >= 3 && h >= 3) {
                    int centerX = rect.x0 + w / 2;
                    int centerY = rect.y0 + h / 2;
                    if (centerX != doorX || centerY != doorY) {
                        if (levelNumber == 3 || levelNumber == 12 || levelNumber == 21 || levelNumber == 30) {
                            level.tiles[centerX][centerY] |= 32;
                            level.specialShopX = centerX;
                            level.specialShopY = centerY;
                        } else {
                            level.tiles[centerX][centerY] |= 8;
                        }
                        pickedSpecialRoom = true;
                    }
                }
            }
        }
    }

    ConnectRooms(level.tiles, rng, connectedSet, unconnectedSet);
    int roomCount = static_cast<int>(roomList.size());

    // One monster spawned in every room, at its door position.
    for (int i = 0; i < roomCount; i++) {
        const RoomRect& room = roomList[i];
        int tierRoll = LingoRandomInt(rng, 10);
        int bucket = tierRoll <= 4 ? 0 : tierRoll <= 7 ? 1 : tierRoll <= 9 ? 2 : 3;
        int tierIndex = std::max(0, std::min(36, level.tier - 1));
        int monsterType = kMonsterTable[tierIndex][bucket];

        GeneratedMonsterSpawn spawn;
        spawn.x = room.doorX;
        spawn.y = room.doorY;
        spawn.monsterType = monsterType;
        spawn.hp = monsters.Stat(monsterType, 14);
        level.tiles[spawn.x][spawn.y] |= 2;
        level.monsters.push_back(spawn);
    }

    std::vector<int> roomIndex(roomCount);
    std::vector<int> roomWeight(roomCount);
    for (int i = 0; i < roomCount; i++) {
        roomIndex[i] = i;
        roomWeight[i] = LingoRandomInt(rng, 1000);
    }

    // Insertion sort, descending by roomWeight -- transcribed exactly
    // (not std::sort/std::stable_sort) so tie-breaking matches the
    // original bit-for-bit.
    for (int i = 1; i < roomCount; i++) {
        int w = roomWeight[i];
        int idx = roomIndex[i];
        int j;
        for (j = i - 1; j >= 0 && roomWeight[j] < w; j--) {
            roomWeight[j + 1] = roomWeight[j];
            roomIndex[j + 1] = roomIndex[j];
        }
        roomWeight[j + 1] = w;
        roomIndex[j + 1] = idx;
    }

    std::vector<int> top5(roomIndex.begin(), roomIndex.begin() + 5);
    PlaceChests(top5, roomList, level.tier, rng, items, level);

    level.neighborNorth = geomRow.north;
    level.neighborEast = geomRow.east;
    level.neighborSouth = geomRow.south;
    level.neighborWest = geomRow.west;
    level.stairsUpDir = geomRow.stairsUpDir;
    level.stairsDownDir = geomRow.stairsDownDir;

    return level;
}

}  // namespace dawnstar
