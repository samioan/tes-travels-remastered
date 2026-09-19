#include "dungeon/dungeon_runtime.h"

#include <algorithm>
#include <stdexcept>

namespace stormhold {

namespace {

// Dungeon.isWalkable(x,y)'s exact bit test (1=wall, 2=monster, 8=no-
// spawn/special, 32=blocked) -- same set monster/monster_runtime.cpp's
// own Move() already duplicates rather than sharing, matching this
// port's established "small formula, don't share it across modules that
// shouldn't depend on each other" precedent (e.g. combat/
// combat_resolution.cpp's own inlined fatigueCostMultiplier).
bool IsWalkable(const GeneratedLevel& level, int x, int y) {
    uint8_t tile = level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)];
    if (tile & 0x01) return false;
    if (tile & 0x02) return false;
    if (tile & 0x08) return false;
    return !(tile & 0x20);
}

}  // namespace

void DungeonRuntime::RemoveMonster(GeneratedLevel& level, WorldRegistry& world, int16_t spawnId) {
    auto& registry = world.monsters[static_cast<size_t>(level.number - 1)];
    auto it = registry.find(spawnId);
    if (it == registry.end()) {
        throw std::runtime_error(
            "DungeonRuntime::RemoveMonster: spawnId not registered -- the real ESGame.killMonster() reads "
            "record[4]/record[5] before checking its own null Hashtable.remove() result here, a confirmed real "
            "latent NullPointerException for exactly this case (see this method's own header comment)");
    }

    const std::array<uint8_t, 28>& record = it->second;
    int x = record[4];
    int y = record[5];
    level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] =
        static_cast<uint8_t>(level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] & ~static_cast<uint8_t>(2));
    registry.erase(it);
}

std::optional<MonsterState> DungeonRuntime::MonsterAt(const GeneratedLevel& level, const WorldRegistry& world, int x,
                                                        int y) {
    uint8_t tile = level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)];
    if (tile & 0x01) return std::nullopt;
    if (!(tile & 0x02)) return std::nullopt;

    const auto& registry = world.monsters[static_cast<size_t>(level.number - 1)];
    for (const auto& [spawnId, record] : registry) {
        MonsterState m = MonsterRuntime::FromBytes(record);
        if (m.tileX == x && m.tileY == y) return m;
    }
    return std::nullopt;
}

void DungeonRuntime::StoreChest(GeneratedLevel& level, WorldRegistry& world, const std::array<int8_t, 8>& record) {
    int x = record[0];
    int y = record[1];
    world.chests[static_cast<size_t>(level.number - 1)][PackTileKey(x, y)] = record;
    level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] |= 16;
}

void DungeonRuntime::RemoveChest(GeneratedLevel& level, WorldRegistry& world, const std::array<int8_t, 8>& record) {
    int x = record[0];
    int y = record[1];
    uint8_t tile = level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)];
    if (tile & 0x01) return;
    if (!(tile & 0x10)) return;

    world.chests[static_cast<size_t>(level.number - 1)].erase(PackTileKey(x, y));
    level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] =
        static_cast<uint8_t>(level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] & ~static_cast<uint8_t>(16));
}

void DungeonRuntime::AddDroppedItem(GeneratedLevel& level, WorldRegistry& world, const std::array<int8_t, 7>& record) {
    int x = record[0];
    int y = record[1];
    world.droppedItems[static_cast<size_t>(level.number - 1)].push_back(record);
    level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] |= 4;
}

void DungeonRuntime::RemoveDroppedItem(GeneratedLevel& level, WorldRegistry& world,
                                        const std::array<int8_t, 7>& record) {
    int x = record[0];
    int y = record[1];
    uint8_t tile = level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)];
    if (tile & 0x01) return;
    if (!(tile & 0x04)) return;

    auto& items = world.droppedItems[static_cast<size_t>(level.number - 1)];
    auto it = std::find(items.begin(), items.end(), record);
    if (it != items.end()) items.erase(it);

    if (CountDroppedItemsAt(world, level.number - 1, x, y) == 0) {
        level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] = static_cast<uint8_t>(
            level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] & ~static_cast<uint8_t>(4));
    }
}

int DungeonRuntime::CountDroppedItemsAt(const WorldRegistry& world, int levelIndex, int x, int y) {
    int count = 0;
    for (const auto& record : world.droppedItems[static_cast<size_t>(levelIndex)]) {
        if (record[0] == x && record[1] == y) count++;
    }
    return count;
}

std::optional<std::array<int8_t, 7>> DungeonRuntime::FirstDroppedItemAt(const WorldRegistry& world, int levelIndex,
                                                                         int x, int y) {
    for (const auto& record : world.droppedItems[static_cast<size_t>(levelIndex)]) {
        if (record[0] == x && record[1] == y) return record;
    }
    return std::nullopt;
}

std::vector<std::array<int8_t, 7>> DungeonRuntime::DroppedItemsAt(const WorldRegistry& world, int levelIndex, int x,
                                                                    int y) {
    std::vector<std::array<int8_t, 7>> result;
    for (const auto& record : world.droppedItems[static_cast<size_t>(levelIndex)]) {
        if (record[0] == x && record[1] == y) result.push_back(record);
    }
    return result;
}

void DungeonRuntime::RefreshTileFlags(GeneratedLevel& level, const WorldRegistry& world) {
    int levelIndex = level.number - 1;

    for (const auto& [spawnId, record] : world.monsters[static_cast<size_t>(levelIndex)]) {
        MonsterState m = MonsterRuntime::FromBytes(record);
        level.tiles[static_cast<size_t>(m.tileX)][static_cast<size_t>(m.tileY)] |= 2;
    }

    for (const auto& [key, record] : world.chests[static_cast<size_t>(levelIndex)]) {
        level.tiles[static_cast<size_t>(record[0])][static_cast<size_t>(record[1])] |= 16;
    }

    for (const auto& record : world.droppedItems[static_cast<size_t>(levelIndex)]) {
        level.tiles[static_cast<size_t>(record[0])][static_cast<size_t>(record[1])] |= 4;
    }
}

void DungeonRuntime::SpawnAmbushMonsters(GeneratedLevel& level, WorldRegistry& world, int count, JavaRandom& rng,
                                          const MonsterDatabase& monsterDb, int16_t& spawnIdCounter) {
    int roomCount = static_cast<int>(level.rooms.size());
    if (roomCount == 0) return;

    for (int i = 0; i < count; i++) {
        int x, y;
        int monsterType;
        do {
            int roomIdx = RandomInt0Based(rng, roomCount);
            monsterType = MonsterRuntime::PickMonsterType(rng, level.tier, -1);
            const GeneratedRoomRect& room = level.rooms[static_cast<size_t>(roomIdx)];
            int w = room.x1 - room.x0 + 1;
            int h = room.y1 - room.y0 + 1;
            x = room.x0 + RandomInt0Based(rng, w);
            y = room.y0 + RandomInt0Based(rng, h);
        } while (!IsWalkable(level, x, y));

        int16_t spawnId = ++spawnIdCounter;
        MonsterState m = MonsterRuntime::Spawn(spawnId, monsterType, level.number, monsterDb);
        m.tileX = static_cast<int8_t>(x);
        m.tileY = static_cast<int8_t>(y);
        level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] |= 2;
        StoreMonster(world, m);
    }
}

void DungeonRuntime::RegisterGeneratedSpawns(const GeneratedLevel& level, WorldRegistry& world,
                                              const MonsterDatabase& monsterDb) {
    for (const GeneratedMonsterSpawn& spawn : level.monsters) {
        MonsterState m = MonsterRuntime::Spawn(static_cast<int16_t>(spawn.spawnId), spawn.monsterType, level.number,
                                                monsterDb);
        m.tileX = static_cast<int8_t>(spawn.x);
        m.tileY = static_cast<int8_t>(spawn.y);
        StoreMonster(world, m);
    }

    for (const GeneratedChestSpawn& chest : level.chests) {
        // Dungeon.placeChests()'s own record-building, transcribed exactly
        // -- see this method's own declaration comment for the confirmed
        // simplification on byte 3's top 2 bits.
        std::array<int8_t, 8> record{};
        record[0] = static_cast<int8_t>(chest.x);
        record[1] = static_cast<int8_t>(chest.y);
        record[2] = 0;  // confirmed always-zero on-disk byte -- see GeneratedChestSpawn's own doc comment
        record[3] = static_cast<int8_t>(level.tier);
        int low = chest.itemId & 0xFF;
        int high = 0;
        if (low == 86) high = (chest.itemId >> 8) & 0xFF;
        record[4] = static_cast<int8_t>(low);
        record[7] = static_cast<int8_t>(high);
        record[5] = static_cast<int8_t>((chest.spawnId >> 8) & 0xFF);
        record[6] = static_cast<int8_t>(chest.spawnId & 0xFF);

        world.chests[static_cast<size_t>(level.number - 1)][PackTileKey(chest.x, chest.y)] = record;
    }
}

uint8_t DungeonRuntime::TileAt(const GeneratedLevel& level, int x, int y, const LevelLookup& levels) {
    int nx = x;
    int ny = y;
    int neighborId = level.number;
    const GeneratedLevel* neighbor = nullptr;

    if (x < 0) {
        neighborId = level.neighborWest;
        if (neighborId <= 0) return 1;
        neighbor = &levels(neighborId);
        nx = neighbor->width - 1;
        if (neighborId == 1 || level.number == 1) ny = ny + (neighbor->height - level.height) / 2;
    } else if (x >= level.width) {
        neighborId = level.neighborEast;
        if (neighborId <= 0) return 1;
        neighbor = &levels(neighborId);
        nx = 0;
        if (neighborId == 1 || level.number == 1) ny = ny + (neighbor->height - level.height) / 2;
    } else if (y < 0) {
        neighborId = level.neighborNorth;
        if (neighborId <= 0) return 1;
        neighbor = &levels(neighborId);
        ny = neighbor->height - 1;
        if (neighborId == 1 || level.number == 1) nx = nx + (neighbor->width - level.width) / 2;
    } else if (y >= level.height) {
        neighborId = level.neighborSouth;
        if (neighborId <= 0) return 1;
        neighbor = &levels(neighborId);
        ny = 0;
        if (neighborId == 1 || level.number == 1) nx = nx + (neighbor->width - level.width) / 2;
    }

    if (neighborId != level.number) {
        if (nx < 0 || nx >= neighbor->width) return 1;
        if (ny < 0 || ny >= neighbor->height) return 1;
        return neighbor->populated ? neighbor->tiles[static_cast<size_t>(nx)][static_cast<size_t>(ny)] : 1;
    }
    return level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)];
}

CorridorViewGrid DungeonRuntime::SampleCorridorView(const GeneratedLevel& level, int x, int y, int facing,
                                                     const LevelLookup& levels) {
    CorridorViewGrid grid{};

    if (facing != 1 && facing != 3) {
        if (facing == 2 || facing == 4) {
            int step = (facing == 2) ? 1 : -1;
            grid[0][0] = TileAt(level, x, y - step, levels);
            // Deliberate asymmetry vs. the facing==1/3 branch below (which
            // fills the equivalent slot with a real TileAt call) --
            // confirmed directly from Dungeon.sampleCorridorView(), not a
            // transcription slip.
            grid[1][0] = 0;
            grid[2][0] = TileAt(level, x, y + step, levels);

            int fx = x + step;
            for (int i = 0; i < 5; i++) grid[static_cast<size_t>(i)][1] = TileAt(level, fx, y + (i - 2) * step, levels);
            fx = x + 2 * step;
            for (int i = 0; i < 7; i++) grid[static_cast<size_t>(i)][2] = TileAt(level, fx, y + (i - 3) * step, levels);
            fx = x + 3 * step;
            for (int i = 0; i < 9; i++) grid[static_cast<size_t>(i)][3] = TileAt(level, fx, y + (i - 4) * step, levels);
            fx = x + 4 * step;
            for (int i = 0; i < 9; i++) grid[static_cast<size_t>(i)][4] = TileAt(level, fx, y + (i - 4) * step, levels);
        }
    } else {
        int step = (facing == 1) ? 1 : -1;
        grid[0][0] = TileAt(level, x - step, y, levels);
        grid[1][0] = TileAt(level, x, y, levels);
        grid[2][0] = TileAt(level, x + step, y, levels);

        int fy = y - step;
        for (int i = 0; i < 5; i++) grid[static_cast<size_t>(i)][1] = TileAt(level, x + (i - 2) * step, fy, levels);
        fy = y - 2 * step;
        for (int i = 0; i < 7; i++) grid[static_cast<size_t>(i)][2] = TileAt(level, x + (i - 3) * step, fy, levels);
        fy = y - 3 * step;
        for (int i = 0; i < 9; i++) grid[static_cast<size_t>(i)][3] = TileAt(level, x + (i - 4) * step, fy, levels);
        fy = y - 4 * step;
        for (int i = 0; i < 9; i++) grid[static_cast<size_t>(i)][4] = TileAt(level, x + (i - 4) * step, fy, levels);
    }

    return grid;
}

}  // namespace stormhold
