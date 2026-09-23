#include "assets/dungeon_names.h"

#include "assets/binary_reader.h"

namespace stormhold {

DungeonNames DungeonNames::Load(const AssetRoot& assets) {
    DungeonNames result;
    std::ifstream stream = assets.OpenFile("dungnamesin.dat");
    BinaryReader in(stream);

    for (auto& row : result.names) {
        for (auto& name : row) {
            name = in.ReadUTF();
        }
    }

    return result;
}

}  // namespace stormhold
