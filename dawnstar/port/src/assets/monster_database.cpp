#include "assets/monster_database.h"

namespace dawnstar {

MonsterDatabase MonsterDatabase::Load(DatArchive& archive) {
    MonsterDatabase db;

    // monstersin.dat's count is a u32 (Monster.load() reads it via
    // in.readInt()), unlike itemsin.dat/spellsin.dat's u16 counts.
    BinaryReader in = archive.OpenResource("monstersin.dat");
    uint32_t typeCount = in.ReadU32();
    db.typeName.resize(typeCount);
    db.typeStats.resize(typeCount);

    for (auto& name : db.typeName) name = in.ReadUTF();

    for (auto& row : db.typeStats) {
        for (auto& stat : row) stat = in.ReadU8();
    }

    return db;
}

}  // namespace dawnstar
