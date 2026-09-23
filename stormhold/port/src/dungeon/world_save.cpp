#include "dungeon/world_save.h"

#include <sstream>

#include "assets/binary_reader.h"
#include "assets/binary_writer.h"

namespace stormhold {

std::vector<uint8_t> WorldSave::ToBytes(const WorldRegistry& world) {
    std::ostringstream buf;
    BinaryWriter out(buf);

    for (size_t level = 1; level < world.monsters.size(); level++) {
        out.WriteS32(static_cast<int32_t>(world.monsters[level].size()));
        for (const auto& [spawnId, record] : world.monsters[level]) {
            MonsterState m = MonsterRuntime::FromBytes(record);
            MonsterRuntime::WriteTo(out, m);
        }
    }

    for (size_t level = 1; level < world.chests.size(); level++) {
        out.WriteS32(static_cast<int32_t>(world.chests[level].size()));
        for (const auto& [key, record] : world.chests[level]) {
            for (int8_t b : record) out.WriteS8(b);
        }
    }

    for (size_t level = 0; level < world.droppedItems.size(); level++) {
        out.WriteS32(static_cast<int32_t>(world.droppedItems[level].size()));
        for (const auto& record : world.droppedItems[level]) {
            for (int8_t b : record) out.WriteS8(b);
        }
    }

    std::string str = buf.str();
    return std::vector<uint8_t>(str.begin(), str.end());
}

WorldRegistry WorldSave::FromBytes(const std::vector<uint8_t>& data, size_t levelCount) {
    std::istringstream buf(std::string(data.begin(), data.end()));
    BinaryReader in(buf);
    WorldRegistry world(levelCount);

    for (size_t level = 1; level < world.monsters.size(); level++) {
        int32_t count = in.ReadS32();
        for (int32_t i = 0; i < count; i++) {
            MonsterState m = MonsterRuntime::ReadFrom(in);
            world.monsters[level][m.spawnId] = MonsterRuntime::ToBytes(m);
        }
    }

    for (size_t level = 1; level < world.chests.size(); level++) {
        int32_t count = in.ReadS32();
        for (int32_t i = 0; i < count; i++) {
            std::array<int8_t, 8> record{};
            for (int8_t& b : record) b = in.ReadS8();
            int32_t key = PackTileKey(record[0], record[1]);
            world.chests[level][key] = record;
        }
    }

    for (size_t level = 0; level < world.droppedItems.size(); level++) {
        int32_t count = in.ReadS32();
        for (int32_t i = 0; i < count; i++) {
            std::array<int8_t, 7> record{};
            for (int8_t& b : record) b = in.ReadS8();
            world.droppedItems[level].push_back(record);
        }
    }

    return world;
}

}  // namespace stormhold
