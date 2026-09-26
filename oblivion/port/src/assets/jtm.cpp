#include "assets/jtm.h"

#include <stdexcept>

namespace oblivion {

JtmMap ParseJtm(const std::vector<uint8_t>& d) {
    if (d.size() < 2) throw std::runtime_error("jtm: too short");
    JtmMap m;
    m.width = d[0];
    m.height = d[1];
    const int cells = m.width * m.height;
    size_t pos = 2;
    while (pos < d.size()) {
        std::vector<uint8_t> layer(cells, 0);
        int run = 0, value = 0;  // remaining cells of the current 0xFF run
        // Stored row by row (y outer, x inner); a run may span rows.
        for (int y = 0; y < m.height; y++) {
            for (int x = 0; x < m.width; x++) {
                if (run == 0) {
                    if (pos >= d.size()) throw std::runtime_error("jtm: layer runs past end");
                    uint8_t b = d[pos++];
                    if (b == 0xFF) {
                        if (pos + 2 > d.size()) throw std::runtime_error("jtm: truncated run");
                        run = d[pos++];
                        value = d[pos++];
                        if (run < 1) run = 1;  // the engine writes one cell for count 0 or 1
                    } else {
                        run = 1;
                        value = b;
                    }
                }
                layer[x * m.height + y] = static_cast<uint8_t>(value);
                run--;
            }
        }
        m.layers.push_back(std::move(layer));
    }
    if (m.layers.empty()) throw std::runtime_error("jtm: no layers");
    return m;
}

}  // namespace oblivion
