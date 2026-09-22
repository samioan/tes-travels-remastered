#include "dungeon/dungeon_runtime.h"

namespace dawnstar {

namespace {

// Dungeon.java's own static final String[] NAMES, transcribed verbatim.
const char* const kDungeonNames[37] = {
    "Dawnstar",         "North Creek",       "North Creek 2",     "North Creek 3",
    "Ice Spike",        "Ice Spike 2",       "Ice Spike 3",       "Blind Fjord",
    "Blind Fjord 2",    "Blind Fjord 3",     "Slipneck Fjord",    "Slipneck Fjord 2",
    "Slipneck Fjord 3", "Troll Pace",        "Troll Pace 2",      "Troll Pace 3",
    "Ice Tribe Haven",  "Ice Tribe Haven 2", "Ice Tribe Haven 3", "Dawnstar Run",
    "Dawnstar Run 2",   "Dawnstar Run 3",    "Massacre Caves",    "Massacre Caves 2",
    "Massacre Caves 3", "Frostheim",         "Frostheim 2",       "Frostheim 3",
    "Glacier Run",      "Glacier Run 2",     "Glacier Run 3",     "Troll Hole",
    "Troll Hole 2",     "Troll Hole 3",      "Ice Council",       "Ice Council 2",
    "Ice Council 3",
};

}  // namespace

const char* DungeonRuntime::DisplayName(int levelNumber) { return kDungeonNames[levelNumber - 1]; }

void DungeonRuntime::RegisterGeneratedSpawns(GeneratedLevel& level, WorldRegistry& world) {
    auto& monsterMap = world.monsters[static_cast<size_t>(level.number - 1)];
    for (const GeneratedMonsterSpawn& spawn : level.monsters) {
        MonsterState m;
        m.spawnId = static_cast<int16_t>(spawn.spawnId);
        m.monsterType = static_cast<int8_t>(spawn.monsterType);
        m.hp = static_cast<int8_t>(spawn.hp);
        m.x = static_cast<int8_t>(spawn.x);
        m.y = static_cast<int8_t>(spawn.y);
        m.dungeonLevel = static_cast<int8_t>(level.number);
        monsterMap[PackPosKey(spawn.x, spawn.y)] = MonsterRuntime::ToBytes(m);
    }

    auto& chestMap = world.chests[static_cast<size_t>(level.number - 1)];
    for (const GeneratedChestSpawn& chest : level.chests) {
        std::array<uint8_t, 8> record{};
        record[0] = static_cast<uint8_t>(chest.x);
        record[1] = static_cast<uint8_t>(chest.y);
        record[2] = chest.guaranteedGift ? 1 : 0;
        record[3] = static_cast<uint8_t>(level.tier);
        uint8_t low = static_cast<uint8_t>(chest.itemId & 0xFF);
        uint8_t high = 0;
        if (low == 86) high = static_cast<uint8_t>((chest.itemId >> 8) & 0xFF);
        record[4] = low;
        record[7] = high;
        uint16_t spawnId = static_cast<uint16_t>(chest.spawnId);
        record[5] = static_cast<uint8_t>((spawnId >> 8) & 0xFF);
        record[6] = static_cast<uint8_t>(spawnId & 0xFF);
        chestMap[PackPosKey(chest.x, chest.y)] = record;
    }
}

void DungeonRuntime::PopulateRandomMonsters(std::vector<GeneratedLevel>& levels, WorldRegistry& world, int levelIndex,
                                             int count, JavaRandom& rng, const MonsterDatabase& monsterDb,
                                             int16_t& spawnIdCounter) {
    const GeneratedLevel& level = levels[static_cast<size_t>(levelIndex)];
    for (int i = 0; i < count; i++) {
        while (!TrySpawnMonsterNear(levels, world, levelIndex, RandomIntBelow(rng, level.width),
                                     RandomIntBelow(rng, level.height), -1, rng, monsterDb, spawnIdCounter)) {
        }
    }
}

bool DungeonRuntime::TrySpawnMonsterNear(std::vector<GeneratedLevel>& levels, WorldRegistry& world, int levelIndex,
                                          int x, int y, int forcedTypeOrSentinel, JavaRandom& rng,
                                          const MonsterDatabase& monsterDb, int16_t& spawnIdCounter) {
    GeneratedLevel& level = levels[static_cast<size_t>(levelIndex)];

    int sentinel = -1;
    int typeArg = forcedTypeOrSentinel;
    if (typeArg == 42 || typeArg == 41) {
        sentinel = typeArg;
        typeArg = level.tier;
    }
    if (typeArg < 0) {
        typeArg = level.tier;
    }

    DungeonView view(levels, levelIndex);
    for (int i = 0; i <= 4; i++) {
        int candidateX = x;
        int candidateY = y;
        if (i < 2) {
            candidateX += 2 * i - 1;
        } else {
            candidateY += 2 * i - 5;
        }

        if (view.IsWalkable(candidateX, candidateY)) {
            int monsterType = MonsterRuntime::PickMonsterType(rng, typeArg, sentinel);
            spawnIdCounter++;
            MonsterState m = MonsterRuntime::Spawn(spawnIdCounter, monsterType, level.number, monsterDb);
            m.x = static_cast<int8_t>(candidateX);
            m.y = static_cast<int8_t>(candidateY);

            world.monsters[static_cast<size_t>(levelIndex)][PackPosKey(candidateX, candidateY)] =
                MonsterRuntime::ToBytes(m);
            level.tiles[static_cast<size_t>(candidateX)][static_cast<size_t>(candidateY)] = static_cast<uint8_t>(
                level.tiles[static_cast<size_t>(candidateX)][static_cast<size_t>(candidateY)] | 2);
            return true;
        }
    }
    return false;
}

void DungeonRuntime::RemoveMonster(GeneratedLevel& level, WorldRegistry& world, int x, int y) {
    auto& monsters = world.monsters[static_cast<size_t>(level.number - 1)];
    auto it = monsters.find(PackPosKey(x, y));
    if (it == monsters.end()) return;

    monsters.erase(it);
    level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] =
        static_cast<uint8_t>(level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] & ~static_cast<uint8_t>(2));
}

void DungeonRuntime::AddDroppedItem(GeneratedLevel& level, WorldRegistry& world,
                                     const std::array<uint8_t, 7>& record) {
    int x = record[0];
    int y = record[1];
    world.droppedItems[static_cast<size_t>(level.number - 1)].push_back(record);
    level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] =
        static_cast<uint8_t>(level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] | 4);
}

void DungeonRuntime::RemoveChest(GeneratedLevel& level, WorldRegistry& world,
                                  const std::array<uint8_t, 8>& record) {
    int x = record[0];
    int y = record[1];
    uint8_t flags = level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)];
    if ((flags & 1) != 0) return;
    if ((flags & 16) == 0) return;

    world.chests[static_cast<size_t>(level.number - 1)].erase(PackPosKey(x, y));
    level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] = static_cast<uint8_t>(flags & ~static_cast<uint8_t>(16));
}

void DungeonRuntime::RemoveDroppedItem(GeneratedLevel& level, WorldRegistry& world,
                                       const std::array<uint8_t, 7>& record) {
    int x = record[0];
    int y = record[1];
    uint8_t flags = level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)];
    if ((flags & 1) != 0) return;
    if ((flags & 4) == 0) return;

    // Content-equality removal of the first match -- see the header's
    // doc comment on why this differs from the original's
    // reference-identity Vector.removeElement().
    auto& items = world.droppedItems[static_cast<size_t>(level.number - 1)];
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (*it == record) {
            items.erase(it);
            break;
        }
    }
}

void DungeonRuntime::ClearDroppedItemFlag(GeneratedLevel& level, int x, int y) {
    level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] =
        static_cast<uint8_t>(level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] & ~static_cast<uint8_t>(4));
}

std::vector<std::array<uint8_t, 7>> DungeonRuntime::DroppedItemsAt(const WorldRegistry& world, int levelIndex, int x,
                                                                    int y) {
    std::vector<std::array<uint8_t, 7>> result;
    for (const auto& item : world.droppedItems[static_cast<size_t>(levelIndex)]) {
        if (item[0] == x && item[1] == y) result.push_back(item);
    }
    return result;
}

void DungeonRuntime::RefreshTileFlags(GeneratedLevel& level, const WorldRegistry& world, int levelIndex) {
    for (int x = 0; x < level.width; x++) {
        for (int y = 0; y < level.height; y++) {
            uint8_t flags = level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)];
            flags = static_cast<uint8_t>(flags & ~static_cast<uint8_t>(2));
            flags = static_cast<uint8_t>(flags & ~static_cast<uint8_t>(16));
            flags = static_cast<uint8_t>(flags & ~static_cast<uint8_t>(4));
            level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] = flags;
        }
    }

    // Dungeon.java parses x/y back out of the Hashtable KEY here (not
    // the stored byte[] record) -- matched exactly rather than reading
    // record[4]/[5] (Monster.toBytes()'s own x/y fields), even though
    // both always agree given how TrySpawnMonsterNear above stores
    // them.
    for (const auto& [key, record] : world.monsters[static_cast<size_t>(levelIndex)]) {
        (void)record;
        int x, y;
        UnpackPosKey(key, &x, &y);
        level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] =
            static_cast<uint8_t>(level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] | 2);
    }

    for (const auto& [key, chest] : world.chests[static_cast<size_t>(levelIndex)]) {
        int x = chest[0];
        int y = chest[1];
        level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] =
            static_cast<uint8_t>(level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] | 16);
    }

    for (const auto& item : world.droppedItems[static_cast<size_t>(levelIndex)]) {
        int x = item[0];
        int y = item[1];
        level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] =
            static_cast<uint8_t>(level.tiles[static_cast<size_t>(x)][static_cast<size_t>(y)] | 4);
    }
}

void DungeonRuntime::SampleSquareView(const std::vector<GeneratedLevel>& levels, int levelIndex, int x, int y,
                                      int direction, int size, const WorldRegistry& world,
                                      std::array<std::array<uint8_t, 17>, 17>& out) {
    DungeonView view(levels, levelIndex);
    int half = size / 2;
    const auto& monsterMap = world.monsters[static_cast<size_t>(levelIndex)];

    for (int row = 0; row < size; row++) {
        for (int col = 0; col < size; col++) {
            int sx, sy;
            if (direction == 1 || direction == 3) {
                int sign = direction == 1 ? 1 : -1;
                sx = x + (col - half) * sign;
                sy = y + (row - half) * sign;
            } else {
                int sign = direction == 2 ? 1 : -1;
                sx = x - (row - half) * sign;
                sy = y + (col - half) * sign;
            }

            uint8_t tile = view.TileAt(sx, sy);
            uint8_t value = static_cast<uint8_t>(tile & 1);
            if ((value & 1) == 0) {
                if ((tile & 4) == 0 && (tile & 16) == 0 && (tile & 32) == 0) {
                    value = static_cast<uint8_t>(tile & 8);
                } else {
                    value = static_cast<uint8_t>(value | 4);
                }

                if ((tile & 2) != 0) {
                    // See this method's own header doc comment: the
                    // lookup always uses `levelIndex`'s own registry and
                    // the raw (sx, sy), even when TileAt actually
                    // resolved `tile` from a neighboring level.
                    auto it = monsterMap.find(PackPosKey(sx, sy));
                    if (it != monsterMap.end()) {
                        MonsterState m = MonsterRuntime::FromBytes(it->second);
                        if (m.flag) value = static_cast<uint8_t>(value | 2);
                    }
                }
            }

            out[static_cast<size_t>(col)][static_cast<size_t>(row)] = value;
        }
    }
}

}  // namespace dawnstar
