#pragma once
#include <map>
#include <string>
#include <vector>

namespace oblivion {

// One /lang_N.txt: "<id> <text>|" entries (Game.loadLangPack builds an
// id -> offset table over the raw buffer; a map is equivalent).
class LangPack {
public:
    void Parse(const std::vector<uint8_t>& data);
    // nullptr if the id is not in this pack.
    const std::string* Find(int id) const;
    bool empty() const { return strings_.empty(); }

private:
    std::map<int, std::string> strings_;
};

// Game.getString: pack 0 (loaded once at start) first, then the level pack
// (LOAD_LANG replaces it).
class Strings {
public:
    LangPack pack0, pack1;
    std::string Get(int id) const;
    // Game.stringToId: id of the pack-0 entry with exactly this text, or -1.
    int IdOf(const std::string& text) const;
};

}  // namespace oblivion
