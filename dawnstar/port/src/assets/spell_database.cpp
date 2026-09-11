#include "assets/spell_database.h"

namespace dawnstar {

SpellDatabase SpellDatabase::Load(DatArchive& archive) {
    SpellDatabase db;

    BinaryReader in = archive.OpenResource("spellsin.dat");
    uint16_t count = in.ReadU16();
    db.all.resize(count);

    for (auto& s : db.all) s.name = in.ReadUTF();
    for (auto& s : db.all) s.skillRequired = in.ReadS8();
    for (auto& s : db.all) s.magickaCost = in.ReadS8();
    for (auto& s : db.all) s.power = in.ReadS8();
    for (auto& s : db.all) s.school = in.ReadS8();
    for (auto& s : db.all) s.durationMultiplier = in.ReadS8();
    for (auto& s : db.all) s.icon = in.ReadS8();
    for (auto& s : db.all) s.description = in.ReadUTF();

    return db;
}

}  // namespace dawnstar
