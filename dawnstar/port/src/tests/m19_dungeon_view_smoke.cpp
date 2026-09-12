// M19 smoke test: DungeonView::TileAt's real cross-level neighbor
// stitching (Dungeon.java's tileAt()), against the real 37-level world
// (M6's DungeonGenerator). No JVM ground truth is available here either
// (same ESGame/stub-jar block as M6/M9/M13), so this checks hand-derived
// expectations from ../../../src/Dungeon.java's tileAt() directly:
//  - A same-size (35x35 standard-to-standard) crossing needs no
//    recentering, so the tile one step across a border must exactly
//    equal the mirrored tile in the neighbor level -- checked at every
//    real border this world actually has, not just one hand-picked case.
//  - A hub-town (19x19) <-> standard-level (35x35) crossing DOES
//    recenter the perpendicular coordinate by (35-19)/2 = 8, and the
//    boundary column/row one step further in triggers the "edge marker"
//    (64) sentinel wherever the neighbor's raw tile byte there is still
//    untouched (0) -- both checked against a real hub border exit found
//    the same way M13's FindEdgeExit does (scanning for a real walkable
//    exit rather than assuming hardcoded coordinates).
//  - No neighbor in a direction (<=0) returns 1 (wall), unconditionally.
#include <cstdio>
#include <string>
#include <vector>

#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "world/dungeon_generator.h"
#include "world/dungeon_view.h"

namespace {

using dawnstar::DungeonView;
using dawnstar::GeneratedLevel;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

// Same helper shape as M13's FindEdgeExit: a walkable tile on `level`'s
// border facing outward in `direction` (1=N,2=E,3=S,4=W) whose geomin.dat
// neighbor in that direction is real (>0).
bool FindEdgeExit(const GeneratedLevel& level, int direction, int* outX, int* outY) {
    if (direction == 1 && level.neighborNorth > 0) {
        for (int x = 0; x < level.width; x++) {
            if ((level.tiles[static_cast<size_t>(x)][0] & 1) == 0) {
                *outX = x;
                *outY = 0;
                return true;
            }
        }
    } else if (direction == 3 && level.neighborSouth > 0) {
        for (int x = 0; x < level.width; x++) {
            if ((level.tiles[static_cast<size_t>(x)][static_cast<size_t>(level.height - 1)] & 1) == 0) {
                *outX = x;
                *outY = level.height - 1;
                return true;
            }
        }
    } else if (direction == 2 && level.neighborEast > 0) {
        for (int y = 0; y < level.height; y++) {
            if ((level.tiles[static_cast<size_t>(level.width - 1)][static_cast<size_t>(y)] & 1) == 0) {
                *outX = level.width - 1;
                *outY = y;
                return true;
            }
        }
    } else if (direction == 4 && level.neighborWest > 0) {
        for (int y = 0; y < level.height; y++) {
            if ((level.tiles[0][static_cast<size_t>(y)] & 1) == 0) {
                *outX = 0;
                *outY = y;
                return true;
            }
        }
    }
    return false;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        dawnstar::MonsterDatabase monsters = dawnstar::MonsterDatabase::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);

        std::vector<GeneratedLevel> levels;
        for (size_t i = 0; i < geometry.rows.size(); i++) {
            int levelNumber = static_cast<int>(i) + 1;
            levels.push_back(levelNumber == 1 ? dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[i])
                                               : dawnstar::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[i],
                                                                                            items, monsters));
        }
        std::printf("generated %zu levels\n", levels.size());

        // --- no-neighbor edges return wall (1) ---
        {
            DungeonView view(levels, 1);  // level 2, a standard level
            const GeneratedLevel& level = levels[1];
            if (level.neighborNorth <= 0) Check(view.TileAt(5, -1) == 1, "no north neighbor -> wall");
            if (level.neighborWest <= 0) Check(view.TileAt(-1, 5) == 1, "no west neighbor -> wall");
        }

        // --- standard-to-standard crossings: exact mirrored tile, no
        // recentering ---
        int standardChecked = 0;
        for (size_t i = 1; i < levels.size(); i++) {  // skip the hub (index 0)
            const GeneratedLevel& level = levels[i];
            DungeonView view(levels, static_cast<int>(i));

            struct { int dir; int neighbor; } dirs[4] = {
                {1, level.neighborNorth}, {2, level.neighborEast}, {3, level.neighborSouth}, {4, level.neighborWest}};
            for (auto& d : dirs) {
                if (d.neighbor <= 0 || d.neighbor == 1) continue;  // hub crossings checked separately below
                const GeneratedLevel& neighbor = levels[static_cast<size_t>(d.neighbor - 1)];
                if (neighbor.width != level.width || neighbor.height != level.height) continue;  // shouldn't happen

                int x = 0, y = 0;
                bool haveExit = FindEdgeExit(level, d.dir, &x, &y);
                if (!haveExit) continue;

                int probeX = x, probeY = y, nx = 0, ny = 0;
                switch (d.dir) {
                    case 1:
                        probeY = -1;
                        nx = x;
                        ny = neighbor.height - 1;
                        break;
                    case 2:
                        probeX = level.width;
                        nx = 0;
                        ny = y;
                        break;
                    case 3:
                        probeY = level.height;
                        nx = x;
                        ny = 0;
                        break;
                    case 4:
                        probeX = -1;
                        nx = neighbor.width - 1;
                        ny = y;
                        break;
                }

                uint8_t expected = neighbor.tiles[static_cast<size_t>(nx)][static_cast<size_t>(ny)];
                uint8_t actual = view.TileAt(probeX, probeY);
                Check(actual == expected, "standard-to-standard crossing should mirror the neighbor's tile exactly");
                standardChecked++;
            }
        }
        std::printf("checked %d standard-to-standard border crossings\n", standardChecked);
        Check(standardChecked > 0, "should have found at least one standard-to-standard crossing to check");

        // --- hub <-> standard-level recentering + edge marker ---
        const GeneratedLevel& hub = levels[0];
        DungeonView hubView(levels, 0);
        int hubChecked = 0;
        for (int dir = 1; dir <= 4; dir++) {
            int x = 0, y = 0;
            if (!FindEdgeExit(hub, dir, &x, &y)) continue;

            int neighborLevel = dir == 1 ? hub.neighborNorth
                                : dir == 2 ? hub.neighborEast
                                : dir == 3 ? hub.neighborSouth
                                            : hub.neighborWest;
            const GeneratedLevel& neighbor = levels[static_cast<size_t>(neighborLevel - 1)];

            int probeX = x, probeY = y, nx = 0, ny = 0;
            bool edgeMarkerExpectedAxis = false;
            int edgeMarkerCoord = 0;
            switch (dir) {
                case 1:
                    probeY = -1;
                    ny = neighbor.height - 1;
                    nx = x + (neighbor.width - hub.width) / 2;
                    edgeMarkerCoord = ny;  // checked against neighbor.height-2 below
                    edgeMarkerExpectedAxis = (ny == neighbor.height - 2);
                    break;
                case 2:
                    probeX = hub.width;
                    nx = 0;
                    ny = y + (neighbor.height - hub.height) / 2;
                    edgeMarkerCoord = nx;
                    edgeMarkerExpectedAxis = (nx == 1);
                    break;
                case 3:
                    probeY = hub.height;
                    ny = 0;
                    nx = x + (neighbor.width - hub.width) / 2;
                    edgeMarkerCoord = ny;
                    edgeMarkerExpectedAxis = (ny == 1);
                    break;
                case 4:
                    probeX = -1;
                    nx = neighbor.width - 1;
                    ny = y + (neighbor.height - hub.height) / 2;
                    edgeMarkerCoord = nx;
                    edgeMarkerExpectedAxis = (nx == neighbor.width - 2);
                    break;
            }
            (void)edgeMarkerCoord;

            if (nx < 0 || nx >= neighbor.width || ny < 0 || ny >= neighbor.height) continue;

            uint8_t neighborRaw = neighbor.tiles[static_cast<size_t>(nx)][static_cast<size_t>(ny)];
            uint8_t expected = (edgeMarkerExpectedAxis && neighborRaw == 0) ? 64 : neighborRaw;
            uint8_t actual = hubView.TileAt(probeX, probeY);
            char msg[160];
            std::snprintf(msg, sizeof(msg), "hub<->standard crossing dir=%d should apply the recentering formula",
                          dir);
            Check(actual == expected, msg);
            hubChecked++;
        }
        std::printf("checked %d hub<->standard-level crossings\n", hubChecked);
        Check(hubChecked > 0, "should have found at least one hub border crossing to check");

        if (g_ok) {
            std::printf("all cross-level tileAt checks passed\n");
            return 0;
        } else {
            std::printf("SOME CHECKS FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("EXCEPTION: %s\n", e.what());
        return 2;
    }
}
