#pragma once
#include <fstream>
#include <stdexcept>
#include <string>

namespace stormhold {

// Stormhold has no datfiles.lmp/imgfiles.lmp archive, unlike dawnstar --
// every *in.dat/.cus/.png resource is its own top-level jar resource, read
// directly via ESGame.getResource()/Util.openResource (plain
// getResourceAsStream(), no archive indirection -- see
// ../../docs/ASSET_FORMATS.md's header note). AssetRoot mirrors that
// directly: a plain root directory (defaulting to ../extracted/, this
// project's tools/extract_jar.py output) that OpenFile() resolves resource
// names against, with no scanning/indexing to do since there's no archive
// format here at all.
class AssetRoot {
public:
    explicit AssetRoot(std::string rootDir) : rootDir_(std::move(rootDir)) {}

    // Opens `<root>/<name>` in binary mode. Throws std::runtime_error if
    // the file can't be opened -- same "well-formed asset set always has
    // every name the game asks for" assumption dawnstar's DatArchive
    // documents, just surfaced as a real exception here since there's no
    // archive-internal scan loop to (not) fall off the end of.
    std::ifstream OpenFile(const std::string& name) const {
        std::ifstream file(rootDir_ + "/" + name, std::ios::binary);
        if (!file) {
            throw std::runtime_error("AssetRoot: cannot open " + name);
        }
        return file;
    }

private:
    std::string rootDir_;
};

}  // namespace stormhold
