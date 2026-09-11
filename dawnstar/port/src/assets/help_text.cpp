#include "assets/help_text.h"

#include <stdexcept>

namespace dawnstar {

namespace {

// ESGame.loadHelpStrings()'s hardcoded title-fragment indices, in
// help-topic order.
const int kTitleIndex[12] = {0, 2, 5, 7, 13, 15, 18, 23, 25, 28, 31, 33};
// Hardcoded body-fragment index ranges (inclusive), same order.
const int kBodyStart[12] = {1, 3, 6, 8, 14, 16, 19, 24, 26, 29, 32, 34};
const int kBodyEnd[12] = {1, 4, 6, 12, 14, 17, 22, 24, 27, 30, 32, 34};

}  // namespace

HelpText HelpText::Load(DatArchive& archive) {
    BinaryReader in = archive.OpenResource("helptext.dat");
    uint32_t count = in.ReadU32();
    if (count != 35) {
        throw std::runtime_error("HelpText: expected 35 fragments, got " + std::to_string(count));
    }

    std::vector<std::string> fragments(count);
    for (auto& s : fragments) s = in.ReadUTF();

    HelpText help;
    help.titles.resize(12);
    help.bodies.resize(12);

    for (int i = 0; i < 12; i++) {
        help.titles[i] = fragments[kTitleIndex[i]];
        std::string body;
        for (int f = kBodyStart[i]; f <= kBodyEnd[i]; f++) body += fragments[f];
        help.bodies[i] = body;
    }

    return help;
}

}  // namespace dawnstar
