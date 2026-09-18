#include "assets/monster_database.h"

#include "assets/binary_reader.h"

namespace stormhold {

MonsterDatabase MonsterDatabase::Load(const AssetRoot& assets) {
    MonsterDatabase db;

    // monstersin.dat's count is a u32 (Monster.loadTypes() reads it via
    // in.readInt()), unlike itemsin.dat/spellsin.dat's u16 counts.
    std::ifstream stream = assets.OpenFile("monstersin.dat");
    BinaryReader in(stream);

    uint32_t typeCount = in.ReadU32();
    db.typeName.resize(typeCount);
    db.typeStats.resize(typeCount);

    for (auto& name : db.typeName) name = in.ReadUTF();

    for (auto& row : db.typeStats) {
        for (auto& stat : row) stat = in.ReadU8();
    }

    return db;
}

}  // namespace stormhold
