#include "assets/item_database.h"

#include "assets/binary_reader.h"

namespace stormhold {

ItemDatabase ItemDatabase::Load(const AssetRoot& assets) {
    ItemDatabase db;

    {
        std::ifstream stream = assets.OpenFile("itemsin.dat");
        BinaryReader in(stream);

        uint16_t categoryCount = in.ReadU16();
        db.categoryNames.resize(categoryCount);
        for (auto& s : db.categoryNames) s = in.ReadUTF();

        uint16_t itemCount = in.ReadU16();
        db.name.resize(itemCount);
        db.category.resize(itemCount);
        db.subtype.resize(itemCount);
        db.questFlags.resize(itemCount);
        db.buyPrice.resize(itemCount);
        db.sellPrice.resize(itemCount);
        db.equipSlot.resize(itemCount);

        for (auto& v : db.name) v = in.ReadUTF();
        for (auto& v : db.category) v = in.ReadS8();
        for (auto& v : db.subtype) v = in.ReadS8();
        for (auto& v : db.questFlags) v = in.ReadS8();
        for (auto& v : db.buyPrice) v = in.ReadS16();
        for (auto& v : db.sellPrice) v = in.ReadS16();
        for (auto& v : db.equipSlot) v = in.ReadS8();
    }

    {
        std::ifstream stream = assets.OpenFile("droppeditemsin.dat");
        BinaryReader in(stream);

        uint16_t rows = in.ReadU16();
        uint16_t cols = in.ReadU16();
        db.lootTable.assign(rows, std::vector<int8_t>(cols));
        for (auto& row : db.lootTable) {
            for (auto& v : row) v = in.ReadS8();
        }
    }

    return db;
}

}  // namespace stormhold
