// M8 smoke test: WardenState's threshold escalation and tile mutation,
// including the confirmed wardenLeaves() index bug (see world/warden.h's
// own header comment) reproduced deliberately rather than "fixed".
#include <cstdio>

#include "assets/asset_root.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "world/dungeon_generator.h"
#include "world/warden.h"

namespace {

bool Expect(bool cond, const char* what) {
    if (!cond) std::printf("  FAIL: %s\n", what);
    return cond;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        stormhold::AssetRoot assets(root);
        stormhold::ItemDatabase items = stormhold::ItemDatabase::Load(assets);
        stormhold::MonsterDatabase monsters = stormhold::MonsterDatabase::Load(assets);
        stormhold::DungeonGeometry geometry = stormhold::DungeonGeometry::Load(assets);

        stormhold::GeneratedLevel hub = stormhold::DungeonGenerator::BuildHubLevel(geometry.rows[0]);
        stormhold::GeneratedLevel level2 =
            stormhold::DungeonGenerator::PopulateLevel(2, geometry.rows[1], items, monsters);

        bool ok = true;

        // Varus's hub tile isn't one of the 6 always-present shop tiles
        // BuildHubLevel marks (M6's own finding) -- bit 32 should start
        // clear there.
        ok &= Expect(!(hub.tiles[stormhold::WardenState::kShopX][stormhold::WardenState::kShopY] & 32),
                     "hub tile at Varus's position should start with bit 32 clear");

        stormhold::WardenState warden;

        ok &= Expect(!warden.ShouldVisit(12), "should not visit yet at elapsedCounter=12");
        ok &= Expect(warden.ShouldVisit(13), "should visit at elapsedCounter=13 (1st threshold)");

        warden.Arrive(hub);
        ok &= Expect(warden.present, "present should be true after Arrive");
        ok &= Expect(warden.visitCount == 1, "visitCount should be 1 after 1st Arrive");
        ok &= Expect((hub.tiles[stormhold::WardenState::kShopX][stormhold::WardenState::kShopY] & 32) != 0,
                     "hub tile should have bit 32 set after Arrive");
        ok &= Expect(!warden.ShouldVisit(1000), "should not visit again while already present");

        // Leave() -- reproduces the confirmed dungeons[1]-vs-dungeons[0]
        // index bug: the hub tile is NOT simply restored with bit 32
        // cleared, it's overwritten with level 2's own tile value at the
        // same (x, y), bit 32 cleared.
        warden.Leave(hub, level2);
        uint8_t expectedBuggyValue =
            static_cast<uint8_t>(level2.tiles[stormhold::WardenState::kShopX][stormhold::WardenState::kShopY] & ~32);
        ok &= Expect(!warden.present, "present should be false after Leave");
        ok &= Expect(hub.tiles[stormhold::WardenState::kShopX][stormhold::WardenState::kShopY] == expectedBuggyValue,
                     "hub tile after Leave should equal level 2's tile at the same position (the bug), not the "
                     "hub's own prior value");
        ok &= Expect(!(hub.tiles[stormhold::WardenState::kShopX][stormhold::WardenState::kShopY] & 32),
                     "hub tile should have bit 32 clear after Leave");

        // 2nd/3rd visit thresholds (26/39), and the cap past visitCount==3
        // (the original's if/else chain has no 4th branch).
        ok &= Expect(!warden.ShouldVisit(25), "should not visit yet at elapsedCounter=25 (before 2nd threshold)");
        ok &= Expect(warden.ShouldVisit(26), "should visit at elapsedCounter=26 (2nd threshold)");
        warden.Arrive(hub);
        warden.Leave(hub, level2);
        ok &= Expect(warden.visitCount == 2, "visitCount should be 2 after 2nd Arrive");

        ok &= Expect(!warden.ShouldVisit(38), "should not visit yet at elapsedCounter=38 (before 3rd threshold)");
        ok &= Expect(warden.ShouldVisit(39), "should visit at elapsedCounter=39 (3rd threshold)");
        warden.Arrive(hub);
        ok &= Expect(warden.visitCount == 3, "visitCount should be 3 after 3rd Arrive");
        warden.Leave(hub, level2);
        ok &= Expect(!warden.ShouldVisit(100000),
                     "should never visit again past visitCount==3, matching the original's missing 4th branch");

        if (!ok) {
            std::fprintf(stderr, "m8_warden_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m8_warden_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
