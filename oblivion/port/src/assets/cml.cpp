#include "assets/cml.h"

#include <stdexcept>

namespace oblivion {

namespace {

struct Reader {
    const std::vector<uint8_t>& d;
    size_t p = 0;
    uint8_t U8() {
        if (p >= d.size()) throw std::runtime_error("cml: read past end");
        return d[p++];
    }
    int16_t S8() { return static_cast<int8_t>(U8()); }
    int U16() { int h = U8(); return (h << 8) | U8(); }
    std::string Text(size_t n) {
        if (p + n > d.size()) throw std::runtime_error("cml: string past end");
        std::string s(d.begin() + p, d.begin() + p + n);
        p += n;
        return s;
    }
};

// The bit-flagged record shared by groups, frames and subframes (see
// tools/parse_cml.py): a 16-bit mask selects up to 10 fields, in order.
struct Rec {
    int f[10] = {0};
};

Rec ReadRecord(Reader& r) {
    static const int kBits[10] = {0x200, 0x100, 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};
    Rec rec;
    int mask = r.U16();
    for (int i = 0; i < 10; i++) {
        if (!(mask & kBits[i])) continue;
        if (i == 1 || i == 2) rec.f[i] = r.U16();
        else if (i >= 5) rec.f[i] = static_cast<int8_t>(r.U8());
        else rec.f[i] = r.U8();
    }
    return rec;
}

void Unpack(const Rec& r, SpriteFrame& f) {
    f.groupId = static_cast<int8_t>(r.f[0]);
    f.offsetX = static_cast<int16_t>(r.f[1]);
    f.offsetY = static_cast<int16_t>(r.f[2]);
    f.width = static_cast<int16_t>(r.f[3]);
    f.height = static_cast<int16_t>(r.f[4]);
    f.dx = static_cast<int8_t>(r.f[5]);
    f.dy = static_cast<int8_t>(r.f[6]);
    f.hold = static_cast<int8_t>(r.f[7]);
    f.isSprite = static_cast<int8_t>(r.f[8]);
}

}  // namespace

SpriteSet ParseCml(const std::vector<uint8_t>& data, ImageCache& images) {
    Reader r{data};
    SpriteSet set;
    std::string prefix = r.Text(r.U8());
    while (r.p != data.size()) {
        int nameId = r.U8();
        std::string name = r.Text(r.U8());
        if (name.empty() || name[0] != '/') name = prefix + name;
        Rec rec = ReadRecord(r);
        if (rec.f[0] == 0) rec.f[0] = nameId;
        int colorKeys = r.U8();  // parsed by the original but never used
        for (int i = 0; i < colorKeys * 6; i++) r.U8();
        int frameCount = r.U8();
        if (name == "/4.png") continue;  // SpriteRenderer.load skips this one image entirely

        std::shared_ptr<Image> image = images.Get(name);
        if (frameCount == 0) {
            SpriteGroup g;
            SpriteFrame f;
            Unpack(rec, f);
            f.imagePath = name;
            f.width = static_cast<int16_t>(image->width);
            f.height = static_cast<int16_t>(image->height);
            g.groupId = f.groupId;
            g.hold = f.hold;
            g.frames.push_back(f);
            set.groups.push_back(std::move(g));
        } else {
            for (int fi = 0; fi < frameCount; fi++) {
                Rec frameRec = ReadRecord(r);
                SpriteGroup g;
                g.groupId = static_cast<int8_t>(frameRec.f[0]);
                g.hold = static_cast<int8_t>(frameRec.f[7]);
                int subCount = r.U8();
                for (int s = 0; s < subCount; s++) {
                    Rec sub = ReadRecord(r);
                    SpriteFrame f;
                    Unpack(sub, f);
                    f.groupId = g.groupId;
                    f.hold = g.hold;
                    f.imagePath = name;
                    f.frameChain = 1;
                    g.frames.push_back(f);
                }
                set.groups.push_back(std::move(g));
            }
        }
    }
    return set;
}

SpriteGroup* SpriteSet::Find(int groupId) {
    for (auto& g : groups)
        if (g.groupId == static_cast<int8_t>(groupId)) return &g;
    return nullptr;
}

const SpriteGroup* SpriteSet::Find(int groupId) const {
    return const_cast<SpriteSet*>(this)->Find(groupId);
}

int SpriteSet::Width(int groupId) const {
    const SpriteGroup* g = Find(groupId);
    return (g && !g->frames.empty()) ? g->frames[0].width : 0;
}

int SpriteSet::Height(int groupId) const {
    const SpriteGroup* g = Find(groupId);
    return (g && !g->frames.empty()) ? g->frames[0].height : 0;
}

bool SpriteSet::AdvanceFrame(int groupId) {
    SpriteGroup* g = Find(groupId);
    if (!g) return true;
    if (g->current + 1 < g->frames.size()) {
        g->current++;
        return false;
    }
    if (g->hold != 1) return true;  // freeze on the last frame
    g->current = 0;
    return false;
}

bool SpriteSet::SetFrame(int groupId, int index) {
    SpriteGroup* g = Find(groupId);
    if (!g) return true;
    if (index < 0 || static_cast<size_t>(index) >= g->frames.size()) return true;
    g->current = static_cast<size_t>(index);
    return false;
}

void SpriteSet::ResetFrame(int groupId) {
    if (SpriteGroup* g = Find(groupId)) g->current = 0;
}

}  // namespace oblivion
