#include "assets/item_database.h"

namespace dawnstar {

ItemDatabase ItemDatabase::Load(DatArchive& archive) {
    ItemDatabase db;

    {
        BinaryReader in = archive.OpenResource("itemsin.dat");
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
        BinaryReader in = archive.OpenResource("droppeditemsin.dat");
        uint16_t rows = in.ReadU16();
        uint16_t cols = in.ReadU16();
        db.lootTable.assign(rows, std::vector<int8_t>(cols));
        for (auto& row : db.lootTable) {
            for (auto& v : row) v = in.ReadS8();
        }
    }

    return db;
}

int ItemDatabase::RandomGiftItemOfSubtype(JavaRandom& rng, int subtypeWanted) const {
    int first = -1;
    int last = -1;

    for (int i = 0; i < ItemCount(); i++) {
        if (category[i] == 11 && subtype[i] == static_cast<int8_t>(subtypeWanted)) {
            if (first == -1) first = i;
            last = i;
        }
    }

    int span = last - first + 1;
    int pick = first + RandomIntBelow(rng, span);
    return 1 + pick;
}

int ItemDatabase::RollLoot(JavaRandom& rng, int depth, int bonusRolls) const {
    int roll = LingoRandomInt(rng, 100);
    int best = roll;

    for (int i = 1; i < bonusRolls; i++) {
        roll = LingoRandomInt(rng, 100);
        if (roll > best) best = roll;
    }

    roll = best;
    int rarityCol;
    if (roll <= 64) {
        rarityCol = 0;
    } else if (roll <= 75) {
        rarityCol = 1;
    } else if (roll <= 90) {
        rarityCol = 3;
    } else {
        rarityCol = 4;
    }

    int depthRows = static_cast<int>(lootTable.size());
    int depthRoll = LingoRandomInt(rng, 10);
    depthRoll += depth - 2;
    if (depthRoll > depthRows - 1) depthRoll = depthRows - 1;
    if (depthRoll < 0) depthRoll = 0;

    int low = lootTable[depthRoll][rarityCol];
    int result = low;
    if (rarityCol == 1) {
        int high = lootTable[depthRoll][2];
        result |= high << 8;
    }

    return result;
}

}  // namespace dawnstar
