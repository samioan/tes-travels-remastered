#include "assets/lang.h"

#include <cctype>
#include <cstdint>

namespace oblivion {

void LangPack::Parse(const std::vector<uint8_t>& data) {
    strings_.clear();
    size_t i = 0;
    while (i < data.size()) {
        size_t end = i;
        while (end < data.size() && data[end] != '|') end++;
        // Skip the line break(s) left between entries.
        size_t p = i;
        while (p < end && std::isspace(data[p])) p++;
        int id = 0;
        size_t digits = 0;
        while (p < end && std::isdigit(data[p])) {
            id = id * 10 + (data[p] - '0');
            p++;
            digits++;
        }
        if (digits && p < end && data[p] == ' ') {
            strings_[id] = std::string(data.begin() + static_cast<std::ptrdiff_t>(p) + 1,
                                       data.begin() + static_cast<std::ptrdiff_t>(end));
        }
        i = end + 1;
    }
}

const std::string* LangPack::Find(int id) const {
    auto it = strings_.find(id);
    return it == strings_.end() ? nullptr : &it->second;
}

std::string Strings::Get(int id) const {
    if (const std::string* s = pack0.Find(id)) return *s;
    if (const std::string* s = pack1.Find(id)) return *s;
    return "";
}

int Strings::IdOf(const std::string& text) const {
    // Iteration order of the map is ascending id, like the original scan.
    for (int id = 1; id < 4096; id++) {
        const std::string* s = pack0.Find(id);
        if (s && *s == text) return id;
    }
    return -1;
}

}  // namespace oblivion
