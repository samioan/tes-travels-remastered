#include "assets/monster_image_names.h"

namespace dawnstar {

MonsterImageNames MonsterImageNames::Load(DatArchive& archive) {
    MonsterImageNames result;
    BinaryReader in = archive.OpenResource("monsterfilenamesin.dat");
    for (auto& bucket : result.names) {
        for (auto& name : bucket) name = in.ReadUTF();
    }
    return result;
}

}  // namespace dawnstar
