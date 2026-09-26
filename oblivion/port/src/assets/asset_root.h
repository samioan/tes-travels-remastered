#pragma once
#include <cstdint>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace oblivion {

// The original loads every resource with getResourceAsStream("/name"), i.e.
// straight out of the jar root. AssetRoot mirrors that: a directory holding
// the jar's contents (this repo's tools/extract_jar.py output,
// oblivion/extracted/), with names given exactly as the game spells them
// ("/l01_1.jtm" -- the leading slash is accepted and ignored).
class AssetRoot {
public:
    explicit AssetRoot(std::string dir) : dir_(std::move(dir)) {}

    std::vector<uint8_t> Read(const std::string& name) const {
        std::string rel = (!name.empty() && name[0] == '/') ? name.substr(1) : name;
        std::ifstream f(dir_ + "/" + rel, std::ios::binary);
        if (!f) throw std::runtime_error("AssetRoot: cannot open " + name);
        return std::vector<uint8_t>(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    }

    const std::string& dir() const { return dir_; }

private:
    std::string dir_;
};

}  // namespace oblivion
