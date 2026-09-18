#include "assets/character_data.h"

#include <stdexcept>

#include "assets/binary_reader.h"

namespace stormhold {

namespace {

// Renamed-source counterpart of Player.java's readStringArray(): a u16
// count followed by that many readUTF()-style strings.
std::vector<std::string> ReadStringArray(BinaryReader& in) {
    uint16_t count = in.ReadU16();
    std::vector<std::string> out(count);
    for (auto& s : out) s = in.ReadUTF();
    return out;
}

}  // namespace

CharacterData CharacterData::Load(const AssetRoot& assets) {
    CharacterData data;
    std::ifstream stream = assets.OpenFile("charin.dat");
    BinaryReader in(stream);

    data.statLabels = ReadStringArray(in);
    data.attributeNames = ReadStringArray(in);
    data.classNames = ReadStringArray(in);
    data.raceNames = ReadStringArray(in);
    data.skillNames = ReadStringArray(in);

    // Player.loadCharacterData() throws here rather than proceeding -- the
    // 13+2*skillCount class-template row width below is only correct for
    // the 14 skills every other skill-indexed table in the codebase
    // (Player.skills[], Spell.skillRequired, ...) assumes.
    if (data.skillNames.size() != 14) {
        throw std::runtime_error(
            "CharacterData: expected 14 skills, got " + std::to_string(data.skillNames.size()));
    }

    int skillCount = static_cast<int>(data.skillNames.size());
    data.skillAttributeIndex.resize(skillCount);
    for (auto& v : data.skillAttributeIndex) v = in.ReadS16();

    int cols = 13 + 2 * skillCount;
    data.classTemplates.assign(data.classNames.size(), std::vector<int16_t>(cols));
    for (auto& row : data.classTemplates) {
        for (auto& v : row) v = in.ReadS16();
    }

    return data;
}

}  // namespace stormhold
