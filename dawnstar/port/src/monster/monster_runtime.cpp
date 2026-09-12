#include "monster/monster_runtime.h"

#include "world/dungeon_view.h"

namespace dawnstar {

namespace {

// Monster.java's private faceToward(): picks the larger-distance axis
// first (ties broken randomly), then move()s it, falling back to the
// other axis if blocked.
void FaceToward(MonsterState& m, int targetX, int targetY, std::vector<GeneratedLevel>& levels,
                 JavaRandom& globalRng) {
    int dx = JavaAbs(targetX - m.x);
    int dy = JavaAbs(targetY - m.y);

    int horiz;
    if (m.x < targetX) {
        horiz = 2;
    } else if (m.x > targetX) {
        horiz = 4;
    } else {
        horiz = -1;
    }

    int vert;
    if (m.y < targetY) {
        vert = 3;
    } else if (m.y > targetY) {
        vert = 1;
    } else {
        vert = -1;
    }

    int primary, secondary;
    if (dx > dy) {
        primary = horiz;
        secondary = vert;
    } else if (dx < dy) {
        primary = vert;
        secondary = horiz;
    } else {
        int coinFlip = RandomIntBelow(globalRng, 2);
        if (coinFlip == 0) {
            primary = horiz;
            secondary = vert;
        } else {
            primary = vert;
            secondary = horiz;
        }
    }

    if (!MonsterRuntime::Move(m, primary, levels)) {
        MonsterRuntime::Move(m, secondary, levels);
    }
}

}  // namespace

int MonsterRuntime::PickMonsterType(JavaRandom& rng, int difficultyTier, int forcedType) {
    if (forcedType >= 0) return forcedType;

    int tierIndex = difficultyTier - 1;
    if (tierIndex < 0) tierIndex = 0;
    if (tierIndex > 36) tierIndex = 36;

    int tierRoll = LingoRandomInt(rng, 10);
    int bucket;
    if (tierRoll <= 4) {
        bucket = 0;
    } else if (tierRoll <= 7) {
        bucket = 1;
    } else if (tierRoll <= 9) {
        bucket = 2;
    } else {
        bucket = 3;
    }

    return DungeonGenerator::MonsterTypeForTierBucket(tierIndex, bucket);
}

MonsterState MonsterRuntime::Spawn(int16_t spawnId, int monsterType, int dungeonLevel, const MonsterDatabase& db) {
    MonsterState m;
    m.spawnId = spawnId;
    m.monsterType = static_cast<int8_t>(monsterType);
    m.hp = static_cast<int8_t>(db.Stat(monsterType, 14));
    m.flag = false;
    m.dungeonLevel = static_cast<int8_t>(dungeonLevel);
    m.aiPhase = 0;
    return m;
}

void MonsterRuntime::TakeDamage(MonsterState& m, int amount) {
    int hp = m.hp & 0xFF;
    if (amount > hp) amount = hp;
    hp -= amount;
    m.hp = static_cast<int8_t>(hp);
}

std::array<uint8_t, 28> MonsterRuntime::ToBytes(const MonsterState& m) {
    std::array<uint8_t, 28> out{};
    out[0] = static_cast<uint8_t>((static_cast<uint16_t>(m.spawnId) >> 8) & 0xFF);
    out[1] = static_cast<uint8_t>(static_cast<uint16_t>(m.spawnId) & 0xFF);
    out[2] = static_cast<uint8_t>(m.monsterType);
    out[3] = static_cast<uint8_t>(m.hp);
    out[4] = static_cast<uint8_t>(m.x);
    out[5] = static_cast<uint8_t>(m.y);
    out[6] = m.flag ? 1 : 0;
    out[7] = static_cast<uint8_t>(m.dungeonLevel);
    out[8] = static_cast<uint8_t>(m.moveCooldown);
    out[9] = static_cast<uint8_t>(m.aiPhase);

    uint64_t ts = static_cast<uint64_t>(m.timestamp);
    for (int i = 0; i < 8; i++) {
        out[10 + i] = static_cast<uint8_t>((ts >> (56 - 8 * i)) & 0xFF);
    }
    for (int i = 0; i < 10; i++) {
        out[18 + i] = static_cast<uint8_t>(m.scratch[static_cast<size_t>(i)]);
    }

    return out;
}

MonsterState MonsterRuntime::FromBytes(const std::array<uint8_t, 28>& in) {
    MonsterState m;
    uint16_t hi = in[0];
    uint16_t lo = in[1];
    m.spawnId = static_cast<int16_t>(static_cast<uint16_t>((hi << 8) | lo));
    m.monsterType = static_cast<int8_t>(in[2]);
    m.hp = static_cast<int8_t>(in[3]);
    m.x = static_cast<int8_t>(in[4]);
    m.y = static_cast<int8_t>(in[5]);
    m.flag = in[6] != 0;
    m.dungeonLevel = static_cast<int8_t>(in[7]);
    m.moveCooldown = static_cast<int8_t>(in[8]);
    m.aiPhase = static_cast<int8_t>(in[9]);

    uint64_t ts = 0;
    for (int i = 0; i < 8; i++) ts = (ts << 8) | in[10 + i];
    m.timestamp = static_cast<int64_t>(ts);

    for (int i = 0; i < 10; i++) m.scratch[static_cast<size_t>(i)] = static_cast<int8_t>(in[18 + i]);

    return m;
}

MonsterState MonsterRuntime::ReadFrom(BinaryReader& in) {
    MonsterState m;
    m.spawnId = in.ReadS16();
    m.monsterType = in.ReadS8();
    m.hp = in.ReadS8();
    m.x = in.ReadS8();
    m.y = in.ReadS8();
    m.flag = in.ReadU8() != 0;
    m.dungeonLevel = in.ReadS8();
    m.moveCooldown = in.ReadS8();
    m.aiPhase = in.ReadS8();

    uint64_t hi = in.ReadU32();
    uint64_t lo = in.ReadU32();
    m.timestamp = static_cast<int64_t>((hi << 32) | lo);

    for (int i = 0; i < 10; i++) m.scratch[static_cast<size_t>(i)] = in.ReadS8();

    return m;
}

void MonsterRuntime::WriteTo(BinaryWriter& out, const MonsterState& m) {
    out.WriteS16(m.spawnId);
    out.WriteS8(m.monsterType);
    out.WriteS8(m.hp);
    out.WriteS8(m.x);
    out.WriteS8(m.y);
    out.WriteBool(m.flag);
    out.WriteS8(m.dungeonLevel);
    out.WriteS8(m.moveCooldown);
    out.WriteS8(m.aiPhase);

    uint64_t ts = static_cast<uint64_t>(m.timestamp);
    out.WriteU32(static_cast<uint32_t>((ts >> 32) & 0xFFFFFFFFu));
    out.WriteU32(static_cast<uint32_t>(ts & 0xFFFFFFFFu));

    for (int i = 0; i < 10; i++) out.WriteS8(m.scratch[static_cast<size_t>(i)]);
}

bool MonsterRuntime::Move(MonsterState& m, int direction, std::vector<GeneratedLevel>& levels) {
    int delta = 1;
    int newX = m.x;
    int newY = m.y;

    switch (direction) {
        case 1:
            delta = -1;
            [[fallthrough]];
        case 3:
            newX = m.x;
            newY = m.y + delta;
            break;
        case 4:
            delta = -1;
            [[fallthrough]];
        case 2:
            newY = m.y;
            newX = m.x + delta;
            break;
        default:
            return false;
    }

    GeneratedLevel& level = levels[static_cast<size_t>(m.dungeonLevel - 1)];
    DungeonView view(level);
    if (!view.IsWalkable(newX, newY)) return false;
    if (IsStairwayTile(level, newX, newY)) return false;

    // ESGame.monsters[]-hashtable removal + store(): SKIPPED, no live
    // registry exists yet (see class comment in monster_runtime.h).
    level.tiles[static_cast<size_t>(m.x)][static_cast<size_t>(m.y)] = static_cast<uint8_t>(
        level.tiles[static_cast<size_t>(m.x)][static_cast<size_t>(m.y)] & ~static_cast<uint8_t>(2));
    level.tiles[static_cast<size_t>(newX)][static_cast<size_t>(newY)] = static_cast<uint8_t>(
        level.tiles[static_cast<size_t>(newX)][static_cast<size_t>(newY)] | static_cast<uint8_t>(2));
    m.x = static_cast<int8_t>(newX);
    m.y = static_cast<int8_t>(newY);

    return true;
}

bool MonsterRuntime::IsStairwayTile(const GeneratedLevel& level, int x, int y) {
    if (level.stairsUpDir != 1 && level.stairsDownDir != 1) {
        if (level.stairsUpDir != 3 && level.stairsDownDir != 3) {
            if (level.stairsUpDir != 4 && level.stairsDownDir != 4) {
                if ((level.stairsUpDir == 2 || level.stairsDownDir == 2) && x == 30 && y == 17) {
                    return true;
                }
            } else if (x == 5 && y == 17) {
                return true;
            }
        } else if (x == 17 && y == 30) {
            return true;
        }
    } else if (x == 17 && y == 5) {
        return true;
    }

    return false;
}

bool MonsterRuntime::Chase(MonsterState& m, int targetX, int targetY, std::vector<GeneratedLevel>& levels,
                           JavaRandom& globalRng) {
    bool moved = false;
    if (m.moveCooldown == 0) {
        FaceToward(m, targetX, targetY, levels, globalRng);
        m.moveCooldown++;
        moved = true;
    } else if (m.moveCooldown >= 4) {
        m.moveCooldown = 0;
    } else {
        m.moveCooldown++;
    }

    m.aiPhase = 0;
    // store(): SKIPPED, see class comment in monster_runtime.h.
    return moved;
}

MonsterRuntime::DeathDrop MonsterRuntime::OnDeath(const MonsterState& m, const MonsterDatabase& monsterDb,
                                                   const ItemDatabase& items, int levelTier, bool guaranteed,
                                                   int16_t spawnId, JavaRandom& globalRng) {
    DeathDrop result;

    int dropChance = RawStat(m, monsterDb, 15);
    if (guaranteed) dropChance = 100;
    int lootRow = RawStat(m, monsterDb, 16);

    int roll = LingoRandomInt(globalRng, 100);
    bool drops = roll <= dropChance;
    if (!drops && !guaranteed) return result;

    int itemId = items.RollLoot(globalRng, levelTier, lootRow);
    uint8_t low = static_cast<uint8_t>(itemId & 0xFF);
    uint8_t high = 0;
    if (low == 86) high = static_cast<uint8_t>((itemId >> 8) & 0xFF);

    result.record[0] = static_cast<uint8_t>(m.x);
    result.record[1] = static_cast<uint8_t>(m.y);
    result.record[2] = low;
    result.record[5] = high;

    uint16_t spawnIdU = static_cast<uint16_t>(spawnId);
    result.record[3] = static_cast<uint8_t>((spawnIdU >> 8) & 0xFF);
    result.record[4] = static_cast<uint8_t>(spawnIdU & 0xFF);
    result.record[6] = 1;
    if (guaranteed) result.record[6] = static_cast<uint8_t>(result.record[6] | 4);

    result.dropped = true;
    return result;
}

}  // namespace dawnstar
