#include "monster/monster_runtime.h"

namespace stormhold {

int MonsterRuntime::PickMonsterType(JavaRandom& rng, int levelTier, int forcedTypeIndex) {
    if (forcedTypeIndex >= 0) return forcedTypeIndex;

    int zone = levelTier - 1;
    if (zone < 0) zone = 0;
    if (zone > 36) zone = 36;

    int bucketRoll = RandomInt1Based(rng, 10);
    int bucket;
    if (bucketRoll <= 4) {
        bucket = 0;
    } else if (bucketRoll <= 7) {
        bucket = 1;
    } else if (bucketRoll <= 9) {
        bucket = 2;
    } else {
        bucket = 3;
    }

    return DungeonGenerator::MonsterTypeForTierBucket(zone, bucket);
}

MonsterState MonsterRuntime::Spawn(int16_t spawnId, int typeIndex, int dungeonLevel, const MonsterDatabase& db) {
    MonsterState m;
    m.spawnId = spawnId;
    m.typeIndex = static_cast<int8_t>(typeIndex);
    m.currentHp = db.RawStat(typeIndex, 14);
    m.unconfirmedFlag = false;
    m.dungeonLevel = static_cast<int8_t>(dungeonLevel);
    m.aiPhase = 0;
    return m;
}

void MonsterRuntime::TakeDamage(MonsterState& m, int amount) {
    int hp = m.currentHp & 0xFF;
    if (amount > hp) amount = hp;
    hp -= amount;
    m.currentHp = static_cast<int8_t>(hp);
}

std::array<uint8_t, 28> MonsterRuntime::ToBytes(const MonsterState& m) {
    std::array<uint8_t, 28> out{};
    out[0] = static_cast<uint8_t>((static_cast<uint16_t>(m.spawnId) >> 8) & 0xFF);
    out[1] = static_cast<uint8_t>(static_cast<uint16_t>(m.spawnId) & 0xFF);
    out[2] = static_cast<uint8_t>(m.typeIndex);
    out[3] = static_cast<uint8_t>(m.currentHp);
    out[4] = static_cast<uint8_t>(m.tileX);
    out[5] = static_cast<uint8_t>(m.tileY);
    out[6] = m.unconfirmedFlag ? 1 : 0;
    out[7] = static_cast<uint8_t>(m.dungeonLevel);
    out[8] = static_cast<uint8_t>(m.chaseCadence);
    out[9] = static_cast<uint8_t>(m.aiPhase);

    uint64_t ts = static_cast<uint64_t>(m.unconfirmedTimestamp);
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
    m.typeIndex = static_cast<int8_t>(in[2]);
    m.currentHp = static_cast<int8_t>(in[3]);
    m.tileX = static_cast<int8_t>(in[4]);
    m.tileY = static_cast<int8_t>(in[5]);
    m.unconfirmedFlag = in[6] != 0;
    m.dungeonLevel = static_cast<int8_t>(in[7]);
    m.chaseCadence = static_cast<int8_t>(in[8]);
    m.aiPhase = static_cast<int8_t>(in[9]);

    uint64_t ts = 0;
    for (int i = 0; i < 8; i++) ts = (ts << 8) | in[10 + i];
    m.unconfirmedTimestamp = static_cast<int64_t>(ts);

    for (int i = 0; i < 10; i++) m.scratch[static_cast<size_t>(i)] = static_cast<int8_t>(in[18 + i]);

    return m;
}

MonsterState MonsterRuntime::ReadFrom(BinaryReader& in) {
    MonsterState m;
    m.spawnId = in.ReadS16();
    m.typeIndex = in.ReadS8();
    m.currentHp = in.ReadS8();
    m.tileX = in.ReadS8();
    m.tileY = in.ReadS8();
    m.unconfirmedFlag = in.ReadU8() != 0;
    m.dungeonLevel = in.ReadS8();
    m.chaseCadence = in.ReadS8();
    m.aiPhase = in.ReadS8();

    uint64_t hi = in.ReadU32();
    uint64_t lo = in.ReadU32();
    m.unconfirmedTimestamp = static_cast<int64_t>((hi << 32) | lo);

    for (int i = 0; i < 10; i++) m.scratch[static_cast<size_t>(i)] = in.ReadS8();

    return m;
}

void MonsterRuntime::WriteTo(BinaryWriter& out, const MonsterState& m) {
    out.WriteS16(m.spawnId);
    out.WriteS8(m.typeIndex);
    out.WriteS8(m.currentHp);
    out.WriteS8(m.tileX);
    out.WriteS8(m.tileY);
    out.WriteBool(m.unconfirmedFlag);
    out.WriteS8(m.dungeonLevel);
    out.WriteS8(m.chaseCadence);
    out.WriteS8(m.aiPhase);

    uint64_t ts = static_cast<uint64_t>(m.unconfirmedTimestamp);
    out.WriteU32(static_cast<uint32_t>((ts >> 32) & 0xFFFFFFFFu));
    out.WriteU32(static_cast<uint32_t>(ts & 0xFFFFFFFFu));

    for (int i = 0; i < 10; i++) out.WriteS8(m.scratch[static_cast<size_t>(i)]);
}

bool MonsterRuntime::Move(MonsterState& m, int dir, std::vector<GeneratedLevel>& levels) {
    int step = 1;
    int newX = m.tileX;
    int newY = m.tileY;

    switch (dir) {
        case 1:
            step = -1;
            [[fallthrough]];
        case 3:
            newX = m.tileX;
            newY = m.tileY + step;
            break;
        case 4:
            step = -1;
            [[fallthrough]];
        case 2:
            newY = m.tileY;
            newX = m.tileX + step;
            break;
        default:
            return false;
    }

    if (newX < 0 || newY < 0) return false;

    GeneratedLevel& level = levels[static_cast<size_t>(m.dungeonLevel - 1)];
    if (newX >= level.width || newY >= level.height) return false;
    if (IsStairwayTile(level, newX, newY)) return false;

    // Dungeon.isWalkable(x,y): bits 1 (wall), 2 (monster), 8 (no-spawn/
    // special), 32 (blocked) -- see this method's doc comment on why this
    // is a different bit set than PlayerMovement::IsWalkableTileBits.
    uint8_t destTile = level.tiles[static_cast<size_t>(newX)][static_cast<size_t>(newY)];
    if (destTile & 0x01) return false;
    if (destTile & 0x02) return false;
    if (destTile & 0x08) return false;
    if (destTile & 0x20) return false;

    level.tiles[static_cast<size_t>(m.tileX)][static_cast<size_t>(m.tileY)] = static_cast<uint8_t>(
        level.tiles[static_cast<size_t>(m.tileX)][static_cast<size_t>(m.tileY)] & ~static_cast<uint8_t>(0x02));
    level.tiles[static_cast<size_t>(newX)][static_cast<size_t>(newY)] = static_cast<uint8_t>(destTile | 0x02);
    m.tileX = static_cast<int8_t>(newX);
    m.tileY = static_cast<int8_t>(newY);
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

bool MonsterRuntime::IsAdjacent(MonsterState& m, int targetX, int targetY) {
    if (DistanceTo(m, targetX, targetY) == 1) return true;
    m.aiPhase = 0;
    return false;
}

void MonsterRuntime::Chase(MonsterState& m, int targetX, int targetY, std::vector<GeneratedLevel>& levels,
                            JavaRandom& globalRng) {
    if (!IsWithinRange(m, targetX, targetY)) return;

    if (m.chaseCadence != 0) {
        if (m.chaseCadence >= 4) {
            m.chaseCadence = 0;
        } else {
            m.chaseCadence++;
        }
        return;
    }

    int dx = JavaAbs(targetX - m.tileX);
    int dy = JavaAbs(targetY - m.tileY);

    int xDir;
    if (m.tileX < targetX) {
        xDir = 2;
    } else if (m.tileX > targetX) {
        xDir = 4;
    } else {
        xDir = -1;
    }

    int yDir;
    if (m.tileY < targetY) {
        yDir = 3;
    } else if (m.tileY > targetY) {
        yDir = 1;
    } else {
        yDir = -1;
    }

    int first, second;
    if (dx > dy) {
        first = xDir;
        second = yDir;
    } else if (dx < dy) {
        first = yDir;
        second = xDir;
    } else {
        int coin = RandomInt0Based(globalRng, 2);
        if (coin == 0) {
            first = xDir;
            second = yDir;
        } else {
            first = yDir;
            second = xDir;
        }
    }

    if (!Move(m, first, levels)) {
        Move(m, second, levels);
    }
    m.chaseCadence++;
}

MonsterRuntime::DeathDrop MonsterRuntime::OnDeath(const MonsterState& m, const MonsterDatabase& monsterDb,
                                                   const ItemDatabase& items, int levelTier, bool guaranteedDrop,
                                                   int16_t dropSpawnId, JavaRandom& globalRng) {
    DeathDrop result;

    int dropChance = monsterDb.RawStat(m.typeIndex, 15);
    if (guaranteedDrop) dropChance = 100;
    int lootBonus = monsterDb.RawStat(m.typeIndex, 16);

    int roll = RandomInt1Based(globalRng, 100);
    bool dropped = roll <= dropChance;
    if (!dropped && !guaranteedDrop) return result;

    int itemRoll = items.RollLoot(globalRng, levelTier, lootBonus);
    uint8_t low = static_cast<uint8_t>(itemRoll & 0xFF);
    uint8_t high = 0;
    if (low == 86) high = static_cast<uint8_t>((itemRoll >> 8) & 0xFF);

    result.record[0] = static_cast<uint8_t>(m.tileX);
    result.record[1] = static_cast<uint8_t>(m.tileY);
    result.record[2] = low;

    uint16_t spawnIdU = static_cast<uint16_t>(dropSpawnId);
    result.record[3] = static_cast<uint8_t>((spawnIdU >> 8) & 0xFF);
    result.record[4] = static_cast<uint8_t>(spawnIdU & 0xFF);
    result.record[5] = high;
    result.record[6] = 1;
    if (guaranteedDrop) result.record[6] = static_cast<uint8_t>(result.record[6] | 4);

    result.dropped = true;
    return result;
}

}  // namespace stormhold
