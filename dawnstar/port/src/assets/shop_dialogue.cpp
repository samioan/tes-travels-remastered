#include "assets/shop_dialogue.h"

#include <fstream>
#include <stdexcept>

namespace dawnstar {

namespace {
const int kGroupSizes[10] = {3, 3, 3, 3, 14, 16, 16, 16, 16, 77};
}  // namespace

ShopDialogue ShopDialogue::Load(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("ShopDialogue: cannot open " + path);
    BinaryReader in(file);

    ShopDialogue dlg;
    dlg.groups.resize(10);

    for (int g = 0; g < 10; g++) {
        uint32_t count = in.ReadU32();
        if (static_cast<int>(count) != kGroupSizes[g]) {
            throw std::runtime_error("ShopDialogue: group " + std::to_string(g) + " expected " +
                                      std::to_string(kGroupSizes[g]) + " strings, got " +
                                      std::to_string(count));
        }
        dlg.groups[g].resize(count);
        for (auto& s : dlg.groups[g]) s = in.ReadUTF();
    }

    return dlg;
}

}  // namespace dawnstar
