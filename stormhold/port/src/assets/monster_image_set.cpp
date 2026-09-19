#include "assets/monster_image_set.h"

#include "assets/binary_reader.h"

namespace stormhold {

namespace {
constexpr int kChunkStart[5] = {0, 7, 14, 21, 28};
constexpr int kChunkCount[5] = {7, 7, 7, 7, 5};
constexpr int kImagelessTypes[5] = {4, 11, 18, 23, 30};

bool IsImagelessMonsterType(int typeIndex) {
    for (int t : kImagelessTypes) {
        if (t == typeIndex) return true;
    }
    return false;
}
}  // namespace

MonsterImageSet MonsterImageSet::Load(const AssetRoot& assets) {
    MonsterImageSet result;

    std::ifstream stream = assets.OpenFile("monsterfilenamesin.dat");
    BinaryReader in(stream);
    std::array<std::array<std::string, 7>, 5> names;
    for (auto& chunk : names) {
        for (auto& name : chunk) name = in.ReadUTF();
    }

    for (int chunk = 0; chunk < 5; chunk++) {
        for (int i = 0; i < kChunkCount[chunk]; i++) {
            int typeIndex = kChunkStart[chunk] + i;
            if (!IsImagelessMonsterType(typeIndex)) {
                result.images[static_cast<size_t>(typeIndex)] = RawImage::Load(assets, names[static_cast<size_t>(chunk)][static_cast<size_t>(i)]);
            }
        }
    }

    return result;
}

}  // namespace stormhold
