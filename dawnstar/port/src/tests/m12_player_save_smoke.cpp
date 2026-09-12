// M12 smoke test: builds a real character (M11's PlayerCreation) for
// each of the 7 classes, fills in the "full save"-only fields
// (position/camp/timers/effects/event-flags/buffs/traitor-suspicion)
// with varied, non-default values, round-trips through
// PlayerSave::ToBytes/FromBytes, and checks the result field-by-field
// against the real Player.java source (see player_save.cpp).
//
// The one field group that is *not* checked for a clean round-trip is
// traitorIndex/traitorSuspicionCount: Player.java's toBytes(true) has a
// real operator-precedence bug (`traitorIndex << 2 + traitorSuspicionCount`
// parses as `traitorIndex << (2 + traitorSuspicionCount)`, not
// `(traitorIndex << 2) + traitorSuspicionCount` the read-back side
// assumes), so on real hardware this data is already lossy for most
// input pairs. This test hand-derives the exact (buggy) expected packed
// byte and decoded values for two cases -- one where the bug corrupts
// the data, one boundary case (traitorSuspicionCount==0) where it
// happens not to -- and asserts the port matches that hand trace exactly,
// i.e. faithfully reproduces the bug rather than "fixing" it.
#include <cstdio>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/item_database.h"
#include "player/player_creation.h"
#include "player/player_save.h"
#include "util/java_random.h"

namespace {

bool CheckPlayerStateEquiv(const dawnstar::PlayerState& a, const dawnstar::PlayerState& b) {
    bool ok = true;
    auto fail = [&](const char* what) {
        std::printf("  FAIL: %s mismatch\n", what);
        ok = false;
    };

    if (a.name != b.name) fail("name");
    if (a.classIndex != b.classIndex) fail("classIndex");
    if (a.raceIndex != b.raceIndex) fail("raceIndex");
    if (a.coreStats != b.coreStats) fail("coreStats");
    if (a.attributeIncreaseFlags != b.attributeIncreaseFlags) fail("attributeIncreaseFlags");
    if (a.gold != b.gold) fail("gold");
    if (a.attributes != b.attributes) fail("attributes");
    if (a.classMagickaFactor != b.classMagickaFactor) fail("classMagickaFactor");
    if (a.classUnknownPair != b.classUnknownPair) fail("classUnknownPair");
    if (a.skills != b.skills) fail("skills");
    if (a.inventoryCount != b.inventoryCount) fail("inventoryCount");
    if (a.inventoryItemIds != b.inventoryItemIds) fail("inventoryItemIds");
    if (a.inventoryItemData != b.inventoryItemData) fail("inventoryItemData");
    if (a.equippedItems != b.equippedItems) fail("equippedItems");
    if (a.knownSpellsMask != b.knownSpellsMask) fail("knownSpellsMask");
    if (a.selectedSpellId != b.selectedSpellId) fail("selectedSpellId");
    if (a.giftPointsFound != b.giftPointsFound) fail("giftPointsFound");
    if (a.rumorRevealStep != b.rumorRevealStep) fail("rumorRevealStep");
    if (a.ailmentMask != b.ailmentMask) fail("ailmentMask");
    if (a.trollThirstTimer != b.trollThirstTimer) fail("trollThirstTimer");
    if (a.glacierCurseTimer != b.glacierCurseTimer) fail("glacierCurseTimer");
    if (a.terrifiedTimer != b.terrifiedTimer) fail("terrifiedTimer");
    if (a.unconfirmedZ != b.unconfirmedZ) fail("unconfirmedZ");
    if (a.currentLevel != b.currentLevel) fail("currentLevel");
    if (a.tileX != b.tileX) fail("tileX");
    if (a.tileY != b.tileY) fail("tileY");
    if (a.facing != b.facing) fail("facing");
    if (a.campLevel != b.campLevel) fail("campLevel");
    if (a.campX != b.campX) fail("campX");
    if (a.campY != b.campY) fail("campY");
    if (a.campFacing != b.campFacing) fail("campFacing");
    if (a.effectDurations != b.effectDurations) fail("effectDurations");
    if (a.combatTargetSpawnId != b.combatTargetSpawnId) fail("combatTargetSpawnId");
    if (a.tempArmorBonus != b.tempArmorBonus) fail("tempArmorBonus");
    if (a.increaseHarmBuff != b.increaseHarmBuff) fail("increaseHarmBuff");
    if (a.increaseArmorBuff != b.increaseArmorBuff) fail("increaseArmorBuff");
    if (a.safeCampingBuff != b.safeCampingBuff) fail("safeCampingBuff");
    if (a.eventFlags != b.eventFlags) fail("eventFlags");

    // traitorIndex/traitorSuspicionCount AND specialEncounterResolved/
    // roamingSpecialMonsterPresent are all packed into the same single
    // byte (see player_save.cpp), so the traitor-packing bug can corrupt
    // any of the four -- none of the four are asserted here unconditionally.
    // They're instead checked against the hand-derived packed-byte
    // formula in main().

    return ok;
}

// Hand trace of Player.java's toBytes(true)'s buggy packed-byte formula
// (see player_save.cpp's comment) -- computed independently from the
// port's own implementation, straight off the Java source, to serve as
// an external check on it rather than testing the code against itself.
int8_t ExpectedPackedByte(int8_t traitorIndex, int8_t traitorSuspicionCount, bool specialEncounterResolved,
                           bool roamingSpecialMonsterPresent) {
    int packed = static_cast<int8_t>(traitorIndex << (2 + traitorSuspicionCount));
    if (specialEncounterResolved) packed = static_cast<int8_t>(packed + 16);
    if (roamingSpecialMonsterPresent) packed = static_cast<int8_t>(packed + 32);
    return static_cast<int8_t>(packed);
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::CharacterData charData = dawnstar::CharacterData::Load(archive);
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);

        dawnstar::JavaRandom globalRng(778899);
        bool ok = true;

        for (int classIdx = 0; classIdx < charData.ClassCount(); classIdx++) {
            dawnstar::PlayerState p =
                dawnstar::PlayerCreation::CreateCharacter(classIdx, "Roundtrip", charData, items, globalRng);

            // Fill in the full-save-only fields with varied, non-default
            // values so the round-trip actually exercises them.
            p.giftPointsFound = static_cast<int16_t>(100 + classIdx);
            p.rumorRevealStep = static_cast<int16_t>(classIdx);
            p.ailmentMask = static_cast<int8_t>(1 << (classIdx % 8));
            p.trollThirstTimer = static_cast<int16_t>(-1);
            p.glacierCurseTimer = static_cast<int16_t>(30 - classIdx);
            p.terrifiedTimer = 0;
            p.unconfirmedZ = (classIdx % 2) == 0;
            p.currentLevel = 1 + classIdx;
            p.tileX = classIdx;
            p.tileY = 8 - classIdx;
            p.facing = 1 + (classIdx % 4);
            p.campLevel = static_cast<int8_t>(classIdx);
            p.campX = 5;
            p.campY = 5;
            p.campFacing = 2;
            for (int i = 0; i < 25; i++) {
                p.effectDurations[static_cast<size_t>(i)] = static_cast<int8_t>(i - 12 + classIdx);
            }
            p.combatTargetSpawnId = static_cast<int16_t>(1000 + classIdx);
            p.tempArmorBonus = static_cast<int16_t>(classIdx * 3);
            p.increaseHarmBuff = (classIdx % 3) == 0;
            p.increaseArmorBuff = (classIdx % 3) == 1;
            p.safeCampingBuff = (classIdx % 3) == 2;
            for (int i = 0; i < 96; i++) {
                p.eventFlags[static_cast<size_t>(i)] = ((i + classIdx) % 7) == 0;
            }

            // classIdx==0: force the concrete hand-verified example from
            // the file header (traitorIndex=2, traitorSuspicionCount=1
            // -> corrupted to (0,0) on read-back). classIdx==1: force
            // the traitorSuspicionCount==0 boundary case, where the bug
            // happens not to manifest. Other classes: exercise varied
            // combinations without asserting a specific outcome, beyond
            // matching the hand-derived formula.
            if (classIdx == 0) {
                p.traitorIndex = 2;
                p.traitorSuspicionCount = 1;
            } else if (classIdx == 1) {
                p.traitorIndex = 3;
                p.traitorSuspicionCount = 0;
            } else {
                p.traitorSuspicionCount = static_cast<int8_t>(classIdx % 4);
            }
            p.specialEncounterResolved = (classIdx % 2) == 0;
            p.roamingSpecialMonsterPresent = (classIdx % 4) == 0;

            std::vector<uint8_t> bytes = dawnstar::PlayerSave::ToBytes(p);
            dawnstar::PlayerState p2 = dawnstar::PlayerSave::FromBytes(bytes);

            std::printf("class[%d] %-12s bytes=%zu traitor(idx=%d,cnt=%d)->(idx=%d,cnt=%d)\n", classIdx,
                        charData.classNames[classIdx].c_str(), bytes.size(), p.traitorIndex, p.traitorSuspicionCount,
                        p2.traitorIndex, p2.traitorSuspicionCount);

            if (!CheckPlayerStateEquiv(p, p2)) ok = false;

            int8_t expectedPacked = ExpectedPackedByte(p.traitorIndex, p.traitorSuspicionCount,
                                                        p.specialEncounterResolved, p.roamingSpecialMonsterPresent);
            int8_t expectedSuspicion = static_cast<int8_t>(expectedPacked % 4);
            int8_t expectedTraitor = static_cast<int8_t>((expectedPacked >> 2) % 4);
            bool expectedSpecial = (expectedPacked & 16) == 16;
            bool expectedRoaming = (expectedPacked & 32) == 32;
            if (p2.traitorSuspicionCount != expectedSuspicion || p2.traitorIndex != expectedTraitor ||
                p2.specialEncounterResolved != expectedSpecial || p2.roamingSpecialMonsterPresent != expectedRoaming) {
                std::printf(
                    "  FAIL: decoded traitor packing doesn't match hand-derived formula (expected idx=%d cnt=%d "
                    "special=%d roaming=%d, got idx=%d cnt=%d special=%d roaming=%d)\n",
                    expectedTraitor, expectedSuspicion, expectedSpecial, expectedRoaming, p2.traitorIndex,
                    p2.traitorSuspicionCount, p2.specialEncounterResolved, p2.roamingSpecialMonsterPresent);
                ok = false;
            }
            if (classIdx == 1 && (p2.traitorIndex != p.traitorIndex || p2.traitorSuspicionCount != p.traitorSuspicionCount)) {
                std::printf("  FAIL: expected the traitorSuspicionCount==0 boundary case to round-trip cleanly\n");
                ok = false;
            }
            if (classIdx == 0 && (p2.traitorIndex != 0 || p2.traitorSuspicionCount != 0)) {
                std::printf("  FAIL: expected the documented (2,1)->(0,0) corruption case\n");
                ok = false;
            }
        }

        if (!ok) {
            std::fprintf(stderr, "m12_player_save_smoke: FAILED\n");
            return 1;
        }
        std::printf("all round-trip checks passed (including the hand-verified traitor-packing bug)\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m12_player_save_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
