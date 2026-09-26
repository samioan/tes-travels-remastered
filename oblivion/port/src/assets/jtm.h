#pragma once
#include <cstdint>
#include <vector>

namespace oblivion {

// A parsed .jtm tile map (Game.loadMap). Layer 0 is the collision layer; the
// remaining layers are visual tile layers, the last being the object overlay
// drawn together with the actors. Every layer holds width*height cells in the
// engine's own order, index = x * height + y. See docs/ASSET_FORMATS.md and
// tools/parse_jtm.py.
struct JtmMap {
    int width = 0;
    int height = 0;
    std::vector<std::vector<uint8_t>> layers;  // layers[0] = collision

    const std::vector<uint8_t>& collision() const { return layers[0]; }
    int cellCount() const { return width * height; }
};

// Throws std::runtime_error on malformed data.
JtmMap ParseJtm(const std::vector<uint8_t>& data);

}  // namespace oblivion
