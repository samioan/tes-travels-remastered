#include "assets/spell_database.h"

#include "assets/binary_reader.h"

namespace stormhold {

SpellDatabase SpellDatabase::Load(const AssetRoot& assets) {
    SpellDatabase db;

    std::ifstream stream = assets.OpenFile("spellsin.dat");
    BinaryReader in(stream);

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

}  // namespace stormhold
