#include "assets/shop_dialogue.h"

#include "assets/binary_reader.h"

namespace stormhold {

namespace {
constexpr int kGroupCount = 8;
constexpr uint32_t kExpectedGroupSizes[kGroupCount] = {20, 20, 20, 20, 5, 22, 5, 41};
}  // namespace

ShopDialogue ShopDialogue::Load(const AssetRoot& assets) {
    ShopDialogue dialogue;
    std::ifstream stream = assets.OpenFile("npcstrings.dat");
    BinaryReader in(stream);

    dialogue.groups.resize(kGroupCount);
    for (int group = 0; group < kGroupCount; group++) {
        uint32_t count = in.ReadU32();
        if (count != kExpectedGroupSizes[group]) {
            throw std::runtime_error("ShopDialogue: group " + std::to_string(group) + " expected " +
                                      std::to_string(kExpectedGroupSizes[group]) + " lines, got " +
                                      std::to_string(count));
        }

        dialogue.groups[static_cast<size_t>(group)].resize(count);
        for (auto& line : dialogue.groups[static_cast<size_t>(group)]) {
            line = in.ReadUTF();
        }
    }

    return dialogue;
}

}  // namespace stormhold
