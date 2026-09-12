#include "dungeon/dungeon_runtime.h"

namespace dawnstar {

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

}  // namespace dawnstar
