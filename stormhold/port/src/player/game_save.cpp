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

bool GameSave::Save(const std::string& path, const PlayerState& player, const WorldRegistry& world,
                     const ShopState& shop, const WardenState& warden) {
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
    // M55: the world blob now gets its own length prefix too -- it's no
    // longer the last thing in the file, see this class's own header
    // comment on the file-layout change.
    out.WriteU32(static_cast<uint32_t>(worldBytes.size()));
    file.write(reinterpret_cast<const char*>(worldBytes.data()), static_cast<std::streamsize>(worldBytes.size()));
    Shop::WriteTo(out, shop);
    WardenState::WriteTo(out, warden);

    return file.good();
}

bool GameSave::Load(const std::string& path, size_t levelCount, PlayerState& outPlayer, WorldRegistry& outWorld,
                     ShopState& outShop, WardenState& outWarden) {
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

        uint32_t worldLen = in.ReadU32();
        std::vector<uint8_t> worldBytes(worldLen);
        if (worldLen > 0) {
            file.read(reinterpret_cast<char*>(worldBytes.data()), static_cast<std::streamsize>(worldLen));
        }
        if (!file) return false;

        outPlayer = PlayerSave::FromBytes(playerBytes);
        outWorld = WorldSave::FromBytes(worldBytes, levelCount);
        outShop = Shop::ReadFrom(in);
        outWarden = WardenState::ReadFrom(in);
    } catch (const std::exception&) {
        return false;
    }

    return true;
}

}  // namespace stormhold
