#include "world/dungeon_generator.h"

#include <algorithm>

namespace stormhold {

namespace {

// Dungeon.java's DIFFICULTY_TIER_LOOKUP: level index (levelNumber - 2,
// i.e. levels 2..37) -> a "difficulty tier" in [1,36], NOT the level
// number itself.
const int kDifficultyTierLookup[36] = {1,  5,  9,  13, 14, 15, 22, 23, 24, 2,  6,  10, 19, 20, 21, 31, 32,
                                        33, 3,  7,  11, 16, 17, 18, 28, 29, 30, 4,  8,  12, 25, 26, 27, 34,
                                        35, 36};

// Dungeon.java's MONSTER_TYPE_BY_TIER: [zone][rarityBucket 0-3] -> monster
// type id. zone is tier - 1 (the permuted tier value), clamped to [0,36].
const int kMonsterTypeByTier[37][4] = {
    {1, 2, 1, 3},    {6, 7, 8, 6},    {1, 2, 3, 1},     {6, 7, 8, 7},     {3, 4, 11, 12},   {8, 9, 11, 12},
    {3, 4, 12, 13},  {8, 9, 12, 13},  {4, 5, 12, 13},   {9, 10, 12, 13},  {4, 5, 13, 14},   {9, 10, 13, 14},
    {12, 13, 4, 5},  {12, 13, 9, 10}, {13, 14, 15, 16}, {14, 15, 16, 17}, {15, 16, 17, 18}, {16, 17, 18, 21},
    {17, 18, 19, 26}, {18, 19, 20, 21}, {19, 20, 26, 27}, {21, 22, 26, 27}, {21, 22, 27, 28}, {22, 23, 27, 28},
    {22, 23, 28, 29}, {23, 24, 28, 29}, {23, 24, 29, 30}, {24, 25, 29, 30}, {26, 27, 28, 31}, {27, 28, 31, 32},
    {28, 29, 32, 33}, {29, 30, 33, 34}, {31, 32, 34, 35}, {32, 33, 36, 37}, {34, 35, 37, 38}, {38, 39, 40, 35},
    {38, 39, 40, 35}};

// Dungeon.java's CHEST_GIFT_SUBTYPE_BY_TIER, indexed by [tier-1]: the
// "gift" item subtype rolled for chest #1 (the guaranteed special item).
const int kChestGiftSubtypeByTier[36] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 1, 1, 3, 1, 3, 3,
                                          1, 3, 3, 3, 3, 5, 3, 5, 5, 3, 5, 5, 5, 5, 5, 5, 5, 5};

// The 6 always-present hub-town shop tiles Dungeon.java's hub-grid
// constructor marks special (bit 32) -- Shop.SHOP_X[0..5]/SHOP_Y[0..5] in
// the Java source (this port has no Shop class yet, so these are inlined
// here with their real values). SHOP_X[6]/SHOP_Y[6] (the Warden, (9,9)) is
// deliberately excluded -- see BuildHubLevel's doc comment.
const int kHubShopX[6] = {12, 3, 15, 6, 7, 12};
const int kHubShopY[6] = {3, 7, 7, 13, 2, 13};

// rollRoomRect()'s own random draws use `Math.abs(rng.nextInt()) %
// bound` -- ABS THEN MOD -- unlike every other random draw in
// Dungeon.java, which uses `Math.abs(rng.nextInt() % bound)` (MOD then
// ABS, i.e. RandomInt0Based). Confirmed by reading rollRoomRect() line by
// line: the `Math.abs(...)` call closes before the `%` in every one of
// its 6 draws. Kept local rather than in java_random.h since nothing else
// in this codebase uses this order.
int32_t AbsThenMod(JavaRandom& rng, int32_t bound) { return JavaAbs(rng.NextInt()) % bound; }

int32_t PackCoord(int x, int y) { return (x << 16) | (y & 0xFFFF); }
void UnpackCoord(int32_t packed, int& x, int& y) {
    x = (packed >> 16) & 0xFFFF;
    y = packed & 0xFFFF;
}

// carveRect(): clears every tile in a w x h rectangle to floor(0), EXCEPT
// tiles already marked with bit 3 (mask 8) -- those are preserved
// untouched. Unlike dawnstar's own CarveRect, Stormhold's version only
// ever special-cases bit 8, never bit 32 too -- there's no "shop room"
// tile-preservation case here since generation itself never sets bit 32
// (see BuildHubLevel's doc comment on the shop-mechanic difference).
void CarveRect(std::vector<std::vector<uint8_t>>& tiles, int x, int w, int y, int h) {
    for (int i = x; i < x + w; i++) {
        for (int j = y; j < y + h; j++) {
            if (tiles[i][j] != 8) tiles[i][j] = 0;
        }
    }
}

struct RoomRect {
    int x0, y0, x1, y1, doorX, doorY;
};

// rollRoomRect(): 2-5 tiles wide/tall, positioned within [3,31]x[3,31],
// plus a random interior "door" point. All 6 draws use AbsThenMod (see
// its own comment above).
RoomRect RollRoomRect(JavaRandom& rng) {
    const int origin = 3;
    const int maxCoord = 31;
    const int sizeRange = 4;
    RoomRect rect;
    int w = 2 + AbsThenMod(rng, sizeRange);
    int h = 2 + AbsThenMod(rng, sizeRange);
    int xRange = maxCoord - origin + 1 - (w - 1);
    int yRange = maxCoord - origin + 1 - (h - 1);
    rect.x0 = origin + AbsThenMod(rng, xRange);
    rect.y0 = origin + AbsThenMod(rng, yRange);
    rect.x1 = rect.x0 + (w - 1);
    rect.y1 = rect.y0 + (h - 1);
    rect.doorX = rect.x0 + AbsThenMod(rng, w);
    rect.doorY = rect.y0 + AbsThenMod(rng, h);
    return rect;
}

// tryPlaceRoom(): succeeds only if every tile in the room's bounding box
// (+1 tile margin, clamped to [0,34]) is still untouched wall (0 would
// mean already-carved floor). On success, carves the room and -- since a
// minimum 2x2 room is never a single point -- always registers it.
bool TryPlaceRoom(const RoomRect& rect, std::vector<std::vector<uint8_t>>& tiles, std::vector<RoomRect>& roomList) {
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

// carveStairwell(): fixed 1-wide stairway corridor from the level border
// to a point 5 tiles in, for compass direction dir (1=N,2=E,3=S,4=W).
// Returns the packed coordinate of its inner endpoint.
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

// carveCorridorBetween(): unpacks both room-ids to (x,y) centers, picks a
// random L-shape order (horizontal segment first or vertical first), and
// carves the two connecting rectangle strips.
void CarveCorridorBetween(std::vector<std::vector<uint8_t>>& tiles, JavaRandom& rng, int32_t fromPacked,
                           int32_t toPacked) {
    int fx, fy, tx, ty;
    UnpackCoord(fromPacked, fx, fy);
    UnpackCoord(toPacked, tx, ty);
    int horizontalFirst = RandomInt0Based(rng, 2);

    if (horizontalFirst == 0) {
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

// connectRooms(): nearest-neighbor-chain heuristic (not a true MST): for
// each registered room (in registration order), finds its closest
// still-unconnected room and carves a corridor to it.
void ConnectRooms(std::vector<std::vector<uint8_t>>& tiles, JavaRandom& rng, const std::vector<int32_t>& connectedSet,
                   std::vector<int32_t>& unconnectedSet) {
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

        if (haveNearest) CarveCorridorBetween(tiles, rng, from, nearest);
        if (selfIndex != -1) unconnectedSet.erase(unconnectedSet.begin() + selfIndex);
    }
}

// randomRoomIndices(): picks 5 distinct random room indices by assigning
// every room a random 1-based key in [1,1000] (Util.randomInt, i.e.
// RandomInt1Based) and insertion-sorting descending -- equivalent to
// sampling without replacement. Transcribed as an insertion sort directly
// (not std::sort/std::stable_sort) so tie-breaking matches the original
// bit-for-bit.
std::vector<int> RandomRoomIndices(JavaRandom& rng, int roomCount, int count) {
    std::vector<int> indices(roomCount);
    std::vector<int> keys(roomCount);
    for (int i = 0; i < roomCount; i++) {
        indices[i] = i;
        keys[i] = RandomInt1Based(rng, 1000);
    }

    for (int i = 1; i < roomCount; i++) {
        int key = keys[i];
        int idx = indices[i];
        int j;
        for (j = i - 1; j >= 0 && keys[j] < key; j--) {
            keys[j + 1] = keys[j];
            indices[j + 1] = indices[j];
        }
        keys[j + 1] = key;
        indices[j + 1] = idx;
    }

    return std::vector<int>(indices.begin(), indices.begin() + count);
}

// placeChests(): 5 chests in 5 random rooms. The first is a guaranteed
// "gift"/special item; the other 4 roll normal loot (2 bonus rolls).
// Avoids any tile already marked with bit 3 (mask 8).
void PlaceChests(const std::vector<int>& roomIdx, const std::vector<RoomRect>& roomList, int tier, JavaRandom& rng,
                  const ItemDatabase& items, GeneratedLevel& level) {
    int giftSubtype = kChestGiftSubtypeByTier[tier - 1];
    bool first = true;

    for (int i = 0; i < 5; i++) {
        const RoomRect& room = roomList[roomIdx[i]];
        int itemId = first ? items.RandomGiftItemOfSubtype(rng, giftSubtype) : items.RollLoot(rng, tier, 2);

        int w = room.x1 - room.x0 + 1;
        int h = room.y1 - room.y0 + 1;
        int x = room.x0 + RandomInt0Based(rng, w);
        int y;
        for (y = room.y0 + RandomInt0Based(rng, h); (level.tiles[x][y] & 8) != 0;
             y = room.y0 + RandomInt0Based(rng, h)) {
            x = room.x0 + RandomInt0Based(rng, w);
        }

        GeneratedChestSpawn chest;
        chest.x = x;
        chest.y = y;
        chest.guaranteedGift = first;
        // Dungeon.placeChests() draws a random [0,2] "tier bits" value
        // here (`Math.abs(rng.nextInt() % 3) << 6`) and packs it into the
        // chest record's byte 3 alongside `tier` -- no confirmed read
        // site for those top 2 bits anywhere in ../../../src/, but the
        // draw is preserved (RNG state must advance identically) even
        // though its result has no observable effect on anything this
        // port models.
        RandomInt0Based(rng, 3);
        chest.itemId = itemId;
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

int DungeonGenerator::MonsterTypeForTierBucket(int tierIndex, int bucket) { return kMonsterTypeByTier[tierIndex][bucket]; }

GeneratedLevel DungeonGenerator::BuildHubLevel(const DungeonGeomRow& geomRow) {
    GeneratedLevel level;
    level.number = 1;
    level.tier = InitTier(1);
    level.width = 19;
    level.height = 19;
    level.tiles.assign(19, std::vector<uint8_t>(19, 1));

    for (int x = 0; x < 19; x++) level.tiles[x][9] = 0;
    for (int y = 0; y < 19; y++) level.tiles[9][y] = 0;
    for (int dx = 0; dx < 3; dx++) {
        for (int dy = 0; dy < 3; dy++) level.tiles[8 + dx][8 + dy] = 0;
    }

    level.tiles[4][8] = 0;
    level.tiles[4][7] = 0;
    level.tiles[3][7] = 0;
    level.tiles[16][8] = 0;
    level.tiles[16][7] = 0;
    level.tiles[15][7] = 0;
    level.tiles[8][3] = 0;
    level.tiles[7][3] = 0;
    level.tiles[7][2] = 0;
    level.tiles[10][4] = 0;
    level.tiles[11][4] = 0;
    level.tiles[12][4] = 0;
    level.tiles[12][3] = 0;
    level.tiles[6][14] = 0;
    level.tiles[7][14] = 0;
    level.tiles[8][14] = 0;
    level.tiles[9][14] = 0;
    level.tiles[10][14] = 0;
    level.tiles[11][14] = 0;
    level.tiles[12][14] = 0;
    level.tiles[6][13] = 0;
    level.tiles[12][13] = 0;

    for (int i = 0; i < 6; i++) {
        level.tiles[kHubShopX[i]][kHubShopY[i]] |= 32;
    }

    level.neighborNorth = geomRow.north;
    level.neighborEast = geomRow.east;
    level.neighborSouth = geomRow.south;
    level.neighborWest = geomRow.west;
    level.stairsUpDir = geomRow.stairsUpDir;
    level.stairsDownDir = geomRow.stairsDownDir;
    level.visited = false;
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

    // NOTE the seed formula: levelNumber * 5000, not dawnstar's * 8000
    // (../../src/Dungeon.java's own header comment, M5's own finding).
    JavaRandom rng(static_cast<int64_t>(levelNumber) * 5000);

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

    bool markedCenter = false;
    int placed = 0;

    while (placed < 15) {
        RoomRect rect = RollRoomRect(rng);
        if (TryPlaceRoom(rect, level.tiles, roomList)) {
            placed++;
            int doorCoord = PackCoord(rect.doorX, rect.doorY);
            RegisterRoom(doorCoord, connectedSet, unconnectedSet);

            // After placing >=2 rooms, the first room found that's at
            // least 3x3 (and whose center isn't already the room's own
            // door tile) gets its center tile marked with bit 3 (mask 8)
            // -- exact purpose not confirmed (see Dungeon.java's own
            // class header), but set exactly once per level, on EVERY
            // level -- unlike dawnstar, there is no level-number-gated
            // "this is a special shopkeeper room" branch here at all (see
            // BuildHubLevel's doc comment on the shop-mechanic
            // difference).
            if (placed >= 2 && !markedCenter) {
                int w = rect.x1 - rect.x0 + 1;
                int h = rect.y1 - rect.y0 + 1;
                if (w >= 3 && h >= 3) {
                    int cx = rect.x0 + w / 2;
                    int cy = rect.y0 + h / 2;
                    if (cx != rect.doorX || cy != rect.doorY) {
                        level.tiles[cx][cy] |= 8;
                        markedCenter = true;
                    }
                }
            }
        }
    }

    ConnectRooms(level.tiles, rng, connectedSet, unconnectedSet);
    int roomCount = static_cast<int>(roomList.size());

    // One monster spawned at the center (door position) of every placed
    // room. Level 37's LAST room is a forced monster type 41 (Dungeon.
    // java's own class header comment -- NOT dawnstar's 42) -- every
    // other room/level rolls a type via the tier/bucket roll below.
    for (int i = 0; i < roomCount; i++) {
        const RoomRect& room = roomList[i];
        int monsterType;
        if (levelNumber == 37 && i == roomCount - 1) {
            monsterType = 41;
        } else {
            int tierRoll = RandomInt1Based(rng, 10);
            int bucket = tierRoll <= 4 ? 0 : tierRoll <= 7 ? 1 : tierRoll <= 9 ? 2 : 3;
            int zone = std::max(0, std::min(36, level.tier - 1));
            monsterType = kMonsterTypeByTier[zone][bucket];
        }

        GeneratedMonsterSpawn spawn;
        spawn.x = room.doorX;
        spawn.y = room.doorY;
        spawn.monsterType = monsterType;
        spawn.hp = monsters.RawStat(monsterType, 14);
        spawn.spawnId = i + 1;
        level.tiles[spawn.x][spawn.y] |= 2;
        level.monsters.push_back(spawn);
    }

    std::vector<int> top5 = RandomRoomIndices(rng, roomCount, 5);
    PlaceChests(top5, roomList, level.tier, rng, items, level);

    level.neighborNorth = geomRow.north;
    level.neighborEast = geomRow.east;
    level.neighborSouth = geomRow.south;
    level.neighborWest = geomRow.west;
    level.stairsUpDir = geomRow.stairsUpDir;
    level.stairsDownDir = geomRow.stairsDownDir;
    level.visited = false;

    return level;
}

}  // namespace stormhold
