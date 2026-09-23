#include "player/game_save.h"

#include <filesystem>
#include <fstream>

#include "assets/binary_reader.h"
#include "assets/binary_writer.h"
#include "dungeon/world_save.h"
#include "player/player_save.h"

namespace stormhold {

bool GameSave::Exists(const std::string& path) {
    if (path.empty()) return false;
    std::error_code error;
    return std::filesystem::is_regular_file(path, error) && !error;
}

bool GameSave::Save(const std::string& path, const PlayerState& player, const WorldRegistry& world) {
    if (path.empty()) return false;

    std::vector<uint8_t> playerBytes = PlayerSave::ToBytes(player);
    std::vector<uint8_t> worldBytes = WorldSave::ToBytes(world);

    std::error_code error;
    std::filesystem::path fsPath(path);
    if (fsPath.has_parent_path()) {
        std::filesystem::create_directories(fsPath.parent_path(), error);
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) return false;

    BinaryWriter out(file);
    out.WriteU32(static_cast<uint32_t>(playerBytes.size()));
    file.write(reinterpret_cast<const char*>(playerBytes.data()), static_cast<std::streamsize>(playerBytes.size()));
    file.write(reinterpret_cast<const char*>(worldBytes.data()), static_cast<std::streamsize>(worldBytes.size()));

    return file.good();
}

bool GameSave::Load(const std::string& path, size_t levelCount, PlayerState& outPlayer, WorldRegistry& outWorld) {
    if (path.empty()) return false;

    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    try {
        BinaryReader in(file);
        uint32_t playerLen = in.ReadU32();

        std::vector<uint8_t> playerBytes(playerLen);
        if (playerLen > 0) {
            file.read(reinterpret_cast<char*>(playerBytes.data()), static_cast<std::streamsize>(playerLen));
        }
        if (!file) return false;

        std::vector<uint8_t> worldBytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        outPlayer = PlayerSave::FromBytes(playerBytes);
        outWorld = WorldSave::FromBytes(worldBytes, levelCount);
    } catch (const std::exception&) {
        return false;
    }

    return true;
}

}  // namespace stormhold
