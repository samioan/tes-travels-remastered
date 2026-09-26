#include <stdexcept>

#include "world/game_world.h"
#include "world/items.h"

namespace oblivion {

namespace {

void Put16(std::vector<uint8_t>& out, int v) {
    out.push_back(static_cast<uint8_t>(v >> 8));
    out.push_back(static_cast<uint8_t>(v));
}

struct Reader {
    const std::vector<uint8_t>& d;
    size_t pos;
    int U8() {
        if (pos >= d.size()) throw std::runtime_error("save record truncated");
        return d[pos++];
    }
    int8_t S8() { return static_cast<int8_t>(U8()); }
    int U16() {
        const int hi = U8();
        return hi << 8 | U8();
    }
    int16_t S16() { return static_cast<int16_t>(U16()); }
};

}  // namespace

// ActorSystem.serialize. Fields are big-endian; level and weapon are bytes
// written as 16 bits (sign-extended), the inventory is `(equipped ? 128 : 0) |
// category, id` pairs up to the first empty slot.
void World::SerializePlayer(std::vector<uint8_t>& out) const {
    const Actor& a = *player_;
    out.push_back(static_cast<uint8_t>(a.slot));
    out.push_back(static_cast<uint8_t>(a.classId));
    out.push_back(static_cast<uint8_t>(a.xp >> 16));
    out.push_back(static_cast<uint8_t>(a.xp >> 8));
    out.push_back(static_cast<uint8_t>(a.xp));
    Put16(out, a.level);
    Put16(out, a.strength);
    Put16(out, a.intelligence);
    Put16(out, a.agility);
    Put16(out, a.speed);
    Put16(out, a.endurance);
    Put16(out, a.willpower);
    Put16(out, a.weapon);
    Put16(out, a.sightRange);
    Put16(out, a.attackRange);
    out.push_back(static_cast<uint8_t>(a.team));
    Put16(out, gold);
    out.push_back(static_cast<uint8_t>(a.cmlPath.size()));
    out.insert(out.end(), a.cmlPath.begin(), a.cmlPath.end());
    int count = 0;
    while (count < 255 && a.inventory[count] != 0) count++;
    out.push_back(static_cast<uint8_t>(count));
    for (int i = 0; i < count; i++) {
        const int category = a.inventory[i] >> 8 & 0xFF, id = a.inventory[i] & 0xFF;
        const bool equipped = category == 1 ? Items::HasArmor(a, id) : category == 0 ? a.weapon == id : false;
        out.push_back(static_cast<uint8_t>((equipped ? 128 : 0) | category));
        out.push_back(static_cast<uint8_t>(id));
    }
}

// ActorSystem.fromRecord.
bool World::RestorePlayer(const std::vector<uint8_t>& data, size_t offset) {
    try {
        Reader r{data, offset};
        auto a = std::make_shared<Actor>();
        a->slot = r.S8();
        a->classId = r.S8();
        a->xp = r.U8() << 16 | r.U8() << 8 | r.U8();
        a->level = static_cast<int8_t>(r.U16());
        a->strength = r.S16();
        a->intelligence = r.S16();
        a->agility = r.S16();
        a->speed = r.S16();
        a->endurance = r.S16();
        a->willpower = r.S16();
        a->weapon = static_cast<int8_t>(r.U16());
        a->sightRange = r.S16();
        a->attackRange = r.S16();
        a->team = r.S8();
        gold = r.U16();
        const int pathLen = r.U8();
        std::string cml;
        for (int i = 0; i < pathLen; i++) cml.push_back(static_cast<char>(r.U8()));
        a->name = "Champion";
        const int items = r.U8();
        ActorSystem::Init(*a, cml, a->slot, SpritesFor(cml));
        Items::SetClass(*a, a->classId, true, script_);
        for (int i = 0; i < 255; i++) a->inventory[i] = 0;
        static const int kTable[3] = {4, 1, 2};  // category -> weapons / armour / consumables
        for (int i = 0; i < items; i++) {
            int category = r.U8();
            const int id = r.U8();
            const bool equipped = (category & 128) != 0;
            category &= 0x7F;
            if (category > 2) continue;
            if (const int* row = script_.GetRow(kTable[category], id)) Items::AddItem(*a, category, row, script_, equipped);
        }
        Items::RecalcDerivedStats(*a, script_);
        player_ = a;
        actors_[0] = a;
        return true;
    } catch (const std::exception& e) {
        unimplemented_[std::string("save restore: ") + e.what()]++;
        return false;
    }
}

}  // namespace oblivion
